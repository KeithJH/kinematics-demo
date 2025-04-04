# VectorOfLargeStruct
Much like `VectorOfStruct` this implementation uses a conventional AoS layout using a `std::vector`. Uniquely this solution has a larger structure for points which mimics a likely layout for larger applications. Potential uses could be fields for other dimensions of motion (though they'd still need to loaded anyways) or even data unrelated to kinematics such as rendering information like color. While the additional data is unused in this example, it still spreads out position and speed values throughout memory. This is a good contrast as the "clever" solutions for vectorizing `VectorOfStruct` should no longer work as well.

```
struct Point
{
    float position;
    float speed;
    float _unused[6];
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
| gcc      | O0           | default      |     52084 |
| gcc      | debug        | default      |      5868 |
| gcc      | O1           | default      |      5542 |
| gcc      | O2           | default      |      5311 |
| gcc      | O3           | default      |      4787 |
| gcc      | O3           | native       |      3512 |
| clang    | debug        | default      |      5420 |
| clang    | O3           | default      |      5452 |
| clang    | O3           | native       |      7423 |

Focusing on compiling with `g++` (version 13.3.0) we still see optimizations that are likely benefiting from vectorization, but less than in `VectorOfStruct`. There we get a best time of 1180 ms compared to 3512 ms here.

Taking a quick look at `clang++` (version 18.1.3), however, we still don't see the same improvement and even a performance regression when compiling for native architecture.

## Assembly Analysis
### g++ -O0
The same as with `VectorOfStruct`, here with no optimization a significant amount of time is understandably spent calling into iterator functionality.

```
$ perf record -D 100 ./out/gcc/O0/default/VectorOfLargeStruct
$ perf report -Mintel
  46.06%  VectorOfLargeSt  VectorOfLargeStruct  [.] main
  19.33%  VectorOfLargeSt  VectorOfLargeStruct  [.] bool __gnu_cxx::operator!=<main::Point*, std::vector<main::Point...
  14.92%  VectorOfLargeSt  VectorOfLargeStruct  [.] __gnu_cxx::__normal_iterator<main::Point*, std::vector<main::Point...
   9.82%  VectorOfLargeSt  VectorOfLargeStruct  [.] __gnu_cxx::__normal_iterator<main::Point*, std::vector<main::Point...
   9.39%  VectorOfLargeSt  VectorOfLargeStruct  [.] __gnu_cxx::__normal_iterator<main::Point*, std::vector<main::Point...

```

### g++ -Og
Here we have debug symbols, which helps map which instructions are doing which calculations. Notably while vector instructions are being used (like `mulss`), they are the scalar versions for single precision floats (as denoted by the suffix `ss`).

```
$ perf record -D 100 ./out/gcc/debug/default/VectorOfLargeStruct
$ perf report -Mintel
<snip>
       │160:   comiss   xmm2,xmm1
       │     ↓ jbe      19e
       │     (point.position > POSITION_LIMIT && point.speed > 0))
       │     {
       │     point.speed *= -1;
       │165:   xorps    xmm1,XMMWORD PTR [rip+0x8bf]
       │       movss    DWORD PTR [rdx+0x4],xmm1
       │
       │     _GLIBCXX20_CONSTEXPR
       │     __normal_iterator&
       │     operator++() _GLIBCXX_NOEXCEPT
       │     {
       │     ++_M_current;
  8.05 │171:   lea      rax,[rdx+0x20]
       │     _GLIBCXX_NODISCARD _GLIBCXX20_CONSTEXPR
       │     inline bool
       │     operator!=(const __normal_iterator<_Iterator, _Container>& __lhs,
       │     const __normal_iterator<_Iterator, _Container>& __rhs)
       │     _GLIBCXX_NOEXCEPT
       │     { return __lhs.base() != __rhs.base(); }
  8.33 │175:   mov      rdx,rax
       │     for (auto &point : points)
  7.91 │       cmp      rcx,rax
       │     ↓ je       1b2
       │     point.position += point.speed * DELTA_TIME;
  6.62 │       movss    xmm1,DWORD PTR [rax+0x4]
  6.76 │       movaps   xmm0,xmm1
  6.91 │       mulss    xmm0,DWORD PTR [rip+0x826]
  5.68 │       addss    xmm0,DWORD PTR [rax]
  5.60 │       movss    DWORD PTR [rax],xmm0
       │     if ((point.position < 0 && point.speed < 0) ||
  5.53 │       pxor     xmm2,xmm2
 11.32 │       comiss   xmm2,xmm0
  5.81 │     ↑ ja       160
 14.32 │19e:   comiss   xmm0,DWORD PTR [rip+0x812]        # 200c <_IO_stdin_used+0xc>
  7.10 │     ↑ jbe      171
       │     (point.position > POSITION_LIMIT && point.speed > 0))
  0.05 │       pxor     xmm0,xmm0
  0.00 │       comiss   xmm1,xmm0
  0.01 │     ↑ ja       165
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
It's at this point that we start to see "true" vectorization, as commands with the suffix `ps` (for packed, single precision floats) are being used. It is also where we see the additional overhead of shuffling as compared to `VectorOfStruct`. As a loose comparison we see a total of 14 `shufps` commands here compared to 2 in `VectorOfStruct`. That may not be entirely fair as the 2 sets of `mulps` and `addps` suggest some loop unrolling compared to 1 of each in `VectorOfStruct`. Even with that, however, there is still much more shuffling of data around as suggested by the performance difference.

```
$ perf record -D 100 ./out/gcc/O3/default/VectorOfLargeStruct
$ perf report -Mintel
<snip>
  0.80 │158:   movups     xmm2,XMMWORD PTR [rax]
  1.43 │       movups     xmm13,XMMWORD PTR [rax+0x10]
  1.49 │       sub        rax,0xffffffffffffff80
  0.79 │       movups     xmm10,XMMWORD PTR [rax-0x60]
  0.80 │       movups     xmm1,XMMWORD PTR [rax-0x40]
  0.83 │       movaps     xmm0,xmm2
  0.79 │       movups     xmm9,XMMWORD PTR [rax-0x20]
  1.65 │       movups     xmm8,XMMWORD PTR [rax-0x10]
  0.67 │       shufps     xmm0,xmm13,0xdd
  0.79 │       movups     xmm13,XMMWORD PTR [rax-0x50]
  0.78 │       movaps     xmm11,xmm10
  0.89 │       shufps     xmm11,xmm13,0xdd
  0.77 │       movups     xmm13,XMMWORD PTR [rax-0x30]
  0.91 │       shufps     xmm0,xmm11,0x88
  0.83 │       movaps     xmm11,xmm1
  0.98 │       shufps     xmm11,xmm13,0xdd
  0.93 │       movaps     xmm13,xmm9
  1.00 │       shufps     xmm13,xmm8,0xdd
  1.07 │       movups     xmm8,XMMWORD PTR [rax-0x70]
  0.94 │       shufps     xmm11,xmm13,0x88
  0.89 │       movups     xmm13,XMMWORD PTR [rax-0x50]
  1.26 │       shufps     xmm0,xmm11,0x88
  1.30 │       movups     xmm11,XMMWORD PTR [rax-0x30]
  1.21 │       shufps     xmm2,xmm8,0x88
  1.39 │       movups     xmm8,XMMWORD PTR [rax-0x10]
  1.26 │       shufps     xmm10,xmm13,0x88
  1.14 │       shufps     xmm2,xmm10,0x88
  1.80 │       shufps     xmm1,xmm11,0x88
  1.81 │       shufps     xmm9,xmm8,0x88
  1.75 │       shufps     xmm1,xmm9,0x88
  1.79 │       shufps     xmm2,xmm1,0x88
  2.01 │       movaps     xmm1,xmm0
  1.87 │       mulps      xmm1,xmm7
  2.13 │       movaps     xmm9,xmm3
  2.36 │       cmpltps    xmm9,xmm0
  2.20 │       addps      xmm2,xmm1
  2.11 │       movaps     xmm1,xmm5
  2.08 │       cmpltps    xmm1,xmm2
  2.22 │       movaps     xmm10,xmm2
  2.14 │       cmpltps    xmm10,xmm3
  1.85 │       pand       xmm9,xmm1
  1.85 │       movaps     xmm1,xmm0
  1.84 │       cmpltps    xmm1,xmm3
  1.87 │       pxor       xmm1,xmm9
  2.09 │       pand       xmm1,xmm10
  1.14 │       pxor       xmm1,xmm9
  1.12 │       movaps     xmm9,xmm0
  1.02 │       xorps      xmm9,xmm4
  1.06 │       andps      xmm9,xmm1
  1.23 │       andnps     xmm1,xmm0
  1.11 │       orps       xmm1,xmm9
  1.22 │       movaps     xmm9,xmm3
  1.24 │       movaps     xmm0,xmm1
  1.04 │       cmpltps    xmm9,xmm1
  1.19 │       mulps      xmm0,xmm7
  1.15 │       addps      xmm0,xmm2
  1.06 │       movaps     xmm2,xmm5
  0.71 │       cmpltps    xmm2,xmm0
  0.75 │       movaps     xmm10,xmm0
  0.89 │       cmpltps    xmm10,xmm3
  0.75 │       pand       xmm9,xmm2
  0.83 │       movaps     xmm2,xmm1
  0.85 │       cmpltps    xmm2,xmm3
  0.75 │       pxor       xmm2,xmm9
  0.69 │       pand       xmm2,xmm10
  0.82 │       pxor       xmm2,xmm9
  0.71 │       movaps     xmm9,xmm1
  0.79 │       xorps      xmm9,xmm4
  0.76 │       andps      xmm9,xmm2
  0.66 │       andnps     xmm2,xmm1
  0.76 │       movaps     xmm1,xmm2
  0.80 │       orps       xmm1,xmm9
  0.82 │       movaps     xmm2,xmm1
  0.80 │       shufps     xmm2,xmm0,0x0
  0.81 │       shufps     xmm2,xmm0,0xe2
  0.75 │       movlps     QWORD PTR [rax-0x80],xmm2
  0.76 │       movaps     xmm2,xmm0
  0.82 │       unpckhps   xmm0,xmm1
  0.72 │       shufps     xmm2,xmm1,0x55
  0.68 │       shufps     xmm2,xmm1,0xe8
  0.71 │       movlps     QWORD PTR [rax-0x60],xmm2
  0.74 │       movaps     xmm1,xmm0
  0.75 │       shufps     xmm1,xmm0,0xd4
  0.77 │       shufps     xmm0,xmm0,0xde
  0.80 │       movlps     QWORD PTR [rax-0x40],xmm1
  0.76 │       movlps     QWORD PTR [rax-0x20],xmm0
  0.78 │       cmp        rcx,rax
       │     ↑ jne        158
<snip>
```

### g++ -O3 -march=native
Once we start compiling for the native architecture (on a Zen 4 test system) we even start to see full 512-bit vector instructions instead of the default 128-bit ones. Not only that, but we also see our first fused-multiply-add instruction, which is perfect for the kinematics use case. Comparisons also start using masking functionality (with `k` values). Shifting data around is also much more prevalent.

```
$ perf record -D 100 ./out/gcc/O3/native/VectorOfLargeStruct
$ perf report -Mintel
<snip>
  0.35 │ 230:   vmovups      zmm2,ZMMWORD PTR [rax]
  0.43 │        vmovups      zmm0,ZMMWORD PTR [rax+0x80]
  0.53 │        vmovups      zmm27,ZMMWORD PTR [rax+0x180]
  0.32 │        add          rax,0x200
  0.80 │        vmovups      zmm28,ZMMWORD PTR [rax-0x80]
  0.82 │        vpermt2ps    zmm0,zmm5,ZMMWORD PTR [rax-0x140]
  0.97 │        vpermt2ps    zmm2,zmm5,ZMMWORD PTR [rax-0x1c0]
  0.69 │        vpermt2ps    zmm27,zmm5,ZMMWORD PTR [rax-0x40]
  1.14 │        vpermt2ps    zmm2,zmm3,zmm0
  1.06 │        vmovups      zmm0,ZMMWORD PTR [rax-0x100]
  0.66 │        vpermt2ps    zmm28,zmm3,ZMMWORD PTR [rax-0x40]
  0.96 │        vpermt2ps    zmm0,zmm5,ZMMWORD PTR [rax-0xc0]
  0.69 │        vpermt2ps    zmm0,zmm3,zmm27
  0.91 │        vmovups      zmm27,ZMMWORD PTR [rax-0x180]
  0.94 │        vpermt2ps    zmm2,zmm3,zmm0
  1.03 │        vmovups      zmm0,ZMMWORD PTR [rax-0x200]
  0.88 │        vcmpgtps     k1,zmm2,zmm4
  0.74 │        vpermt2ps    zmm27,zmm3,ZMMWORD PTR [rax-0x140]
  1.36 │        vpermt2ps    zmm0,zmm3,ZMMWORD PTR [rax-0x1c0]
  1.40 │        vpermt2ps    zmm0,zmm3,zmm27
  0.86 │        vmovups      zmm27,ZMMWORD PTR [rax-0x100]
  1.20 │        vpermt2ps    zmm27,zmm3,ZMMWORD PTR [rax-0xc0]
  1.14 │        vpermt2ps    zmm27,zmm3,zmm28
  1.34 │        vpermt2ps    zmm0,zmm3,zmm27
  1.23 │        vxorps       zmm27,zmm2,zmm6
  1.27 │        vfmadd231ps  zmm0,zmm2,zmm9
  1.32 │        vcmpgtps     k0{k1},zmm0,zmm7
  1.58 │        vcmpltps     k1,zmm2,zmm4
  1.53 │        kxorw        k1,k1,k0
  1.35 │        vcmpltps     k1{k1},zmm0,zmm4
  1.41 │        kxorw        k1,k1,k0
  1.16 │        vmovaps      zmm2{k1},zmm27
  1.41 │        vcmpgtps     k1,zmm2,zmm4
  1.45 │        vfmadd231ps  zmm0,zmm2,zmm9
  1.81 │        vxorps       zmm27,zmm2,zmm6
  1.61 │        vcmpgtps     k0{k1},zmm0,zmm7
  1.64 │        vcmpltps     k1,zmm2,zmm4
  1.63 │        kxorw        k1,k1,k0
  1.45 │        vcmpltps     k1{k1},zmm0,zmm4
  1.32 │        kxorw        k1,k1,k0
  1.25 │        vmovaps      zmm2{k1},zmm27
  1.43 │        vmovaps      zmm27,zmm0
  1.38 │        vpermt2ps    zmm27,zmm26,zmm2
  1.35 │        vmovq        rdx,xmm27
  1.13 │        vmovaps      zmm27,zmm0
  1.12 │        vpermt2ps    zmm27,zmm25,zmm2
  1.08 │        mov          QWORD PTR [rax-0x200],rdx
  1.11 │        vmovq        rdx,xmm27
  1.21 │        vmovaps      zmm27,zmm0
  1.03 │        vpermt2ps    zmm27,zmm24,zmm2
  1.00 │        mov          QWORD PTR [rax-0x1e0],rdx
  1.02 │        vmovq        rdx,xmm27
  1.02 │        vmovaps      zmm27,zmm0
  0.95 │        vpermt2ps    zmm27,zmm23,zmm2
  0.75 │        mov          QWORD PTR [rax-0x1c0],rdx
  1.10 │        vmovq        rdx,xmm27
  1.05 │        vmovaps      zmm27,zmm0
  0.86 │        vpermt2ps    zmm27,zmm22,zmm2
  0.84 │        mov          QWORD PTR [rax-0x1a0],rdx
  1.00 │        vmovq        rdx,xmm27
  0.92 │        vmovaps      zmm27,zmm0
  0.81 │        vpermt2ps    zmm27,zmm21,zmm2
  0.94 │        mov          QWORD PTR [rax-0x180],rdx
  0.84 │        vmovq        rdx,xmm27
  0.82 │        vmovaps      zmm27,zmm0
  0.90 │        vpermt2ps    zmm27,zmm20,zmm2
  0.92 │        mov          QWORD PTR [rax-0x160],rdx
  0.80 │        vmovq        rdx,xmm27
  0.93 │        vmovaps      zmm27,zmm0
  0.57 │        vpermt2ps    zmm27,zmm19,zmm2
  0.70 │        mov          QWORD PTR [rax-0x140],rdx
  0.77 │        vmovq        rdx,xmm27
  0.75 │        vmovaps      zmm27,zmm0
  1.05 │        vpermt2ps    zmm0,zmm10,zmm2
  1.20 │        vpermt2ps    zmm27,zmm11,zmm2
  0.65 │        mov          QWORD PTR [rax-0x60],rdx
  0.55 │        vmovlps      QWORD PTR [rax-0x20],xmm0
  0.64 │        vmovq        rdx,xmm27
  1.06 │        mov          QWORD PTR [rax-0x40],rdx
  0.72 │        cmp          rax,rdi
       │      ↑ jne          230
<snip>
```

### clang++ -O3 -march=native
`clang` also attempts to vectorize the code, but in a slightly different way that appears to not work as well on the test system. It still uses `vpermt2ps` but also `vscatterdps` (scatter packed single using signed DWORD indices), `vblendps` (blend packed single precision floats), and `vinsertf64x4` (insert packed floating point values).

```
$ perf record -D 100 ./out/clang/O3/native/VectorOfLargeStruct
$ perf report -Mintel
<snip>
  0.23 │ a70:   vmovups         zmm10,ZMMWORD PTR [rbx+0x100]
  0.11 │        vmovups         zmm25,ZMMWORD PTR [rbx+0x180]
  0.30 │        vmovups         zmm9,ZMMWORD PTR [rbx+0x140]
  0.32 │        vmovups         zmm24,ZMMWORD PTR [rbx+0x1c0]
  0.32 │        vmovups         zmm3,ZMMWORD PTR [rbx]
  0.28 │        vmovups         zmm13,ZMMWORD PTR [rbx+0x80]
  0.13 │        vmovups         zmm6,ZMMWORD PTR [rbx+0x40]
  0.10 │        vmovups         zmm7,ZMMWORD PTR [rbx+0xc0]
  0.25 │        kmovd           k1,ebp
  0.10 │        vmovaps         zmm26,zmm25
  0.10 │        vmovaps         zmm27,zmm10
  0.06 │        vpermt2ps       zmm26,zmm14,zmm24
  0.07 │        vpermt2ps       zmm27,zmm14,zmm9
  0.09 │        vmovaps         zmm5,zmm13
  0.05 │        vmovaps         zmm4,zmm3
  0.05 │        vpermt2ps       zmm5,zmm14,zmm7
  0.08 │        vpermt2ps       zmm4,zmm14,zmm6
  0.07 │        vpermt2ps       zmm13,zmm15,zmm7
  0.06 │        vpermt2ps       zmm3,zmm15,zmm6
  0.08 │        vpermt2ps       zmm25,zmm15,zmm24
  0.07 │        vpermt2ps       zmm10,zmm15,zmm9
  0.04 │        vmovapd         zmm27{k1},zmm26
  0.08 │        vmovaps         zmm26,ZMMWORD PTR [rip+0x137d]        # 3040 <_IO_stdin_used+0x40>
  0.04 │        vblendps        ymm4,ymm4,ymm5,0xf0
  0.05 │        vblendps        ymm3,ymm3,ymm13,0xf0
  0.02 │        vmovapd         zmm10{k1},zmm25
  0.07 │        kxnorw          k1,k0,k0
  0.05 │        vmovaps         zmm13,ZMMWORD PTR [rip+0x139d]        # 3080 <_IO_stdin_used+0x80>
  0.05 │        vinsertf64x4    zmm4,zmm27,ymm4,0x0
  0.07 │        vinsertf64x4    zmm3,zmm10,ymm3,0x0
  0.12 │        vxorpd          xmm27,xmm27,xmm27
  0.11 │        vfmadd231ps     zmm4,zmm3,zmm16
  0.07 │        vxorps          zmm5,zmm3,zmm19
 12.91 │        vscatterdps     DWORD PTR [rbx+zmm26*1]{k1},zmm4
  0.06 │        vcmpltps        k1,zmm4,zmm27
  0.05 │        vcmpltps        k0{k1},zmm3,zmm5
  0.06 │        knotw           k1,k0
  0.08 │        vcmpgtps        k1{k1},zmm4,zmm20
  0.08 │        vcmpltps        k1{k1},zmm5,zmm3
  0.07 │        korw            k1,k1,k0
  7.81 │        vscatterdps     DWORD PTR [rbx+zmm13*1]{k1},zmm5
  0.22 │        add             rbx,0x200
  0.24 │        add             r15,0xfffffffffffffff0
  0.01 │      ↑ jne             a70
<snip>
```

## Additional Data
Quite a bit of data to fully dig into, but one thing that stands out is that the `clang++` native solution actually is more `backend_bound_memory` than `backend_bound_cpu`, unlike the `g++` native solution.

### g++ -Og
```
$ perf stat -D100 -ddd ./out/gcc/debug/default/VectorOfLargeStruct
          6,101.35 msec task-clock                       #    1.000 CPUs utilized
                13      context-switches                 #    2.131 /sec
                 3      cpu-migrations                   #    0.492 /sec
                 7      page-faults                      #    1.147 /sec
    33,585,688,938      cycles                           #    5.505 GHz                         (35.69%)
       301,619,737      stalled-cycles-frontend          #    0.90% frontend cycles idle        (35.69%)
   138,359,313,230      instructions                     #    4.12  insn per cycle
                                                         #    0.00  stalled cycles per insn     (35.69%)
    29,650,682,616      branches                         #    4.860 G/sec                       (35.70%)
         2,450,870      branch-misses                    #    0.01% of all branches             (35.73%)
    49,464,605,147      L1-dcache-loads                  #    8.107 G/sec                       (35.72%)
     4,941,739,784      L1-dcache-load-misses            #    9.99% of all L1-dcache accesses   (35.73%)
        54,779,335      L1-icache-loads                  #    8.978 M/sec                       (35.73%)
            20,881      L1-icache-load-misses            #    0.04% of all L1-icache accesses   (35.73%)
        77,861,968      dTLB-loads                       #   12.761 M/sec                       (35.73%)
        77,046,097      dTLB-load-misses                 #   98.95% of all dTLB cache accesses  (35.73%)
             2,605      iTLB-loads                       #  426.955 /sec                        (35.73%)
               531      iTLB-load-misses                 #   20.38% of all iTLB cache accesses  (35.71%)
     4,920,290,623      L1-dcache-prefetches             #  806.427 M/sec                       (35.69%)

$ perf stat -D100 -MPipelineL2 ./out/gcc/debug/default/VectorOfLargeStruct
         2,707,808      ex_ret_brn_misp                  #      0.1 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (25.01%)
   148,396,786,184      de_src_op_disp.all                                                      (25.01%)
           485,577      resyncs_or_nc_redirects                                                 (25.01%)
    33,159,827,306      ls_not_halted_cyc                                                       (25.01%)
   148,154,670,762      ex_ret_ops                                                              (25.01%)
     5,280,720,921      ex_no_retire.load_not_complete   #     10.9 %  backend_bound_cpu
                                                         #     10.6 %  backend_bound_memory     (25.01%)
    42,846,947,881      de_no_dispatch_per_slot.backend_stalls                                  (25.01%)
    10,733,051,787      ex_no_retire.not_complete                                               (25.01%)
    33,155,255,849      ls_not_halted_cyc                                                       (25.01%)
        13,998,949      ex_ret_ucode_ops                 #     74.4 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (24.99%)
    33,169,607,697      ls_not_halted_cyc                                                       (24.99%)
   148,044,431,598      ex_ret_ops                                                              (24.99%)
     5,449,429,645      de_no_dispatch_per_slot.no_ops_from_frontend                #      1.8 %  frontend_bound_bandwidth  (24.99%)
       299,000,905      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      0.9 %  frontend_bound_latency   (24.99%)
    33,174,251,783      ls_not_halted_cyc                                                       (24.99%)
```

### g++ -O3 -march=native
```
$ perf stat -D100 -ddd ./out/gcc/O3/native/VectorOfLargeStruct
          3,558.13 msec task-clock                       #    1.000 CPUs utilized
                15      context-switches                 #    4.216 /sec
                 6      cpu-migrations                   #    1.686 /sec
                 7      page-faults                      #    1.967 /sec
    19,593,844,151      cycles                           #    5.507 GHz                         (35.74%)
       191,293,441      stalled-cycles-frontend          #    0.98% frontend cycles idle        (35.76%)
    32,128,525,136      instructions                     #    1.64  insn per cycle
                                                         #    0.01  stalled cycles per insn     (35.76%)
       321,409,488      branches                         #   90.331 M/sec                       (35.76%)
         1,644,840      branch-misses                    #    0.51% of all branches             (35.76%)
    14,804,816,535      L1-dcache-loads                  #    4.161 G/sec                       (35.71%)
     2,466,350,705      L1-dcache-load-misses            #   16.66% of all L1-dcache accesses   (35.69%)
        84,081,050      L1-icache-loads                  #   23.631 M/sec                       (35.69%)
           377,713      L1-icache-load-misses            #    0.45% of all L1-icache accesses   (35.69%)
        38,593,287      dTLB-loads                       #   10.847 M/sec                       (35.69%)
        38,324,957      dTLB-load-misses                 #   99.30% of all dTLB cache accesses  (35.69%)
               249      iTLB-loads                       #   69.981 /sec                        (35.69%)
                25      iTLB-load-misses                 #   10.04% of all iTLB cache accesses  (35.69%)
     2,440,534,756      L1-dcache-prefetches             #  685.904 M/sec                       (35.69%)

$ perf stat -D100 -MPipelineL2 ./out/gcc/O3/native/VectorOfLargeStruct
         1,110,192      ex_ret_brn_misp                  #      0.1 %  bad_speculation_mispredicts
                                                         #      0.1 %  bad_speculation_pipeline_restarts  (25.01%)
    32,105,612,095      de_src_op_disp.all                                                      (25.01%)
         1,443,253      resyncs_or_nc_redirects                                                 (25.01%)
    19,296,601,784      ls_not_halted_cyc                                                       (25.01%)
    31,834,025,458      ex_ret_ops                                                              (25.01%)
     1,545,772,136      ex_no_retire.load_not_complete   #     60.0 %  backend_bound_cpu
                                                         #     10.0 %  backend_bound_memory     (25.00%)
    81,106,565,113      de_no_dispatch_per_slot.backend_stalls                                  (25.00%)
    10,801,065,251      ex_no_retire.not_complete                                               (25.00%)
    19,290,480,862      ls_not_halted_cyc                                                       (25.00%)
         7,081,600      ex_ret_ucode_ops                 #     27.5 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (25.00%)
    19,297,315,853      ls_not_halted_cyc                                                       (25.00%)
    31,841,739,771      ex_ret_ops                                                              (25.00%)
     1,984,010,573      de_no_dispatch_per_slot.no_ops_from_frontend                #      0.8 %  frontend_bound_bandwidth  (24.99%)
       169,609,417      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      0.9 %  frontend_bound_latency   (24.99%)
    19,304,708,837      ls_not_halted_cyc                                                       (24.99%)
```

### clang++ -O3 -march=native
```
$ perf stat -D100 -ddd ./out/clang/O3/native/VectorOfLargeStruct
          7,500.89 msec task-clock                       #    1.000 CPUs utilized
                33      context-switches                 #    4.399 /sec
                13      cpu-migrations                   #    1.733 /sec
                 7      page-faults                      #    0.933 /sec
    41,311,099,209      cycles                           #    5.507 GHz                         (35.68%)
       935,273,606      stalled-cycles-frontend          #    2.26% frontend cycles idle        (35.68%)
    26,532,187,446      instructions                     #    0.64  insn per cycle
                                                         #    0.04  stalled cycles per insn     (35.70%)
       637,833,427      branches                         #   85.034 M/sec                       (35.71%)
         2,433,235      branch-misses                    #    0.38% of all branches             (35.72%)
    21,516,761,055      L1-dcache-loads                  #    2.869 G/sec                       (35.74%)
     4,957,010,384      L1-dcache-load-misses            #   23.04% of all L1-dcache accesses   (35.74%)
       109,450,133      L1-icache-loads                  #   14.592 M/sec                       (35.74%)
           332,714      L1-icache-load-misses            #    0.30% of all L1-icache accesses   (35.74%)
        78,093,561      dTLB-loads                       #   10.411 M/sec                       (35.74%)
        77,581,999      dTLB-load-misses                 #   99.34% of all dTLB cache accesses  (35.72%)
               501      iTLB-loads                       #   66.792 /sec                        (35.71%)
               400      iTLB-load-misses                 #   79.84% of all iTLB cache accesses  (35.69%)
     4,931,232,826      L1-dcache-prefetches             #  657.420 M/sec                       (35.68%)

$ perf stat -D100 -MPipelineL2 ./out/clang/O3/native/VectorOfLargeStruct
         2,431,825      ex_ret_brn_misp                  #      0.1 %  bad_speculation_mispredicts
                                                         #      0.3 %  bad_speculation_pipeline_restarts  (25.00%)
   136,398,055,895      de_src_op_disp.all                                                      (25.00%)
         4,832,990      resyncs_or_nc_redirects                                                 (25.00%)
    41,594,033,935      ls_not_halted_cyc                                                       (25.00%)
   135,343,614,064      ex_ret_ops                                                              (25.00%)
    13,433,685,176      ex_no_retire.load_not_complete   #     11.6 %  backend_bound_cpu
                                                         #     30.1 %  backend_bound_memory     (25.00%)
   104,146,445,690      de_no_dispatch_per_slot.backend_stalls                                  (25.00%)
    18,633,859,395      ex_no_retire.not_complete                                               (25.00%)
    41,580,860,483      ls_not_halted_cyc                                                       (25.00%)
   110,119,327,172      ex_ret_ucode_ops                 #     10.2 %  retiring_fastpath
                                                         #     44.1 %  retiring_microcode       (25.00%)
    41,605,802,445      ls_not_halted_cyc                                                       (25.00%)
   135,458,397,961      ex_ret_ops                                                              (25.00%)
     8,942,261,185      de_no_dispatch_per_slot.no_ops_from_frontend                #      1.3 %  frontend_bound_bandwidth  (25.00%)
       946,793,543      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      2.3 %  frontend_bound_latency   (25.00%)
    41,611,704,651      ls_not_halted_cyc                                                       (25.00%)
```
