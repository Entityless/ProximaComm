
# Usage

``` c++
// before
MPI_Allreduce(MPI_IN_PLACE, send_arr1, arr_sz, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
MPI_Allreduce(MPI_IN_PLACE, send_arr2, arr_sz, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
MPI_Allreduce(MPI_IN_PLACE, send_arr3, arr_sz, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

// after
ProximaComm<ProximaWeirdLocker> p;
p.ForkThreadsForAllreduce(1, arr_sz / 100000);
p.AllreduceSum<double>(send_arr1, buff_arr, arr_sz, MPI_COMM_WORLD);
p.AllreduceSum<double>(send_arr2, buff_arr, arr_sz, MPI_COMM_WORLD);
p.AllreduceSum<double>(send_arr3, buff_arr, arr_sz, MPI_COMM_WORLD);
p.FinalizeThreads();

// the count of nodes in the communicator should be 2 ^ n (n is a positive integer)
```
See main.cpp for details.

# Benchmark

## Environment

### Hardware environment

 - **CPU**: 2 * Intel(R) Xeon(R) CPU E5-2660 v3 @ 2.60GHz (HT disabled, 2 * 10 cores) (cpu MHz: 2899.914 when the program is running)
 - **Memory**: 256GB DDR4 (frequency unknown)

### Software environment

 - **System**:  CentOS Linux release 7.2.1511 (Core)
 - **Compiler**: icpc (ICC) 17.0.4 20170411

## Method

Create an `double` array with `arr_size`. Do `loop_count` times MPI_Allreduce on it.

See main.cpp for details.

## Result

### loop_count = 100, arr_size = 2000000, slice = 20
| node count | default allreduce-sum | ProximaComm::AllreduceSum |
| --- | --- | --- |
| 2 | 0.393524 | 0.306398 |
| 4 | 1.048445 | 0.907220 |
| 8 | 1.329426 | 1.358847 |

### loop_count = 20, arr_size = 10000000, slice = 100
| node count | default allreduce-sum | ProximaComm::AllreduceSum |
| --- | --- | --- |
| 2 | 0.575960 | 0.305054 |
| 4 | 1.291704 | 0.862830 |
| 8 | 1.593653 | 1.292144 |

### loop_count = 10, arr_size = 20000000, slice = 200
| node count | default allreduce-sum | ProximaComm::AllreduceSum |
| --- | --- | --- |
| 2 | 0.592859 | 0.304615 |
| 4 | 1.290048 | 0.852752 |
| 8 | 1.580036 | 1.272127 |
