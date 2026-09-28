========
mpiBench
========

-----------------------------------------------
MPI collective operations performance benchmark
-----------------------------------------------

:Manual section: 1
:Manual group: Benchmarking Tools
:Version: 1.0
:Date: 2024

SYNOPSIS
========

**mpiBench** [*options*] [*operations*]

DESCRIPTION
===========

**mpiBench** is a comprehensive benchmark for measuring the performance of MPI collective operations. It tests blocking and non-blocking collective operations across various message sizes and can operate on multiple communicator topologies including Cartesian dimensions and partitioned communicators.

The benchmark automatically adjusts iteration counts based on time limits to ensure consistent measurement quality across different message sizes and system configurations.

ALGORITHM
=========

Measurement Strategy
--------------------

For each collective operation and message size:

1. **Prime**: Execute operation once to warm up caches
2. **Estimate**: Run 5 iterations to estimate execution time
3. **Adjust**: Calculate iteration count based on time limit
4. **Measure**: Execute operation with adjusted iteration count
5. **Gather**: Collect timing data from all ranks
6. **Report**: Print min, max, and average times

Timing Methodology
------------------

- Uses ``MPI_Wtime()`` by default (recommended)
- Optional ``gettimeofday()`` support via compile flag
- Excludes barrier synchronization from timing by default
- Reports time per iteration in microseconds

OPTIONS
=======

Message Size Control
--------------------

**-b** *byte*
    Beginning message size in bytes. Default: 0

    Examples: ``-b 1K``, ``-b 4096``, ``-b 1M``

**-e** *byte*
    Ending message size in bytes. Default: 256K

    Examples: ``-e 1M``, ``-e 4096K``

**-m** *byte*
    Process memory buffer limit (send+recv) in bytes. Default: 1G

    Prevents excessive memory allocation for large-scale tests

Iteration Control
-----------------

**-i** *itrs*
    Maximum number of iterations for a single test. Default: 1000

    Actual iterations may be lower based on time limit

**-t** *usec*
    Time limit for any single test in microseconds. Default: 0 (infinity)

    If neither ``-i`` nor ``-t`` specified, defaults to 50000 microseconds

Communicator Configuration
---------------------------

**-d** *ndim*
    Number of Cartesian dimensions to split processes. Default: 0 (MPI_COMM_WORLD only)

    Creates 1-D subcommunicators along each dimension for testing

**-p** *size*
    Minimum partition size (number of ranks) to divide MPI_COMM_WORLD

    Recursively divides communicator in half until reaching partition size

Data Validation
---------------

**-c**
    Check receive buffer for expected data in last iteration only

**-C**
    Check receive buffer for expected data every iteration

    Warning: Significantly increases runtime

General Options
---------------

**-h**
    Print help screen and exit

Byte String Format
------------------

``<byte>`` arguments accept: ``[0-9]+[KMG][bB]``

Examples:
    - ``32K`` or ``32KB`` = 32,768 bytes
    - ``64M`` or ``64MB`` = 67,108,864 bytes
    - ``1G`` or ``1GB`` = 1,073,741,824 bytes

OPERATIONS
==========

Blocking Collectives
--------------------

**Barrier**
    Synchronization barrier

**Bcast**
    Broadcast from root to all ranks

**Alltoall**
    All-to-all with equal-sized messages

**Alltoallv**
    All-to-all with variable-sized messages

**Allgather**
    Gather from all ranks to all ranks

**Allgatherv**
    Gather with variable-sized contributions

**Gather**
    Gather to root rank

**Gatherv**
    Gather with variable-sized contributions to root

**Scatter**
    Scatter from root to all ranks

**Allreduce**
    Reduction with result distributed to all ranks

**Reduce**
    Reduction with result at root rank

Non-Blocking Collectives
-------------------------

**Ibcast**, **Ialltoall**, **Ialltoallv**, **Iallgather**, **Iallgatherv**, **Igather**, **Igatherv**, **Iscatter**, **Iallreduce**, **Ireduce**

Non-blocking variants of the above operations using ``MPI_I*`` functions with ``MPI_Wait``.

EXAMPLES
========

Basic Usage
-----------

Test all collectives with default parameters::

    mpirun -n 16 mpiBench

Test specific operations::

    mpirun -n 32 mpiBench Barrier Bcast Allreduce

Message Size Sweep
------------------

Test from 1KB to 1MB messages::

    mpirun -n 64 mpiBench -b 1K -e 1M

With time limit of 100ms per test::

    mpirun -n 64 mpiBench -b 1K -e 1M -t 100000

Communicator Topologies
-----------------------

Test on 3D Cartesian topology::

    mpirun -n 64 mpiBench -d 3 Alltoall

Test with partitioned communicators (minimum 8 ranks per partition)::

    mpirun -n 128 mpiBench -p 8 Bcast

Data Validation
---------------

Check correctness on last iteration::

    mpirun -n 16 mpiBench -c Bcast Allgather

Check every iteration (slow)::

    mpirun -n 16 mpiBench -C -i 10 Bcast

OUTPUT
======

Header Information
------------------

::

    START mpiBench
    (or START mpiBench CUDA / START mpiBench ROCm)

Timing Results
--------------

Each line shows results for one collective operation at one message size::

    Barrier              Bytes:       0  Iters:    1000  Avg:   45.2341  Min:   43.1234  Max:   48.5678  Comm: MPI_COMM_WORLD  Ranks: 16
    Bcast                Bytes:    1024  Iters:     500  Avg:  123.4567  Min:  120.1234  Max:  128.9012  Comm: MPI_COMM_WORLD  Ranks: 16

Fields:
    - **Operation**: Name of collective operation
    - **Bytes**: Message size in bytes
    - **Iters**: Number of iterations performed
    - **Avg**: Average time per iteration (microseconds)
    - **Min**: Minimum time across all ranks
    - **Max**: Maximum time across all ranks
    - **Comm**: Communicator used for test
    - **Ranks**: Number of ranks in communicator

Footer Information
------------------

::

    Message buffers (KB): 8192
    END mpiBench

INTERPRETATION
==============

Performance Metrics
-------------------

**Average Time**
    Mean execution time across all ranks. Best indicator of typical performance.

**Minimum Time**
    Fastest rank. May indicate best-case performance or measurement noise.

**Maximum Time**
    Slowest rank. Critical for understanding worst-case behavior and load imbalance.

**Time Variance**
    Large difference between min and max suggests:

    - Network contention
    - Load imbalance
    - NUMA effects
    - System interference

Scaling Analysis
----------------

**Strong Scaling**
    Fix message size, increase ranks. Time should decrease.

**Weak Scaling**
    Increase message size proportionally with ranks. Time should remain constant.

**Message Size Effects**
    - Small messages: Latency-dominated
    - Large messages: Bandwidth-dominated
    - Transition point varies by operation and system

Common Patterns
---------------

**Good Performance**
    - Consistent times across ranks (min ≈ max)
    - Predictable scaling with message size
    - Similar performance across communicators

**Performance Issues**
    - Large variance (max >> min)
    - Non-monotonic scaling
    - Sudden performance drops at specific sizes

COMMUNICATOR TOPOLOGIES
=======================

MPI_COMM_WORLD
--------------

Default communicator containing all ranks. Always tested first.

Cartesian Dimensions (-d)
--------------------------

Creates 1-D subcommunicators along each dimension of a Cartesian topology.

Example with ``-d 3`` and 64 ranks:
    - Creates 4×4×4 Cartesian grid
    - Tests 3 subcommunicators:
        - CartDim-1of3: 16 ranks each (4 groups)
        - CartDim-2of3: 16 ranks each (4 groups)
        - CartDim-3of3: 16 ranks each (4 groups)

Use case: Analyze performance along different network dimensions

Partitioned Communicators (-p)
-------------------------------

Recursively divides MPI_COMM_WORLD in half until reaching partition size.

Example with ``-p 8`` and 64 ranks:
    - PartSize-64: 64 ranks (1 group)
    - PartSize-32: 32 ranks each (2 groups)
    - PartSize-16: 16 ranks each (4 groups)
    - PartSize-8: 8 ranks each (8 groups)

Use case: Understand performance at different scales

COMPILE-TIME OPTIONS
====================

Timing Methods
--------------

**Default (MPI_Wtime)**
    Recommended. Monotonic and portable.

**-DUSE_GETTIMEOFDAY**
    Use ``gettimeofday()`` instead. May have issues with clock adjustments.

Barrier Behavior
----------------

**Default**
    Excludes ``MPI_Barrier`` from timing measurements.

**-DINCLUDE_BARRIER**
    Includes barrier synchronization in timing.

**-DNO_BARRIER**
    Removes barrier between consecutive collective calls.

**-DUSE_MPI_BARRIER**
    Uses MPI implementation's barrier instead of generic barrier.

Non-Blocking Collectives
-------------------------

**Default (HAVE_NBC=1)**
    Enables non-blocking collective tests.

Set ``HAVE_NBC=0`` if MPI implementation lacks non-blocking collective support.

GPU Support
-----------

**-DENABLE_CUDA**
    Build CUDA-enabled version for NVIDIA GPUs.

**-DENABLE_HIP**
    Build ROCm-enabled version for AMD GPUs.

MEMORY MANAGEMENT
=================

Buffer Allocation
-----------------

mpiBench allocates send and receive buffers based on:

- Maximum message size (``-e``)
- Number of ranks
- Operations being tested

For operations like Alltoall and Allgather:
    ``buffer_size = max_message_size × num_ranks``

Memory Limit
------------

The ``-m`` option prevents excessive allocation:

- Automatically reduces max message size if needed
- Ensures ``buffer_size × 2 ≤ memory_limit``
- Default: 1GB per process

TYPICAL USE CASES
=================

Network Characterization
------------------------

Sweep message sizes to understand latency and bandwidth::

    mpirun -n 32 mpiBench -b 1 -e 1M -t 50000

Collective Operation Comparison
--------------------------------

Compare blocking vs non-blocking::

    mpirun -n 64 mpiBench Allreduce Iallreduce

Topology Analysis
-----------------

Test performance across different communicator structures::

    mpirun -n 128 mpiBench -d 3 -p 16 Alltoall

Acceptance Testing
------------------

Verify system meets performance requirements::

    mpirun -n 256 mpiBench -b 4K -e 4M -t 100000 > results.txt

Regression Testing
------------------

Compare performance across software versions or configurations::

    mpirun -n 64 mpiBench Barrier Bcast Allreduce > baseline.txt
    # After changes
    mpirun -n 64 mpiBench Barrier Bcast Allreduce > new.txt
    diff baseline.txt new.txt

LIMITATIONS
===========

- **Memory scaling**: O(N) memory per rank for some N-rank collectives
- **Time scaling**: O(N²) for testing all communicator combinations
- **Single root**: Rooted operations always use rank 0
- **Fixed reduction**: Allreduce/Reduce always use MPI_SUM on MPI_DOUBLE
- **No overlap testing**: Non-blocking collectives use immediate MPI_Wait

ENVIRONMENT
===========

DIAGNOSTICS
===========

Common Errors
-------------

**"Allocating memory ... requesting X bytes"**
    Insufficient memory. Reduce ``-e`` or increase ``-m``.

**"Must define number of operations per measurement"**
    Invalid ``-i 0`` specified.

**"Invalid flag -X"**
    Unknown command-line option.

**"Error in GPU device initialization"**
    GPU not available or driver issue (GPU builds only).

SEE ALSO
========

**mpiGraph**\(1), **osu_bw**\(1), **osu_latency**\(1), **IMB-MPI1**\(1)

MPI Standard: https://www.mpi-forum.org/

AUTHORS
=======

Written by Adam Moody and Nathan Hanford.

COPYRIGHT
=========

Copyright (c) 2007-2026, Lawrence Livermore National Security (LLNS), LLC

Produced at Lawrence Livermore National Laboratory (LLNL).

UCRL-CODE-232117. All rights reserved.

This file is part of Phloem. For details, see https://github.com/llnl/phloem/

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

NOTES
=====

**Performance Tips**

1. Use ``-t`` to set reasonable time limits for large-scale tests
2. Start with small message sizes to identify latency issues
3. Use ``-d`` to understand network topology effects
4. Compare blocking vs non-blocking for overlap opportunities
5. Enable data checking (``-c``) periodically to verify correctness
