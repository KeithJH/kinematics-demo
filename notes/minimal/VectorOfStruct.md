# VectorOfStruct
Conventional AoS layout using a `std::vector`. This means position and speed values are interleaved in memory, which can present a challenge for vectorization.

```
struct Point
{
    float position;
    float speed;
};

std::vector<Point> points;
```

```
for (auto &point : points)
{
    point.position += point.speed * DELTA_TIME;

    if ((point.position < 0 && point.speed < 0) ||
        (point.position > POSITION_LIMIT && point.speed > 0))
    {
        point.speed *= -1;
    }
}
```

## Benchmark Comparison
| Compiler | Optimization | Architecture | Time (ms) |
|----------|--------------|--------------|-----------|
| gcc      | O0           | default      |     52824 |
| gcc      | debug        | default      |      4931 |
| gcc      | O1           | default      |      4543 |
| gcc      | O2           | default      |      4208 |
| gcc      | O3           | default      |      2782 |
| gcc      | O3           | native       |      1180 |
| clang    | debug        | default      |      4267 |
| clang    | O3           | default      |      4277 |
| clang    | O3           | native       |      5813 |

Focusing on compiling with `g++` (version 13.3.0) we see a somewhat unexpected amount of improvement as we increase the optimization level and eventually allowing native instructions. Optimzation level 0 takes nearly a full minute, but becomes much more reasonable with even debug optimizations enabled. Levels 1 and 2 are much the same, though level 3 has a sizable improvement that's made even beter with native instructions, which is reminiscent of the improvements we'd expect to see with vectorization.

Taking a quick look at `clang++` (version 18.1.3), however, we don't see the same improvement and even a performance regression when compiling for native architecture.

## Assembly Analysis
### g++ -O0
With no optimization a significant amount of time is understandably spent calling into iterator functionality.

```
$ perf record -D 100 ./out/gcc/O0/default/VectorOfStruct
$ perf report -Mintel
  45.37%  VectorOfStruct  VectorOfStruct    [.] main
  19.94%  VectorOfStruct  VectorOfStruct    [.] bool __gnu_cxx::operator!=<main::Point*, std::vector<main::Point...
  15.07%  VectorOfStruct  VectorOfStruct    [.] __gnu_cxx::__normal_iterator<main::Point*, std::vector<main::Point...
   9.89%  VectorOfStruct  VectorOfStruct    [.] __gnu_cxx::__normal_iterator<main::Point*, std::vector<main::Point...
   9.15%  VectorOfStruct  VectorOfStruct    [.] __gnu_cxx::__normal_iterator<main::Point*, std::vector<main::Point...
```

### g++ -Og
Here we have debug symbols, which helps map which instructions are doing which calculations. Notably while vector instructions are being used (like `mulss`), they are the scalar versions for single precision floats (as denoted by the suffix `ss`).

```
$ perf record -D 100 ./out/gcc/debug/default/VectorOfStruct
$ perf report -Mintel
<snip>
       │160:   comiss   xmm2,xmm1
       │     ↓ jbe      19e
       │     (point.position > POSITION_LIMIT && point.speed > 0))
       │     {
       │     point.speed *= -1;
       │165:   xorps    xmm1,XMMWORD PTR [rip+0x8fd]
       │       movss    DWORD PTR [rdx+0x4],xmm1
       │
       │     _GLIBCXX20_CONSTEXPR
       │     __normal_iterator&
       │     operator++() _GLIBCXX_NOEXCEPT
       │     {
       │     ++_M_current;
  6.31 │171:   lea      rax,[rdx+0x8]
       │     _GLIBCXX_NODISCARD _GLIBCXX20_CONSTEXPR
       │     inline bool
       │     operator!=(const __normal_iterator<_Iterator, _Container>& __lhs,
       │     const __normal_iterator<_Iterator, _Container>& __rhs)
       │     _GLIBCXX_NOEXCEPT
       │     { return __lhs.base() != __rhs.base(); }
  6.30 │175:   mov      rdx,rax
       │     for (auto &point : points)
  6.33 │       cmp      rcx,rax
       │     ↓ je       1b2
       │     point.position += point.speed * DELTA_TIME;
  6.50 │       movss    xmm1,DWORD PTR [rax+0x4]
  6.61 │       movaps   xmm0,xmm1
  6.74 │       mulss    xmm0,DWORD PTR [rip+0x864]
  6.54 │       addss    xmm0,DWORD PTR [rax]
  6.22 │       movss    DWORD PTR [rax],xmm0
       │     if ((point.position < 0 && point.speed < 0) ||
  6.42 │       pxor     xmm2,xmm2
 13.55 │       comiss   xmm2,xmm0
  6.84 │     ↑ ja       160
 14.58 │19e:   comiss   xmm0,DWORD PTR [rip+0x850]        # 200c <_IO_stdin_used+0xc>
  6.98 │     ↑ jbe      171
       │     (point.position > POSITION_LIMIT && point.speed > 0))
  0.02 │       pxor     xmm0,xmm0
  0.04 │       comiss   xmm1,xmm0
  0.02 │     ↑ ja       165
       │     ↑ jmp      171
       │     for (size_t iteration = 0; iteration < numIterations; iteration++)
       │1b2:   add      rsi,0x1
       │1b6:   cmp      rsi,rbx
       │     ↓ jae      1c7
       │     for (auto &point : points)
       │       mov      rax,QWORD PTR [rsp+0x10]
       │     : _M_current(__i) { }
       │       mov      rcx,QWORD PTR [rsp+0x18]
       │     ↑ jmp      175
<snip>
```

### g++ -O3
It's at this point that we start to see "true" vectorization, as commands with the suffix `ps` (for packed, single precision floats) are being used. It seems that since the `Point` structure we are using only has two members the compiler is able to see an opportunity to cleverly vectorize with nearly-adjacent values in memory. Notable commands include:
* `shufps`: Packed interleave shuffle of quadruplets of single precision floats. Allows rearranging registers so that `position` and `speed` values are split for further vectorized commands.
* `unpcklps`: Unpack and interleave low packed single precision floats. Allows rearranging registers so that results from computations on `position` and `speed` values can be stored back as `Points` in memory.

```
$ perf record -D 100 ./out/gcc/O3/default/VectorOfStruct
$ perf report -Mintel
<snip>
  1.38 │170:   movups     xmm3,XMMWORD PTR [rdx]
  1.33 │       movups     xmm5,XMMWORD PTR [rdx+0x10]
  1.39 │       add        rdx,0x20
  1.42 │       movaps     xmm2,xmm3
  1.51 │       shufps     xmm3,xmm5,0x88
  1.50 │       shufps     xmm2,xmm5,0xdd
  1.46 │       movaps     xmm0,xmm2
  1.63 │       movaps     xmm5,xmm4
  1.80 │       movaps     xmm1,xmm2
  1.72 │       mulps      xmm0,xmm10
  1.54 │       cmpltps    xmm5,xmm2
  1.70 │       cmpltps    xmm1,xmm4
  2.27 │       addps      xmm3,xmm0
  2.17 │       movaps     xmm0,xmm9
  2.23 │       cmpltps    xmm0,xmm3
  2.37 │       pand       xmm5,xmm0
  2.23 │       movaps     xmm0,xmm3
  2.34 │       cmpltps    xmm0,xmm4
  2.46 │       pxor       xmm1,xmm5
  2.41 │       pand       xmm0,xmm1
  2.35 │       movaps     xmm1,xmm9
  2.63 │       pxor       xmm0,xmm5
  2.52 │       movaps     xmm5,xmm2
  2.51 │       xorps      xmm5,xmm8
  2.46 │       andps      xmm5,xmm0
  2.44 │       andnps     xmm0,xmm2
  2.78 │       orps       xmm0,xmm5
  3.16 │       movaps     xmm2,xmm0
  3.01 │       mulps      xmm2,xmm10
  2.84 │       addps      xmm2,xmm3
  2.43 │       movaps     xmm3,xmm4
  2.42 │       cmpltps    xmm3,xmm0
  1.86 │       cmpltps    xmm1,xmm2
  1.37 │       movaps     xmm5,xmm2
  1.61 │       cmpltps    xmm5,xmm4
  1.49 │       pand       xmm3,xmm1
  1.39 │       movaps     xmm1,xmm0
  1.62 │       cmpltps    xmm1,xmm4
  1.37 │       pxor       xmm1,xmm3
  1.39 │       pand       xmm1,xmm5
  1.62 │       pxor       xmm1,xmm3
  1.51 │       movaps     xmm3,xmm0
  1.30 │       xorps      xmm3,xmm8
  1.38 │       andps      xmm3,xmm1
  1.22 │       andnps     xmm1,xmm0
  1.40 │       movaps     xmm0,xmm2
  1.13 │       orps       xmm1,xmm3
  1.51 │       unpcklps   xmm0,xmm1
  1.49 │       unpckhps   xmm2,xmm1
  1.46 │       movups     XMMWORD PTR [rdx-0x20],xmm0
  3.11 │       movups     XMMWORD PTR [rdx-0x10],xmm2
  2.35 │       cmp        rcx,rdx
<snip>
```

### g++ -O3 -march=native
Once we start compiling for the native architecture (on a Zen 4 test system) we even start to see full 512-bit vector instructions instead of the default 128-bit ones. Not only that, but we also see our first fused-multiply-add instruction, which is perfect for the kinematics use case. Comparisons also start using masking functionality (with `k` values). Shifting data around is similar but now uses `vpermt2ps` (full permute from two tables).

```
$ perf record -D 100 ./out/gcc/O3/native/VectorOfStruct
$ perf report -Mintel
<snip>
  0.95 │1b0:   vmovups      zmm1,ZMMWORD PTR [rax]
  1.53 │       vmovups      zmm0,ZMMWORD PTR [rax]
  1.41 │       sub          rax,0xffffffffffffff80
  1.52 │       vpermt2ps    zmm1,zmm18,ZMMWORD PTR [rax-0x40]
  2.09 │       vpermt2ps    zmm0,zmm17,ZMMWORD PTR [rax-0x40]
  2.41 │       vxorps       zmm3,zmm1,zmm4
  2.30 │       vcmpgtps     k1,zmm1,zmm2
  2.23 │       vfmadd231ps  zmm0,zmm1,zmm6
  2.27 │       vcmpgtps     k0{k1},zmm0,zmm5
  1.96 │       vcmpltps     k1,zmm1,zmm2
  2.19 │       kxorw        k1,k1,k0
  2.69 │       vcmpltps     k1{k1},zmm0,zmm2
  2.59 │       kxorw        k1,k1,k0
  4.56 │       vmovaps      zmm1{k1},zmm3
  4.29 │       vcmpgtps     k1,zmm1,zmm2
  7.23 │       vfmadd231ps  zmm0,zmm1,zmm6
  9.29 │       vxorps       zmm3,zmm1,zmm4
  9.17 │       vcmpgtps     k0{k1},zmm0,zmm5
  9.53 │       vcmpltps     k1,zmm1,zmm2
  7.42 │       kxorw        k1,k1,k0
  7.81 │       vcmpltps     k1{k1},zmm0,zmm2
  4.63 │       kxorw        k1,k1,k0
  1.19 │       vmovaps      zmm1{k1},zmm3
  1.14 │       vmovaps      zmm3,zmm0
  1.66 │       vpermt2ps    zmm3,zmm16,zmm1
  1.22 │       vpermt2ps    zmm0,zmm15,zmm1
  1.49 │       vmovups      ZMMWORD PTR [rax-0x80],zmm3
  2.09 │       vmovups      ZMMWORD PTR [rax-0x40],zmm0
  1.11 │       cmp          r8,rax
       │     ↑ jne          1b0
<snip>
```

### clang++ -O3 -march=native
`clang` also attempts to vectorize the code, but in a slightly different way that appears to not work as well on the test system. It still uses `vpermt2ps` but also `vscatterdps` (scatter packed single using signed DWORD indices).

```
$ perf record -D 100 ./out/clang/O3/native/VectorOfStruct
$ perf report -Mintel
<snip>
  0.16 │8e0:   vmovups      zmm18,ZMMWORD PTR [r12]
  0.12 │       vmovups      zmm19,ZMMWORD PTR [r12+0x40]
  0.17 │       kxnorw       k1,k0,k0
  0.16 │       vmovaps      zmm20,zmm18
  0.09 │       vpermt2ps    zmm20,zmm10,zmm19
  0.16 │       vpermt2ps    zmm18,zmm11,zmm19
  0.18 │       vfmadd231ps  zmm20,zmm18,zmm12
  0.08 │       vxorps       zmm19,zmm18,zmm15
 12.05 │       vscatterdps  DWORD PTR [r12+zmm13*1]{k1},zmm20
  0.08 │       vcmpltps     k1,zmm20,zmm14
  0.08 │       vcmpltps     k0{k1},zmm18,zmm19
  0.09 │       knotw        k1,k0
  0.08 │       vcmpgtps     k1{k1},zmm20,zmm16
  0.07 │       vcmpltps     k1{k1},zmm19,zmm18
  0.11 │       korw         k1,k1,k0
 10.84 │       vscatterdps  DWORD PTR [r12+zmm17*1]{k1},zmm19
  0.22 │       sub          r12,0xffffffffffffff80
  0.25 │       add          r14,0xfffffffffffffff0
       │     ↑ jne          8e0
<snip>
```

## Additional Data
Quite a bit of data to fully dig into, but one thing that stands out is that the `clang++` native solution actually is more `backend_bound_memory` than `backend_bound_cpu`, unlike the `g++` native solution.

### g++ -Og
```
$ perf stat -D100 -ddd ./out/gcc/debug/default/VectorOfStruct
          4,880.65 msec task-clock                       #    1.000 CPUs utilized
                13      context-switches                 #    2.664 /sec
                 5      cpu-migrations                   #    1.024 /sec
                 7      page-faults                      #    1.434 /sec
    26,925,434,164      cycles                           #    5.517 GHz                         (35.67%)
        86,144,973      stalled-cycles-frontend          #    0.32% frontend cycles idle        (35.68%)
   137,672,332,952      instructions                     #    5.11  insn per cycle
                                                         #    0.00  stalled cycles per insn     (35.71%)
    29,490,565,066      branches                         #    6.042 G/sec                       (35.72%)
         2,249,641      branch-misses                    #    0.01% of all branches             (35.75%)
    49,156,394,576      L1-dcache-loads                  #   10.072 G/sec                       (35.75%)
     1,230,358,616      L1-dcache-load-misses            #    2.50% of all L1-dcache accesses   (35.75%)
        39,521,469      L1-icache-loads                  #    8.098 M/sec                       (35.77%)
            20,472      L1-icache-load-misses            #    0.05% of all L1-icache accesses   (35.76%)
        19,549,240      dTLB-loads                       #    4.005 M/sec                       (35.73%)
           104,146      dTLB-load-misses                 #    0.53% of all dTLB cache accesses  (35.72%)
             2,457      iTLB-loads                       #  503.417 /sec                        (35.69%)
               342      iTLB-load-misses                 #   13.92% of all iTLB cache accesses  (35.65%)
     1,224,712,544      L1-dcache-prefetches             #  250.932 M/sec                       (35.65%)

$ perf stat -D100 -MPipelineL2 ./out/gcc/debug/default/VectorOfStruct
         2,160,921      ex_ret_brn_misp                  #      0.1 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (24.94%)
   147,632,704,625      de_src_op_disp.all                                                      (24.94%)
           147,631      resyncs_or_nc_redirects                                                 (24.94%)
    26,884,754,166      ls_not_halted_cyc                                                       (24.94%)
   147,483,529,653      ex_ret_ops                                                              (24.94%)
        75,471,148      ex_no_retire.load_not_complete   #      5.8 %  backend_bound_cpu
                                                         #      0.1 %  backend_bound_memory     (25.09%)
     9,575,611,252      de_no_dispatch_per_slot.backend_stalls                                  (25.09%)
     5,037,941,586      ex_no_retire.not_complete                                               (25.09%)
    26,880,018,200      ls_not_halted_cyc                                                       (25.09%)
        10,645,341      ex_ret_ucode_ops                 #     91.3 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (24.93%)
    26,889,657,796      ls_not_halted_cyc                                                       (24.93%)
   147,389,079,874      ex_ret_ops                                                              (24.93%)
     4,030,323,686      de_no_dispatch_per_slot.no_ops_from_frontend                #      2.2 %  frontend_bound_bandwidth  (25.04%)
        85,956,501      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      0.3 %  frontend_bound_latency   (25.04%)
    26,895,507,705      ls_not_halted_cyc                                                       (25.04%)
```

### g++ -O3 -march=native
```
$ perf stat -D100 -ddd ./out/gcc/O3/native/VectorOfStruct
          1,097.49 msec task-clock                       #    1.000 CPUs utilized
                 3      context-switches                 #    2.734 /sec
                 3      cpu-migrations                   #    2.734 /sec
                 7      page-faults                      #    6.378 /sec
     6,054,275,101      cycles                           #    5.516 GHz                         (35.60%)
        23,952,523      stalled-cycles-frontend          #    0.40% frontend cycles idle        (35.69%)
     8,688,365,047      instructions                     #    1.44  insn per cycle
                                                         #    0.00  stalled cycles per insn     (35.77%)
       292,128,569      branches                         #  266.178 M/sec                       (35.87%)
           389,164      branch-misses                    #    0.13% of all branches             (35.96%)
     4,072,521,880      L1-dcache-loads                  #    3.711 G/sec                       (35.98%)
       582,029,053      L1-dcache-load-misses            #   14.29% of all L1-dcache accesses   (35.89%)
        12,831,312      L1-icache-loads                  #   11.691 M/sec                       (35.80%)
            12,351      L1-icache-load-misses            #    0.10% of all L1-icache accesses   (35.70%)
         9,129,427      dTLB-loads                       #    8.318 M/sec                       (35.62%)
            17,756      dTLB-load-misses                 #    0.19% of all dTLB cache accesses  (35.53%)
             1,364      iTLB-loads                       #    1.243 K/sec                       (35.53%)
                16      iTLB-load-misses                 #    1.17% of all iTLB cache accesses  (35.54%)
       575,968,525      L1-dcache-prefetches             #  524.804 M/sec                       (35.54%)

$ perf stat -D100 -MPipelineL2 ./out/gcc/O3/native/VectorOfStruct
           658,678      ex_ret_brn_misp                  #      0.0 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (24.97%)
     8,989,920,867      de_src_op_disp.all                                                      (24.97%)
            93,728      resyncs_or_nc_redirects                                                 (24.97%)
     6,077,648,637      ls_not_halted_cyc                                                       (24.97%)
     8,978,428,440      ex_ret_ops                                                              (24.97%)
        18,331,518      ex_no_retire.load_not_complete   #     73.8 %  backend_bound_cpu
                                                         #      0.4 %  backend_bound_memory     (25.05%)
    27,073,485,348      de_no_dispatch_per_slot.backend_stalls                                  (25.05%)
     3,308,778,426      ex_no_retire.not_complete                                               (25.05%)
     6,076,379,621      ls_not_halted_cyc                                                       (25.05%)
         2,370,423      ex_ret_ucode_ops                 #     24.6 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (24.94%)
     6,078,965,799      ls_not_halted_cyc                                                       (24.94%)
     8,984,247,434      ex_ret_ops                                                              (24.94%)
       378,205,895      de_no_dispatch_per_slot.no_ops_from_frontend                #      0.6 %  frontend_bound_bandwidth  (25.05%)
        24,217,112      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      0.4 %  frontend_bound_latency   (25.05%)
     6,080,221,100      ls_not_halted_cyc                                                       (25.05%)
```

### clang++ -O3 -march=native
```
$ perf stat -D100 -ddd ./out/clang/O3/native/VectorOfStruct
          5,770.19 msec task-clock                       #    1.000 CPUs utilized
                13      context-switches                 #    2.253 /sec
                 2      cpu-migrations                   #    0.347 /sec
                 7      page-faults                      #    1.213 /sec
    31,833,793,529      cycles                           #    5.517 GHz                         (35.71%)
       732,110,851      stalled-cycles-frontend          #    2.30% frontend cycles idle        (35.73%)
    11,785,569,551      instructions                     #    0.37  insn per cycle
                                                         #    0.06  stalled cycles per insn     (35.74%)
       634,425,726      branches                         #  109.949 M/sec                       (35.74%)
         2,414,587      branch-misses                    #    0.38% of all branches             (35.75%)
    12,340,151,949      L1-dcache-loads                  #    2.139 G/sec                       (35.73%)
     1,234,056,286      L1-dcache-load-misses            #   10.00% of all L1-dcache accesses   (35.71%)
        45,043,736      L1-icache-loads                  #    7.806 M/sec                       (35.70%)
            31,516      L1-icache-load-misses            #    0.07% of all L1-icache accesses   (35.70%)
        19,580,890      dTLB-loads                       #    3.393 M/sec                       (35.70%)
            58,168      dTLB-load-misses                 #    0.30% of all dTLB cache accesses  (35.70%)
             6,227      iTLB-loads                       #    1.079 K/sec                       (35.70%)
               249      iTLB-load-misses                 #    4.00% of all iTLB cache accesses  (35.70%)
     1,228,066,140      L1-dcache-prefetches             #  212.829 M/sec                       (35.70%)

$ perf stat -D100 -MPipelineL2 ./out/clang/O3/native/VectorOfStruct
         2,487,915      ex_ret_brn_misp                  #      0.0 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (24.95%)
   119,518,686,163      de_src_op_disp.all                                                      (24.95%)
           209,106      resyncs_or_nc_redirects                                                 (24.95%)
    31,836,967,827      ls_not_halted_cyc                                                       (24.95%)
   119,478,110,867      ex_ret_ops                                                              (24.95%)
     9,969,513,985      ex_no_retire.load_not_complete   #      4.2 %  backend_bound_cpu
                                                         #     29.3 %  backend_bound_memory     (25.07%)
    63,991,925,239      de_no_dispatch_per_slot.backend_stalls                                  (25.07%)
    11,403,330,560      ex_no_retire.not_complete                                               (25.07%)
    31,830,348,678      ls_not_halted_cyc                                                       (25.07%)
   109,604,097,468      ex_ret_ucode_ops                 #      5.2 %  retiring_fastpath
                                                         #     57.4 %  retiring_microcode       (24.92%)
    31,842,841,357      ls_not_halted_cyc                                                       (24.92%)
   119,539,987,860      ex_ret_ops                                                              (24.92%)
     7,414,823,782      de_no_dispatch_per_slot.no_ops_from_frontend                #      1.6 %  frontend_bound_bandwidth  (25.07%)
       725,712,257      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      2.3 %  frontend_bound_latency   (25.07%)
    31,849,878,310      ls_not_halted_cyc                                                       (25.07%)
```
