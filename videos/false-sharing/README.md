# False Sharing Experiments
## Goal
Explore potential performance degredation of an embarrassingly parallel problem due to false sharing by benchmarking various implementations of a basic kinematic problem.

## Structure
Independent folders for each type of implementation, such as `01_local_points` and `07_multiple_arrays`, that are compiled into a combined benchmark application.

## Summary of findings
Even without any direct contention on kinematic data, if there is any indirect contention caused by accessing (and modifying) the same cachelines performance quickly degrades. In the worst case scenario where threads are using a global data structure and accessing it in an interleaved pattern, performance can suffer by as much as 100x.

There are a variety of ways to minimize, or completely remove, the contention over cachelines which all result in performance characteristics expected of truly independent workloads.

## Dependencies
[Google benchmark](https://github.com/google/benchmark) for, well, benchmarking. Mostly tested while installed separately on the system, but will otherwise try to fetch with `FetchContent`.

## Building
Built using CMake, with optional presets for `debug` and `release`. Both presets respect `CXX` to optionally change the desired compiler. Only tested on Linux with `g++` and `clang++`.
```
$ cmake --list-presets
Available configure presets:

  "debug"   - debug
  "release" - release
```

Building with a preset configuration:
```
$ cmake --preset release
$ cmake --build out/build/release
```

Building with a manual configuration:
```
$ CXX=clang++ CMAKE_BUILD_TYPE=RelWithDebInfo cmake -S. -Bmybuild
$ cmake --build mybuild
```

## Running
The application can be run like any other Google benchmark program.
```
$ ./out/build/release/bench [standard_benchmark_args]
```
