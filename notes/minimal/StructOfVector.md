# StructOfVector
SoA style layout using parallel `std::vector` fields. This means data for a particular field is entirely contiguous in memory which typically allows for easier vectorization.

```
struct Points
{
	std::vector<float> position;
	std::vector<float> speed;
};

Points points;
```

```
for (size_t point = 0; point < numPoints; point++)
{
    points.position[point] += points.speed[point] * DELTA_TIME;

    if ((points.position[point] < 0 && points.speed[point] < 0) ||
        (points.position[point] > POSITION_LIMIT && points.speed[point] > 0))
    {
        points.speed[point] *= -1;
    }
}
```

## Benchmark Comparison
| Compiler | Optimization | Architecture | Time (ms) |
|----------|--------------|--------------|-----------|
| gcc      | O0           | default      |     40969 |
| gcc      | debug        | default      |      6981 |
| gcc      | O1           | default      |      4541 |
| gcc      | O2           | default      |      4275 |
| gcc      | O3           | default      |      4299 |
| gcc      | O3           | native       |      1363 |
| clang    | debug        | default      |      4236 |
| clang    | O3           | default      |      3277 |
| clang    | O3           | native       |      1645 |

Focusing on compiling with `g++` (version 13.3.0) we see improvement with optimization level, but somewhat surprisingly we don't see any better than the 1180 ms time that we had with `VectorOfStruct`. This solution, however, should not have the same drawbacks as `VectorOfLargeStruct` so even with additional fields in `Points` we should consistently keep our performance.

On a better note, taking a quick look at `clang++` (version 18.1.3), we finally do not have a performance regression with native architecture.

## Assembly Analysis
### g++ -O0
With no optimization a significant amount of time is understandably spent calling into `std::vector<>::operator[]`

```
$ perf record -D 100 ./out/gcc/O0/default/StructOfVector
$ perf report -Mintel
  51.63%  StructOfVector  StructOfVector    [.] main
  48.24%  StructOfVector  StructOfVector    [.] std::vector<float, std::allocator<float> >::operator[](unsigned long)
```

### g++ -Og
Here we have debug symbols, which helps map which instructions are doing which calculations. Notably while vector instructions are being used (like `mulss`), they are the scalar versions for single precision floats (as denoted by the suffix `ss`).

```
$ perf record -D 100 ./out/gcc/debug/default/StructOfVector
$ perf report -Mintel
<snip>
       │     for (size_t point = 0; point < numPoints; point++)
       │     {
       │     points.position[point] += points.speed[point] * DELTA_TIME;
       │
       │     if ((points.position[point] < 0 && points.speed[point] < 0) ||
       │       comiss   xmm1,DWORD PTR [rcx]
       │     ↓ jbe      202
       │1a4:   add      rax,QWORD PTR [rsp+0x28]
       │     (points.position[point] > POSITION_LIMIT && points.speed[point] > 0))
       │     {
       │     points.speed[point] *= -1;
       │       movss    xmm0,DWORD PTR [rax]
  0.00 │       xorps    xmm0,XMMWORD PTR [rip+0xc03]
       │       movss    DWORD PTR [rax],xmm0
       │     for (size_t point = 0; point < numPoints; point++)
  4.49 │1b8:   add      rdx,0x1
  4.44 │1bc:   cmp      rdx,rbx
       │     ↓ jae      222
  4.25 │       lea      rax,[rdx*4+0x0]
  4.89 │       mov      rcx,rax
  4.80 │       add      rcx,QWORD PTR [rsp+0x28]
       │     points.position[point] += points.speed[point] * DELTA_TIME;
  4.67 │       movss    xmm0,DWORD PTR [rip+0xb66]
  4.85 │       mulss    xmm0,DWORD PTR [rcx]
  4.61 │       mov      rcx,rax
  4.93 │       add      rcx,QWORD PTR [rsp+0x10]
  4.51 │       addss    xmm0,DWORD PTR [rcx]
  4.51 │       movss    DWORD PTR [rcx],xmm0
  4.61 │       mov      rcx,rax
  4.14 │       add      rcx,QWORD PTR [rsp+0x10]
       │     if ((points.position[point] < 0 && points.speed[point] < 0) ||
  4.61 │       movss    xmm0,DWORD PTR [rcx]
  4.34 │       pxor     xmm1,xmm1
 10.62 │       comiss   xmm1,xmm0
  5.27 │     ↑ ja       197
 10.11 │202:   comiss   xmm0,DWORD PTR [rip+0xb3a]        # 200c <_IO_stdin_used+0xc>
  5.30 │     ↑ jbe      1b8
  0.01 │       mov      rcx,rax
  0.01 │       add      rcx,QWORD PTR [rsp+0x28]
       │     (points.position[point] > POSITION_LIMIT && points.speed[point] > 0))
  0.01 │       movss    xmm1,DWORD PTR [rcx]
       │       pxor     xmm0,xmm0
       │       comiss   xmm1,xmm0
       │     ↑ ja       1a4
       │     ↑ jmp      1b8
       │     for (size_t i = 0; i < numIterations; i++)
  0.00 │222:   add      rsi,0x1
       │226:   cmp      rsi,rbp
       │     ↓ jae      232
       │     for (size_t point = 0; point < numPoints; point++)
       │       mov      edx,0x0
       │     ↑ jmp      1bc
<snip>
```

### g++ -O3
Interestingly we are not seeing performant vectorization here as scalar versions of instructions are still being used. While more exploration is needed, it appears that "packed" commands are used if at least AVX1 is allowed while compiling (`-mavx`), even on newer versions of `g++`.

```
$ perf record -D 100 ./out/gcc/O3/default/StructOfVector
$ perf report -Mintel
<snip>
 15.40 │220:   comiss   xmm0,DWORD PTR [rip+0xbf5]        # 200c <_IO_stdin_used+0xc>
  7.47 │     ↓ jbe      239
  0.02 │       movss    xmm0,DWORD PTR [rax]
  0.04 │       comiss   xmm0,xmm1
  0.01 │     ↓ jbe      239
  0.02 │232:   xorps    xmm0,xmm3
       │       movss    DWORD PTR [rax],xmm0
  7.12 │239:   add      rax,0x4
  7.43 │       add      rdx,0x4
  7.30 │       cmp      rax,rcx
       │     ↓ je       271
  6.84 │246:   movss    xmm0,DWORD PTR [rax]
  8.94 │       mulss    xmm0,xmm2
  7.31 │       addss    xmm0,DWORD PTR [rdx]
 14.80 │       comiss   xmm1,xmm0
  9.63 │       movss    DWORD PTR [rdx],xmm0
  7.66 │     ↑ jbe      220
       │       movss    xmm0,DWORD PTR [rax]
       │       comiss   xmm1,xmm0
       │     ↑ ja       232
       │       add      rax,0x4
       │       add      rdx,0x4
       │       cmp      rax,rcx
       │     ↑ jne      246
       │271:   add      rsi,0x1
       │       cmp      r12,rsi
       │     ↑ jne      218
<snip>
```

### g++ -O3 -march=native
Once we start compiling for the native architecture (on a Zen 4 test system) we even start to see "true" vectorization, including FMA commands. Notably there is no shuffling of data required, as there was with the AoS solutions.

```
$ perf record -D 100 ./out/gcc/O3/native/StructOfVector
$ perf report -Mintel
<snip>
 10.16 │3d0:   inc          r14
  1.60 │       add          r10,0x40
  1.73 │       add          r11,0x40
  1.70 │       cmp          r14,r8
       │     ↓ je           445
  3.04 │3e0:   vmovups      zmm0,ZMMWORD PTR [r10]
  1.51 │       vfmadd213ps  zmm0,zmm4,ZMMWORD PTR [r11]
  7.31 │       vmovups      ZMMWORD PTR [r11],zmm0
  3.43 │       vmovups      zmm1,ZMMWORD PTR [r10]
  3.44 │       vcmpgtps     k1,zmm1,zmm2
  3.19 │       vcmpgtps     k0{k1},zmm0,zmm3
  3.37 │       vcmpltps     k1,zmm1,zmm2
 11.03 │       kxorw        k1,k1,k0
 11.61 │       kmovw        eax,k0
 11.33 │       vcmpltps     k1{k1},zmm0,zmm2
 13.37 │       kmovw        edx,k1
 11.90 │       cmp          ax,dx
  0.12 │     ↑ je           3d0
<snip>
```

### clang++ -O3 -march=native
`clang` now also vectorizes the code, and without the usage of `vscatterdps` the performance seems on-par with the gcc generated binary.

```
$ perf record -D 100 ./out/clang/O3/native/StructOfVector
$ perf report -Mintel
<snip>
  0.90 │450:   vbroadcastss zmm11,DWORD PTR [rip+0x9de]        # 2008 <_IO_stdin_used+0x8>
  0.76 │       vmovups      zmm9,ZMMWORD PTR [rdi+r9*4-0x40]
  0.92 │       vmovups      zmm10,ZMMWORD PTR [rdi+r9*4]
  0.87 │       vbroadcastss zmm13,DWORD PTR [rip+0x9c9]        # 200c <_IO_stdin_used+0xc>
  0.87 │       vfmadd213ps  zmm9,zmm11,ZMMWORD PTR [r12+r9*4]
  2.35 │       vfmadd213ps  zmm10,zmm11,ZMMWORD PTR [r12+r9*4+0x40]
  5.07 │       vmovups      ZMMWORD PTR [r12+r9*4],zmm9
  4.45 │       vmovups      ZMMWORD PTR [r12+r9*4+0x40],zmm10
  2.67 │       vcmpltps     k1,zmm9,zmm8
  0.76 │       vcmpltps     k2,zmm10,zmm8
  0.95 │       vcmpnltps    k0,zmm9,zmm8
  1.06 │       vcmpnltps    k3,zmm10,zmm8
  1.01 │       vmovups      zmm11,ZMMWORD PTR [rdi+r9*4-0x40]
  1.12 │       vmovups      zmm12,ZMMWORD PTR [rdi+r9*4]
  1.31 │       vxorps       zmm14,zmm11,zmm13
  1.07 │       vxorps       zmm13,zmm12,zmm13
  1.28 │       vcmpnltps    k4{k1},zmm11,zmm14
  1.29 │       vcmpnltps    k5{k2},zmm12,zmm13
  1.96 │       vcmpltps     k1{k1},zmm11,zmm14
  1.85 │       vcmpltps     k2{k2},zmm12,zmm13
  1.82 │       korw         k4,k4,k0
  2.83 │       korw         k3,k5,k3
  2.22 │       vcmpltps     k3{k3},zmm15,zmm10
  2.43 │       vcmpltps     k4{k4},zmm15,zmm9
  4.96 │       vcmpltps     k0{k4},zmm14,zmm11
  4.87 │       vcmpltps     k3{k3},zmm13,zmm12
  5.00 │       korw         k1,k0,k1
  8.65 │       korw         k2,k3,k2
 17.31 │       vmovups      ZMMWORD PTR [rdi+r9*4-0x40]{k1},zmm14
 11.23 │       vmovups      ZMMWORD PTR [rdi+r9*4]{k2},zmm13
  4.98 │       add          r9,0x20
  1.20 │       cmp          rcx,r9
       │     ↑ jne          450
<snip>
```

## Additional Data
Quite a bit of data to fully dig into, but one thing that stands out is that the `clang++` native solution now roughly matches the `g++` native solution in its mix of `backend_bound_memory` and `backend_bound_cpu`.

### g++ -O3 -march=native
```
$ perf stat -D100 -ddd ./out/gcc/O3/native/StructOfVector
          1,266.77 msec task-clock                       #    1.000 CPUs utilized
                12      context-switches                 #    9.473 /sec
                 7      cpu-migrations                   #    5.526 /sec
                 7      page-faults                      #    5.526 /sec
     6,986,954,085      cycles                           #    5.516 GHz                         (35.58%)
        71,147,413      stalled-cycles-frontend          #    1.02% frontend cycles idle        (35.65%)
    10,521,948,827      instructions                     #    1.51  insn per cycle
                                                         #    0.01  stalled cycles per insn     (35.72%)
     1,170,686,613      branches                         #  924.151 M/sec                       (35.82%)
           850,931      branch-misses                    #    0.07% of all branches             (35.90%)
     5,315,808,683      L1-dcache-loads                  #    4.196 G/sec                       (35.92%)
     1,255,755,809      L1-dcache-load-misses            #   23.62% of all L1-dcache accesses   (35.91%)
        15,137,264      L1-icache-loads                  #   11.949 M/sec                       (35.85%)
            88,550      L1-icache-load-misses            #    0.58% of all L1-icache accesses   (35.77%)
        18,363,478      dTLB-loads                       #   14.496 M/sec                       (35.69%)
            45,572      dTLB-load-misses                 #    0.25% of all dTLB cache accesses  (35.61%)
               452      iTLB-loads                       #  356.813 /sec                        (35.54%)
               247      iTLB-load-misses                 #   54.65% of all iTLB cache accesses  (35.52%)
     1,163,609,208      L1-dcache-prefetches             #  918.564 M/sec                       (35.52%)

$ perf stat -D100 -MPipelineL2 ./out/gcc/O3/native/StructOfVector
           814,218      ex_ret_brn_misp                  #      0.2 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (24.96%)
     9,921,123,378      de_src_op_disp.all                                                      (24.96%)
           246,735      resyncs_or_nc_redirects                                                 (24.96%)
     6,030,318,824      ls_not_halted_cyc                                                       (24.96%)
     9,843,884,025      ex_ret_ops                                                              (24.96%)
     1,135,809,742      ex_no_retire.load_not_complete   #     48.2 %  backend_bound_cpu
                                                         #     20.9 %  backend_bound_memory     (25.10%)
    24,978,838,963      de_no_dispatch_per_slot.backend_stalls                                  (25.10%)
     3,754,491,199      ex_no_retire.not_complete                                               (25.10%)
     6,029,000,003      ls_not_halted_cyc                                                       (25.10%)
         3,322,489      ex_ret_ucode_ops                 #     27.2 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (24.99%)
     6,031,507,226      ls_not_halted_cyc                                                       (24.99%)
     9,837,312,981      ex_ret_ops                                                              (24.99%)
     1,015,643,532      de_no_dispatch_per_slot.no_ops_from_frontend                #      1.7 %  frontend_bound_bandwidth  (24.94%)
        66,285,441      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      1.1 %  frontend_bound_latency   (24.94%)
     6,032,942,046      ls_not_halted_cyc                                                       (24.94%)
```

### clang++ -O3 -march=native
```
$ perf stat -D100 -ddd ./out/clang/O3/native/StructOfVector
          1,524.19 msec task-clock                       #    1.000 CPUs utilized
                 6      context-switches                 #    3.937 /sec
                 5      cpu-migrations                   #    3.280 /sec
                 7      page-faults                      #    4.593 /sec
     8,407,349,183      cycles                           #    5.516 GHz                         (35.64%)
        82,684,554      stalled-cycles-frontend          #    0.98% frontend cycles idle        (35.64%)
     9,750,198,156      instructions                     #    1.16  insn per cycle
                                                         #    0.01  stalled cycles per insn     (35.64%)
       298,827,853      branches                         #  196.057 M/sec                       (35.67%)
           507,506      branch-misses                    #    0.17% of all branches             (35.73%)
     7,740,987,167      L1-dcache-loads                  #    5.079 G/sec                       (35.76%)
     1,310,471,872      L1-dcache-load-misses            #   16.93% of all L1-dcache accesses   (35.76%)
        21,867,766      L1-icache-loads                  #   14.347 M/sec                       (35.76%)
            26,683      L1-icache-load-misses            #    0.12% of all L1-icache accesses   (35.76%)
        18,556,740      dTLB-loads                       #   12.175 M/sec                       (35.76%)
            39,943      dTLB-load-misses                 #    0.22% of all dTLB cache accesses  (35.75%)
               598      iTLB-loads                       #  392.340 /sec                        (35.76%)
             1,091      iTLB-load-misses                 #  182.44% of all iTLB cache accesses  (35.73%)
     1,176,162,605      L1-dcache-prefetches             #  771.666 M/sec                       (35.66%)

$ perf stat -D100 -MPipelineL2 ./out/clang/O3/native/StructOfVector
           589,551      ex_ret_brn_misp                  #      0.0 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (24.93%)
    10,674,471,531      de_src_op_disp.all                                                      (24.93%)
           322,195      resyncs_or_nc_redirects                                                 (24.93%)
     8,459,542,356      ls_not_halted_cyc                                                       (24.93%)
    10,645,808,060      ex_ret_ops                                                              (24.93%)
     1,613,170,538      ex_no_retire.load_not_complete   #     53.0 %  backend_bound_cpu
                                                         #     24.6 %  backend_bound_memory     (25.08%)
    39,396,804,372      de_no_dispatch_per_slot.backend_stalls                                  (25.08%)
     5,091,252,076      ex_no_retire.not_complete                                               (25.08%)
     8,458,163,691      ls_not_halted_cyc                                                       (25.08%)
         3,304,367      ex_ret_ucode_ops                 #     21.0 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (24.95%)
     8,461,206,243      ls_not_halted_cyc                                                       (24.95%)
    10,651,906,132      ex_ret_ops                                                              (24.95%)
       709,071,265      de_no_dispatch_per_slot.no_ops_from_frontend                #      0.8 %  frontend_bound_bandwidth  (25.05%)
        51,608,318      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      0.6 %  frontend_bound_latency   (25.05%)
     8,463,101,155      ls_not_halted_cyc                                                       (25.05%)

```
