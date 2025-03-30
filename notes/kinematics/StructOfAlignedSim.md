# StructOfAlignedSim
SoA layout that uses `float*` fields manually managed with `new[]` and `delete[]` while specifying alignment. With a strict enough alignment it is hoped that compilers will start to use the aligned version of loading data, which may perform better on some systems.

```
struct Bodies
{
        float *x, *y;
        float *horizontalSpeed, *verticalSpeed;
        Color *color;
};
Bodies _bodies;

void UpdateHelper(const float deltaTime, float *__restrict__ bodiesX, float *__restrict__ bodiesY,
                  float *__restrict__ bodiesHorizontalSpeed,
                  float *__restrict__ bodiesVerticalSpeed)
{
	bodiesX = std::assume_aligned<ALIGNMENT_SIZE>(bodiesX);
	bodiesY = std::assume_aligned<ALIGNMENT_SIZE>(bodiesY);
	bodiesHorizontalSpeed = std::assume_aligned<ALIGNMENT_SIZE>(bodiesHorizontalSpeed);
	bodiesVerticalSpeed = std::assume_aligned<ALIGNMENT_SIZE>(bodiesVerticalSpeed);

	const auto numBodies = GetNumBodies();
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
Aligned vector loads do not appear to give noticeable performance improvement on the system tested.

### gcc
```
$ g++ --version | head -1
g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0

$ ./out/build/release/bench/kinematics-demo-bench --benchmark-no-analysis --rng-seed 433152058
benchmark name                            samples    iterations          mean
-------------------------------------------------------------------------------
Update StructOfAlignedSim: 1000000             100             1    209.637 us
Update StructOfAlignedSim: 5000000             100             1    2.16666 ms
```

### clang
```
$ clang++ --version | head -1
Ubuntu clang version 18.1.3 (1ubuntu1)

$ ./out/build/clang/bench/kinematics-demo-bench --benchmark-no-analysis --rng-seed 433152058
benchmark name                            samples    iterations          mean
-------------------------------------------------------------------------------
Update StructOfAlignedSim: 1000000             100             1    139.924 us
Update StructOfAlignedSim: 5000000             100             1    2.43298 ms
```

## Assembly Analysis
Both version use the aligned load instructions (`vmovaps` versus `vmovups`)! Otherwise essentially the same as the `StructOfPointer` counterparts.

### gcc
```
$ perf record -D 1000 ./out/build/release/gui/kinematics-demo-gui 5000000
<wait some time, then close>

$ perf annotate -Mintel
kinematics::StructOfAlignedSim::UpdateHelper(float, float*, float*, float*, float*)
<snip>
  0.99 │ 90:   vcmpltps     k1,zmm1,zmm0
  1.04 │       vaddps       zmm2,zmm3,zmm7
  0.84 │       vcmpltps     k0{k1},zmm2,zmm0
  0.87 │       knotw        k1,k0
  0.94 │       kmovw        ebx,k0
  1.11 │       vaddps       zmm2{k1}{z},zmm3,zmm5
  2.05 │       vcmpgtps     k1{k1},zmm1,zmm0
  1.89 │       vcmpgtps     k1{k1},zmm2,zmm12
  2.24 │       kmovw        eax,k1
  1.82 │       or           ax,bx
       │     ↓ jne          170
  1.85 │       inc          r10
  1.85 │       add          rdx,0x40
  0.46 │       cmp          r11,r10
       │     ↓ je           191
  0.53 │ dd:   vmovaps      zmm2,ZMMWORD PTR [rcx+rdx*1]
  0.48 │       vmovaps      zmm1,ZMMWORD PTR [r8+rdx*1]
  0.86 │       vmovaps      zmm3,zmm8
  0.43 │       vfmadd213ps  zmm3,zmm1,ZMMWORD PTR [rdi+rdx*1]
  3.95 │       vmovaps      ZMMWORD PTR [rdi+rdx*1],zmm3
  2.05 │       vcmpltps     k2,zmm2,zmm0
  1.97 │       vcmpgtps     k1,zmm2,zmm0
  1.88 │       vmovaps      zmm4,zmm2
  2.08 │       vfmadd213ps  zmm4,zmm8,ZMMWORD PTR [rsi+rdx*1]
  8.06 │       vaddps       zmm9,zmm4,zmm7
 16.60 │       vmovaps      ZMMWORD PTR [rsi+rdx*1],zmm4
  7.89 │       vcmpltps     k2{k2},zmm9,zmm0
  8.22 │       knotw        k3,k2
  8.11 │       vaddps       zmm9{k3}{z},zmm4,zmm5
  2.89 │       kmovw        ebx,k3
  2.76 │       vcmpgtps     k0{k1},zmm9,zmm13
  2.92 │       kmovw        eax,k0
  3.25 │       and          eax,ebx
  2.91 │       kmovw        ebx,k2
  2.77 │       or           ax,bx
  0.01 │     ↑ je           90
<snip>
```

### clang
```
$ perf record -D 1000 ./out/build/clang/gui/kinematics-demo-gui 5000000
<wait some time, then close>

$ perf annotate -Mintel
kinematics::StructOfAlignedSim::Update(float)
<snip>
  0.63 │180:   vmovaps      zmm10,ZMMWORD PTR [rsi+r9*4]
  0.66 │       vmovaps      zmm11,ZMMWORD PTR [rcx+r9*4]
  1.30 │       vfmadd231ps  zmm11,zmm10,zmm3
  1.72 │       vxorps       zmm15,zmm10,zmm8
  1.28 │       vcmpltps     k1,zmm10,zmm15
  2.17 │       vmovaps      ZMMWORD PTR [rcx+r9*4],zmm11
  1.31 │       vaddps       zmm14,zmm11,zmm6
  0.84 │       vaddps       zmm11,zmm11,zmm9
  0.80 │       vmovaps      zmm12,ZMMWORD PTR [r8+r9*4]
  0.88 │       vmovaps      zmm13,ZMMWORD PTR [rdx+r9*4]
  1.22 │       vcmpltps     k0{k1},zmm14,zmm7
  1.20 │       vcmpltps     k1,zmm15,zmm10
  1.23 │       vcmpltps     k1{k1},zmm4,zmm11
  6.62 │       korw         k1,k1,k0
  6.57 │       vfmadd231ps  zmm13,zmm12,zmm3
  6.55 │       vxorps       zmm11,zmm12,zmm8
 12.38 │       vaddps       zmm10,zmm13,zmm6
 24.45 │       vmovaps      ZMMWORD PTR [rdx+r9*4],zmm13
 13.38 │       vmovups      ZMMWORD PTR [rsi+r9*4]{k1},zmm15
  6.76 │       vcmpltps     k1,zmm12,zmm11
  0.75 │       vaddps       zmm13,zmm13,zmm9
  0.65 │       vcmpltps     k0{k1},zmm10,zmm7
  0.66 │       vcmpltps     k1,zmm11,zmm12
  1.50 │       vcmpltps     k1{k1},zmm5,zmm13
  1.11 │       korw         k1,k1,k0
  1.77 │       vmovups      ZMMWORD PTR [r8+r9*4]{k1},zmm11
  0.82 │       add          r9,0x10
  0.78 │       cmp          rdi,r9
       │     ↑ jne          180
<snip>
```

## Additional Data
### gcc
```
$ ./out/build/release/gui/kinematics-demo-gui 5000000 &

$ perf stat -ddd --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '32854':

          5,222.58 msec task-clock                       #    1.001 CPUs utilized
             9,244      context-switches                 #    1.770 K/sec
                56      cpu-migrations                   #   10.723 /sec
                 0      page-faults                      #    0.000 /sec
    28,550,482,658      cycles                           #    5.467 GHz                         (35.60%)
       750,766,474      stalled-cycles-frontend          #    2.63% frontend cycles idle        (35.68%)
    26,966,395,584      instructions                     #    0.94  insn per cycle
                                                         #    0.03  stalled cycles per insn     (35.81%)
     2,342,431,105      branches                         #  448.520 M/sec                       (35.90%)
        21,592,521      branch-misses                    #    0.92% of all branches             (35.93%)
     9,324,469,246      L1-dcache-loads                  #    1.785 G/sec                       (35.66%)
     2,902,620,563      L1-dcache-load-misses            #   31.13% of all L1-dcache accesses   (35.86%)
       268,913,786      L1-icache-loads                  #   51.491 M/sec                       (35.72%)
         1,028,667      L1-icache-load-misses            #    0.38% of all L1-icache accesses   (35.70%)
        48,286,769      dTLB-loads                       #    9.246 M/sec                       (35.87%)
        46,726,992      dTLB-load-misses                 #   96.77% of all dTLB cache accesses  (35.73%)
           958,773      iTLB-loads                       #  183.582 K/sec                       (35.53%)
           647,472      iTLB-load-misses                 #   67.53% of all iTLB cache accesses  (35.55%)
     2,877,712,099      L1-dcache-prefetches             #  551.013 M/sec                       (35.46%)

       5.216792815 seconds time elapsed

$ perf stat -MPipelineL2 --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '32854':

        33,315,013      ex_ret_brn_misp                  #      0.7 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (24.93%)
    42,284,732,756      de_src_op_disp.all                                                      (24.93%)
         1,476,757      resyncs_or_nc_redirects                                                 (24.93%)
    43,483,236,417      ls_not_halted_cyc                                                       (24.93%)
    40,387,777,092      ex_ret_ops                                                              (24.93%)
    26,022,776,903      ex_no_retire.load_not_complete   #     20.1 %  backend_bound_cpu
                                                         #     60.3 %  backend_bound_memory     (24.91%)
   209,654,133,773      de_no_dispatch_per_slot.backend_stalls                                  (24.91%)
    34,682,828,102      ex_no_retire.not_complete                                               (24.91%)
    43,472,693,967      ls_not_halted_cyc                                                       (24.91%)
       118,573,175      ex_ret_ucode_ops                 #     15.4 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (25.16%)
    43,511,023,923      ls_not_halted_cyc                                                       (25.16%)
    40,404,519,861      ex_ret_ops                                                              (25.16%)
     8,727,048,218      de_no_dispatch_per_slot.no_ops_from_frontend                #      0.6 %  frontend_bound_bandwidth  (25.00%)
     1,182,074,073      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      2.7 %  frontend_bound_latency   (25.00%)
    43,537,167,471      ls_not_halted_cyc                                                       (25.00%)

       7.918889084 seconds time elapsed
```

### clang
```
$ ./out/build/clang/gui/kinematics-demo-gui 5000000 &

$ perf stat -ddd --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
   Performance counter stats for process id '32926':

          7,379.25 msec task-clock                       #    1.001 CPUs utilized
            10,786      context-switches                 #    1.462 K/sec
                42      cpu-migrations                   #    5.692 /sec
                 0      page-faults                      #    0.000 /sec
    40,339,906,568      cycles                           #    5.467 GHz                         (35.67%)
       953,304,878      stalled-cycles-frontend          #    2.36% frontend cycles idle        (35.71%)
    25,519,401,840      instructions                     #    0.63  insn per cycle
                                                         #    0.04  stalled cycles per insn     (35.85%)
     1,050,900,278      branches                         #  142.413 M/sec                       (35.87%)
        15,067,124      branch-misses                    #    1.43% of all branches             (35.87%)
    13,948,204,234      L1-dcache-loads                  #    1.890 G/sec                       (35.83%)
     3,390,830,899      L1-dcache-load-misses            #   24.31% of all L1-dcache accesses   (35.89%)
       334,419,362      L1-icache-loads                  #   45.319 M/sec                       (35.79%)
         1,237,696      L1-icache-load-misses            #    0.37% of all L1-icache accesses   (35.65%)
        56,270,879      dTLB-loads                       #    7.626 M/sec                       (35.56%)
        54,487,777      dTLB-load-misses                 #   96.83% of all dTLB cache accesses  (35.60%)
         1,264,138      iTLB-loads                       #  171.310 K/sec                       (35.54%)
           690,876      iTLB-load-misses                 #   54.65% of all iTLB cache accesses  (35.55%)
     2,542,222,506      L1-dcache-prefetches             #  344.510 M/sec                       (35.62%)

       7.368769675 seconds time elapsed

$ perf stat -MPipelineL2 --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '32926':

        16,856,897      ex_ret_brn_misp                  #      0.2 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (24.99%)
    32,983,812,889      de_src_op_disp.all                                                      (24.99%)
         2,609,821      resyncs_or_nc_redirects                                                 (24.99%)
    46,356,162,068      ls_not_halted_cyc                                                       (24.99%)
    32,459,059,820      ex_ret_ops                                                              (24.99%)
    33,288,565,688      ex_no_retire.load_not_complete   #     11.3 %  backend_bound_cpu
                                                         #     73.8 %  backend_bound_memory     (25.06%)
   236,779,866,377      de_no_dispatch_per_slot.backend_stalls                                  (25.06%)
    38,398,492,801      ex_no_retire.not_complete                                               (25.06%)
    46,350,102,482      ls_not_halted_cyc                                                       (25.06%)
       109,685,239      ex_ret_ucode_ops                 #     11.6 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (25.01%)
    46,387,977,783      ls_not_halted_cyc                                                       (25.01%)
    32,486,466,589      ex_ret_ops                                                              (25.01%)
     8,295,992,535      de_no_dispatch_per_slot.no_ops_from_frontend                #      0.6 %  frontend_bound_bandwidth  (24.95%)
     1,108,869,155      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      2.4 %  frontend_bound_latency   (24.95%)
    46,405,846,473      ls_not_halted_cyc                                                       (24.95%)

       8.435126446 seconds time elapsed
```
