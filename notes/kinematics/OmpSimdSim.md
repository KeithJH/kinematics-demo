# OmpSimdSim
Same layout as `StructOfVectorSim`, but uses OpenMP for vectorizing code.

```
void UpdateHelper(const float deltaTime, float *__restrict__ bodiesX, float *__restrict__ bodiesY,
                  float *__restrict__ bodiesHorizontalSpeed, float *__restrict__ bodiesVerticalSpeed)
{
	const auto numBodies = GetNumBodies();
#pragma omp simd
	for (auto i = 0zu; i < numBodies; i++)
	{
		// Update position based on speed
		bodiesX[i] += bodiesHorizontalSpeed[i] * deltaTime;
		bodiesY[i] += bodiesVerticalSpeed[i] * deltaTime;

		// Bounce horizontally
		if (BounceCheck(bodiesX[i], bodiesHorizontalSpeed[i], _width))
		{
			bodiesHorizontalSpeed[i] *= -1;
		}

		// Bounce vertically
		if (BounceCheck(bodiesY[i], bodiesVerticalSpeed[i], _height))
		{
			bodiesVerticalSpeed[i] *= -1;
		}
	}
}
```
## Benchmark Comparison
### gcc
```
$ g++ --version | head -1
g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0

$ ./out/build/release/bench/kinematics-demo-bench --benchmark-no-analysis --rng-seed 433152058
benchmark name                            samples    iterations          mean
-------------------------------------------------------------------------------
<BENCH>
Update OmpSimdSim: 1000000                     100             1    216.402 us
Update OmpSimdSim: 5000000                     100             1    2.09003 ms
```

### clang
```
$ clang++ --version | head -1
Ubuntu clang version 18.1.3 (1ubuntu1)

$ ./out/build/clang/bench/kinematics-demo-bench --benchmark-no-analysis --rng-seed 433152058
benchmark name                            samples    iterations          mean
-------------------------------------------------------------------------------
Update OmpSimdSim: 1000000                     100             1    152.299 us
Update OmpSimdSim: 5000000                     100             1    2.46644 ms
```

## Assembly Analysis
No immediately noticeable difference from the non-OpenMP vectoriztion.

### gcc
```
$ perf record -D 1000 ./out/build/release/gui/kinematics-demo-gui 5000000
<wait some time, then close>

$ perf annotate -Mintel
kinematics::OmpSimdSim::UpdateHelper(float, float*, float*, float*, float*)
<snip>
  1.32 │ a0:   vcmpltps     k1,zmm1,zmm0
  1.20 │       vaddps       zmm2,zmm3,zmm7
  1.23 │       vcmpltps     k0{k1},zmm2,zmm0
  1.28 │       knotw        k1,k0
  1.27 │       kmovw        r13d,k0
  1.28 │       vaddps       zmm2{k1}{z},zmm3,zmm5
  1.32 │       vcmpgtps     k1{k1},zmm1,zmm0
  1.42 │       vcmpgtps     k1{k1},zmm2,zmm12
  1.23 │       kmovw        eax,k1
  1.26 │       or           ax,r13w
  0.01 │     ↓ jne          180
  1.30 │       inc          r10
  1.36 │       add          rdx,0x40
  0.50 │       cmp          r10,rbx
       │     ↓ je           1b0
  0.41 │ ee:   vmovups      zmm2,ZMMWORD PTR [rcx+rdx*1]
  0.59 │       vmovups      zmm1,ZMMWORD PTR [r8+rdx*1]
  0.96 │       vmovaps      zmm3,zmm8
  0.56 │       vfmadd213ps  zmm3,zmm1,ZMMWORD PTR [rdi+rdx*1]
  8.01 │       vmovups      ZMMWORD PTR [rdi+rdx*1],zmm3
  4.26 │       vcmpltps     k2,zmm2,zmm0
  4.21 │       vcmpgtps     k1,zmm2,zmm0
  4.12 │       vmovaps      zmm4,zmm2
  4.18 │       vfmadd213ps  zmm4,zmm8,ZMMWORD PTR [rsi+rdx*1]
  6.74 │       vaddps       zmm9,zmm4,zmm7
 13.14 │       vmovups      ZMMWORD PTR [rsi+rdx*1],zmm4
  6.53 │       vcmpltps     k2{k2},zmm9,zmm0
  6.44 │       knotw        k3,k2
  6.47 │       kmovw        r12d,k2
  2.68 │       vaddps       zmm9{k3}{z},zmm4,zmm5
  2.83 │       kmovw        r13d,k3
  2.51 │       vcmpgtps     k0{k1},zmm9,zmm13
  2.63 │       kmovw        eax,k0
  2.79 │       and          eax,r13d
  2.72 │       or           ax,r12w
       │     ↑ je           a0
<snip>
```

### clang
```
$ perf record -D 1000 ./out/build/clang/gui/kinematics-demo-gui 5000000
<wait some time, then close>

$ perf annotate -Mintel
kinematics::OmpSimdSim::UpdateHelper(float, float*, float*, float*, float*)
<snip>
  0.90 │180:   vmovups      zmm10,ZMMWORD PTR [rcx+r10*4]
  0.76 │       vmovups      zmm11,ZMMWORD PTR [rsi+r10*4]
  0.48 │       vmovups      zmm12,ZMMWORD PTR [r8+r10*4]
  1.53 │       vmovups      zmm13,ZMMWORD PTR [rdx+r10*4]
  1.22 │       vfmadd231ps  zmm11,zmm10,zmm3
  1.36 │       vxorps       zmm15,zmm10,zmm8
  1.23 │       vfmadd231ps  zmm13,zmm12,zmm3
  1.87 │       vcmpltps     k1,zmm10,zmm15
  1.94 │       vaddps       zmm14,zmm11,zmm6
  9.55 │       vmovups      ZMMWORD PTR [rsi+r10*4],zmm11
  4.34 │       vaddps       zmm11,zmm11,zmm9
 14.71 │       vmovups      ZMMWORD PTR [rdx+r10*4],zmm13
  7.51 │       vcmpltps     k0{k1},zmm14,zmm7
  6.52 │       vcmpltps     k1,zmm15,zmm10
  6.16 │       vaddps       zmm10,zmm13,zmm6
  6.33 │       vaddps       zmm13,zmm13,zmm9
  5.09 │       vcmpltps     k1{k1},zmm4,zmm11
  5.29 │       vxorps       zmm11,zmm12,zmm8
  5.35 │       korw         k1,k1,k0
  6.39 │       vmovups      ZMMWORD PTR [rcx+r10*4]{k1},zmm15
  3.05 │       vcmpltps     k1,zmm12,zmm11
  0.96 │       vcmpltps     k0{k1},zmm10,zmm7
  0.95 │       vcmpltps     k1,zmm11,zmm12
  0.90 │       vcmpltps     k1{k1},zmm5,zmm13
  1.12 │       korw         k1,k1,k0
  2.35 │       vmovups      ZMMWORD PTR [r8+r10*4]{k1},zmm11
  1.10 │       add          r10,0x10
  1.06 │       cmp          rdi,r10
       │     ↑ jne          180
<snip>
```

## Additional Data
### gcc
```
$ ./out/build/release/gui/kinematics-demo-gui 5000000 &

$ perf stat -ddd --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '31443':

          5,639.52 msec task-clock                       #    1.001 CPUs utilized
             9,699      context-switches                 #    1.720 K/sec
               117      cpu-migrations                   #   20.746 /sec
                 0      page-faults                      #    0.000 /sec
    30,827,993,066      cycles                           #    5.466 GHz                         (35.50%)
       836,256,575      stalled-cycles-frontend          #    2.71% frontend cycles idle        (35.53%)
    28,173,926,503      instructions                     #    0.91  insn per cycle
                                                         #    0.03  stalled cycles per insn     (35.88%)
     2,450,026,806      branches                         #  434.439 M/sec                       (35.91%)
        24,099,665      branch-misses                    #    0.98% of all branches             (36.00%)
    11,400,268,395      L1-dcache-loads                  #    2.021 G/sec                       (36.01%)
     3,088,500,603      L1-dcache-load-misses            #   27.09% of all L1-dcache accesses   (36.04%)
       277,023,979      L1-icache-loads                  #   49.122 M/sec                       (35.83%)
         1,247,039      L1-icache-load-misses            #    0.45% of all L1-icache accesses   (35.75%)
        50,369,265      dTLB-loads                       #    8.931 M/sec                       (35.64%)
        48,652,114      dTLB-load-misses                 #   96.59% of all dTLB cache accesses  (35.55%)
         1,055,957      iTLB-loads                       #  187.242 K/sec                       (35.48%)
           620,933      iTLB-load-misses                 #   58.80% of all iTLB cache accesses  (35.45%)
     2,994,585,105      L1-dcache-prefetches             #  531.000 M/sec                       (35.45%)

       5.631940438 seconds time elapsed

$ perf stat -MPipelineL2 --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '31443':

        47,641,469      ex_ret_brn_misp                  #      0.7 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (24.94%)
    57,926,321,273      de_src_op_disp.all                                                      (24.94%)
         2,152,589      resyncs_or_nc_redirects                                                 (24.94%)
    61,729,721,447      ls_not_halted_cyc                                                       (24.94%)
    55,104,498,184      ex_ret_ops                                                              (24.94%)
    35,573,254,326      ex_no_retire.load_not_complete   #     20.5 %  backend_bound_cpu
                                                         #     60.2 %  backend_bound_memory     (25.03%)
   299,048,140,951      de_no_dispatch_per_slot.backend_stalls                                  (25.03%)
    47,689,982,568      ex_no_retire.not_complete                                               (25.03%)
    61,713,748,438      ls_not_halted_cyc                                                       (25.03%)
       168,680,672      ex_ret_ucode_ops                 #     14.8 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (24.95%)
    61,763,800,440      ls_not_halted_cyc                                                       (24.95%)
    55,164,197,902      ex_ret_ops                                                              (24.95%)
    12,892,057,596      de_no_dispatch_per_slot.no_ops_from_frontend                #      0.6 %  frontend_bound_bandwidth  (25.08%)
     1,761,848,048      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      2.9 %  frontend_bound_latency   (25.08%)
    61,799,648,939      ls_not_halted_cyc                                                       (25.08%)

      11.240445448 seconds time elapsed
```

### clang
```
$ ./out/build/clang/gui/kinematics-demo-gui 5000000 &

$ perf stat -ddd --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '31517':

          4,152.13 msec task-clock                       #    1.001 CPUs utilized
             5,992      context-switches                 #    1.443 K/sec
                81      cpu-migrations                   #   19.508 /sec
                 0      page-faults                      #    0.000 /sec
    22,751,460,510      cycles                           #    5.479 GHz                         (35.58%)
       430,690,447      stalled-cycles-frontend          #    1.89% frontend cycles idle        (35.56%)
    14,123,454,578      instructions                     #    0.62  insn per cycle
                                                         #    0.03  stalled cycles per insn     (35.50%)
       578,766,830      branches                         #  139.390 M/sec                       (35.63%)
         7,757,822      branch-misses                    #    1.34% of all branches             (35.60%)
     9,645,261,498      L1-dcache-loads                  #    2.323 G/sec                       (35.58%)
     1,930,478,374      L1-dcache-load-misses            #   20.01% of all L1-dcache accesses   (35.67%)
       182,437,622      L1-icache-loads                  #   43.938 M/sec                       (35.71%)
           635,738      L1-icache-load-misses            #    0.35% of all L1-icache accesses   (35.84%)
        31,215,256      dTLB-loads                       #    7.518 M/sec                       (35.94%)
        30,129,106      dTLB-load-misses                 #   96.52% of all dTLB cache accesses  (35.92%)
           630,557      iTLB-loads                       #  151.863 K/sec                       (35.90%)
           358,833      iTLB-load-misses                 #   56.91% of all iTLB cache accesses  (35.88%)
     1,622,699,729      L1-dcache-prefetches             #  390.811 M/sec                       (35.70%)

       4.146285329 seconds time elapsed

$ perf stat -MPipelineL2 --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '31517':

        12,400,775      ex_ret_brn_misp                  #      0.1 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (24.96%)
    24,494,390,769      de_src_op_disp.all                                                      (24.96%)
           910,546      resyncs_or_nc_redirects                                                 (24.96%)
    35,711,732,169      ls_not_halted_cyc                                                       (24.96%)
    24,241,563,800      ex_ret_ops                                                              (24.96%)
    24,476,941,257      ex_no_retire.load_not_complete   #     11.1 %  backend_bound_cpu
                                                         #     74.4 %  backend_bound_memory     (25.00%)
   183,033,146,655      de_no_dispatch_per_slot.backend_stalls                                  (25.00%)
    28,126,848,218      ex_no_retire.not_complete                                               (25.00%)
    35,701,299,051      ls_not_halted_cyc                                                       (25.00%)
        83,849,743      ex_ret_ucode_ops                 #     11.3 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (24.96%)
    35,731,878,825      ls_not_halted_cyc                                                       (24.96%)
    24,284,589,350      ex_ret_ops                                                              (24.96%)
     6,426,419,951      de_no_dispatch_per_slot.no_ops_from_frontend                #      0.9 %  frontend_bound_bandwidth  (25.08%)
       763,572,620      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      2.1 %  frontend_bound_latency   (25.08%)
    35,748,384,952      ls_not_halted_cyc                                                       (25.08%)

       6.499426813 seconds time elapsed
```
