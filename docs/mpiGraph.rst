========
mpiGraph
========

---------------------------------------------------
MPI point-to-point bandwidth measurement benchmark
---------------------------------------------------

:Manual section: 1
:Manual group: Benchmarking Tools
:Version: 1.6
:Date: 2024

SYNOPSIS
========

**mpiGraph** [*message_size*] [*iterations*] [*window*]

DESCRIPTION
===========

**mpiGraph** measures point-to-point bandwidth between all pairs of MPI ranks using a ring-based communication pattern. It produces NxN matrices showing send and receive bandwidths between every pair of ranks.

The benchmark arranges N MPI ranks in a logical ring (0 to N-1) and executes N-1 communication steps. In each step, each rank sends to a partner D positions to the right and receives from a partner D positions to the left, where D ranges from 1 to N-1.

ALGORITHM
=========

Ring-Based Communication Pattern
---------------------------------

1. **Logical arrangement**: Ranks 0 through N-1 form a circular array
2. **Distance iteration**: For each distance D from 1 to N-1:

   - Each rank sends to rank (my_rank + D) % N
   - Each rank receives from rank (my_rank - D + N) % N
   - Bandwidth is measured for both send and receive operations

3. **Result**: Each rank communicates with every other rank exactly once

Communication Method
--------------------

- Uses non-blocking MPI operations (``MPI_Isend`` and ``MPI_Irecv``)
- Supports windowing: multiple outstanding messages per rank pair
- Measures send and receive bandwidths independently using ``MPI_Testall``

OPTIONS
=======

*message_size*
    Size of each message in bytes. Default: 16384 (4096*4)

*iterations*
    Number of times to measure bandwidth between each rank pair. Default: 100

*window*
    Number of outstanding send/receive operations per rank pair. Default: 50

EXAMPLES
========

Run with default parameters (16KB messages, 100 iterations, window of 50)::

    srun -N 16 -n 16 mpiGraph

Run with 1MB messages, 50 iterations, window of 10::

    srun -N 16 -n 16 mpiGraph 1048576 50 10

Run on 4 nodes with 8 ranks per node::

    srun -N 4 -n 32 mpiGraph

OUTPUT
======

The benchmark produces the following output:

Header Information
------------------

::

    START mpiGraph v1.6
    MsgSize    16384
    Times      100
    Window     50
    Procs      16

Progress Updates
----------------

During execution, rank 0 prints progress::

    1 of 16 (6.2%)
    2 of 16 (12.5%)
    ...

Summary Statistics
------------------

**Send Statistics**::

    Send max    12543.234
    Send avg    11234.567

**Receive Statistics**::

    Recv max    12456.789
    Recv avg    11123.456

Bandwidth Matrices
------------------

**Send Bandwidth Matrix** (MB/s from row to column)::

    Send        node1:0    node2:1    node3:2    ...
    node1:0 to  0.000      11234.567  11456.789  ...
    node2:1 to  11345.678  0.000      11567.890  ...
    ...

**Receive Bandwidth Matrix** (MB/s from column to row)::

    Recv          node1:0    node2:1    node3:2    ...
    node1:0 from  0.000      11234.567  11456.789  ...
    node2:1 from  11345.678  0.000      11567.890  ...
    ...

INTERPRETATION
==============

Bandwidth Values
----------------

- **Diagonal entries**: Always 0.000 (no self-communication)
- **Off-diagonal entries**: Bandwidth in MB/s between rank pairs
- **Send max/avg**: Maximum and average send bandwidth across all pairs
- **Recv max/avg**: Maximum and average receive bandwidth across all pairs

Performance Analysis
--------------------

**Good performance indicators:**

- Uniform bandwidth values across the matrix
- Send and receive bandwidths are similar
- Values close to theoretical network bandwidth

**Performance issues:**

- Large variance in bandwidth values
- Significantly lower values for specific rank pairs
- Asymmetric send/receive bandwidths

Common patterns:

- **Intra-node communication**: Higher bandwidth between ranks on same node
- **Inter-node communication**: Lower bandwidth between ranks on different nodes
- **Network topology effects**: Bandwidth may vary based on physical network layout

TIMING METHODS
==============

The benchmark supports two timing methods (compile-time option):

MPI_Wtime (default, recommended)
---------------------------------

Uses ``MPI_Wtime()`` for high-resolution timing. Recommended because it's monotonic and not affected by system clock adjustments.

Compile without ``-DUSE_GETTIMEOFDAY``

gettimeofday
------------

Uses ``gettimeofday()`` for timing. May produce incorrect results on systems where the system clock can jump backwards.

Compile with ``-DUSE_GETTIMEOFDAY``

TYPICAL USE CASES
=================

Network Characterization
------------------------

Run mpiGraph to understand your cluster's network topology and identify:

- Bandwidth between nodes
- Network bottlenecks
- Asymmetric communication paths

Performance Debugging
---------------------

Use mpiGraph to diagnose application performance issues:

- Identify slow network links
- Detect network contention
- Verify network configuration

LIMITATIONS
===========

- **Memory usage**: Allocates O(N²) memory on rank 0 for result gathering
- **Runtime**: Scales as O(N²) with number of ranks
- **Single communicator**: Only tests ``MPI_COMM_WORLD``
- **Point-to-point only**: Does not test collective operations

SEE ALSO
========

**osu_bw**\(1), **osu_latency**\(1), **IMB-MPI1**\(1), **mpptest**\(1)

MPI Standard: https://www.mpi-forum.org/

AUTHORS
=======

Written by Adam Moody.

COPYRIGHT
=========

Copyright (c) 2007-2008, Lawrence Livermore National Security (LLNS), LLC

Produced at the Lawrence Livermore National Laboratory (LLNL)

UCRL-CODE-232117. All rights reserved.

This file is part of mpiGraph. For details, see https://github.com/llnl/phloem/

LICENSE
=======

This software is licensed under the MIT License.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

See the LICENSE file in the source distribution for full license text.

BUGS
====

Report bugs to: https://github.com/llnl/phloem/issues
