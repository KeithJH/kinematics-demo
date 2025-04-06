# StructOfPointer
SoA layout that uses `float*` fields manually managed with `new[]` and `delete[]`.

```
struct Points
{
	float *position;
	float *speed;
};

Points points;
points.position = new float[numPoints];
points.speed = new float[numPoints];
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
| gcc      | O0           | default      |     19277 |
| gcc      | debug        | default      |      5309 |
| gcc      | O1           | default      |      4326 |
| gcc      | O2           | default      |      4293 |
| gcc      | O3           | default      |      3878 |
| gcc      | O3           | native       |       622 |
| clang    | debug        | default      |      4236 |
| clang    | O3           | default      |      2634 |
| clang    | O3           | native       |       760 |

Focusing on compiling with `g++` (version 13.3.0) we see improvement with optimization level, getting even better improvement over previous solutions with a best time of 622 ms versus 1000+ ms. Even without optimization times are improved, likely as there are less function calls and such (which were already quickly improved with inlining, hence other optimization levels aren't impacted as much).

Looking at `clang++` (version 18.1.3), we also see the same general progression of improvement with optimization level!

## Assembly Analysis
### g++ -O0
Even with no optimization we actualy get somewhat usable assembly!

```
$ perf record -D 100 ./out/gcc/O0/default/StructOfPointer
$ perf report -Mintel
<snip>
       │239:   mov      QWORD PTR [rbp-0x38],0x0
       │     ↓ jmp      335
  3.01 │246:   mov      rax,QWORD PTR [rbp-0x20]
  2.65 │       mov      rdx,QWORD PTR [rbp-0x38]
  2.51 │       shl      rdx,0x2
  2.55 │       add      rax,rdx
  2.57 │       movss    xmm1,DWORD PTR [rax]
  2.64 │       mov      rax,QWORD PTR [rbp-0x18]
  2.42 │       mov      rdx,QWORD PTR [rbp-0x38]
  2.33 │       shl      rdx,0x2
  2.47 │       add      rax,rdx
  2.31 │       movss    xmm2,DWORD PTR [rax]
  2.20 │       movss    xmm0,DWORD PTR [rip+0xb93]
  2.27 │       mulss    xmm0,xmm2
  2.22 │       mov      rax,QWORD PTR [rbp-0x20]
  2.13 │       mov      rdx,QWORD PTR [rbp-0x38]
  2.15 │       shl      rdx,0x2
  2.05 │       add      rax,rdx
  2.10 │       addss    xmm0,xmm1
  2.04 │       movss    DWORD PTR [rax],xmm0
  2.40 │       mov      rax,QWORD PTR [rbp-0x20]
  2.41 │       mov      rdx,QWORD PTR [rbp-0x38]
  2.37 │       shl      rdx,0x2
  2.51 │       add      rax,rdx
  3.77 │       movss    xmm1,DWORD PTR [rax]
  2.46 │       pxor     xmm0,xmm0
  4.40 │       comiss   xmm0,xmm1
  2.14 │     ↓ jbe      2c7
       │       mov      rax,QWORD PTR [rbp-0x18]
       │       mov      rdx,QWORD PTR [rbp-0x38]
       │       shl      rdx,0x2
       │       add      rax,rdx
       │       movss    xmm1,DWORD PTR [rax]
       │       pxor     xmm0,xmm0
       │       comiss   xmm0,xmm1
       │     ↓ ja       2ff
  2.02 │2c7:   mov      rax,QWORD PTR [rbp-0x20]
  2.10 │       mov      rdx,QWORD PTR [rbp-0x38]
  2.22 │       shl      rdx,0x2
  2.25 │       add      rax,rdx
  2.15 │       movss    xmm0,DWORD PTR [rax]
  6.10 │       comiss   xmm0,DWORD PTR [rip+0xb2a]
  2.13 │     ↓ jbe      330
  0.00 │       mov      rax,QWORD PTR [rbp-0x18]
  0.00 │       mov      rdx,QWORD PTR [rbp-0x38]
  0.00 │       shl      rdx,0x2
       │       add      rax,rdx
  0.00 │       movss    xmm0,DWORD PTR [rax]
       │       pxor     xmm1,xmm1
       │       comiss   xmm0,xmm1
       │     ↓ jbe      330
  0.00 │2ff:   mov      rax,QWORD PTR [rbp-0x18]
       │       mov      rdx,QWORD PTR [rbp-0x38]
       │       shl      rdx,0x2
       │       add      rax,rdx
       │       movss    xmm0,DWORD PTR [rax]
       │       mov      rax,QWORD PTR [rbp-0x18]
       │       mov      rdx,QWORD PTR [rbp-0x38]
       │       shl      rdx,0x2
       │       add      rax,rdx
       │       movss    xmm1,DWORD PTR [rip+0xaee]
       │       xorps    xmm0,xmm1
       │       movss    DWORD PTR [rax],xmm0
  6.06 │330:   add      QWORD PTR [rbp-0x38],0x1
  3.00 │335:   mov      rax,QWORD PTR [rbp-0x38]
  6.89 │       cmp      rax,QWORD PTR [rbp-0x58]
       │     ↑ jb       246
       │       add      QWORD PTR [rbp-0x40],0x1
       │348:   mov      rax,QWORD PTR [rbp-0x40]
       │       cmp      rax,QWORD PTR [rbp-0x50]
       │     ↑ jb       239
<snip>
```

### g++ -Og
Here we have debug symbols, which helps map which instructions are doing which calculations. Notably while vector instructions are being used (like `mulss`), they are the scalar versions for single precision floats (as denoted by the suffix `ss`).

```
$ perf record -D 100 ./out/gcc/debug/default/StructOfPointer
$ perf report -Mintel
<snip>
       │     for (size_t point = 0; point < numPoints; point++)
       │     {
       │     points.position[point] += points.speed[point] * DELTA_TIME;
       │
       │     if ((points.position[point] < 0 && points.speed[point] < 0) ||
       │1af:   comiss   xmm1,DWORD PTR [rax]
       │     ↓ jbe      1f8
       │     (points.position[point] > POSITION_LIMIT && points.speed[point] > 0))
       │     {
       │     points.speed[point] *= -1;
       │1b4:   movss    xmm0,DWORD PTR [rax]
       │       xorps    xmm0,XMMWORD PTR [rip+0xc48]
       │       movss    DWORD PTR [rax],xmm0
       │     for (size_t point = 0; point < numPoints; point++)
  6.69 │1c3:   add      rdx,0x1
  8.13 │1c7:   cmp      rdx,rbx
       │     ↓ jae      210
       │     points.position[point] += points.speed[point] * DELTA_TIME;
  7.84 │       lea      rax,[rdx*4+0x0]
  5.80 │       lea      rcx,[r12+rax*1]
  5.85 │       add      rax,rbp
  5.54 │       movss    xmm0,DWORD PTR [rip+0xbdc]
  6.03 │       mulss    xmm0,DWORD PTR [rax]
  5.85 │       addss    xmm0,DWORD PTR [rcx]
  5.72 │       movss    DWORD PTR [rcx],xmm0
       │     if ((points.position[point] < 0 && points.speed[point] < 0) ||
  5.68 │       pxor     xmm1,xmm1
 12.35 │       comiss   xmm1,xmm0
  6.09 │     ↑ ja       1af
 11.73 │1f8:   comiss   xmm0,DWORD PTR [rip+0xbc4]        # 200c <_IO_stdin_used+0xc>
  6.65 │     ↑ jbe      1c3
       │     (points.position[point] > POSITION_LIMIT && points.speed[point] > 0))
  0.01 │       movss    xmm1,DWORD PTR [rax]
  0.01 │       pxor     xmm0,xmm0
  0.01 │       comiss   xmm1,xmm0
       │     ↑ ja       1b4
       │     ↑ jmp      1c3
       │     for (size_t i = 0; i < numIterations; i++)
       │210:   add      rsi,0x1
       │214:   cmp      rsi,r13
       │     ↓ jae      220
       │     for (size_t point = 0; point < numPoints; point++)
       │       mov      edx,0x0
       │     ↑ jmp      1c7
<snip>
```

### g++ -O3
Interestingly, similar to `StructOfVector`, we are not seeing performant vectorization here as scalar versions of instructions are still being used. While more exploration is needed, it appears that "packed" commands are used if at least AVX1 is allowed while compiling (`-mavx`).

```
$ perf record -D 100 ./out/gcc/O3/default/StructOfPointer
$ perf report -Mintel
<snip>
  8.35 │190:   comiss   xmm1,DWORD PTR [rip+0xd25]        # 200c <_IO_stdin_used+0xc>
  4.29 │     ↓ jbe      1a5
  0.01 │       comiss   xmm0,xmm2
       │     ↓ jbe      1a5
  0.01 │19e:   xorps    xmm0,xmm6
       │       movss    DWORD PTR [rdx],xmm0
  4.33 │1a5:   movaps   xmm4,xmm0
  4.33 │       mulss    xmm4,xmm3
  4.41 │       addss    xmm1,xmm4
  8.69 │       comiss   xmm2,xmm1
  4.05 │       movss    DWORD PTR [rax],xmm1
  4.05 │     ↓ ja       250
  8.38 │1bd:   comiss   xmm1,DWORD PTR [rip+0xcf8]        # 200c <_IO_stdin_used+0xc>
  4.54 │     ↓ jbe      1d2
  0.01 │       comiss   xmm0,xmm2
  0.01 │     ↓ jbe      1d2
       │1cb:   xorps    xmm0,xmm5
  0.03 │       movss    DWORD PTR [rdx],xmm0
  3.92 │1d2:   add      rax,0x4
  3.82 │       add      rdx,0x4
  4.03 │       cmp      r13,rax
       │     ↓ je       26a
  4.00 │1e3:   movss    xmm0,DWORD PTR [rdx]
  4.35 │       movss    xmm1,DWORD PTR [rax]
  4.09 │       movaps   xmm4,xmm0
  3.98 │       mulss    xmm4,xmm3
  3.83 │       addss    xmm1,xmm4
  8.44 │       comiss   xmm2,xmm1
  4.05 │     ↑ jbe      190
<snip>
```

### g++ -O3 -march=native
Once we start compiling for the native architecture (on a Zen 4 test system) we even start to see "true" vectorization, including FMA commands. Notably there is no shuffling of data required, as there was with the AoS solutions. Without digging in, it may also be unrolling the loop with two `vfmadd231ps` instructions (ignoring duplicate "versions" for "tail" computations not shown here).

```
$ perf record -D 100 ./out/gcc/O3/native/StructOfPointer
$ perf report -Mintel
<snip>
  4.37 │1e0:   vcmpgtps     k1,zmm0,zmm2
  2.83 │       vfmadd231ps  zmm1,zmm0,zmm4
  7.01 │       vmovups      ZMMWORD PTR [rdx],zmm1
  2.90 │       vcmpgtps     k0{k1},zmm1,zmm3
  3.03 │       vcmpltps     k1,zmm0,zmm2
  4.46 │       kxorw        k1,k1,k0
  3.08 │       kmovw        r11d,k0
  3.37 │       vcmpltps     k5{k1},zmm1,zmm2
  3.83 │       kmovw        eax,k5
  2.81 │       cmp          r11w,ax
  0.05 │     ↓ jne          610
  3.56 │       inc          rsi
  3.04 │       add          rdx,0x40
  3.22 │       add          rcx,0x40
  2.86 │       cmp          rsi,r10
       │     ↓ je           638
  2.88 │232:   vmovups      zmm0,ZMMWORD PTR [rcx]
  3.92 │       vcmpgtps     k1,zmm0,zmm2
  3.64 │       vmovaps      zmm1,zmm0
  4.25 │       vfmadd213ps  zmm1,zmm4,ZMMWORD PTR [rdx]
  4.50 │       vcmpgtps     k0{k1},zmm1,zmm3
  4.83 │       vcmpltps     k1,zmm0,zmm2
  5.29 │       kxorw        k1,k1,k0
  5.10 │       kmovw        r11d,k0
  5.02 │       vcmpltps     k4{k1},zmm1,zmm2
  4.57 │       kmovw        eax,k4
  5.09 │       cmp          r11w,ax
  0.05 │     ↑ je           1e0
  0.09 │       xor          r11d,eax
       │       vxorps       zmm0,zmm0,zmm8
  0.18 │       kmovw        k4,r11d
       │       vmovups      ZMMWORD PTR [rcx]{k4},zmm0
       │       vmovups      zmm0,ZMMWORD PTR [rcx]
       │     ↑ jmp          1e0
<snip>
```

### clang++ -O3 -march=native
Fairly similar to the assembly generated by `g++ -O3 -march=native`.

```
$ perf record -D 100 ./out/clang/O3/native/StructOfPointer
$ perf report -Mintel
<snip>
  0.52 │990:   vmovaps      zmm12,ZMMWORD PTR [rsi+r8*4-0x40]
  0.18 │       vmovaps      zmm14,ZMMWORD PTR [rbx+r8*4]
  0.43 │       vmovaps      zmm13,ZMMWORD PTR [rsi+r8*4]
  0.54 │       vmovaps      zmm15,ZMMWORD PTR [rbx+r8*4+0x40]
  0.54 │       vfmadd231ps  zmm14,zmm12,zmm7
  0.25 │       vfmadd231ps  zmm15,zmm13,zmm7
  0.60 │       vxorps       zmm16,zmm12,zmm10
  0.36 │       vxorps       zmm17,zmm13,zmm10
  0.40 │       vcmpltps     k1,zmm14,zmm9
  0.65 │       vcmpltps     k2,zmm15,zmm9
  0.83 │       vmovaps      ZMMWORD PTR [rbx+r8*4],zmm14
  1.46 │       vmovaps      ZMMWORD PTR [rbx+r8*4+0x40],zmm15
  0.72 │       vcmpltps     k0{k1},zmm12,zmm16
  1.35 │       vcmpltps     k1{k2},zmm13,zmm17
  1.39 │       knotw        k2,k0
  1.20 │       knotw        k3,k1
  0.94 │       vcmpltps     k2{k2},zmm11,zmm14
  0.90 │       vcmpltps     k3{k3},zmm11,zmm15
  1.76 │       vcmpltps     k2{k2},zmm16,zmm12
  1.85 │       vcmpltps     k3{k3},zmm17,zmm13
  1.20 │       korw         k2,k2,k0
  1.70 │       korw         k1,k3,k1
  3.93 │       vmovups      ZMMWORD PTR [rsi+r8*4-0x40]{k2},zmm16
  0.63 │       vmovups      ZMMWORD PTR [rsi+r8*4]{k1},zmm17
  0.51 │       add          r8,0x20
  0.32 │       cmp          rcx,r8
       │     ↑ jne          990
<snip>
```

## Additional Data

### g++ -O3 -march=native
```
$ perf stat -D100 -ddd ./out/gcc/O3/native/StructOfPointer
            547.27 msec task-clock                       #    1.000 CPUs utilized
                16      context-switches                 #   29.236 /sec
                 4      cpu-migrations                   #    7.309 /sec
                 7      page-faults                      #   12.791 /sec
     3,017,940,933      cycles                           #    5.514 GHz                         (35.73%)
        29,572,809      stalled-cycles-frontend          #    0.98% frontend cycles idle        (35.87%)
     7,520,566,737      instructions                     #    2.49  insn per cycle
                                                         #    0.00  stalled cycles per insn     (35.87%)
       807,677,585      branches                         #    1.476 G/sec                       (35.88%)
           822,928      branch-misses                    #    0.10% of all branches             (35.88%)
     1,626,971,757      L1-dcache-loads                  #    2.973 G/sec                       (35.76%)
       538,488,907      L1-dcache-load-misses            #   33.10% of all L1-dcache accesses   (35.63%)
         5,498,743      L1-icache-loads                  #   10.048 M/sec                       (35.65%)
            30,471      L1-icache-load-misses            #    0.55% of all L1-icache accesses   (35.63%)
         8,463,271      dTLB-loads                       #   15.464 M/sec                       (35.64%)
            28,122      dTLB-load-misses                 #    0.33% of all dTLB cache accesses  (35.63%)
               171      iTLB-loads                       #  312.458 /sec                        (35.62%)
               112      iTLB-load-misses                 #   65.50% of all iTLB cache accesses  (35.61%)
       537,291,500      L1-dcache-prefetches             #  981.760 M/sec                       (35.61%)

$ perf stat -D100 -MPipelineL2 ./out/gcc/O3/native/StructOfPointer
           630,660      ex_ret_brn_misp                  #      0.2 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (25.03%)
     7,030,845,743      de_src_op_disp.all                                                      (25.03%)
           127,162      resyncs_or_nc_redirects                                                 (25.03%)
     2,968,114,680      ls_not_halted_cyc                                                       (25.03%)
     6,979,946,403      ex_ret_ops                                                              (25.03%)
        19,677,008      ex_no_retire.load_not_complete   #     57.8 %  backend_bound_cpu
                                                         #      0.7 %  backend_bound_memory     (25.14%)
    10,422,236,082      de_no_dispatch_per_slot.backend_stalls                                  (25.14%)
     1,583,375,575      ex_no_retire.not_complete                                               (25.14%)
     2,967,475,722      ls_not_halted_cyc                                                       (25.14%)
         1,194,993      ex_ret_ucode_ops                 #     39.1 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (24.89%)
     2,968,620,178      ls_not_halted_cyc                                                       (24.89%)
     6,971,819,402      ex_ret_ops                                                              (24.89%)
       324,498,932      de_no_dispatch_per_slot.no_ops_from_frontend                #      1.2 %  frontend_bound_bandwidth  (24.95%)
        17,402,703      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      0.6 %  frontend_bound_latency   (24.95%)
     2,969,195,241      ls_not_halted_cyc                                                       (24.95%)
```

### clang++ -O3 -march=native
```
$ perf stat -D100 -ddd ./out/clang/O3/native/StructOfPointer
            695.39 msec task-clock                       #    1.000 CPUs utilized
                 9      context-switches                 #   12.942 /sec
                 5      cpu-migrations                   #    7.190 /sec
                 7      page-faults                      #   10.066 /sec
     3,835,566,784      cycles                           #    5.516 GHz
        56,543,430      stalled-cycles-frontend          #    1.47% frontend cycles idle        (35.48%)
     7,484,402,910      instructions                     #    1.95  insn per cycle
                                                         #    0.01  stalled cycles per insn     (35.63%)
       278,726,648      branches                         #  400.819 M/sec                       (35.75%)
           254,891      branch-misses                    #    0.09% of all branches             (35.91%)
     4,440,067,699      L1-dcache-loads                  #    6.385 G/sec                       (35.94%)
     1,107,828,907      L1-dcache-load-misses            #   24.95% of all L1-dcache accesses   (35.95%)
        13,006,555      L1-icache-loads                  #   18.704 M/sec                       (35.95%)
            27,572      L1-icache-load-misses            #    0.21% of all L1-icache accesses   (35.96%)
        17,349,906      dTLB-loads                       #   24.950 M/sec                       (35.91%)
            46,201      dTLB-load-misses                 #    0.27% of all dTLB cache accesses  (35.78%)
               294      iTLB-loads                       #  422.783 /sec                        (35.61%)
               583      iTLB-load-misses                 #  198.30% of all iTLB cache accesses  (35.49%)
     1,105,403,156      L1-dcache-prefetches             #    1.590 G/sec                       (35.34%)

$ perf stat -D100 -MPipelineL2 ./out/clang/O3/native/StructOfPointer
           242,105      ex_ret_brn_misp                  #      0.0 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (25.01%)
     8,344,774,623      de_src_op_disp.all                                                      (25.01%)
           229,290      resyncs_or_nc_redirects                                                 (25.01%)
     3,859,564,861      ls_not_halted_cyc                                                       (25.01%)
     8,322,518,839      ex_ret_ops                                                              (25.01%)
        31,645,127      ex_no_retire.load_not_complete   #     58.9 %  backend_bound_cpu
                                                         #      1.1 %  backend_bound_memory     (25.06%)
    13,880,542,334      de_no_dispatch_per_slot.backend_stalls                                  (25.06%)
     1,766,130,286      ex_no_retire.not_complete                                               (25.06%)
     3,858,900,265      ls_not_halted_cyc                                                       (25.06%)
         2,644,118      ex_ret_ucode_ops                 #     35.9 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (25.02%)
     3,860,518,941      ls_not_halted_cyc                                                       (25.02%)
     8,314,846,564      ex_ret_ops                                                              (25.02%)
       561,747,103      de_no_dispatch_per_slot.no_ops_from_frontend                #      0.7 %  frontend_bound_bandwidth  (24.91%)
        64,766,681      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      1.7 %  frontend_bound_latency   (24.91%)
     3,861,009,242      ls_not_halted_cyc                                                       (24.91%)
```
