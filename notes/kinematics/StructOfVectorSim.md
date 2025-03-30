# StructOfVectorSim
Structure of Arrays (SoA) style layout using parallel `std::vector` fields. This means data for a particular field is entirely contiguous in memory which typically allows for easier vectorization, though requires closer attention to keep parallel arrays "in sync" with each other. In order to get the best performance a common `UpdateHelper` method is used to easily describe that there is no overlap between the memory used for each of the fields with `__restrict__`. Making it clear that there is no aliasing involved should make it easier for compilers to decide that it is safe to use various vector instructions.

```
struct Bodies
{
      std::vector<float> x, y; // center position
      std::vector<float> horizontalSpeed, verticalSpeed;
      std::vector<Color> color;
};
Bodies _bodies;

void Update(const float deltaTime)
{
	UpdateHelper(deltaTime, _bodies.x.data(), _bodies.y.data(), _bodies.horizontalSpeed.data(), _bodies.verticalSpeed.data());
}

void UpdateHelper(const float deltaTime, float *__restrict__ bodiesX, float *__restrict__ bodiesY,
                              float *__restrict__ bodiesHorizontalSpeed, float *__restrict__ bodiesVerticalSpeed)
{
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
Compiling the same sources with both `g++` (version 13.3.0) and `clang++` (version 18.1.3) results in mixed results, but within the same ballpark.

### gcc
```
$ g++ --version | head -1
g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0

$ ./out/build/release/bench/kinematics-demo-bench --benchmark-no-analysis --rng-seed 433152058
benchmark name                            samples    iterations          mean
-------------------------------------------------------------------------------
Update StructOfVectorSim: 1000000              100             1    246.917 us
Update StructOfVectorSim: 5000000              100             1    2.20558 ms
```

### clang
```
$ clang++ --version | head -1
Ubuntu clang version 18.1.3 (1ubuntu1)

$ ./out/build/clang/bench/kinematics-demo-bench --benchmark-no-analysis --rng-seed 433152058
benchmark name                            samples    iterations          mean
-------------------------------------------------------------------------------
Update StructOfVectorSim: 1000000              100             1    187.942 us
Update StructOfVectorSim: 5000000              100             1    2.68468 ms
```

## Assembly Analysis
Generally speaking the generated assembly is fairly similar between both `g++` and `clang++`. Both are now able to primarily use 512-bit fused-multiply-adds (other variants, down to even scalar, are included for handling the "tail" calculations but don't get excercised enough to show up in the copied portions here)and masking logic around the bounce calculations. It may be here that there is a meaningful difference, but would require more investigation. Also of note, though perhaps not impactful, is that `clang++` seems to prefer to load all the values into `zmm` registers first, while `g++` seems more likely to operate with memory addresses in vector instructions such as `vfmadd213ps`.

### gcc
```
$ perf record -D 1000 ./out/build/release/gui/kinematics-demo-gui 5000000
<wait some time, then close>

$ perf annotate -Mintel
<snip>
  1.11 │ c0:   vcmpltps     k1,zmm1,zmm0
  1.25 │       vaddps       zmm2,zmm3,zmm7
  1.11 │       vcmpltps     k0{k1},zmm2,zmm0
  1.04 │       knotw        k1,k0
  1.05 │       kmovw        r9d,k0
  1.08 │       vaddps       zmm2{k1}{z},zmm3,zmm5
  1.34 │       vcmpgtps     k1{k1},zmm1,zmm0
  1.37 │       vcmpgtps     k1{k1},zmm2,zmm12
  1.42 │       kmovw        eax,k1
  1.31 │       or           ax,r9w
       │     ↓ jne          1d0
  1.32 │       inc          rsi
  1.37 │       add          rdx,0x40
  0.54 │       cmp          rdi,rsi
       │     ↓ je           200
  0.44 │10e:   vmovups      zmm2,ZMMWORD PTR [r13+rdx*1+0x0]
  0.59 │       vmovups      zmm1,ZMMWORD PTR [r14+rdx*1]
  0.85 │       vmovaps      zmm3,zmm8
  0.51 │       vfmadd213ps  zmm3,zmm1,ZMMWORD PTR [r12+rdx*1]
  9.62 │       vmovups      ZMMWORD PTR [r12+rdx*1],zmm3
  4.95 │       vcmpltps     k2,zmm2,zmm0
  4.87 │       vcmpgtps     k1,zmm2,zmm0
  4.90 │       vmovaps      zmm4,zmm2
  5.02 │       vfmadd213ps  zmm4,zmm8,ZMMWORD PTR [rbx+rdx*1]
  5.91 │       vaddps       zmm9,zmm4,zmm7
 12.12 │       vmovups      ZMMWORD PTR [rbx+rdx*1],zmm4
  6.10 │       vcmpltps     k2{k2},zmm9,zmm0
  5.86 │       knotw        k3,k2
  5.96 │       kmovw        r10d,k2
  2.66 │       vaddps       zmm9{k3}{z},zmm4,zmm5
  2.60 │       kmovw        r11d,k3
  2.57 │       vcmpgtps     k0{k1},zmm9,zmm13
  2.58 │       kmovw        eax,k0
  2.56 │       and          eax,r11d
  2.69 │       or           ax,r10w
  0.01 │     ↑ je           c0
  0.52 │       kmovw        k4,eax
  0.02 │       vxorps       zmm2,zmm2,zmm14
  0.01 │       vmovups      ZMMWORD PTR [r13+rdx*1+0x0]{k4},zmm2
  0.00 │     ↑ jmp          c0
<snip>
```

### clang
```
$ perf record -D 1000 ./out/build/clang/gui/kinematics-demo-gui 5000000
<wait some time, then close>

$ perf annotate -Mintel
<snip>
nop
  0.96 │190:   vmovups      zmm9,ZMMWORD PTR [r14+rdx*4]
  0.70 │       vmovups      zmm10,ZMMWORD PTR [r12+rdx*4]
  0.50 │       vmovups      zmm11,ZMMWORD PTR [rbx+rdx*4]
  1.12 │       vmovups      zmm12,ZMMWORD PTR [r15+rdx*4]
  0.95 │       vfmadd231ps  zmm10,zmm9,zmm2
  0.89 │       vxorps       zmm14,zmm9,zmm7
  0.96 │       vfmadd231ps  zmm12,zmm11,zmm2
  1.08 │       vcmpltps     k1,zmm9,zmm14
  1.19 │       vaddps       zmm13,zmm10,zmm5
  2.73 │       vmovups      ZMMWORD PTR [r12+rdx*4],zmm10
  1.17 │       vaddps       zmm10,zmm10,zmm8
  3.33 │       vmovups      ZMMWORD PTR [r15+rdx*4],zmm12
  1.70 │       vcmpltps     k0{k1},zmm13,zmm6
  6.49 │       vcmpltps     k1,zmm14,zmm9
  6.80 │       vaddps       zmm9,zmm12,zmm5
  6.77 │       vaddps       zmm12,zmm12,zmm8
 11.51 │       vcmpltps     k1{k1},zmm3,zmm10
 11.56 │       vxorps       zmm10,zmm11,zmm7
 11.55 │       korw         k1,k1,k0
 12.89 │       vmovups      ZMMWORD PTR [r14+rdx*4]{k1},zmm14
  6.41 │       vcmpltps     k1,zmm11,zmm10
  1.24 │       vcmpltps     k0{k1},zmm9,zmm6
  1.19 │       vcmpltps     k1,zmm10,zmm11
  1.05 │       vcmpltps     k1{k1},zmm4,zmm12
  1.11 │       korw         k1,k1,k0
  2.11 │       vmovups      ZMMWORD PTR [rbx+rdx*4]{k1},zmm10
  0.97 │       add          rdx,0x10
  1.03 │       cmp          rcx,rdx
  0.05 │     ↑ jne          190
<snip>
```

## Additional Data
As expected, both versions are heavily memory restricted. Interestingly the `g++` version gets ~0.92 IPC while the other gets ~0.62 IPC.

### gcc
```
$ ./out/build/release/gui/kinematics-demo-gui 5000000 &

$ perf stat -ddd --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '23675':

          7,311.17 msec task-clock                       #    1.001 CPUs utilized
            12,758      context-switches                 #    1.745 K/sec
               206      cpu-migrations                   #   28.176 /sec
                 0      page-faults                      #    0.000 /sec
    39,988,872,525      cycles                           #    5.470 GHz                         (35.63%)
     1,018,890,192      stalled-cycles-frontend          #    2.55% frontend cycles idle        (35.58%)
    36,795,850,422      instructions                     #    0.92  insn per cycle
                                                         #    0.03  stalled cycles per insn     (35.67%)
     3,199,880,506      branches                         #  437.670 M/sec                       (35.80%)
        30,767,065      branch-misses                    #    0.96% of all branches             (35.80%)
    14,891,108,861      L1-dcache-loads                  #    2.037 G/sec                       (35.69%)
     4,103,611,885      L1-dcache-load-misses            #   27.56% of all L1-dcache accesses   (35.81%)
       369,787,504      L1-icache-loads                  #   50.578 M/sec                       (35.72%)
         1,427,060      L1-icache-load-misses            #    0.39% of all L1-icache accesses   (35.77%)
        65,843,952      dTLB-loads                       #    9.006 M/sec                       (35.78%)
        63,655,133      dTLB-load-misses                 #   96.68% of all dTLB cache accesses  (35.79%)
         1,298,168      iTLB-loads                       #  177.559 K/sec                       (35.72%)
           830,484      iTLB-load-misses                 #   63.97% of all iTLB cache accesses  (35.64%)
     3,210,713,061      L1-dcache-prefetches             #  439.152 M/sec                       (35.59%)

       7.304027591 seconds time elapsed

$ perf stat -MPipelineL2 --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '23675':

        31,634,759      ex_ret_brn_misp                  #      0.7 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (24.96%)
    38,981,537,947      de_src_op_disp.all                                                      (24.96%)
         1,492,667      resyncs_or_nc_redirects                                                 (24.96%)
    41,352,856,108      ls_not_halted_cyc                                                       (24.96%)
    37,095,797,009      ex_ret_ops                                                              (24.96%)
    22,790,249,007      ex_no_retire.load_not_complete   #     22.7 %  backend_bound_cpu
                                                         #     58.1 %  backend_bound_memory     (24.93%)
   200,610,431,519      de_no_dispatch_per_slot.backend_stalls                                  (24.93%)
    31,703,847,460      ex_no_retire.not_complete                                               (24.93%)
    41,345,214,760      ls_not_halted_cyc                                                       (24.93%)
       109,515,716      ex_ret_ucode_ops                 #     14.9 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (24.98%)
    41,381,440,479      ls_not_halted_cyc                                                       (24.98%)
    37,064,759,606      ex_ret_ops                                                              (24.98%)
     8,305,523,759      de_no_dispatch_per_slot.no_ops_from_frontend                #      0.7 %  frontend_bound_bandwidth  (25.13%)
     1,109,980,876      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      2.7 %  frontend_bound_latency   (25.13%)
    41,405,880,037      ls_not_halted_cyc                                                       (25.13%)

       7.532084427 seconds time elapsed
```

### clang
```
$ ./out/build/clang/gui/kinematics-demo-gui 5000000 &

$ perf stat -ddd --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '23843':

          7,450.05 msec task-clock                       #    1.001 CPUs utilized
            10,827      context-switches                 #    1.453 K/sec
                91      cpu-migrations                   #   12.215 /sec
                 0      page-faults                      #    0.000 /sec
    40,764,453,104      cycles                           #    5.472 GHz                         (35.71%)
       894,988,809      stalled-cycles-frontend          #    2.20% frontend cycles idle        (35.88%)
    25,474,105,239      instructions                     #    0.62  insn per cycle
                                                         #    0.04  stalled cycles per insn     (35.86%)
     1,049,489,804      branches                         #  140.870 M/sec                       (35.88%)
        14,746,395      branch-misses                    #    1.41% of all branches             (35.85%)
    17,385,234,854      L1-dcache-loads                  #    2.334 G/sec                       (35.78%)
     3,464,251,085      L1-dcache-load-misses            #   19.93% of all L1-dcache accesses   (35.55%)
       308,365,430      L1-icache-loads                  #   41.391 M/sec                       (35.62%)
         1,223,356      L1-icache-load-misses            #    0.40% of all L1-icache accesses   (35.46%)
        56,309,967      dTLB-loads                       #    7.558 M/sec                       (35.54%)
        54,579,271      dTLB-load-misses                 #   96.93% of all dTLB cache accesses  (35.67%)
         1,178,549      iTLB-loads                       #  158.193 K/sec                       (35.68%)
           776,054      iTLB-load-misses                 #   65.85% of all iTLB cache accesses  (35.70%)
     3,288,733,988      L1-dcache-prefetches             #  441.438 M/sec                       (35.83%)

       7.441660203 seconds time elapsed

$ perf stat -MPipelineL2 --pid=$(pgrep kinematics-demo)
 Performance counter stats for process id '23843':

        18,318,838      ex_ret_brn_misp                  #      0.1 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (24.83%)
    38,370,874,691      de_src_op_disp.all                                                      (24.83%)
         1,401,720      resyncs_or_nc_redirects                                                 (24.83%)
    55,145,228,740      ls_not_halted_cyc                                                       (24.83%)
    38,009,682,864      ex_ret_ops                                                              (24.83%)
    37,925,230,373      ex_no_retire.load_not_complete   #     10.6 %  backend_bound_cpu
                                                         #     75.0 %  backend_bound_memory     (25.10%)
   282,993,299,594      de_no_dispatch_per_slot.backend_stalls                                  (25.10%)
    43,278,967,440      ex_no_retire.not_complete                                               (25.10%)
    55,125,640,981      ls_not_halted_cyc                                                       (25.10%)
       130,455,108      ex_ret_ucode_ops                 #     11.5 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (25.03%)
    55,171,925,197      ls_not_halted_cyc                                                       (25.03%)
    38,087,876,892      ex_ret_ops                                                              (25.03%)
     9,210,376,605      de_no_dispatch_per_slot.no_ops_from_frontend                #      0.8 %  frontend_bound_bandwidth  (25.04%)
     1,096,682,416      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      2.0 %  frontend_bound_latency   (25.04%)
    55,202,334,883      ls_not_halted_cyc                                                       (25.04%)

      10.028978935 seconds time elapsed

<wait some time, then cancel>
```
