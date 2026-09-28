====
sqmr
====

------------------------------------------------------
Sequoia Message Rate Benchmark - MPI message rate test
------------------------------------------------------

:Manual section: 1
:Manual group: Benchmarking Tools
:Version: 1.1.0
:Date: 2024

SYNOPSIS
========

**sqmr** [*options*]

DESCRIPTION
===========

**SQMR** (Sequoia Message Rate Benchmark) measures the maximal message rate of a single compute node by testing concurrent point-to-point communication between "core" ranks (on the node being tested) and "neighbor" ranks (on separate nodes).

The benchmark uses non-blocking MPI operations (``MPI_Isend``/``MPI_Irecv``) with configurable window sizes to measure sustainable message rates under various message sizes and concurrency levels.

RANK TOPOLOGY
=============

Core and Neighbor Ranks
------------------------

SQMR uses a specific rank placement strategy:

**Core Ranks**
    The first ``num_cores`` ranks (0 to num_cores-1) must be placed on the compute node being benchmarked.

**Neighbor Ranks**
    The remaining ranks are distributed as neighbors to the core ranks:

    - Ranks num_cores to (num_cores + num_nbors - 1) are neighbors for core rank 0
    - Ranks (num_cores + num_nbors) to (num_cores + 2×num_nbors - 1) are neighbors for core rank 1
    - And so on...

Total Ranks Required
--------------------

The total number of MPI ranks must be exactly::

    total_ranks = num_cores + (num_cores × num_nbors)

Example Topology
----------------

For an 8-core node with 4 neighbors per core::

    Total ranks needed: 8 + (8 × 4) = 40 ranks

    Core ranks:     0-7   (on the node being tested)
    Neighbors for rank 0:  8-11
    Neighbors for rank 1: 12-15
    Neighbors for rank 2: 16-19
    ...
    Neighbors for rank 7: 36-39

ALGORITHM
=========

Benchmark Procedure
-------------------

For each message size:

1. **Warmup**: Execute 10 warmup iterations
2. **Synchronize**: Barrier across all ranks
3. **Measure**: Time the specified number of iterations
4. **Communication Pattern**:

   - Each core rank posts ``win_size`` concurrent sends/receives to each of its ``num_nbors`` neighbors
   - Uses ``MPI_Isend`` and ``MPI_Irecv`` for non-blocking communication
   - Waits for all operations to complete with ``MPI_Waitall``

5. **Aggregate**: Collect timing data from all core ranks
6. **Report**: Calculate and print message rates and bandwidth

Message Size Progression
-------------------------

Starting from ``min_msgsize``, the benchmark doubles the message size until reaching ``max_msgsize``.

Iteration count decreases by 20% for each larger message size to maintain reasonable runtime.

OPTIONS
=======

Required Options
----------------

**--num_cores** *n*
    Number of MPI ranks on the 'core' node being tested.

    Correlates to the number of cores/ranks per compute node.

    If not specified, defaults to ``mpi_size/2``

**--num_nbors** *n*
    Number of distinct neighbor ranks for each core rank.

    Default: 1

Optional Parameters
-------------------

**--num_iters** *n*
    Number of benchmark iterations for each message size.

    Default: 4096

    Note: Actual iterations decrease for larger messages

**--win_size** *n*
    Number of concurrent send/recv pairs to post for each neighbor in a single iteration.

    Default: 10

    Higher values increase concurrency and may improve message rate

**--min_msgsize** *n*
    Smallest message size to benchmark in bytes.

    Default: 0

**--max_msgsize** *n*
    Largest message size to benchmark in bytes.

    Default: 4194304 (4 MiB)

Display Options
---------------

**--verbose**
    Enable verbose output showing:

    - Configuration parameters
    - Rank-to-neighbor mappings
    - Processor names for each rank

    NOTE: This mode will cause output issues when run at exascale.

**--help**
    Print usage information and exit

EXAMPLES
========

Basic Usage
-----------

srun -N 16 -n 128 sqmr

Message Size Range
------------------

srun -N 16 -n 128 sqmr --min_msgsize 8 --max_msgsize 4194304

High Concurrency Test
---------------------

Increase window size for more concurrent operations::

srun -N 16 -n 128 sqmr --min_msgsize 8 --max_msgsize 4194304 --win_size=2147483648

Verbose Output
--------------

Show detailed configuration and rank placement::

    srun -N 4 -n 32 sqmr --num_cores=8 --num_nbors=4 --verbose


OUTPUT
======

Header Information
------------------

::

    # SQMR v1.1.0 - MPI maximal message rate benchmark
    # Run at 05/12/24 14:30:45, with rank 0 on node001
    #
    # MPI tasks per node                 : 8
    # Neighbor tasks                     : 4
    # Iterations per message size        : 4096
    # Send/Recv operations per iteration : 10
    #

Timing Results
--------------

::

    #                          average                  max                     min
    # msgsize iters     msgs/sec    MiB/sec     msgs/sec    MiB/sec     msgs/sec    MiB/sec
           0   4096   1234567.89       0.00   1245678.90       0.00   1223456.78       0.00
           1   3276   1200000.00       1.14   1210000.00       1.15   1190000.00       1.13
           2   2621   1180000.00       2.25   1190000.00       2.27   1170000.00       2.23
           4   2097   1150000.00       4.38   1160000.00       4.41   1140000.00       4.34
    ...

Column Descriptions
-------------------

**msgsize**
    Message size in bytes

**iters**
    Number of iterations performed for this message size

**msgs/sec (average)**
    Average message rate across all core ranks (messages per second)

**MiB/sec (average)**
    Average bandwidth across all core ranks (MiB/s)

**msgs/sec (max)**
    Maximum message rate among core ranks

**MiB/sec (max)**
    Maximum bandwidth among core ranks

**msgs/sec (min)**
    Minimum message rate among core ranks

**MiB/sec (min)**
    Minimum bandwidth among core ranks

INTERPRETATION
==============

Message Rate Calculation
-------------------------

For each core rank, the message rate is calculated as::

    messages = win_size × num_nbors × iters × 2
    rate = messages / elapsed_time

The factor of 2 accounts for both sends and receives.

Bandwidth Calculation
---------------------

Bandwidth in MiB/s::

    bandwidth = (rate × msgsize) / 1048576

Performance Metrics
-------------------

**Average Rate**
    Mean performance across all core ranks. Best indicator of node capability.

**Maximum Rate**
    Best-performing core rank. May indicate optimal conditions.

**Minimum Rate**
    Worst-performing core rank. Important for understanding bottlenecks.

**Rate Variance**
    Large differences between min and max suggest:

    - Uneven load distribution
    - NUMA effects
    - Network contention
    - System noise

RANK PLACEMENT STRATEGIES
==========================

Slurm Examples
--------------

**Explicit node assignment**::

    # Core ranks on node001, neighbors distributed
    srun --nodelist=node001,node002,node003,node004,node005 \
         -N 5 -n 40 --tasks-per-node=8 \
         sqmr --num_cores=8 --num_nbors=4

**Cyclic distribution**::

    srun -N 9 -n 40 --distribution=cyclic \
         sqmr --num_cores=8 --num_nbors=4

LIMITATIONS
===========

- **Fixed topology**: Requires specific rank placement pattern
- **Exact rank count**: Must have exactly ``num_cores + (num_cores × num_nbors)`` ranks
- **Single core node**: Only tests one node at a time
- **Symmetric communication**: All core ranks have same number of neighbors
- **Fixed message tag**: Uses tag 1 for all messages
- **No data validation**: Does not verify message contents

DIAGNOSTICS
===========

Common Errors
-------------

**"ERROR need exactly X ranks for num_cores Y and num_nbors Z, have W"**
    Incorrect number of MPI ranks. Adjust job size or parameters.

**"ERROR Must specify num_cores > 0"**
    Missing or invalid ``--num_cores`` parameter.

**"ERROR Must specify num_nbors > 0"**
    Missing or invalid ``--num_nbors`` parameter.

**"ERROR Must specify win_size > 0"**
    Invalid ``--win_size`` parameter.

**"ERROR Must specify max_msgsize >= min_msgsize"**
    Message size range is invalid.

**"Error in GPU device initialization"**
    GPU not available or driver issue (GPU builds only).

SEE ALSO
========

**mpiGraph**\(1), **mpiBench**\(1), **osu_mbw_mr**\(1), **IMB-MPI1**\(1)

MPI Standard: https://www.mpi-forum.org/

AUTHORS
=======

Written by Andrew Friedley, Chris Chambreau, and Nathan Hanford

COPYRIGHT
=========

Copyright (c) 2007-2026 Lawrence Livermore National Security (LLNS), LLC

Produced at Lawrence Livermore National Laboratory (LLNL)

Original Code Identifier: UCRL-CODE-400846

This work was performed under the auspices of the U.S. Department of Energy by Lawrence Livermore National Laboratory under Contract DE-AC52-07NA27344.

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
