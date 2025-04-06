# Oversized
SoA layout that uses `float*` fields manually managed with `new[]` and `delete[]` while specifying alignment and ensuring adequate capacity that allows for vector commands to "overrun" the actual amount of `Points` in the calculation to avoid non-vectorized "tail" calculations.

```
struct Points
{
	float *position;
	float *speed;
};

Points points;

const size_t remainder = numPoints % POINTS_MULTIPLE;
const size_t numPointsOversized = remainder ? numPoints + POINTS_MULTIPLE - remainder : numPoints;
points.position = new (ALIGNMENT) float[numPointsOversized];
points.speed = new (ALIGNMENT) float[numPointsOversized];
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
| gcc      | O0           | default      |     19163 |
| gcc      | debug        | default      |      5467 |
| gcc      | O1           | default      |      4262 |
| gcc      | O2           | default      |      4173 |
| gcc      | O3           | default      |      4168 |
| gcc      | O3           | native       |       652 |
| clang    | debug        | default      |      4254 |
| clang    | O3           | default      |      2634 |
| clang    | O3           | native       |       761 |

## Assembly Analysis
### g++ -O0
```
$ perf record -D 100 ./out/gcc/O0/default/Oversized
$ perf report -Mintel
<snip>
       │       mov      QWORD PTR [rbp-0xa8],rax
       │       mov      QWORD PTR [rbp-0x70],0x0
       │     ↓ jmp      3e8
       │2d9:   mov      QWORD PTR [rbp-0x68],0x0
       │     ↓ jmp      3d5
  3.18 │2e6:   mov      rax,QWORD PTR [rbp-0x20]
  2.53 │       mov      rdx,QWORD PTR [rbp-0x68]
  2.66 │       shl      rdx,0x2
  2.59 │       add      rax,rdx
  2.72 │       movss    xmm1,DWORD PTR [rax]
  2.62 │       mov      rax,QWORD PTR [rbp-0x18]
  2.61 │       mov      rdx,QWORD PTR [rbp-0x68]
  2.38 │       shl      rdx,0x2
  2.36 │       add      rax,rdx
  2.50 │       movss    xmm2,DWORD PTR [rax]
  2.33 │       movss    xmm0,DWORD PTR [rip+0xb17]
  2.30 │       mulss    xmm0,xmm2
  2.27 │       mov      rax,QWORD PTR [rbp-0x20]
  2.09 │       mov      rdx,QWORD PTR [rbp-0x68]
  2.07 │       shl      rdx,0x2
  2.35 │       add      rax,rdx
  3.72 │       movss    xmm1,DWORD PTR [rax]
  2.31 │       pxor     xmm0,xmm0
  4.40 │       comiss   xmm0,xmm1
  2.23 │     ↓ jbe      367
       │       mov      rax,QWORD PTR [rbp-0x18]
       │       mov      rdx,QWORD PTR [rbp-0x68]
       │       shl      rdx,0x2
       │       add      rax,rdx
       │       movss    xmm1,DWORD PTR [rax]
       │       pxor     xmm0,xmm0
       │       comiss   xmm0,xmm1
       │     ↓ ja       39f
  2.04 │367:   mov      rax,QWORD PTR [rbp-0x20]
  2.23 │       mov      rdx,QWORD PTR [rbp-0x68]
  2.14 │       shl      rdx,0x2
  2.60 │       add      rax,rdx
  2.26 │       movss    xmm0,DWORD PTR [rax]
  6.11 │       comiss   xmm0,DWORD PTR [rip+0xaae]
  2.22 │     ↓ jbe      3d0
  0.00 │       mov      rax,QWORD PTR [rbp-0x18]
  0.00 │       mov      rdx,QWORD PTR [rbp-0x68]
  0.00 │       shl      rdx,0x2
  0.00 │       add      rax,rdx
  0.00 │       movss    xmm0,DWORD PTR [rax]
       │       pxor     xmm1,xmm1
       │       comiss   xmm0,xmm1
       │     ↓ jbe      3d0
       │39f:   mov      rax,QWORD PTR [rbp-0x18]
       │       mov      rdx,QWORD PTR [rbp-0x68]
       │       shl      rdx,0x2
       │       add      rax,rdx
       │       movss    xmm0,DWORD PTR [rax]
       │       mov      rax,QWORD PTR [rbp-0x18]
       │       mov      rdx,QWORD PTR [rbp-0x68]
       │       shl      rdx,0x2
       │       add      rax,rdx
       │       movss    xmm1,DWORD PTR [rip+0xa6e]
       │       xorps    xmm0,xmm1
       │       movss    DWORD PTR [rax],xmm0
  6.14 │3d0:   add      QWORD PTR [rbp-0x68],0x1
  2.98 │3d5:   mov      rax,QWORD PTR [rbp-0x68]
  6.01 │       cmp      rax,QWORD PTR [rbp-0x40]
  0.00 │     ↑ jb       2e6
       │       add      QWORD PTR [rbp-0x70],0x1
       │3e8:   mov      rax,QWORD PTR [rbp-0x70]
       │       cmp      rax,QWORD PTR [rbp-0x80]
       │     ↑ jb       2d9
<snip>
```

### g++ -Og
```
$ perf record -D 100 ./out/gcc/debug/default/Oversized
$ perf report -Mintel
<snip>
  5.47 │1b6:   add      rdx,0x1
  5.47 │1ba:   cmp      rdx,r12
       │     ↓ jae      204
       │     for (size_t point = 0; point < numPointsOversized; point++)
  5.55 │       lea      rax,[rdx*4+0x0]
 16.49 │       lea      rcx,[rbp+rax*1+0x0]
  5.72 │       add      rax,rbx
  5.79 │       movss    xmm0,DWORD PTR [rip+0xbe8]
  5.57 │       mulss    xmm0,DWORD PTR [rax]
  5.55 │       addss    xmm0,DWORD PTR [rcx]
  5.51 │       movss    DWORD PTR [rcx],xmm0
       │     points.position[point] += points.speed[point] * DELTA_TIME;
  5.78 │       pxor     xmm1,xmm1
 11.08 │       comiss   xmm1,xmm0
  5.41 │     ↑ ja       1a2
 11.05 │1ec:   comiss   xmm0,DWORD PTR [rip+0xbd0]        # 200c <_IO_stdin_used+0xc>
  5.49 │     ↑ jbe      1b6
       │
  0.01 │       movss    xmm1,DWORD PTR [rax]
  0.02 │       pxor     xmm0,xmm0
  0.02 │       comiss   xmm1,xmm0
  0.02 │     ↑ ja       1a7
       │     ↑ jmp      1b6
<snip>
```

### g++ -O3
```
$ perf record -D 100 ./out/gcc/O3/default/Oversized
$ perf report -Mintel
<snip>
 16.34 │178:   comiss   xmm0,DWORD PTR [rip+0xd3d]        # 200c <_IO_stdin_used+0xc>
  7.40 │     ↓ jbe      18f
  0.02 │       comiss   xmm1,xmm2
  0.02 │     ↓ jbe      18f
       │186:   xorps    xmm1,xmm4
       │       movss    DWORD PTR [rbp+rax*4+0x0],xmm1
  7.07 │18f:   add      rax,0x1
  7.41 │       cmp      rax,r14
       │     ↓ je       1c2
  7.08 │198:   movss    xmm1,DWORD PTR [rbp+rax*4+0x0]
  7.45 │       movaps   xmm0,xmm1
  7.54 │       mulss    xmm0,xmm3
  7.09 │       addss    xmm0,DWORD PTR [rbx+rax*4]
 14.93 │       comiss   xmm2,xmm0
 10.15 │       movss    DWORD PTR [rbx+rax*4],xmm0
  7.50 │     ↑ jbe      178
<snip>
```

### g++ -O3 -march=native
Doesn't seem to take advantage of the fact that the entire calculation can be done with 512-bit registers. While I haven't shown it for other solutions, we can see instructions for "tail" calculations.

```
$ perf record -D 100 ./out/gcc/O3/native/Oversized
$ perf report -Mintel
<snip>
  4.87 │1f0:   inc          rcx
  6.05 │       add          rdx,0x40
  6.09 │       cmp          rsi,rcx
       │     ↓ je           270
  6.18 │1fc:   vmovaps      zmm0,ZMMWORD PTR [r14+rdx*1]
  4.82 │       vcmpgtps     k1,zmm0,zmm2
  6.75 │       vmovaps      zmm1,zmm0
  6.06 │       vfmadd213ps  zmm1,zmm4,ZMMWORD PTR [r13+rdx*1+0x0]
 13.51 │       vmovaps      ZMMWORD PTR [r13+rdx*1+0x0],zmm1
  6.47 │       vcmpgtps     k0{k1},zmm1,zmm3
  6.96 │       vcmpltps     k1,zmm0,zmm2
  6.22 │       kxorw        k1,k1,k0
  5.92 │       kmovw        ebx,k0
  5.23 │       vcmpltps     k4{k1},zmm1,zmm2
  6.21 │       kmovw        eax,k4
  8.07 │       cmp          ax,bx
  0.38 │     ↑ je           1f0
<snip>
       │       vfmadd213ps  ymm1,ymm0,YMMWORD PTR [rdx]
<snip>
       │       vfmadd213ss  xmm1,xmm5,DWORD PTR [rcx]
<snip>
```

### clang++ -O3 -march=native
Doesn't seem to take advantage of the fact that the entire calculation can be done with 512-bit registers. While I haven't shown it for other solutions, we can see instructions for "tail" calculations. Oddly there were better results in the larger `kinematics` project, but not here.

```
$ perf record -D 100 ./out/clang/O3/native/Oversized
$ perf report -Mintel
<snip>
  0.45 │820:   vmovaps      zmm12,ZMMWORD PTR [rsi+r8*4-0x40]
  0.37 │       vmovaps      zmm14,ZMMWORD PTR [rbx+r8*4]
  0.26 │       vmovaps      zmm13,ZMMWORD PTR [rsi+r8*4]
  0.30 │       vmovaps      zmm15,ZMMWORD PTR [rbx+r8*4+0x40]
  0.48 │       vfmadd231ps  zmm14,zmm12,zmm7
  0.33 │       vfmadd231ps  zmm15,zmm13,zmm7
  0.22 │       vxorps       zmm16,zmm12,zmm10
  0.33 │       vxorps       zmm17,zmm13,zmm10
  0.71 │       vcmpltps     k1,zmm14,zmm9
  0.59 │       vcmpltps     k2,zmm15,zmm9
  1.17 │       vmovaps      ZMMWORD PTR [rbx+r8*4],zmm14
  1.15 │       vmovaps      ZMMWORD PTR [rbx+r8*4+0x40],zmm15
  1.00 │       vcmpltps     k0{k1},zmm12,zmm16
  1.41 │       vcmpltps     k1{k2},zmm13,zmm17
  1.15 │       knotw        k2,k0
  1.00 │       knotw        k3,k1
  1.24 │       vcmpltps     k2{k2},zmm11,zmm14
  1.23 │       vcmpltps     k3{k3},zmm11,zmm15
  1.48 │       vcmpltps     k2{k2},zmm16,zmm12
  1.37 │       vcmpltps     k3{k3},zmm17,zmm13
  1.93 │       korw         k2,k2,k0
  1.70 │       korw         k1,k3,k1
  3.91 │       vmovups      ZMMWORD PTR [rsi+r8*4-0x40]{k2},zmm16
  0.52 │       vmovups      ZMMWORD PTR [rsi+r8*4]{k1},zmm17
  0.51 │       add          r8,0x20
  0.33 │       cmp          rdx,r8
       │     ↑ jne          820
<snip>
       │       vfmadd231ps  ymm13,ymm12,ymm4
<snip>
       │       vfmadd231ss  xmm13,xmm12,xmm0
<snip>
```
