/*
util.h

Authors: Chris Chambreau and Nathan Hanford

Copyright (c) 2007-2026 Lawrence Livermore National Security (LLNS), LLC.
See COPYRIGHT file for more details.

SPDX-License-Identifier: MIT
See LICENSE file for more details
*/

#ifndef _UTIL_H
#define _UTIL_H

#include <limits.h>

#define KB_SIZE 1024
#define MB_SIZE (KB_SIZE * KB_SIZE)
#define GB_SIZE (MB_SIZE * KB_SIZE)

#define MESS_START_DEF 0
#define MESS_STOP_DEF ( MB_SIZE * 32 )

#define MESS_FACTOR_DEF 2
#define ALLOC_DEF 'b'
#define USE_BARRIER_DEF 0
#define PRINT_PAIRS_DEF 0
#define NEAREST_RANK_DEF 0
#define MIN_MESS_SIZE 0
#define MAX_MESS_SIZE INT_MAX
#define PRESTA_SEED 128

typedef struct
{

  int debugFlag;
  int iters;
  int samples;
  int messFactor;
  int messStart;
  long long messStop;
  int printHostInfo;
  int printPairs;
  int printRankTime;
  int procsPerNode;
  int useBarrier;
  int useNearestRank;
  char *procFile;
  char *messFile;
  char allocPattern;
  int *procList;
  int *testCountList;
  int procListSize;
  int *messList;
  int messListSize;
  int *iterList;
  char *testList;
  int sumLocalBW;
  int verbose;
  int allTasksOnly;

} ARGSTRUCT;


typedef struct
{
  unsigned int tasks;
  unsigned int msize;
  unsigned int iters;
  unsigned int samples;
  unsigned int count;
  double min;
  double tot;
  double mean;
  double max;

} STATSTRUCT;

typedef enum
{ PRESTA_OP_P2P, PRESTA_OP_COLL } PRESTA_OP_TYPE;

extern int rank, wsize;
extern char *targetFile;
extern int *targetArray;
extern int targetListSize;
extern char *targetDirectory;
extern ARGSTRUCT argStruct;
extern int majorVersion;
extern int minorVersion;
extern int patchVersion;
extern char procSrcTitle[256];
extern int presta_check_data;
extern unsigned long long presta_data_err_total;
extern long long presta_global_data_err_total;

int init_gpu();
void init_allocator(const char *type);
void alloc_buffer (void **ptr, size_t len, char *debug );
void free_buffer ( void *ptr );
void init_sbuffer(int rank, char* sbuffer, size_t buffer_size);
void check_sbuffer(int rank, char* sbuffer, size_t buffer_size);
void check_rbuffer(char* buffer, size_t byte_offset, int rank, size_t src_byte_offset, size_t element_count, int rank_local);

#if defined(ENABLE_HIP) || defined(ENABLE_CUDA)
/* CUDA kernel launcher functions - implemented in kernels.cu and kernels.cpp */
void launch_init_buffer_kernel(char* buffer, size_t size, int rank, int numBlocks, int blockSize);
void launch_check_buffer_kernel(char* buffer, size_t size, int rank,
                                 int* error_flag, size_t* error_index,
                                 int numBlocks, int blockSize);
void launch_check_rbuffer_kernel(char* buffer, size_t byte_offset, int rank,
                                  size_t src_byte_offset, size_t element_count,
                                  int* error_flag, size_t* error_index,
                                  int numBlocks, int blockSize);
void cleanup_allocator();
#endif
double getWtimeOh ( void );
void populateData ( char *, int );
/* Compile with -DNO_BARRIER to drop barriers between collective calls
     Default behavior adds barrier between test iterations to sync all procs before issuing next collective,
     which prevents non-root MPI ranks from escaping ahead into future iterations

   Compile with -DINCLUDE_BARRIER to include barrier operation in timing loop results
     Barrier overhead is not subtracted from timing results
     Default behavior is to exclude timing loop barrier from results

   Compile with -DUSE_MPI_BARRIER to use MPI implementation barrier instead of implementation-neutral barrier
     Default behavior is to use generic barrier
*/

#ifdef USE_MPI_BARRIER
#define __BARRIER_IMPL__(comm) MPI_Barrier(comm)
#else
void generic_barrier(MPI_Comm comm);
#define __BARRIER_IMPL__(comm) generic_barrier(comm)
#endif

#ifdef NO_BARRIER
  #define __BAR__(comm)
#else
#ifdef INCLUDE_BARRIER
  #define __BAR__(comm) __BARRIER_IMPL__(comm)
#else
  #define __BAR__(comm) __TIME_PAUSE__ ; __BARRIER_IMPL__(comm) ; __TIME_RESTART__
#endif
#endif
void printTimingInfo ( void );
void printCommTargets ( int procsPerNode, int useNearestRank );
void listRankLocations ( void );
int createActiveComm ( int procs, int procsPerNode,
                       char allocPattern, int useNearestRank,
                       MPI_Comm * activeComm );
int isActiveProc ( MPI_Comm * currCom );
int processArgs ( int argc, char **argv );
void printActivePairs ( int procs, int procsPerNode,
                        char allocPattern, int useNearestRank );
int getTargetRank ( int rank, int procsPerNode, int useNearestRank );
void getPair ( int idx, int procsPerNode,
               char allocPattern, int useNearestRank, int *rank1,
               int *rank2 );
int validateProcCount ( int count, int minCount, int maxCount );
int validateMessageSize ( int messSize, int minSize, int maxSize );
int validateTarget ( int count, int minCount, int maxCount );
int getProcessList ( int minVal, int maxVal,
                     int *listSize, char *listFile, int **list );
int getMessageList ( int minVal, int maxVal,
                     int *listSize, char *listFile, int **list );
int getList ( int ( *validateFunc ) ( int, int, int ), int minVal, int maxVal,
              int *listSize, char *listFile, int **list );
int get2entryList ( int ( *validateFunc ) ( int, int, int ), int minVal,
                    int maxVal, int *listSize, char *listFile, int **list1,
                    int **list2 );
int createSeqIntArray ( int start, int stop, int factor, int **array,
                        int *arraySize );
void prestaDebug ( char *fmt, ... );
void prestaRankDebug ( int targRank, char *fmt, ... );
void prestaRankPrint ( int targRank, char *fmt, ... );
void prestaWarn ( char *fmt, ... );
void prestaAbort ( char *fmt, ... );
int getNextTargetFile ( void );
int numcmp ( const void *num1, const void *num2 );
int validateRank ( int vrank, int minRank, int maxRank );
int getTargetList ( void );
void printGlobGroup ( MPI_Comm currComm );
char *getTimeStr ( void );
void set_data_values ( long long buf_size, void *buf_ptr );
long long check_data_values ( int data_count, void *in_buf, void *valid_buf,
                              MPI_Datatype data_type,
                              PRESTA_OP_TYPE op_type );
void init_stats ( STATSTRUCT * s, unsigned int t, unsigned int m,
                  unsigned int i, unsigned int p );
void update_stats ( STATSTRUCT * s, double r );
extern void printTestNames ( void );
void printSeparator ( void );
void printEnv(void );

extern char *executableName;
#endif
