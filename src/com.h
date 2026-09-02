/*
com.h

Authors: Chris Chambreau and Nathan Hanford

Copyright (c) 2007-2026 Lawrence Livermore National Security (LLNS), LLC.
See COPYRIGHT file for more details.

SPDX-License-Identifier: MIT
See LICENSE file for more details
*/

typedef double ( *TESTFUNC ) ( int, int, MPI_Comm * );

enum TESTTYPE
{
  UNIDIR, UNIDIRNB, BIDIR, BIDIRNB, LATEN, TYPETOT
};

typedef struct
{
  int id;
  char *name;
  double rankResult;
  double maxResult;
  int iters;
  double sendFactor;
  int *messList;
  int messListSize;
  double maxBW;
  int maxBWMessSize;
  TESTFUNC testFunc;

}
TESTPARAMS;

TESTPARAMS *testParams;


void runTest ( TESTPARAMS * testParams );
int generateResults ( TESTPARAMS * testParams, int procs, int messSize,
                      int iters, double *result );
double runUnicomTest ( int bufsize, int iters, MPI_Comm * activeComm );
double runNonblockUnicomTest ( int bufsize, int iters,
                               MPI_Comm * activeComm );
double runNonblockBicomTest ( int bufsize, int iters, MPI_Comm * activeComm );
double runBicomTest ( int bufsize, int iters, MPI_Comm * activeComm );
double runLatencyTest ( int bufsize, int iters, MPI_Comm * activeComm );
void printUse ( void );
void printParameters ( void );
void printReportHeader ( void );
int setupTestListParams ( void );
int initAllTestTypeParams ( TESTPARAMS ** testParams );
void freeBuffers ( TESTPARAMS ** testParams );
int getNextTargetFile ( void );
void printTestNames ( void );
extern int getTargetList ( void );
extern void printSeparator ( void );
extern void printEnv ( void );
