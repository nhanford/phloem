/*
mpiBench.c

Description:
  This source code file provides a variety of blocking, non-blocking, vector, and sub-communicator
  collective operations with a variety of barrier implementations available.

Authors: Adam Moody and Nathan Hanford

Copyright (c) 2007-2026 Lawrence Livermore National Security (LLNS), LLC.
See COPYRIGHT file for more details.

SPDX-License-Identifier: MIT
See LICENSE file for more details
*/


/*
Compile flags:
  -DNO_BARRIER       - Drops MPI_Barrier() call that separates consecutive collective calls
  -DUSE_GETTIMEOFDAY - Use gettimeofday() for timing rather than MPI_WTime()
  -DUSE_MPI_BARRIER  - Use MPI implementation MPI_Barrier for timing loop instead of generic barrier
  -DINCLUDE_BARRIER  - Include barrier operation in timing loop results
  -DENABLE_HIP       - Make all tests GPU-Aware for HIP.
  -DENABLE_CUDA      - Make all tests GPU-Aware for CUDA.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <mpi.h>
#include <math.h>
#include "util.h"

#ifdef ENABLE_HIP
char VERS[] = "ROCm";
#elif defined(ENABLE_CUDA)
char VERS[] = "CUDA";
#else
char VERS[] = "";
#endif

/*
------------------------------------------------------
Globals
------------------------------------------------------
*/
#define KILO (1024)
#define MEGA (KILO*KILO)
#define GIGA (KILO*MEGA)

#define HAVE_NBC 1  /* Change this if the MPI implementation does not support non-blocking collectives */

#define ITRS_EST       (5)        /* Number of iterations used to estimate time */
#define ITRS_RUN       (1000)     /* Number of iterations to run (without timelimits) */
#define MSG_SIZE_START (0)        /* Lower bound of message sizes in bytes */
#define MSG_SIZE_STOP  (256*KILO) /* Upper bound of message sizes in bytes */
#define MAX_PROC_MEM   (1*GIGA)   /* Limit on MPI buffer sizes in bytes */


/* we use a bit mask to flag which collectives to test */
#define BARRIER     (0x000001)
#define BCAST       (0x000002)
#define ALLTOALL    (0x000004)
#define ALLGATHER   (0x000008)
#define GATHER      (0x000010)
#define SCATTER     (0x000020)
#define ALLREDUCE   (0x000040)
#define REDUCE      (0x000080)
#define ALLTOALLV   (0x000100)
#define ALLGATHERV  (0x000200)
#define GATHERV     (0x000400)
#define IBCAST      (0x000800)
#define IALLTOALL   (0x001000)
#define IALLGATHER  (0x002000)
#define IGATHER     (0x004000)
#define ISCATTER    (0x008000)
#define IALLREDUCE  (0x010000)
#define IREDUCE     (0x020000)
#define IALLTOALLV  (0x040000)
#define IALLGATHERV (0x080000)
#define IGATHERV    (0x100000)

#define NUM_TESTS  (21)

char* TEST_NAMES[] = {
  "Barrier", "Bcast", "Alltoall", "Allgather", "Gather", "Scatter", "Allreduce", "Reduce", "Alltoallv", "Allgatherv", "Gatherv",
  "Ibcast", "Ialltoall", "Iallgather", "Igather", "Iscatter", "Iallreduce", "Ireduce", "Ialltoallv", "Iallgatherv", "Igatherv"
};
int   TEST_FLAGS[] = {
   BARRIER,   BCAST,   ALLTOALL,   ALLGATHER,   GATHER,   SCATTER,   ALLREDUCE,   REDUCE,   ALLTOALLV,   ALLGATHERV,   GATHERV,
   IBCAST, IALLTOALL, IALLGATHER, IGATHER, ISCATTER, IALLREDUCE, IREDUCE, IALLTOALLV, IALLGATHERV, IGATHERV
};

int rank_local; /* my MPI rank */
int rank_count; /* number of ranks in job */
int dimid_key;
size_t allocated_memory = 0; /* number of bytes allocated */

/*
------------------------------------------------------
Utility Functions
------------------------------------------------------
*/

/* Print usage syntax and exit */
int usage()
{
    if (rank_local == 0) {
        printf("\n");
        printf("  Usage:  mpiBench [options] [operations]\n");
        printf("\n");
        printf("  Options:\n");
        printf("    -b <byte>  Beginning message size in bytes (default 0)\n");
        printf("    -e <byte>  Ending message size in bytes (default 1K)\n");
        printf("    -m <byte>  Process memory buffer limit (send+recv) in bytes (default 1G)\n");
        printf("    -i <itrs>  Maximum number of iterations for a single test (default 1000)\n");
        printf("    -t <usec>  Time limit for any single test in microseconds (default 0 = infinity)\n");
        printf("    -d <ndim>  Number of Cartesian dimensions to split processes in (default 0 = MPI_COMM_WORLD only)\n");
        printf("    -p <size>  Minimum partition size (number of ranks) to divide MPI_COMM_WORLD by\n");
        printf("    -c         Check receive buffer for expected data in last interation (default disabled)\n");
        printf("    -C         Check receive buffer for expected data every iteration (default disabled)\n");
        printf("    -h         Print this help screen and exit\n");
        printf("    where <byte> = [0-9]+[KMG], e.g., 32K or 64M\n");
        printf("\n");
        printf("  Operations:\n");
        printf("    Barrier\n");
        printf("    Bcast\n");
        printf("    Alltoall, Alltoallv\n");
        printf("    Allgather, Allgatherv\n");
        printf("    Gather, Gatherv\n");
        printf("    Scatter\n");
        printf("    Allreduce\n");
        printf("    Reduce\n");
        printf("    Ibcast\n");
        printf("    Ialltoall, Ialltoallv\n");
        printf("    Iallgather, Iallgatherv\n");
        printf("    Igather, Igatherv\n");
        printf("    Iscatter\n");
        printf("    Iallreduce\n");
        printf("    Ireduce\n");
        printf("\n");
    }
    exit(1);
}

/* Allocate size bytes and keep track of amount allocated */
void* _ALLOC_MAIN_ (size_t size, char* debug)
{
    void* p_buf;
    p_buf = malloc(size);
    if (!p_buf) {
        printf("ERROR:  Allocating memory %s:  requesting %ld bytes\n", debug, size);
        exit(1);
    }
    memset(p_buf, 0, size);
    allocated_memory += size;
    return p_buf;
}

/* Processes byte strings in the following format:
     <float_num>[kKmMgG][bB]
   and returns number of bytes as an size_t
   returns 0 on error
   Examples: 1K, 2.5kb, .5GB
*/
size_t atobytes(char* str)
{
    char* next;
    size_t units = 1;

    double num = strtod(str, &next);
    if (num == 0.0 && next == str) return 0;
    if (*next != 0) {
        /* process units for kilo, mega, or gigabytes */
        switch(*next) {
            case 'k':
            case 'K':
                units = (size_t) KILO;
                break;
            case 'm':
            case 'M':
                units = (size_t) MEGA;
                break;
            case 'g':
            case 'G':
                units = (size_t) GIGA;
                break;
            default:
                printf("ERROR:  unexpected byte string %s\n", str);
                exit(1);
        }
        next++;
        if (*next == 'b' || *next == 'B') { next++; } /* handle optional b or B character, e.g. in 10KB */
        if (*next != 0) {
            printf("ERROR:  unexpected byte string: %s\n", str);
            exit(1);
        }
    }
    if (num < 0) { printf("ERROR:  byte string must be positive: %s\n", str);  exit(1); }
    return (size_t) (num * (double) units);
}

/*
------------------------------------------------------
TIMING CODE - start/stop the timer and measure the difference
------------------------------------------------------
*/

#ifdef USE_GETTIMEOFDAY

/* use gettimeofday() for timers */
#include <sys/time.h>
#define __TIME_START__    (gettimeofday(&g_timeval__start, &g_timezone))
#define __TIME_END__      (gettimeofday(&g_timeval__end  , &g_timezone))
#define __TIME_USECS__    (d_Time_Diff_Micros(g_timeval__start, g_timeval__end))
#define d_Time_Diff_Micros(timeval__start, timeval__end) \
  ( \
    (double) (  (timeval__end.tv_sec  - timeval__start.tv_sec ) * 1000000 \
              + (timeval__end.tv_usec - timeval__start.tv_usec)  ) \
  )
#define d_Time_Micros(timeval) \
  ( \
    (double) (  timeval.tv_sec * 1000000 \
              + timeval.tv_usec  ) \
  )
struct timeval  g_timeval__start, g_timeval__end;
struct timezone g_timezone;

#else

/* use MPI_Wtime for timers (recommened)
   on some systems gettimeofday may be reset backwards by a global clock,
   which can even lead to negative length time intervals
*/
#ifdef INCLUDE_BARRIER
#define __TIME_START__    (g_timeval__start    = MPI_Wtime())
#define __TIME_END__      (g_timeval__end      = MPI_Wtime())
#define __TIME_USECS__    ((g_timeval__end - g_timeval__start) * 1000000.0)
double g_timeval__start, g_timeval__end;

#else   /*  Use timing macros to exclude MPI_Barrier from timing.  */

#define __TIME_START__    g_timeval__running = 0; g_timeval__start    = MPI_Wtime()
#define __TIME_PAUSE__    (g_timeval__running += MPI_Wtime() - g_timeval__start)
#define __TIME_RESTART__    (g_timeval__start = MPI_Wtime())
#define __TIME_END__    (g_timeval__running += MPI_Wtime() - g_timeval__start)
#define __TIME_USECS__    (g_timeval__running * 1000000.0)
double g_timeval__start, g_timeval__end, g_timeval__running;

#endif /* of INCLUDE_BARRIER */

#endif /* of USE_GETTIMEOFDAY */


/* This barrier implementation is taken from the Sphinx MPI benchmarks
   and is based directly on the MPICH barrier implementation...        */

#define GENERIC_BARRIER_TAG 2

/* Gather value from each task and print statistics */
double Print_Timings(double value, char* title, size_t bytes, int iters, MPI_Comm comm)
{
    int i;
    double min, max, avg, dev;
    double* times = NULL;

    if(rank_local == 0) {
        times = (double*) malloc(sizeof(double) * rank_count);
    }

    /* gather single time value from each task to rank 0 */
    MPI_Gather(&value, 1, MPI_DOUBLE, times, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    /* rank 0 computes the min, max, and average over the set */
    if(rank_local == 0) {
        avg = 0;
        dev = 0;
        min = 100000000;
        max = -1;
        for(i = 0; i < rank_count; i++) {
            if(times[i] < min) { min = times[i]; }
            if(times[i] > max) { max = times[i]; }
            avg += times[i];
            dev += times[i] * times[i];
        }
        avg /= (double) rank_count;
        // TODO: FIX
        dev = 0; /*sqrt((dev / (double) rank_count - avg * avg)); */

        /* determine who we are in this communicator */
        int nranks, flag;
        char* str;
        MPI_Attr_get(comm, dimid_key, (void*) &str, &flag);
        MPI_Comm_size(comm, &nranks);

        printf("%-20.20s\t", title);
        printf("Bytes:\t%8zu\tIters:\t%7d\t", bytes, iters);
        printf("Avg:\t%8.4f\tMin:\t%8.4f\tMax:\t%8.4f\t", avg, min, max);
        printf("Comm: %s\tRanks: %d\n", str, nranks);
        fflush(stdout);

        free((void*) times);
    }

    /* broadcast the average value back out */
    MPI_Bcast(&avg, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    return avg;
}

/*
------------------------------------------------------
MAIN
------------------------------------------------------
*/

char *sbuffer;
char *rbuffer;
int  *sendcounts, *sdispls, *recvcounts, *rdispls;
MPI_Request request;
MPI_Status status;

size_t buffer_size = 0;
int check_once;
int check_every;

struct argList {
    int    iters;
    size_t messStart;
    size_t messStop;
    size_t memLimit;
    double timeLimit;
    int    testFlags;
    int    checkOnce;
    int    checkEvery;
    int    ndims;
    int    partSize;
};

int processMPIBArgs(int argc, char **argv, struct argList* args)
{
  int i, j;
  char *argptr;
  char flag;

  /* set to default values */
  args->iters      = ITRS_RUN;
  args->messStart  = (size_t) MSG_SIZE_START;
  args->messStop   = (size_t) MSG_SIZE_STOP;
  args->memLimit   = (size_t) MAX_PROC_MEM;
  args->timeLimit  = 0;
  args->testFlags  = 0x1FFFFF;
  args->checkOnce  = 0;
  args->checkEvery = 0;
  args->ndims      = 0;
  args->partSize   = 0;

  int iters_set = 0;
  int time_set  = 0;
  for (i=0; i<argc; i++)
  {
    /* check for options */
    if (argv[i][0] == '-')
    {
      /* flag is the first char following the '-' */
      flag   = argv[i][1];
      argptr = NULL;

      /* single argument parameters */
      if (strchr("cC", flag))
      {
        switch(flag)
        {
        case 'c':
          args->checkOnce = 1;
          break;
        case 'C':
          args->checkEvery = 1;
          break;
        }
        continue;
      }

      /* check that we've got a valid option */
      if (!strchr("beithmdp", flag))
      {
        printf("\nInvalid flag -%c\n", flag);
        return(0);
      }

      /* handles "-i#" or "-i #" */
      if (argv[i][2] != 0) {
        argptr = &(argv[i][2]);
      } else {
        argptr = argv[i+1];
        i++;
      }

      switch(flag)
      {
      case 'b':
        args->messStart = atobytes(argptr);
        break;
      case 'e':
        args->messStop = atobytes(argptr);
        break;
      case 'i':
        args->iters = atoi(argptr);
        iters_set = 1;
        break;
      case 'm':
	args->memLimit = atobytes(argptr);
        break;
      case 't':
        args->timeLimit = (double) atol(argptr);
        time_set = 1;
        break;
      case 'd':
        args->ndims = atoi(argptr);
        break;
      case 'p':
        args->partSize = atoi(argptr);
        break;
      default:
        return(0);
      }
    }

    /* if the user gave no iteration limit and no time limit, set a reasonable time limit */
    if (!iters_set && !time_set) { args->timeLimit = 50000; }

    /* turn on test flags requested by user
       if user doesn't specify any, all will be run */
    for(j=0; j<NUM_TESTS; j++) {
      if(!strcmp(TEST_NAMES[j], argv[i])) {
        if(args->testFlags == 0x1FFFFF) args->testFlags = 0;
        args->testFlags |= TEST_FLAGS[j];
      }
    }
  }

  if (args->iters == 0)
  {
    printf("\n  Must define number of operations per measurement!\n\n");
    return(0);
  }

  return(1);
}

struct collParams {
    size_t   size;     /* message (element) size in bytes */
    int      iter;     /* number of iterations to test with */
    int      root;     /* root of collective operation */
    MPI_Comm comm;     /* communicator to test collective on */
    int      myrank;   /* my rank in the above communicator */
    int      nranks;   /* number of ranks in the above communicator */
    int      count;    /* element count for collective */
    MPI_Datatype type; /* MPI_Datatype to be used in collective (assumed contiguous) */
    MPI_Op   reduceop; /* MPI_Reduce operation to be used */
};

double time_barrier(struct collParams* p)
{
    int i;
    MPI_Barrier(MPI_COMM_WORLD);

    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        MPI_Barrier(p->comm);
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

double time_bcast(struct collParams* p)
{
    int i;
    char* buffer = (p->myrank == p->root) ? sbuffer : rbuffer;
    MPI_Barrier(MPI_COMM_WORLD);

    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        int check = (check_every || (check_once && i == p->iter-1));
        if (check) {
          init_sbuffer(p->myrank, sbuffer, buffer_size);
        }

        MPI_Bcast(buffer, p->count, p->type, p->root, p->comm);
        __BAR__(p->comm);

        if (check) {
            check_sbuffer(p->myrank, sbuffer, buffer_size);
            check_rbuffer(buffer, 0, p->root, 0, p->size, rank_local);
        }
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

double time_alltoall(struct collParams* p)
{
    int i, j;
    MPI_Barrier(MPI_COMM_WORLD);

    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        int check = (check_every || (check_once && i == p->iter-1));
        if (check) {
            init_sbuffer(p->myrank, sbuffer, buffer_size);
        }

        MPI_Alltoall(sbuffer, p->count, p->type, rbuffer, p->size, p->type, p->comm);
        __BAR__(p->comm);

        if (check) {
            check_sbuffer(p->myrank, sbuffer, buffer_size);
            for (j = 0; j < p->nranks; j++) {
                check_rbuffer(rbuffer, j*p->size, j, p->myrank*p->size, p->size, rank_local);
            }
        }
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

double time_alltoallv(struct collParams* p)
{
    int i, j, k, count;
    int disp = 0;
    int chunksize = p->count / p->nranks;
    if (chunksize == 0) { chunksize = 1; }
    for (i = 0; i < p->nranks; i++) {
        int count = ((i+p->myrank)*chunksize) % (p->count+1);
        sendcounts[i] = count;
        recvcounts[i] = count;
        sdispls[i] = disp;
        rdispls[i] = disp;
        disp += count;
    }
    size_t scale = (p->count > 0) ? (p->size/p->count) : 0;
    MPI_Barrier(MPI_COMM_WORLD);

    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        int check = (check_every || (check_once && i == p->iter-1));
        if (check) {
            init_sbuffer(p->myrank, sbuffer, buffer_size);
        }

        MPI_Alltoallv(sbuffer, sendcounts, sdispls, p->type, rbuffer, recvcounts, rdispls, p->type, p->comm);
        __BAR__(p->comm);

        if (check) {
            check_sbuffer(p->myrank, sbuffer, buffer_size);
            for (k = 0; k < p->nranks; k++) {
                disp = 0;
                for (j = 0; j < p->myrank; j++) { disp += ((j+k)*chunksize) % (p->size+1); }
                check_rbuffer(rbuffer, rdispls[k]*scale, k, disp, recvcounts[k]*scale, rank_local);
            }
        }
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

double time_allgather(struct collParams* p)
{
    int i, j;
    MPI_Barrier(MPI_COMM_WORLD);

    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        int check = (check_every || (check_once && i == p->iter-1));
        if (check) {
            init_sbuffer(p->myrank, sbuffer, buffer_size);
        }

        MPI_Allgather(sbuffer, p->count, p->type, rbuffer, p->count, p->type, p->comm);
        __BAR__(p->comm);

        if (check) {
            check_sbuffer(p->myrank, sbuffer, buffer_size);
            for (j = 0; j < p->nranks; j++) {
                check_rbuffer(rbuffer, j*p->size, j, 0, p->size, rank_local);
            }
        }
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

double time_allgatherv(struct collParams* p)
{
    int i, j, count;
    int disp = 0;
    int chunksize = p->count / p->nranks;
    if (chunksize == 0) { chunksize = 1; }
    for ( i = 0; i < p->nranks; i++) {
        int count = (i*chunksize) % (p->count+1);
        recvcounts[i] = count;
        rdispls[i] = disp;
        disp += count;
    }
    size_t scale = (p->count > 0) ? (p->size/p->count) : 0;
    MPI_Barrier(MPI_COMM_WORLD);

    count = (p->myrank*chunksize) % (p->count+1);
    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        int check = (check_every || (check_once && i == p->iter-1));
        if (check) {
            init_sbuffer(p->myrank, sbuffer, buffer_size);
        }

        MPI_Allgatherv(sbuffer, count, p->type, rbuffer, recvcounts, rdispls, p->type, p->comm);
        __BAR__(p->comm);

        if (check) {
            check_sbuffer(p->myrank, sbuffer, buffer_size);
            for (j = 0; j < p->nranks; j++) {
                check_rbuffer(rbuffer, rdispls[j]*scale, j, 0, recvcounts[j]*scale, rank_local);
            }
        }
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

double time_gather(struct collParams* p)
{
    int i, j;
    MPI_Barrier(MPI_COMM_WORLD);

    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        int check = (check_every || (check_once && i == p->iter-1));
        if (check) {
            init_sbuffer(p->myrank, sbuffer, buffer_size);
        }

        MPI_Gather(sbuffer, p->count, p->type, rbuffer, p->count, p->type, p->root, p->comm);
        __BAR__(p->comm);

        if (check) {
            check_sbuffer(p->myrank, sbuffer, buffer_size);
            if (p->myrank == p->root) {
                for (j = 0; j < p->nranks; j++) {
                    check_rbuffer(rbuffer, j*p->size, j, 0, p->size, rank_local);
                }
            }
        }
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

double time_gatherv(struct collParams* p)
{
    int i, j, count;
    int disp = 0;
    int chunksize = p->count / p->nranks;
    if (chunksize == 0) { chunksize = 1; }
    for ( i = 0; i < p->nranks; i++) {
        int count = (i*chunksize) % (p->count+1);
        recvcounts[i] = count;
        rdispls[i] = disp;
        disp += count;
    }
    size_t scale = (p->count > 0) ? (p->size/p->count) : 0;
    MPI_Barrier(MPI_COMM_WORLD);

    count = (p->myrank*chunksize) % (p->count+1);
    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        int check = (check_every || (check_once && i == p->iter-1));
        if (check) {
            init_sbuffer(p->myrank, sbuffer, buffer_size);
        }

        MPI_Gatherv(sbuffer, count, p->type, rbuffer, recvcounts, rdispls, p->type, p->root, p->comm);
        __BAR__(p->comm);

        if (check) {
            check_sbuffer(p->myrank, sbuffer, buffer_size);
            if (p->myrank == p->root) {
                for (j = 0; j < p->nranks; j++) {
                    check_rbuffer(rbuffer, rdispls[j]*scale, j, 0, recvcounts[j]*scale, rank_local);
                }
            }
        }
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

double time_scatter(struct collParams* p)
{
    int i;
    MPI_Barrier(MPI_COMM_WORLD);

    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        int check = (check_every || (check_once && i == p->iter-1));
        if (check) {
            init_sbuffer(p->myrank, sbuffer, buffer_size);
        }

        MPI_Scatter(sbuffer, p->count, p->type, rbuffer, p->count, p->type, p->root, p->comm);
        __BAR__(p->comm);

        if (check) {
            check_sbuffer(p->myrank, sbuffer, buffer_size);
            check_rbuffer(rbuffer, 0, p->root, p->myrank*p->size, p->size, rank_local);
        }
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

double time_allreduce(struct collParams* p)
{
    int i;
    MPI_Barrier(MPI_COMM_WORLD);

    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        MPI_Allreduce(sbuffer, rbuffer, p->count, p->type, p->reduceop, p->comm);
        __BAR__(p->comm);
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

double time_reduce(struct collParams* p)
{
    int i;
    MPI_Barrier(MPI_COMM_WORLD);

    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        MPI_Reduce(sbuffer, rbuffer, p->count, p->type, p->reduceop, p->root, p->comm);
        __BAR__(p->comm);
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

#if HAVE_NBC == 1
double time_ibcast(struct collParams* p)
{
    int i;
    char* buffer = (p->myrank == p->root) ? sbuffer : rbuffer;
    MPI_Barrier(MPI_COMM_WORLD);

    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        int check = (check_every || (check_once && i == p->iter-1));
        if (check) {
          init_sbuffer(p->myrank, sbuffer, buffer_size);
        }

        MPI_Ibcast(buffer, p->count, p->type, p->root, p->comm, &request);
        MPI_Wait(&request, &status);

        __BAR__(p->comm);

        if (check) {
            check_sbuffer(p->myrank, sbuffer, buffer_size);
            check_rbuffer(buffer, 0, p->root, 0, p->size, rank_local);
        }
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

double time_ialltoall(struct collParams* p)
{
    int i, j;
    MPI_Barrier(MPI_COMM_WORLD);

    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        int check = (check_every || (check_once && i == p->iter-1));
        if (check) {
            init_sbuffer(p->myrank, sbuffer, buffer_size);
        }

        MPI_Ialltoall(sbuffer, p->count, p->type, rbuffer, p->size, p->type, p->comm, &request);
        MPI_Wait(&request, &status);

        __BAR__(p->comm);

        if (check) {
            check_sbuffer(p->myrank, sbuffer, buffer_size);
            for (j = 0; j < p->nranks; j++) {
                check_rbuffer(rbuffer, j*p->size, j, p->myrank*p->size, p->size, rank_local);
            }
        }
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

double time_ialltoallv(struct collParams* p)
{
    int i, j, k, count;
    int disp = 0;
    int chunksize = p->count / p->nranks;
    if (chunksize == 0) { chunksize = 1; }
    for (i = 0; i < p->nranks; i++) {
        int count = ((i+p->myrank)*chunksize) % (p->count+1);
        sendcounts[i] = count;
        recvcounts[i] = count;
        sdispls[i] = disp;
        rdispls[i] = disp;
        disp += count;
    }
    size_t scale = (p->count > 0) ? (p->size/p->count) : 0;
    MPI_Barrier(MPI_COMM_WORLD);

    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        int check = (check_every || (check_once && i == p->iter-1));
        if (check) {
            init_sbuffer(p->myrank, sbuffer, buffer_size);
        }

        MPI_Ialltoallv(sbuffer, sendcounts, sdispls, p->type, rbuffer, recvcounts, rdispls, p->type, p->comm, &request);
        MPI_Wait(&request, &status);
        __BAR__(p->comm);

        if (check) {
            check_sbuffer(p->myrank, sbuffer, buffer_size);
            for (k = 0; k < p->nranks; k++) {
                disp = 0;
                for (j = 0; j < p->myrank; j++) { disp += ((j+k)*chunksize) % (p->size+1); }
                check_rbuffer(rbuffer, rdispls[k]*scale, k, disp, recvcounts[k]*scale, rank_local);
            }
        }
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

double time_iallgather(struct collParams* p)
{
    int i, j;
    MPI_Barrier(MPI_COMM_WORLD);

    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        int check = (check_every || (check_once && i == p->iter-1));
        if (check) {
            init_sbuffer(p->myrank, sbuffer, buffer_size);
        }

        MPI_Iallgather(sbuffer, p->count, p->type, rbuffer, p->count, p->type, p->comm, &request);
        MPI_Wait(&request, &status);
        __BAR__(p->comm);

        if (check) {
            check_sbuffer(p->myrank, sbuffer, buffer_size);
            for (j = 0; j < p->nranks; j++) {
                check_rbuffer(rbuffer, j*p->size, j, 0, p->size, rank_local);
            }
        }
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

double time_iallgatherv(struct collParams* p)
{
    int i, j, count;
    int disp = 0;
    int chunksize = p->count / p->nranks;
    if (chunksize == 0) { chunksize = 1; }
    for ( i = 0; i < p->nranks; i++) {
        int count = (i*chunksize) % (p->count+1);
        recvcounts[i] = count;
        rdispls[i] = disp;
        disp += count;
    }
    size_t scale = (p->count > 0) ? (p->size/p->count) : 0;
    MPI_Barrier(MPI_COMM_WORLD);

    count = (p->myrank*chunksize) % (p->count+1);
    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        int check = (check_every || (check_once && i == p->iter-1));
        if (check) {
            init_sbuffer(p->myrank, sbuffer, buffer_size);
        }

        MPI_Iallgatherv(sbuffer, count, p->type, rbuffer, recvcounts, rdispls, p->type, p->comm, &request);
        MPI_Wait(&request, &status);
        __BAR__(p->comm);

        if (check) {
            check_sbuffer(p->myrank, sbuffer, buffer_size);
            for (j = 0; j < p->nranks; j++) {
                check_rbuffer(rbuffer, rdispls[j]*scale, j, 0, recvcounts[j]*scale, rank_local);
            }
        }
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

double time_igather(struct collParams* p)
{
    int i, j;
    MPI_Barrier(MPI_COMM_WORLD);

    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        int check = (check_every || (check_once && i == p->iter-1));
        if (check) {
            init_sbuffer(p->myrank, sbuffer, buffer_size);
        }

        MPI_Igather(sbuffer, p->count, p->type, rbuffer, p->count, p->type, p->root, p->comm, &request);
        MPI_Wait(&request, &status);
        __BAR__(p->comm);

        if (check) {
            check_sbuffer(p->myrank, sbuffer, buffer_size);
            if (p->myrank == p->root) {
                for (j = 0; j < p->nranks; j++) {
                    check_rbuffer(rbuffer, j*p->size, j, 0, p->size, rank_local);
                }
            }
        }
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

double time_igatherv(struct collParams* p)
{
    int i, j, count;
    int disp = 0;
    int chunksize = p->count / p->nranks;
    if (chunksize == 0) { chunksize = 1; }
    for ( i = 0; i < p->nranks; i++) {
        int count = (i*chunksize) % (p->count+1);
        recvcounts[i] = count;
        rdispls[i] = disp;
        disp += count;
    }
    size_t scale = (p->count > 0) ? (p->size/p->count) : 0;
    MPI_Barrier(MPI_COMM_WORLD);

    count = (p->myrank*chunksize) % (p->count+1);
    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        int check = (check_every || (check_once && i == p->iter-1));
        if (check) {
            init_sbuffer(p->myrank, sbuffer, buffer_size);
        }

        MPI_Igatherv(sbuffer, count, p->type, rbuffer, recvcounts, rdispls, p->type, p->root, p->comm, &request);
        MPI_Wait(&request, &status);
        __BAR__(p->comm);

        if (check) {
            check_sbuffer(p->myrank, sbuffer, buffer_size);
            if (p->myrank == p->root) {
                for (j = 0; j < p->nranks; j++) {
                    check_rbuffer(rbuffer, rdispls[j]*scale, j, 0, recvcounts[j]*scale, rank_local);
                }
            }
        }
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

double time_iscatter(struct collParams* p)
{
    int i;
    MPI_Barrier(MPI_COMM_WORLD);

    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        int check = (check_every || (check_once && i == p->iter-1));
        if (check) {
            init_sbuffer(p->myrank, sbuffer, buffer_size);
        }

        MPI_Iscatter(sbuffer, p->count, p->type, rbuffer, p->count, p->type, p->root, p->comm, &request);
        MPI_Wait(&request, &status);
        __BAR__(p->comm);

        if (check) {
            check_sbuffer(p->myrank, sbuffer, buffer_size);
            check_rbuffer(rbuffer, 0, p->root, p->myrank*p->size, p->size, rank_local);
        }
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

double time_iallreduce(struct collParams* p)
{
    int i;
    MPI_Barrier(MPI_COMM_WORLD);

    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        MPI_Iallreduce(sbuffer, rbuffer, p->count, p->type, p->reduceop, p->comm, &request);
        MPI_Wait(&request, &status);
        __BAR__(p->comm);
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}

double time_ireduce(struct collParams* p)
{
    int i;
    MPI_Barrier(MPI_COMM_WORLD);

    __TIME_START__;
    for (i = 0; i < p->iter; i++) {
        MPI_Ireduce(sbuffer, rbuffer, p->count, p->type, p->reduceop, p->root, p->comm, &request);
        MPI_Wait(&request, &status);
        __BAR__(p->comm);
    }
    __TIME_END__;

    return __TIME_USECS__ / (double)p->iter;
}
#endif /*  HAVE_NBC  */

/* Prime, estimate, and time the collective called by the specified function
   for the given message size, iteration count, and time limit.  Then, print
   out the results.
*/
double get_time(double (*fn)(struct collParams* p), char* title, struct collParams* p, int iter, int time_limit)
{
    double time;
    double time_avg;
    int iter_limit;

    /* Initialize buffer only for non-reduce operations
     * Reduce operations use MPI_DOUBLE, not the char pattern from init_sbuffer
     * Initializing with char pattern can cause type mismatches and segfaults on GPU
     */
    if (p->type != MPI_DOUBLE) {
        init_sbuffer(p->myrank, sbuffer, buffer_size);
    }

    /* prime the collective with an intial call */
    p->iter = 1;
    time = fn(p);

    /* run through a small number of iterations to get a rough estimate of time */
    p->iter = ITRS_EST;
    time = fn(p);

    /* if a time limit has been specified, use the esitmate to limit the maximum number of iterations */
    iter_limit = (time_limit > 0   ) ? (int) (time_limit / time) : iter;
    iter_limit = (iter_limit < iter) ? iter_limit : iter;

    /* use the number calculated by the root (rank 0) which should be the slowest */
    MPI_Bcast(&iter_limit, 1, MPI_INT, 0, MPI_COMM_WORLD);

    /* run the tests (unless the limited iteration count is smaller than that used in the estimate) */
    if(iter_limit > ITRS_EST) {
        p->iter = iter_limit;
        time = fn(p);
    } else {
        iter_limit = ITRS_EST;
    }

    /* Collect and print the timing results recorded by each process */
    Print_Timings(time, title, p->size, iter_limit, p->comm);

    return time;
}

int main (int argc, char *argv[])
{
    int err;
    double time, time_limit, time_maxMsg;

    int iter, iter_limit;
    size_t size, messStart, messStop, mem_limit;
    int testFlags, ndims, partsize;
    int k;

    char  hostname[256];
    char* hostnames;

    int root = 0;

    struct argList args;
    /* process the command-line arguments, printing usage info on error */
    if (!processMPIBArgs(argc, argv, &args)) { usage(); }
    iter        = args.iters;
    messStart   = args.messStart;
    messStop    = args.messStop;
    mem_limit   = args.memLimit;
    time_limit  = args.timeLimit;
    testFlags   = args.testFlags;
    check_once  = args.checkOnce;
    check_every = args.checkEvery;
    ndims       = args.ndims;
    partsize    = args.partSize;

#if defined(ENABLE_HIP) || defined(ENABLE_CUDA)
    err = init_gpu();
    if (err) { printf("Error in GPU device initialization\n"); exit(1); }
    init_allocator("DEVICE");
#else
    init_allocator("HOST");
#endif

    /* initialize MPI */
    err = MPI_Init(&argc, &argv);
    if (err) { printf("Error in MPI_Init\n"); exit(1); }

    /* determine who we are in the MPI world */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_local);
    MPI_Comm_size(MPI_COMM_WORLD, &rank_count);

    /* mark start of mpiBench output */
    if (rank_local == 0) { printf("START mpiBench %s v%d.%d.%d\n", VERS, majorVersion, minorVersion,
             patchVersion); }

    /* TODO [#4]: Parameterize this and change it to write out to a file because this implementation
    does not scale. Also factor this out into util.c. */

    /* collect hostnames of all the processes and print rank layout */
    // gethostname(hostname, sizeof(hostname));
    // hostnames = (char*) _ALLOC_MAIN_(sizeof(hostname)*rank_count, "Hostname array");
    // MPI_Gather(hostname, sizeof(hostname), MPI_CHAR, hostnames, sizeof(hostname), MPI_CHAR, 0, MPI_COMM_WORLD);
    // if (rank_local == 0) {
    //     for(k=0; k<rank_count; k++) {
    //         printf("%d : %s\n", k, &hostnames[k*sizeof(hostname)]);
    //     }
    // }

    if ((testFlags & ALLTOALL) || \
        (testFlags & IALLTOALL) || \
        (testFlags & ALLTOALLV) || \
        (testFlags & IALLTOALLV) || \
        (testFlags & ALLGATHER) || \
        (testFlags & IALLGATHER) || \
        (testFlags & ALLGATHERV) || \
        (testFlags & IALLGATHERV) || \
        (testFlags & GATHER) || \
        (testFlags & IGATHER) || \
        (testFlags & GATHERV) || \
        (testFlags & IGATHERV) || \
        (testFlags & SCATTER) || \
        (testFlags & ISCATTER)) {
        while(messStop*((size_t)rank_count)*2 > mem_limit && messStop > 0) messStop /= 2;
        buffer_size = messStop * rank_count;
    } else {
    buffer_size = messStop;
    }

    alloc_buffer((void **) &sbuffer, buffer_size, "Send Buffer");
    alloc_buffer((void **) &rbuffer, buffer_size, "Receive Buffer");
    sendcounts = (int*) _ALLOC_MAIN_(sizeof(int) * rank_count, "Send Counts");
    sdispls    = (int*) _ALLOC_MAIN_(sizeof(int) * rank_count, "Send Displacements");
    recvcounts = (int*) _ALLOC_MAIN_(sizeof(int) * rank_count, "Recv Counts");
    rdispls    = (int*) _ALLOC_MAIN_(sizeof(int) * rank_count, "Recv Displacements");
    time_maxMsg = 2*time_limit;

    /* if partsize was specified, calculate the number of partions we need */
    int partitions = 0;
    if (partsize > 0) {
        /* keep dividing comm in half until we get to partsize */
        int currentsize = rank_count;
        while (currentsize >= partsize) {
            partitions++;
            currentsize >>= 1;
        }
    }

    /* set up communicators */
    int total_comms = 1+ndims+partitions;
    int current_comm = 0;
    int extra_state;
    MPI_Keyval_create(MPI_NULL_COPY_FN, MPI_NULL_DELETE_FN, &dimid_key, (void*) &extra_state);
    MPI_Comm* comms = (MPI_Comm*) _ALLOC_MAIN_(sizeof(MPI_Comm) * total_comms, "Communicator array");
    char* comm_desc = (char*)     _ALLOC_MAIN_(256              * total_comms, "Communicator description array");

    /* the first communicator is MPI_COMM_WORLD */
    comms[0]  = MPI_COMM_WORLD;
    strcpy(&comm_desc[256*current_comm], "MPI_COMM_WORLD");
    MPI_Attr_put(comms[0], dimid_key, (void*) &comm_desc[256*current_comm]);
    current_comm++;

    /* if ndims is specified, map MPI_COMM_WORLD into ndims Cartesian space, and create 1-D communicators along each dimension */
    int d;
    if (ndims > 0) {
        MPI_Comm comm_dims;
        int* dims    = (int*) _ALLOC_MAIN_(sizeof(int) * ndims, "Dimension array");
        int* periods = (int*) _ALLOC_MAIN_(sizeof(int) * ndims, "Period array");
        for (d=0; d < ndims; d++) {
            dims[d]    = 0; /* set dims[d]=non-zero if you want to explicitly specify the number of processes in this dimension */
            periods[d] = 0; /* set period[d]=1 if you want this dimension to be periodic */
        }

        /*
        given the total number of processes, and the number of dimensions,
        fill in dims with the number of processes along each dimension (split as evenly as possible)
        */
        MPI_Dims_create(rank_count, ndims, dims);

        /*
        then create a cartesian communicator,
        which we'll use to split into ndims 1-D communicators along each dimension
        */
        MPI_Cart_create(MPI_COMM_WORLD, ndims, dims, periods, 0, &comm_dims);

        /*
        split cartesian communicator into ndims 1-D subcommunicators
        (for MPI_Cart_sub, set dims[d]=1 if you want that dimension to remain in subcommunicator)
        */
        int d2;
        for (d=0; d < ndims; d++) {
            for (d2=0; d2 < ndims; d2++) {
                if (d == d2) dims[d2] = 1;
                else         dims[d2] = 0;
            }
            MPI_Cart_sub(comm_dims, dims, &comms[current_comm]);
            sprintf(&comm_desc[256*current_comm], "CartDim-%dof%d", d+1, ndims);
            MPI_Attr_put(comms[current_comm], dimid_key, (void*) &comm_desc[256*current_comm]);
            current_comm++;
        }
    }

    /* if a partsize is specified, recursively divide MPI_COMM_WORLD in half until groups reach partsize */
    int currentsize = rank_count;
    int p = 0;
    while (p < partitions) {
        int partnum = (int) rank_local / currentsize;
        MPI_Comm_split(MPI_COMM_WORLD, partnum, rank_local, &comms[current_comm]);
        sprintf(&comm_desc[256*current_comm], "PartSize-%d", currentsize);
        MPI_Attr_put(comms[current_comm], dimid_key, (void*) &comm_desc[256*current_comm]);
        current_comm++;
        currentsize >>= 1;
        p++;
    }

    /* for each communicator, run collective tests */
    for (d=0; d < total_comms; d++) {
        MPI_Comm comm = comms[d];

        /* determine who we are in this communicator */
        int myrank, nranks;
        MPI_Comm_rank(comm, &myrank);
        MPI_Comm_size(comm, &nranks);

        struct collParams p;
        p.root   = 0;
        p.comm   = comm;
        p.myrank = myrank;
        p.nranks = nranks;
        p.type   = MPI_BYTE;

        /* time requested collectives */
        if(testFlags & BARRIER) {
            p.size = 0;
            p.count = 0;
            get_time(time_barrier, "Barrier", &p, iter, time_limit);
        }

        if(testFlags & BCAST) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                p.count = p.size;
                if(get_time(time_bcast, "Bcast", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }

        if(testFlags & ALLTOALL) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                p.count = p.size;
                if(get_time(time_alltoall, "Alltoall", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }

        if(testFlags & ALLTOALLV) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                p.count = p.size;
                if(get_time(time_alltoallv, "Alltoallv", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }

        if(testFlags & ALLGATHER) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                p.count = p.size;
                if(get_time(time_allgather, "Allgather", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }

        if(testFlags & ALLGATHERV) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                p.count = p.size;
                if(get_time(time_allgatherv, "Allgatherv", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }

        if(testFlags & GATHER) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                p.count = p.size;
                if(get_time(time_gather, "Gather", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }

        if(testFlags & GATHERV) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                p.count = p.size;
                if(get_time(time_gatherv, "Gatherv", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }

        if(testFlags & SCATTER) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                p.count = p.size;
                if(get_time(time_scatter, "Scatter", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }

#if HAVE_NBC == 1
        if(testFlags & IBCAST) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                p.count = p.size;
                if(get_time(time_ibcast, "Ibcast", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }

        if(testFlags & IALLTOALL) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                p.count = p.size;
                if(get_time(time_ialltoall, "Ialltoall", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }

        if(testFlags & IALLTOALLV) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                p.count = p.size;
                if(get_time(time_ialltoallv, "Ialltoallv", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }

        if(testFlags & IALLGATHER) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                p.count = p.size;
                if(get_time(time_iallgather, "Iallgather", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }

        if(testFlags & IALLGATHERV) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                p.count = p.size;
                if(get_time(time_iallgatherv, "Iallgatherv", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }

        if(testFlags & IGATHER) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                p.count = p.size;
                if(get_time(time_igather, "Igather", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }

        if(testFlags & IGATHERV) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                p.count = p.size;
                if(get_time(time_igatherv, "Igatherv", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }

        if(testFlags & ISCATTER) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                p.count = p.size;
                if(get_time(time_iscatter, "Iscatter", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }
#endif /*  HAVE NBC  */

        /* For the reductions, we use MPI_DOUBLE instead of MPI_BYTE
         * Note: init_sbuffer is skipped in get_time() for reduce operations
         * to avoid type mismatch between char pattern and double data
         */
        p.type     = MPI_DOUBLE;
        p.reduceop = MPI_SUM;

        if(testFlags & ALLREDUCE) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                if(p.size < sizeof(double)) continue;
                p.count = p.size / sizeof(double);
                if(get_time(time_allreduce, "Allreduce", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }

        if(testFlags & REDUCE) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                if(p.size < sizeof(double)) continue;
                p.count = p.size / sizeof(double);
                if(get_time(time_reduce, "Reduce", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }

#if HAVE_NBC == 1
        if(testFlags & IALLREDUCE) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                if(p.size < sizeof(double)) continue;
                p.count = p.size / sizeof(double);
                if(get_time(time_iallreduce, "Iallreduce", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }

        if(testFlags & IREDUCE) {
            for(p.size = messStart; p.size <= messStop; p.size = (p.size > 0) ? p.size << 1 : 1) {
                if(p.size < sizeof(double)) continue;
                p.count = p.size / sizeof(double);
                if(get_time(time_ireduce, "Ireduce", &p, iter, time_limit) > time_maxMsg && time_maxMsg > 0.0) break;
            }
        }
#endif /*  HAVE NBC  */
    } /* end loop over communicators */

    /* print memory usage */
    if (rank_local == 0) {
        printf("Message buffers (KB):\t%ld\n", allocated_memory/1024);
    }

    /* mark end of output */
    if (rank_local == 0) { printf("END mpiBench\n"); }

    /* shut down */
    free_buffer(sbuffer);
    free_buffer(rbuffer);
#if defined(ENABLE_HIP) || defined(ENABLE_CUDA)
    /* Clean up persistent GPU error tracking buffers */
    cleanup_allocator();
#endif
    MPI_Finalize();
    return 0;
}
