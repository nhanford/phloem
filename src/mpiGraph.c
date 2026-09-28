/*
mpiGraph.c

Description:
 * Typically, one MPI task is run per node (or per interconnect link).  For a job of
 * N MPI tasks, the N tasks are logically arranged in a ring counting ranks from 0 and
 * increasing to the right, at the end rank 0 is to the right of rank N-1.  Then a
 * series of N-1 steps are executed.  In each step, each MPI task sends to the task D
 * units to the right and simultaneously receives from the task D units to the left.
 * The value of D starts at 1 and runs to N-1, so that by the end of the N-1 steps,
 * each task has sent to and received from every other task in the run, excluding itself.
 * At the end of the run, two NxN matrices of bandwidths are gathered and reported to
 * stdout -- one for send bandwidths and one for receive bandwidths.

Author: Adam Moody

Copyright (c) 2007-2008 Lawrence Livermore National Security (LLNS), LLC.
See COPYRIGHT file for more details.

SPDX-License-Identifier: MIT
See LICENSE file for more details
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include </usr/include/sys/resource.h>
#include "util.h"
/*#include "/usr/global/tools/tests/include/print_mpi_resources.h"*/
char  hostname[256];
char* hostnames;

/* =============================================================
 * TIMER MACROS
 * These macros start/stop the timer and measure the difference
 * =============================================================
 */
#ifdef USE_GETTIMEOFDAY
/* use gettimeofday() for timers */

#include <sys/time.h>
#define __TIME_START__    (gettimeofday(&g_timeval__start, &g_timezone))
#define __TIME_END_SEND__ (gettimeofday(&g_timeval__end_send, &g_timezone))
#define __TIME_END_RECV__ (gettimeofday(&g_timeval__end_recv, &g_timezone))
#define __TIME_USECS_SEND__ (d_Time_Diff_Micros(g_timeval__start, g_timeval__end))
#define __TIME_USECS_RECV__ (d_Time_Diff_Micros(g_timeval__start, g_timeval__end_recv))
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
struct timeval  g_timeval__start, g_timeval__end_send, g_timeval__end_recv;
struct timezone g_timezone;

#else
/* use MPI_Wtime() for timers instead of gettimeofday() (recommended)
 * on some systems gettimeofday may reset backwards via some global clock,
 * which leads to incorrect timing data including negative time periods
 */

#define __TIME_START__    (g_timeval__start    = MPI_Wtime())
#define __TIME_END_SEND__ (g_timeval__end_send = MPI_Wtime())
#define __TIME_END_RECV__ (g_timeval__end_recv = MPI_Wtime())
#define __TIME_USECS_SEND__ ((g_timeval__end_send - g_timeval__start) * 1000000.0)
#define __TIME_USECS_RECV__ ((g_timeval__end_recv - g_timeval__start) * 1000000.0)
double g_timeval__start, g_timeval__end_send, g_timeval__end_recv;

#endif /* of USE_GETTIMEOFDAY */


/* =============================================================
 * MAIN TIMING LOGIC
 * Uses a ring-based (aka. shift-based) algorithm.
 * 1) First, logically arrange the MPI tasks
 *    from left to right from rank 0 to rank N-1 in a circular array.
 * 2) Then, during each step, each task sends messages to the task D uints to the right
 *    and receives from task D units to the left.
 *    In each step, each tasks measures its send and receive bandwidths.
 * 3) There are N-1 such steps so that each task has sent to and received from every task.
 * =============================================================
 */
void code(int mypid, int nnodes, int size, int times, int window)
{
  /* arguments are:
   *   mypid  = rank of this process
   *   nnodes = number of ranks
   *   size   = message size in bytes
   *   times  = number of times to measure bandwidth between task pairs
   *   window = number of outstanding sends and recvs to a single rank
   */
  int i, j, k, w;

  /* allocate memory for all of the messages */
  char* send_message = (char*) malloc(window*size);
  char* recv_message = (char*) malloc(window*size);
  MPI_Status*  status_array  = (MPI_Status*)  malloc(sizeof(MPI_Status) *window*2);
  MPI_Request* request_array = (MPI_Request*) malloc(sizeof(MPI_Request)*window*2);
  double* sendtimes = (double*) malloc(sizeof(double)*times*nnodes);
  double* recvtimes = (double*) malloc(sizeof(double)*times*nnodes);

  int* message_tags = (int*) malloc(window*sizeof(int));
  for (i=0;i<window;i++) { message_tags[i] = i; }

  /* start iterating over distance */
  int distance = 1;
  while (distance < nnodes) {
    /* this test can run for a long time, so print progress to screen as we go */
    float progress = (float) distance / (float) nnodes * 100.0;
    if (mypid == 0) {
      printf("%d of %d (%0.1f%%)\n", distance, nnodes, progress);
      fflush(stdout);
    }

    /* find tasks distance units to the right (send) and left (recv) */
    int sendpid = (mypid + distance + nnodes) % nnodes;
    int recvpid = (mypid - distance + nnodes) % nnodes;

    /* run through 'times' iterations on a given ring */
    for (i=0; i<times; i++) {
      /* couple of synch's to make sure everyone is ready to go */
      MPI_Barrier(MPI_COMM_WORLD);
      MPI_Barrier(MPI_COMM_WORLD);

      __TIME_START__;
      k=-1;
      /* setup a window of irecvs from my partner who is distance steps to my left */
      for (w=0; w<window; w++) {
        k=k+1;
        MPI_Irecv(&recv_message[w*size], size, MPI_BYTE,
                  recvpid, MPI_ANY_TAG, MPI_COMM_WORLD, &request_array[k]);
      }
      /* fire off a window of isends to my send partner distance steps to my right */
      for (w=0; w<window; w++) {
        k=k+1;
        MPI_Isend(&send_message[w*size], size, MPI_BYTE,
                  sendpid, message_tags[w], MPI_COMM_WORLD, &request_array[k]);
      }
      /* time sends and receives separately */
      int flag_sends = 0;
      int flag_recvs = 0;
      while(!flag_sends || !flag_recvs) {
        /* check whether the sends are done */
        if (!flag_sends) {
          MPI_Testall((k+1)/2, &request_array[(k+1)/2-1], &flag_sends, &status_array[(k+1)/2-1]);
          if (flag_sends) { __TIME_END_SEND__; }
        }

        /* check whether the recvs are done */
        if (!flag_recvs) {
          MPI_Testall((k+1)/2, &request_array[0], &flag_recvs, &status_array[0]);
          if (flag_recvs) { __TIME_END_RECV__; }
        }
      }
      sendtimes[sendpid*times+i] = __TIME_USECS_SEND__ / (double) w;
      recvtimes[recvpid*times+i] = __TIME_USECS_RECV__ / (double) w;
    } /* end times loop */
    /* bump up the distance for the next ring step */
    distance++;
  } /* end distance loop */

  /* for each node, compute sum of my bandwidths with that node */
  if(mypid == 0) printf("Gathering results\n");
  double* sendsums = (double*) malloc(sizeof(double)*nnodes);
  double* recvsums = (double*) malloc(sizeof(double)*nnodes);
  for(j=0; j<nnodes; j++) {
    sendsums[j] = 0.0;
    recvsums[j] = 0.0;
    if (j == mypid) continue;
    for(i=0; i<times; i++) {
      double sendval = sendtimes[j*times+i];
      double recvval = recvtimes[j*times+i];
      sendsums[j] += sendval;
      recvsums[j] += recvval;
    }
  }

  /* gather send bw sums to rank 0 */
  double* allsums;
  if (mypid == 0) {
    allsums = (double*) malloc(sizeof(double)*nnodes*nnodes);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Gather(sendsums, nnodes, MPI_DOUBLE, allsums, nnodes, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  /* rank 0 computes send stats and prints result */
  if (mypid == 0) {
    /* compute stats over all nodes */
    double sendsum = 0.0;
    double sendmin = 10000000000000000.0;
    double MBsec   = ((double)(size)) * 1000000.0 / (1024.0*1024.0);
    for(j=0; j<nnodes; j++) {
      for(k=0; k<nnodes; k++) {
        if (j == k) continue;
        double sendval = allsums[j*nnodes+k];
        sendsum += sendval;
        sendmin = (sendval < sendmin) ? sendval : sendmin;
      }
    }

    /* print send stats */
    sendmin /= (double) times;
    sendsum /= (double) (nnodes)*(nnodes-1)*times;
    printf("\nSend max\t%f\n", MBsec/sendmin);
    printf("Send avg\t%f\n", MBsec/sendsum);

    /* print send bandwidth table */
    printf("\n");
    printf("Send\t");
    for(k=0; k<nnodes; k++) {
      printf("%s:%d\t", &hostnames[k*sizeof(hostname)], k);
    }
    printf("\n");
    for(j=0; j<nnodes; j++) {
      printf("%s:%d to\t", &hostnames[j*sizeof(hostname)], j);
      for(k=0; k<nnodes; k++) {
        double val = allsums[j*nnodes+k];
        if (val != 0.0) { val = MBsec * (double) times / val; }
        printf("%0.3f\t", val);
      }
      printf("\n");
    }
  }

  /* gather recv bw sums to rank 0 */
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Gather(recvsums, nnodes, MPI_DOUBLE, allsums, nnodes, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  /* rank 0 computes recv stats and prints result */
  if (mypid == 0) {
    /* compute stats over all nodes */
    double recvsum = 0.0;
    double recvmin = 10000000000000000.0;
    double MBsec   = ((double)(size)) * 1000000.0 / (1024.0*1024.0);
    for(j=0; j<nnodes; j++) {
      for(k=0; k<nnodes; k++) {
        if (j == k) continue;
        double recvval = allsums[j*nnodes+k];
        recvsum += recvval;
        recvmin = (recvval < recvmin) ? recvval : recvmin;
      }
    }

    /* print receive stats */
    recvmin /= (double) times;
    recvsum /= (double) (nnodes)*(nnodes-1)*times;
    printf("\nRecv max\t%f\n", MBsec/recvmin);
    printf("Recv avg\t%f\n", MBsec/recvsum);

    /* print receive bandwidth table */
    printf("\n");
    printf("Recv\t");
    for(k=0; k<nnodes; k++) {
      printf("%s:%d\t", &hostnames[k*sizeof(hostname)], k);
    }
    printf("\n");
    for(j=0; j<nnodes; j++) {
      printf("%s:%d from\t", &hostnames[j*sizeof(hostname)], j);
      for(k=0; k<nnodes; k++) {
        double val = allsums[j*nnodes+k];
        if (val != 0.0) { val = MBsec * (double) times / val; }
        printf("%0.3f\t", val);
      }
      printf("\n");
    }
  }

  /* free off memory */
  free(send_message);
  free(recv_message);
  free(status_array);
  free(request_array);
  free(sendtimes);
  free(recvtimes);
  free(message_tags);

  return;
}

/* =============================================================
 * MAIN DRIVER
 * Inits MPI, reads command-line parameters, and kicks off testing
 * =============================================================
 */
int main(int argc, char **argv)
{
  int rank, ranks, size, times, window;
  int args[3];

  /* start up */
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &ranks);

  /* collect hostnames of all the processes */
  gethostname(hostname, sizeof(hostname));
  hostnames = (char*) malloc(sizeof(hostname)*ranks);
  MPI_Gather(hostname, sizeof(hostname), MPI_CHAR, hostnames, sizeof(hostname), MPI_CHAR, 0, MPI_COMM_WORLD);

  /* set job parameters, read values from command line if they're there */
  size = 4096*4;
  times = 100;
  window = 50;
  if (argc == 4) {
    size   = atoi(argv[1]);
    times  = atoi(argv[2]);
    window = atoi(argv[3]);
  }
  args[0] = size;
  args[1] = times;
  args[2] = window;

  /* print the header */
  if (rank == 0) {
    /* mark start of output */
    printf("START mpiGraph v%d.%d.%d\n", majorVersion, minorVersion,
             patchVersion);
    printf("MsgSize\t%d\nTimes\t%d\nWindow\t%d\n",size,times,window);
    printf("Procs\t%d\n\n",ranks);
  }

  /* synchronize, then start the run */
  MPI_Barrier(MPI_COMM_WORLD);
  code(rank, ranks, size, times, window);
  MPI_Barrier(MPI_COMM_WORLD);

  /* print memory usage */
/*
  if(rank == 0) { printf("\n"); }
  print_mpi_resources();
*/

  /* mark end of output */
  if (rank == 0) { printf("END mpiGraph\n"); }

  /* shut down */
  MPI_Finalize();
  return 0;
}
