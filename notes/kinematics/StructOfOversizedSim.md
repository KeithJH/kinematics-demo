# StructOfOversizedSim
SoA layout that uses `float*` fields manually managed with `new[]` and `delete[]` while specifying alignment and ensuring adequate capacity that allows for vector commands to "overrun" the actual amount of `Bodies` in the simulation to avoid non-vectorized "tail" calculations.

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

	const auto numBodies = _updateBoundary;
	ASSUME(numBodies % 16 == 0);

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
Compiling the same sources with both `g++` (version 13.3.0) and `clang++` (version 18.1.3) results in mixed results, but within the same ballpark. Very similar to the `StructOfVectorSim` results

### gcc
```
$ g++ --version | head -1
g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0

$ ./out/build/release/bench/kinematics-demo-bench --benchmark-no-analysis --rng-seed 433152058
benchmark name                            samples    iterations          mean
-------------------------------------------------------------------------------
Update StructOfOversizedSim: 1000000           100             1     209.47 us
Update StructOfOversizedSim: 5000000           100             1    2.16754 ms
```

### clang
```
$ clang++ --version | head -1
Ubuntu clang version 18.1.3 (1ubuntu1)

$ ./out/build/clang/bench/kinematics-demo-bench --benchmark-no-analysis --rng-seed 433152058
benchmark name                            samples    iterations          mean
-------------------------------------------------------------------------------
Update StructOfOversizedSim: 1000000           100             1    151.402 us
Update StructOfOversizedSim: 5000000           100             1    2.51354 ms
```

## Assembly Analysis
### gcc
The compiler is able to remove some code for handling the "tail" of calculations (usually not shown in these notes for brevity), but still oddly includes both 512- and 256-bit calculations. No 128-bit calculations are used, nor any substantial scalar code.

```
$ perf record -D 1000 ./out/build/release/gui/kinematics-demo-gui 5000000
<wait some time, then close>

$ perf annotate -Mintel
kinematics::StructOfOversizedSim::UpdateHelper(float, float*, float*, float*, float*)
<snip>
  0.94 │ 90:   vcmpltps     k1,zmm2,zmm1
  0.86 │       vaddps       zmm3,zmm4,zmm7
  0.94 │       vcmpltps     k0{k1},zmm3,zmm1
  0.79 │       knotw        k1,k0
  0.95 │       kmovw        ebx,k0
  0.84 │       vaddps       zmm3{k1}{z},zmm4,zmm6
  2.17 │       vcmpgtps     k1{k1},zmm2,zmm1
  1.98 │       vcmpgtps     k1{k1},zmm3,zmm12
  2.05 │       kmovw        eax,k1
  1.92 │       or           ax,bx
       │     ↓ jne          170
  1.96 │       inc          rcx
  1.83 │       add          rdx,0x40
  0.45 │       cmp          r10,rcx
       │     ↓ je           191
  0.57 │ dd:   vmovaps      zmm3,ZMMWORD PTR [r9+rdx*1]
  0.61 │       vmovaps      zmm2,ZMMWORD PTR [r8+rdx*1]
  0.87 │       vmovaps      zmm4,zmm8
  0.51 │       vfmadd213ps  zmm4,zmm2,ZMMWORD PTR [rsi+rdx*1]
  4.80 │       vmovaps      ZMMWORD PTR [rsi+rdx*1],zmm4
  2.08 │       vcmpltps     k2,zmm3,zmm1
  2.15 │       vcmpgtps     k1,zmm3,zmm1
  2.35 │       vmovaps      zmm5,zmm3
  2.39 │       vfmadd213ps  zmm5,zmm8,ZMMWORD PTR [rdi+rdx*1]
  7.88 │       vaddps       zmm9,zmm5,zmm7
 16.07 │       vmovaps      ZMMWORD PTR [rdi+rdx*1],zmm5
  8.03 │       vcmpltps     k2{k2},zmm9,zmm1
  7.79 │       knotw        k3,k2
  8.26 │       vaddps       zmm9{k3}{z},zmm5,zmm6
  2.74 │       kmovw        ebx,k3
  2.68 │       vcmpgtps     k0{k1},zmm9,zmm13
  2.63 │       kmovw        eax,k0
  2.89 │       and          eax,ebx
  2.81 │       kmovw        ebx,k2
  2.84 │       or           ax,bx
  0.02 │     ↑ je           90
<similar flow for ymm registers>
<snip>
```

### clang
The compiler is able to remove even more "tail" calculations, using only 512-bit calculations.
```
$ perf record -D 1000 ./out/build/clang/gui/kinematics-demo-gui 5000000
<wait some time, then close>

$ perf annotate -Mintel
       │     00000000000095f0 <kinematics::StructOfOversizedSim::Update(float)>:
       │       mov          rax,QWORD PTR [rdi+0x78]
       │       test         rax,rax
       │     ↓ je           115
       │       vbroadcastss zmm1,DWORD PTR [rdi+0x8]
       │       vbroadcastss zmm2,DWORD PTR [rdi+0xc]
       │       vbroadcastss zmm3,DWORD PTR [rip+0xa8bfb]        # b2210 <_IO_stdin_used+0x210>
       │       vbroadcastss zmm6,DWORD PTR [rip+0xa8a71]        # b2090 <_IO_stdin_used+0x90>
       │       vbroadcastss zmm5,DWORD PTR [rip+0xa8adb]        # b2104 <_IO_stdin_used+0x104>
       │       mov          rcx,QWORD PTR [rdi+0x40]
       │       mov          rdx,QWORD PTR [rdi+0x48]
       │       mov          rsi,QWORD PTR [rdi+0x50]
       │       mov          r8,QWORD PTR [rdi+0x58]
       │       vbroadcastss zmm0,xmm0
       │       vxorps       xmm4,xmm4,xmm4
       │       xor          edi,edi
       │       data16       cs nop WORD PTR [rax+rax*1+0x0]
  0.98 │ 60:   vmovaps      zmm7,ZMMWORD PTR [rsi+rdi*4]
  1.13 │       vmovaps      zmm8,ZMMWORD PTR [rcx+rdi*4]
  1.93 │       vfmadd231ps  zmm8,zmm7,zmm0
  2.75 │       vxorps       zmm12,zmm7,zmm5
  1.94 │       vcmpltps     k1,zmm7,zmm12
  2.45 │       vmovaps      ZMMWORD PTR [rcx+rdi*4],zmm8
  1.51 │       vaddps       zmm11,zmm8,zmm3
  1.17 │       vaddps       zmm8,zmm8,zmm6
  1.08 │       vmovaps      zmm9,ZMMWORD PTR [r8+rdi*4]
  1.28 │       vmovaps      zmm10,ZMMWORD PTR [rdx+rdi*4]
  1.70 │       vcmpltps     k0{k1},zmm11,zmm4
  1.76 │       vcmpltps     k1,zmm12,zmm7
  1.73 │       vcmpltps     k1{k1},zmm1,zmm8
  6.22 │       korw         k1,k1,k0
  6.24 │       vfmadd231ps  zmm10,zmm9,zmm0
  6.53 │       vxorps       zmm8,zmm9,zmm5
 11.10 │       vaddps       zmm7,zmm10,zmm3
 22.17 │       vmovaps      ZMMWORD PTR [rdx+rdi*4],zmm10
 11.98 │       vmovups      ZMMWORD PTR [rsi+rdi*4]{k1},zmm12
  5.97 │       vcmpltps     k1,zmm9,zmm8
  0.85 │       vaddps       zmm10,zmm10,zmm6
  0.72 │       vcmpltps     k0{k1},zmm7,zmm4
  0.75 │       vcmpltps     k1,zmm8,zmm9
  1.33 │       vcmpltps     k1{k1},zmm2,zmm10
  1.09 │       korw         k1,k1,k0
  1.75 │       vmovups      ZMMWORD PTR [r8+rdi*4]{k1},zmm8
  0.94 │       add          rdi,0x10
  0.91 │       cmp          rax,rdi
  0.05 │     ↑ jne          60
       │115:   vzeroupper
       │     ← ret
```

## Additional Data
### gcc
```
$ ./out/build/release/gui/kinematics-demo-gui 5000000 &

$ perf stat -ddd --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '34395':

          4,902.44 msec task-clock                       #    1.001 CPUs utilized
             8,341      context-switches                 #    1.701 K/sec
               103      cpu-migrations                   #   21.010 /sec
                 0      page-faults                      #    0.000 /sec
    26,795,778,953      cycles                           #    5.466 GHz                         (35.77%)
       706,244,689      stalled-cycles-frontend          #    2.64% frontend cycles idle        (35.84%)
    24,226,228,510      instructions                     #    0.90  insn per cycle
                                                         #    0.03  stalled cycles per insn     (35.87%)
     2,107,483,845      branches                         #  429.885 M/sec                       (35.99%)
        20,403,524      branch-misses                    #    0.97% of all branches             (36.02%)
     8,396,560,628      L1-dcache-loads                  #    1.713 G/sec                       (35.75%)
     2,610,642,103      L1-dcache-load-misses            #   31.09% of all L1-dcache accesses   (35.75%)
       247,116,933      L1-icache-loads                  #   50.407 M/sec                       (35.73%)
           982,238      L1-icache-load-misses            #    0.40% of all L1-icache accesses   (35.59%)
        43,355,905      dTLB-loads                       #    8.844 M/sec                       (35.52%)
        41,958,210      dTLB-load-misses                 #   96.78% of all dTLB cache accesses  (35.59%)
           852,091      iTLB-loads                       #  173.810 K/sec                       (35.59%)
           649,908      iTLB-load-misses                 #   76.27% of all iTLB cache accesses  (35.47%)
     1,955,938,784      L1-dcache-prefetches             #  398.973 M/sec                       (35.53%)

       4.898970398 seconds time elapsed

$ perf stat -MPipelineL2 --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '34395':

        28,162,650      ex_ret_brn_misp                  #      0.7 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (24.88%)
    34,700,574,158      de_src_op_disp.all                                                      (24.88%)
         1,166,489      resyncs_or_nc_redirects                                                 (24.88%)
    38,181,077,381      ls_not_halted_cyc                                                       (24.88%)
    33,044,695,760      ex_ret_ops                                                              (24.88%)
    22,452,988,267      ex_no_retire.load_not_complete   #     22.0 %  backend_bound_cpu
                                                         #     59.8 %  backend_bound_memory     (24.87%)
   187,341,352,204      de_no_dispatch_per_slot.backend_stalls                                  (24.87%)
    30,710,508,182      ex_no_retire.not_complete                                               (24.87%)
    38,173,223,007      ls_not_halted_cyc                                                       (24.87%)
       102,907,056      ex_ret_ucode_ops                 #     14.4 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (25.17%)
    38,198,170,623      ls_not_halted_cyc                                                       (25.17%)
    33,051,388,333      ex_ret_ops                                                              (25.17%)
     6,842,680,662      de_no_dispatch_per_slot.no_ops_from_frontend                #      0.6 %  frontend_bound_bandwidth  (25.08%)
       907,880,396      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      2.4 %  frontend_bound_latency   (25.08%)
    38,217,431,974      ls_not_halted_cyc                                                       (25.08%)

       6.947327283 seconds time elapsed
```

### clang
```
$ ./out/build/clang/gui/kinematics-demo-gui 5000000 &

$ perf stat -ddd --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '34482':

          7,354.15 msec task-clock                       #    1.001 CPUs utilized
            10,502      context-switches                 #    1.428 K/sec
                80      cpu-migrations                   #   10.878 /sec
                 0      page-faults                      #    0.000 /sec
    40,245,740,582      cycles                           #    5.473 GHz                         (35.74%)
       892,306,250      stalled-cycles-frontend          #    2.22% frontend cycles idle        (35.68%)
    24,852,713,939      instructions                     #    0.62  insn per cycle
                                                         #    0.04  stalled cycles per insn     (35.73%)
     1,025,174,472      branches                         #  139.401 M/sec                       (35.64%)
        14,219,622      branch-misses                    #    1.39% of all branches             (35.63%)
    13,663,055,628      L1-dcache-loads                  #    1.858 G/sec                       (35.61%)
     3,298,484,032      L1-dcache-load-misses            #   24.14% of all L1-dcache accesses   (35.67%)
       317,040,045      L1-icache-loads                  #   43.110 M/sec                       (35.72%)
         1,203,528      L1-icache-load-misses            #    0.38% of all L1-icache accesses   (35.73%)
        54,905,959      dTLB-loads                       #    7.466 M/sec                       (35.78%)
        52,965,534      dTLB-load-misses                 #   96.47% of all dTLB cache accesses  (35.80%)
         1,121,563      iTLB-loads                       #  152.507 K/sec                       (35.78%)
           733,636      iTLB-load-misses                 #   65.41% of all iTLB cache accesses  (35.77%)
     3,256,328,190      L1-dcache-prefetches             #  442.788 M/sec                       (35.74%)

       7.346617508 seconds time elapsed

$ perf stat -MPipelineL2 --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '34482':

        19,257,667      ex_ret_brn_misp                  #      0.2 %  bad_speculation_mispredicts
                                                         #      0.1 %  bad_speculation_pipeline_restarts  (24.96%)
    39,561,894,323      de_src_op_disp.all                                                      (24.96%)
         5,204,585      resyncs_or_nc_redirects                                                 (24.96%)
    57,461,791,941      ls_not_halted_cyc                                                       (24.96%)
    38,663,779,391      ex_ret_ops                                                              (24.96%)
    41,172,007,399      ex_no_retire.load_not_complete   #     11.7 %  backend_bound_cpu
                                                         #     74.0 %  backend_bound_memory     (25.05%)
   295,390,183,949      de_no_dispatch_per_slot.backend_stalls                                  (25.05%)
    47,696,831,370      ex_no_retire.not_complete                                               (25.05%)
    57,443,179,138      ls_not_halted_cyc                                                       (25.05%)
       128,831,400      ex_ret_ucode_ops                 #     11.2 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (25.07%)
    57,489,581,197      ls_not_halted_cyc                                                       (25.07%)
    38,715,576,163      ex_ret_ops                                                              (25.07%)
     9,476,928,173      de_no_dispatch_per_slot.no_ops_from_frontend                #      0.5 %  frontend_bound_bandwidth  (24.92%)
     1,287,911,825      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      2.2 %  frontend_bound_latency   (24.92%)
    57,521,818,486      ls_not_halted_cyc                                                       (24.92%)

      10.455506980 seconds time elapsed
```
