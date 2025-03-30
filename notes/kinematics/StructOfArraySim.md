# StructOfArraySim
SoA layout that uses `std::array` fields. Data for particular fields are entirely contiguous with a compile-time cap on data size (implemented as a template value). In order to get the best performance a common `UpdateHelper` method is used, though some optimizations should (and are) found even with out the additional `__restrict__` hints.

```
struct Bodies
{
    std::array<float, MAX_SIZE> x, y;
    std::array<float, MAX_SIZE> horizontalSpeed, verticalSpeed;
    std::array<Color, MAX_SIZE> color;
};
Bodies _bodies;

template <size_t size> void StructOfArraySim<size>::Update(const float deltaTime)
{
	UpdateHelper(deltaTime, _bodies.x.data(), _bodies.y.data(), _bodies.horizontalSpeed.data(), _bodies.verticalSpeed.data());
}
```

## Benchmark Comparison
Compiling the same sources with both `g++` (version 13.3.0) and `clang++` (version 18.1.3) results in mixed results, but within the same ballpark. Very similar to the `StructOfVectorSim` results

### gcc
```
$ g++ --version | head -1
g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0

$ ./out/build/release/bench/kinematics-demo-bench --benchmark-no-analysis --rng-seed 433152058
benchmark name                            samples    iterations          mean
-------------------------------------------------------------------------------
Update StructOfArraySim: 1000000               100             1    229.056 us
Update StructOfArraySim: 5000000               100             1    2.19997 ms
```

### clang
```
$ clang++ --version | head -1
Ubuntu clang version 18.1.3 (1ubuntu1)

$ ./out/build/clang/bench/kinematics-demo-bench --benchmark-no-analysis --rng-seed 433152058
benchmark name                            samples    iterations          mean
-------------------------------------------------------------------------------
Update StructOfArraySim: 1000000               100             1    182.262 us
Update StructOfArraySim: 5000000               100             1    2.48095 ms
```

## Assembly Analysis
Even without the `__restrict__` hints both compilers were able to optimize due to guarantees on `std::array` (not shown below).

While very similar to the `StructOfVectorSim` results (which makes sense as the common `UpdateHelper` is used) it does appear that both compilers inlined a special version for `StructOfArraySim`, perhaps trying to leverage more information provided by a `std::array`. It's not obvious if that made a significant difference, however, as both versions largely remain the same at first glance. The most obvious change is the method of "indexing" into the array while interacting with `zmm` registers.

### gcc
```
$ perf record -D 1000 ./out/build/release/gui/kinematics-demo-gui 5000000
<wait some time, then close>

$ perf annotate -Mintel
<snip>
  0.46 │ e9:   vmovups      zmm2,ZMMWORD PTR [rcx+0x2625a00]
  0.62 │       vmovups      zmm1,ZMMWORD PTR [rcx+0x3938700]
  0.62 │       vmovaps      zmm4,zmm10
  0.58 │       vfmadd213ps  zmm4,zmm1,ZMMWORD PTR [rcx+0x1312d00]
  1.26 │       vmovups      ZMMWORD PTR [rcx+0x1312d00],zmm4
  0.90 │       vcmpltps     k2,zmm2,zmm0
  0.88 │       vcmpgtps     k1,zmm2,zmm0
  0.94 │       vmovaps      zmm5,zmm2
  1.39 │       vfmadd213ps  zmm5,zmm10,ZMMWORD PTR [rcx]
  2.80 │       vaddps       zmm11,zmm5,zmm9
 18.28 │       vmovups      ZMMWORD PTR [rcx],zmm5
  8.93 │       vcmpltps     k2{k2},zmm11,zmm0
  9.18 │       knotw        k3,k2
  9.11 │       vaddps       zmm11{k3}{z},zmm5,zmm6
  7.93 │       kmovw        ebx,k3
  3.06 │       vcmpgtps     k0{k1},zmm11,zmm13
  3.33 │       kmovw        edx,k0
  3.12 │       and          edx,ebx
  3.21 │       kmovw        ebx,k2
  3.05 │       or           dx,bx
  0.01 │     ↓ jne          4a0
  2.57 │16c:   vcmpltps     k1,zmm1,zmm0
  1.39 │       vaddps       zmm2,zmm4,zmm9
  1.24 │       vcmpltps     k0{k1},zmm2,zmm0
  1.28 │       knotw        k1,k0
  1.27 │       kmovw        ebx,k0
  1.29 │       vaddps       zmm2{k1}{z},zmm4,zmm6
  1.57 │       vcmpgtps     k1{k1},zmm1,zmm0
  1.32 │       vcmpgtps     k1{k1},zmm2,zmm12
  1.40 │       kmovw        edx,k1
  1.58 │       or           dx,bx
  0.00 │     ↓ jne          4d0
  1.40 │1a9:   inc          r8
  1.53 │       add          rcx,0x40
  1.19 │       cmp          r9,r8
  0.00 │     ↑ jne          e9
<snip>
```

### clang
```
$ perf record -D 1000 ./out/build/clang/gui/kinematics-demo-gui 5000000
<wait some time, then close>

$ perf annotate -Mintel
<snip>
  2.05 │190:   vmovups      zmm9,ZMMWORD PTR [rcx+rsi*4-0x1312d00]
  2.28 │       vmovups      zmm10,ZMMWORD PTR [rcx+rsi*4-0x3938700]
  4.18 │       vfmadd231ps  zmm10,zmm9,zmm2
  6.04 │       vxorps       zmm14,zmm9,zmm7
  4.03 │       vcmpltps     k1,zmm9,zmm14
  4.38 │       vmovups      ZMMWORD PTR [rcx+rsi*4-0x3938700],zmm10
  2.24 │       vaddps       zmm13,zmm10,zmm5
  2.25 │       vaddps       zmm10,zmm10,zmm8
  2.21 │       vmovups      zmm11,ZMMWORD PTR [rcx+rsi*4]
  2.32 │       vmovups      zmm12,ZMMWORD PTR [rcx+rsi*4-0x2625a00]
  4.26 │       vcmpltps     k0{k1},zmm13,zmm6
  4.10 │       vcmpltps     k1,zmm14,zmm9
  4.06 │       vcmpltps     k1{k1},zmm3,zmm10
  4.95 │       korw         k1,k1,k0
  5.07 │       vfmadd231ps  zmm12,zmm11,zmm2
  5.11 │       vxorps       zmm10,zmm11,zmm7
  6.54 │       vaddps       zmm9,zmm12,zmm5
 13.27 │       vmovups      ZMMWORD PTR [rcx+rsi*4-0x2625a00],zmm12
  7.95 │       vmovups      ZMMWORD PTR [rcx+rsi*4-0x1312d00]{k1},zmm14
  3.99 │       vcmpltps     k1,zmm11,zmm10
  0.91 │       vaddps       zmm12,zmm12,zmm8
  0.90 │       vcmpltps     k0{k1},zmm9,zmm6
  0.87 │       vcmpltps     k1,zmm10,zmm11
  1.44 │       vcmpltps     k1{k1},zmm4,zmm12
  1.13 │       korw         k1,k1,k0
  1.73 │       vmovups      ZMMWORD PTR [rcx+rsi*4]{k1},zmm10
  0.87 │       add          rsi,0x10
  0.84 │       cmp          rdx,rsi
       │     ↑ jne          190
<snip>
```

## Additional Data
### gcc
```
$ ./out/build/release/gui/kinematics-demo-gui 5000000 &

$ perf stat -ddd --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '27109':

          5,751.87 msec task-clock                       #    1.001 CPUs utilized
            10,106      context-switches                 #    1.757 K/sec
               214      cpu-migrations                   #   37.205 /sec
                 0      page-faults                      #    0.000 /sec
    31,487,420,857      cycles                           #    5.474 GHz                         (35.72%)
       789,963,804      stalled-cycles-frontend          #    2.51% frontend cycles idle        (35.88%)
    29,123,462,642      instructions                     #    0.92  insn per cycle
                                                         #    0.03  stalled cycles per insn     (35.72%)
     2,534,617,543      branches                         #  440.660 M/sec                       (35.86%)
        23,237,720      branch-misses                    #    0.92% of all branches             (35.89%)
    11,738,643,464      L1-dcache-loads                  #    2.041 G/sec                       (35.80%)
     3,172,147,478      L1-dcache-load-misses            #   27.02% of all L1-dcache accesses   (35.66%)
       290,937,480      L1-icache-loads                  #   50.581 M/sec                       (35.58%)
         1,173,920      L1-icache-load-misses            #    0.40% of all L1-icache accesses   (35.49%)
        51,948,881      dTLB-loads                       #    9.032 M/sec                       (35.55%)
        50,250,802      dTLB-load-misses                 #   96.73% of all dTLB cache accesses  (35.59%)
         1,076,721      iTLB-loads                       #  187.195 K/sec                       (35.79%)
           550,542      iTLB-load-misses                 #   51.13% of all iTLB cache accesses  (35.79%)
     3,002,586,380      L1-dcache-prefetches             #  522.019 M/sec                       (35.71%)

       5.744305165 seconds time elapsed


$ perf stat -MPipelineL2 --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '27109':

        39,113,007      ex_ret_brn_misp                  #      0.7 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (25.18%)
    50,200,949,128      de_src_op_disp.all                                                      (25.18%)
         1,968,782      resyncs_or_nc_redirects                                                 (25.18%)
    52,694,334,454      ls_not_halted_cyc                                                       (25.18%)
    47,925,464,427      ex_ret_ops                                                              (25.18%)
    30,460,085,095      ex_no_retire.load_not_complete   #     20.4 %  backend_bound_cpu
                                                         #     60.2 %  backend_bound_memory     (24.92%)
   254,793,150,309      de_no_dispatch_per_slot.backend_stalls                                  (24.92%)
    40,753,146,750      ex_no_retire.not_complete                                               (24.92%)
    52,681,078,777      ls_not_halted_cyc                                                       (24.92%)
       145,769,746      ex_ret_ucode_ops                 #     15.1 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (24.93%)
    52,727,754,917      ls_not_halted_cyc                                                       (24.93%)
    47,962,310,971      ex_ret_ops                                                              (24.93%)
    10,680,016,198      de_no_dispatch_per_slot.no_ops_from_frontend                #      0.7 %  frontend_bound_bandwidth  (24.98%)
     1,415,570,013      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      2.7 %  frontend_bound_latency   (24.98%)
    52,753,827,046      ls_not_halted_cyc                                                       (24.98%)

       9.593556870 seconds time elapsed
```

### clang
```
$ ./out/build/clang/gui/kinematics-demo-gui 5000000 &

$ perf stat -ddd --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '27253':

          8,286.88 msec task-clock                       #    1.001 CPUs utilized
            12,079      context-switches                 #    1.458 K/sec
               129      cpu-migrations                   #   15.567 /sec
                 0      page-faults                      #    0.000 /sec
    45,326,817,474      cycles                           #    5.470 GHz                         (35.69%)
       996,717,702      stalled-cycles-frontend          #    2.20% frontend cycles idle        (35.82%)
    28,452,453,064      instructions                     #    0.63  insn per cycle
                                                         #    0.04  stalled cycles per insn     (35.82%)
     1,178,421,187      branches                         #  142.203 M/sec                       (35.67%)
        17,064,859      branch-misses                    #    1.45% of all branches             (35.70%)
    19,420,768,429      L1-dcache-loads                  #    2.344 G/sec                       (35.74%)
     3,891,816,217      L1-dcache-load-misses            #   20.04% of all L1-dcache accesses   (35.70%)
       361,841,618      L1-icache-loads                  #   43.664 M/sec                       (35.74%)
         1,431,043      L1-icache-load-misses            #    0.40% of all L1-icache accesses   (35.81%)
        62,797,936      dTLB-loads                       #    7.578 M/sec                       (35.75%)
        60,866,235      dTLB-load-misses                 #   96.92% of all dTLB cache accesses  (35.64%)
         1,220,561      iTLB-loads                       #  147.288 K/sec                       (35.64%)
           806,012      iTLB-load-misses                 #   66.04% of all iTLB cache accesses  (35.67%)
     3,604,595,644      L1-dcache-prefetches             #  434.976 M/sec                       (35.63%)

       8.277444835 seconds time elapsed

$ perf stat -MPipelineL2 --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '27253':

        16,187,611      ex_ret_brn_misp                  #      0.1 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (25.01%)
    31,505,378,451      de_src_op_disp.all                                                      (25.01%)
         1,213,525      resyncs_or_nc_redirects                                                 (25.01%)
    45,216,196,940      ls_not_halted_cyc                                                       (25.01%)
    31,181,588,643      ex_ret_ops                                                              (25.01%)
    30,672,022,117      ex_no_retire.load_not_complete   #     11.6 %  backend_bound_cpu
                                                         #     73.7 %  backend_bound_memory     (25.22%)
   231,418,429,810      de_no_dispatch_per_slot.backend_stalls                                        (25.22%)
    35,506,602,051      ex_no_retire.not_complete                                               (25.22%)
    45,196,491,777      ls_not_halted_cyc                                                       (25.22%)
       104,129,958      ex_ret_ucode_ops                 #     11.5 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (24.91%)
    45,231,992,865      ls_not_halted_cyc                                                       (24.91%)
    31,212,978,209      ex_ret_ops                                                              (24.91%)
     8,084,941,184      de_no_dispatch_per_slot.no_ops_from_frontend                #      0.6 %  frontend_bound_bandwidth  (24.86%)
     1,079,533,989      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      2.4 %  frontend_bound_latency   (24.86%)
    45,261,692,156      ls_not_halted_cyc                                                       (24.86%)

       8.227905822 seconds time elapsed
```
