===
com
===

-------------------------------------------------------
LLNL MPI Communication Benchmark - Point-to-point tests
-------------------------------------------------------

:Manual section: 1
:Manual group: Benchmarking Tools
:Version: UCRL-CODE-2001-028
:Date: 2026

SYNOPSIS
========

**com** [*options*] [*tests types*]

DESCRIPTION
===========

**com** is a comprehensive MPI point-to-point communication benchmark developed at Lawrence Livermore National Laboratory (LLNL). It measures bandwidth and latency for various communication patterns between MPI ranks.

The benchmark supports multiple test types including unidirectional and bidirectional communication, both blocking and non-blocking, as well as pure latency measurements.

OPTIONS
=======

**-a**
    Print individual test resuts.

    Default: Off

**-b** *bytes*
    Message start size.

    Default: 0

**-c** *integer*
    Samples

    Default: 3

**-d** *file path*
    Task/rank target list source file. The file shall have 2 columns, representing the pairing of the communicating ranks.

**-e** *bytes*
    Message stop size.

    Default: 32 MiB

**-f** *file path*
    Process count source file.

**-h**
    Print use information.

**-i**
    Print process pairs for each measurement. NOTE: Not for use at scale.

    Default: Off

**-k**
    Print individual rank times. NOTE: Not for use at scale.

    Default: Off

**-l**
    Print hostname information. NOTE: Not for use at scale.

    Default: Off

**-m** *file path*
    Message size source file: A single-column file of integers representing the message sizes to test in bytes.

**-n**
    Use barrier within measurement.

    Default: Off

**-o**
    Number of operations between measurements.

    Default: 0

**-p** *[c|b]*
    Allocate processes: c(yclic) or b(lock)

    Default: Block

**-q**
    Print test names.

    Default: TODO

**-r**
    Partner processes with nearby rank.

    Default: Off

**-s** *directory*
    A directory containing files representing rank pair mappings.

**-t** *integer*
    Processes per SMP

    Default: TODO

**-v**
    Verify message data.

    Default: Off

**-w**
    List of full test names.

**-x**
    Calculate bandwidth by volume/longest task time.

    Default: Off

TEST TYPES
==========

The benchmark implements five distinct communication patterns:

UNIDIR - Unidirectional Blocking
---------------------------------

One-way communication using blocking ``MPI_Send`` and ``MPI_Recv``.

- Sender posts ``MPI_Send``
- Receiver posts ``MPI_Recv``
- Measures unidirectional bandwidth
- Blocking operations ensure completion before proceeding

UNIDIRNB - Unidirectional Non-Blocking
---------------------------------------

One-way communication using non-blocking ``MPI_Isend`` and ``MPI_Irecv``.

- Sender posts ``MPI_Isend``
- Receiver posts ``MPI_Irecv``
- Uses ``MPI_Wait`` or ``MPI_Test`` for completion
- Allows potential overlap with computation

BIDIR - Bidirectional Blocking
-------------------------------

Simultaneous two-way communication using blocking operations.

- Both ranks send and receive simultaneously
- Uses ``MPI_Sendrecv`` or paired ``MPI_Send``/``MPI_Recv``
- Measures aggregate bidirectional bandwidth
- Tests full-duplex network capability

BIDIRNB - Bidirectional Non-Blocking
-------------------------------------

Simultaneous two-way communication using non-blocking operations.

- Both ranks post ``MPI_Isend`` and ``MPI_Irecv``
- Completion via ``MPI_Waitall``
- Measures bidirectional bandwidth with overlap potential
- Tests network's ability to handle concurrent operations

LATEN - Latency Test
---------------------

Ping-pong latency measurement.

- Measures round-trip time for small messages
- Typically uses zero-byte or small messages
- Reports half round-trip time as latency
- Critical for understanding communication overhead

ALGORITHM
=========

Test Execution Flow
-------------------

For each test type and message size:

1. **Initialization**: Allocate buffers and setup communicators
2. **Warmup**: Execute several iterations to warm caches
3. **Timing**: Measure specified number of iterations
4. **Aggregation**: Collect results from all ranks
5. **Reporting**: Calculate and print bandwidth/latency statistics

Bandwidth Calculation
---------------------

::

    bandwidth (MB/s) = (message_size × iterations × send_factor) / (time × 1048576)

Where:
    - ``send_factor`` = 1 for unidirectional, 2 for bidirectional
    - ``time`` is measured in seconds
    - Result is in MiB/s (1 MiB = 1048576 bytes)

Latency Calculation
-------------------

::

    latency (μs) = (round_trip_time / 2) × 1000000

USAGE
=====

Basic Execution
---------------

Run all tests with default parameters::

    srun -N 2 -n 2 com

Run specific test type::

    srun -N 2 -n 2 com --test BIDIR

Run with custom message sizes::

    srun -N 2 -n 2 com --min-size 1024 --max-size 1048576

Rank Placement
--------------

**Two ranks on different nodes** (recommended for inter-node testing)::

    # Slurm
    srun -N 2 -n 2 --ntasks-per-node=1 com

    # OpenMPI
    mpirun -n 1 -host node1 : -n 1 -host node2 com

**Two ranks on same node** (intra-node testing)::

    # Slurm
    srun -N 1 -n 2 --ntasks-per-node=2 com

    # OpenMPI
    mpirun -n 2 -host node1 com

OPTIONS
=======

Test Selection
--------------

**--test** *TYPE*
    Run specific test type: UNIDIR, UNIDIRNB, BIDIR, BIDIRNB, LATEN

**--list-tests**
    Display available test types and exit

Message Size Control
--------------------

**--min-size** *bytes*
    Minimum message size to test. Default: 0

**--max-size** *bytes*
    Maximum message size to test. Default: 4194304 (4 MiB)

**--message-list** *size1,size2,...*
    Comma-separated list of specific message sizes to test

Iteration Control
-----------------

**--iterations** *N*
    Number of iterations per test. Default: 1000

    Larger values improve timing accuracy but increase runtime

**--warmup** *N*
    Number of warmup iterations. Default: 10

Output Control
--------------

**--verbose**
    Enable verbose output with detailed timing information

**--quiet**
    Suppress progress messages, only show results

**--output** *file*
    Write results to specified file instead of stdout

Environment Information
-----------------------

**--print-env**
    Display MPI environment variables and system information

**--help**
    Display usage information and exit

EXAMPLES
========

Basic Bandwidth Test
--------------------

Measure bidirectional bandwidth between two nodes::

    srun -N 2 -n 2 --ntasks-per-node=1 com --test BIDIR

Latency Measurement
-------------------

Measure latency with small messages::

    mpirun -n 2 com --test LATEN --max-size 1024

Message Size Sweep
------------------

Test specific message sizes::

    mpirun -n 2 com --message-list 1,4,16,64,256,1024,4096

Compare Blocking vs Non-Blocking
---------------------------------

Run both unidirectional tests::

    mpirun -n 2 com --test UNIDIR > blocking.txt
    mpirun -n 2 com --test UNIDIRNB > nonblocking.txt

Full Test Suite
---------------

Run all tests with extended message sizes::

    srun -N 2 -n 2 com --min-size 1 --max-size 16777216 --iterations 10000

OUTPUT
======

Header Information
------------------

::

    LLNL MPI Communication Benchmark
    UCRL-CODE-2001-028

    Test Configuration:
      Ranks: 2
      Test Type: BIDIR
      Iterations: 1000
      Message Size Range: 0 - 4194304 bytes

Test Results
------------

::

    Test: Bidirectional Blocking (BIDIR)

    Size(B)    Bandwidth(MB/s)    Latency(μs)    Iterations
    -------    ---------------    -----------    ----------
          0              0.00           1.23          1000
          1             12.34           2.45          1000
          2             24.56           3.21          1000
          4             48.23           4.12          1000
       1024           1234.56          10.23          1000
       4096           4567.89          15.67          1000
    1048576          12345.67         100.45          1000
    4194304          23456.78         200.12          1000

    Maximum Bandwidth: 23456.78 MB/s at 4194304 bytes

Column Descriptions
-------------------

**Size(B)**
    Message size in bytes

**Bandwidth(MB/s)**
    Measured bandwidth in MiB/s (1 MiB = 1048576 bytes)

**Latency(μs)**
    Message latency in microseconds (for latency tests)

**Iterations**
    Number of iterations performed for this message size

**Maximum Bandwidth**
    Peak bandwidth achieved and corresponding message size

INTERPRETATION
==============

Performance Metrics
-------------------

**Bandwidth**
    - Measures data transfer rate
    - Higher is better
    - Typically increases with message size
    - Plateaus at network hardware limit

**Latency**
    - Measures communication overhead
    - Lower is better
    - Relatively constant for small messages
    - Increases with message size

Communication Patterns
----------------------

**Unidirectional vs Bidirectional**
    - Unidirectional: Tests one-way bandwidth
    - Bidirectional: Tests full-duplex capability
    - Bidirectional should approach 2× unidirectional on full-duplex networks

**Blocking vs Non-Blocking**
    - Blocking: Simpler, guaranteed completion
    - Non-blocking: Potential for overlap, may show higher bandwidth
    - Difference indicates overlap capability

Scaling Characteristics
-----------------------

**Small Messages (< 1 KB)**
    - Latency-dominated
    - Performance limited by software overhead
    - MPI implementation efficiency critical

**Medium Messages (1 KB - 64 KB)**
    - Transition region
    - Protocol switching may occur
    - Both latency and bandwidth matter

**Large Messages (> 64 KB)**
    - Bandwidth-dominated
    - Performance approaches hardware limits
    - Network hardware capability critical

Common Patterns
---------------

**Good Performance**
    - Smooth bandwidth increase with message size
    - Bidirectional ≈ 2× unidirectional
    - Low latency for small messages
    - Bandwidth approaches theoretical maximum

**Performance Issues**
    - Sudden drops at specific message sizes (protocol changes)
    - Bidirectional << 2× unidirectional (half-duplex or contention)
    - High latency (software overhead or network issues)
    - Bandwidth plateau well below hardware capability

TYPICAL USE CASES
=================

Network Characterization
------------------------

Understand network performance characteristics::

    # Test all patterns
    for test in UNIDIR UNIDIRNB BIDIR BIDIRNB LATEN; do
        srun -N 2 -n 2 com --test $test > ${test}.txt
    done

Acceptance Testing
------------------

Verify network meets specifications::

    srun -N 2 -n 2 com --test BIDIR --min-size 1048576 --max-size 4194304

Compare MPI Implementations
----------------------------

Test different MPI libraries::

    # OpenMPI
    module load openmpi
    mpirun -n 2 com > openmpi_results.txt

    # MPICH
    module load mpich
    mpirun -n 2 com > mpich_results.txt

Regression Testing
------------------

Detect performance changes::

    # Baseline
    srun -N 2 -n 2 com > baseline.txt

    # After changes
    srun -N 2 -n 2 com > new.txt

    # Compare
    diff baseline.txt new.txt

Topology Analysis
-----------------

Test different node pairs::

    # Adjacent nodes
    srun --nodelist=node[1-2] -n 2 com > adjacent.txt

    # Distant nodes
    srun --nodelist=node1,node100 -n 2 com > distant.txt

LIMITATIONS
===========

- **Two-rank only**: Only tests communication between two ranks
- **Point-to-point only**: Does not test collective operations
- **No multi-pair**: Cannot saturate network with multiple concurrent pairs
- **Fixed patterns**: Limited to predefined communication patterns
- **No GPU support**: CPU memory only (no GPU-aware MPI testing)

DIAGNOSTICS
===========

Common Issues
-------------

**"Error: Requires exactly 2 MPI ranks"**
    Must run with exactly 2 ranks. Adjust ``-n`` parameter.

**Very low bandwidth**
    - Check rank placement (same node vs different nodes)
    - Verify network connectivity
    - Check for system interference

**High variance in results**
    - Increase iteration count
    - Reduce system load
    - Check for network contention

**Unexpected latency spikes**
    - System interference (check ``dmesg``)
    - Network congestion
    - NUMA effects

Performance Debugging
---------------------

**Compare intra-node vs inter-node**::

    # Intra-node (should be very fast)
    srun -N 1 -n 2 com --test BIDIR

    # Inter-node (network speed)
    srun -N 2 -n 2 com --test BIDIR

**Check for protocol switching**::

    # Look for sudden bandwidth changes
    com --message-list 1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192

**Verify full-duplex operation**::

    # Bidirectional should be ~2× unidirectional
    com --test UNIDIR > uni.txt
    com --test BIDIR > bi.txt

ENVIRONMENT
===========

MPI Implementation Variables
----------------------------

``OMPI_*``, ``MPICH_*``, ``I_MPI_*``
    MPI implementation-specific tuning parameters

``OMPI_MCA_btl``
    OpenMPI transport layer selection

``MPICH_ASYNC_PROGRESS``
    Enable asynchronous progress (MPICH)

System Variables
----------------

``OMP_NUM_THREADS``
    Should be set to 1 for pure MPI benchmarks

``CUDA_VISIBLE_DEVICES``
    Control GPU visibility (if GPU-aware MPI)

SEE ALSO
========

**mpiGraph**\(1), **mpiBench**\(1), **sqmr**\(1), **osu_bw**\(1), **osu_bibw**\(1), **osu_latency**\(1)

MPI Standard: https://www.mpi-forum.org/

AUTHORS
=======

Developed at Lawrence Livermore National Laboratory (LLNL) by Chris Chambreau and Nathan Hanford.

COPYRIGHT
=========

Copyright (c) 2007-2026 Lawrence Livermore National Security (LLNS), LLC

Produced at Lawrence Livermore National Laboratory (LLNL)

This work was performed under the auspices of the U.S. Department of Energy by Lawrence Livermore National Laboratory under Contract DE-AC52-07NA27344.

UCRL-CODE-2001-028

LICENSE
=======

This work was prepared as an account of work sponsored by an agency of the United States government.

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

**Verbose Output at Scale**

Verbose output is only for debugging rank placement at smaller scales. Running verbose output at system scale will generally overwhelm stdout redirection, which is generally handled by the workload manager.
