# Hybrid
Hybrid approach that uses an array of structures of arrays (AoSoA), still manually managed with `new[]` and `delete[]` while specifying alignment. This should make it easier for optimizers to see that no tail calculations are needed.

```
constexpr size_t BLOCK_SIZE = 16;
struct PointBlock
{
	float position[BLOCK_SIZE];
	float speed[BLOCK_SIZE];
};

PointBlock *points;

const size_t numPointBlocks = (numPoints + BLOCK_SIZE - 1) / BLOCK_SIZE;
points = new (ALIGNMENT) PointBlock[numPointBlocks];
```

```
for (size_t block = 0; block < numPointBlocks; block++)
{
	for (size_t point = 0; point < BLOCK_SIZE; point++)
	{
		points[block].position[point] += points[block].speed[point] * DELTA_TIME;

		if ((points[block].position[point] < 0 && points[block].speed[point] < 0) ||
		    (points[block].position[point] > POSITION_LIMIT && points[block].speed[point] > 0))
		{
			points[block].speed[point] *= -1;
		}
	}
}
```

## Benchmark Comparison
| Compiler | Optimization | Architecture | Time (ms) |
|----------|--------------|--------------|-----------|
| gcc      | O0           | default      |     20103 |
| gcc      | debug        | default      |      5560 |
| gcc      | O1           | default      |      4656 |
| gcc      | O2           | default      |      5786 |
| gcc      | O3           | default      |      5800 |
| gcc      | O2           | native       |       696 |
| gcc      | O3           | native       |       706 |
| clang    | debug        | default      |      5670 |
| clang    | O3           | default      |      2331 |
| clang    | O3           | native       |       890 |

Focusing on compiling with `g++` (version 13.3.0) we see improvement with optimization level, roughly on level with solutions like `StructOfPointer`. One notable observation is that even `O2` sees good performance when compiled with native architecture.

Looking at `clang++` (version 18.1.3), we also see the same general progression of improvement with optimization level!

## Assembly Analysis
### g++ -Og
Here we have debug symbols, which helps map which instructions are doing which calculations. Notably while vector instructions are being used (like `mulss`), they are the scalar versions for single precision floats (as denoted by the suffix `ss`).

```
$ perf record -D 100 ./out/gcc/debug/default/Hybrid
$ perf report -Mintel
<snip>
       │     for (size_t point = 0; point < BLOCK_SIZE; point++)
  5.60 │19b:   add      rax,0x1
  5.81 │19f:   cmp      rax,0xf
       │     ↓ ja       1e7
       │     points[block].position[point] += points[block].speed[point] * DELTA_TIME;
  5.26 │       mov      rdx,rcx
  5.48 │       shl      rdx,0x7
  5.29 │       add      rdx,rbx
  5.34 │       movss    xmm1,DWORD PTR [rdx+rax*4+0x40]
  5.59 │       movaps   xmm0,xmm1
  5.46 │       mulss    xmm0,DWORD PTR [rip+0xbff]
  5.68 │       addss    xmm0,DWORD PTR [rdx+rax*4]
 10.62 │       movss    DWORD PTR [rdx+rax*4],xmm0
       │     if ((points[block].position[point] < 0 && points[block].speed[point] < 0) ||
  5.32 │       pxor     xmm2,xmm2
 10.67 │       comiss   xmm2,xmm0
  5.39 │     ↑ ja       189
 11.25 │1d3:   comiss   xmm0,DWORD PTR [rip+0xbe9]        # 200c <_IO_stdin_used+0xc>
  5.61 │     ↑ jbe      19b
       │     (points[block].position[point] > POSITION_LIMIT && points[block].speed[point] > 0))
  0.00 │       pxor     xmm0,xmm0
  0.04 │       comiss   xmm1,xmm0
       │     ↑ ja       18e
       │     ↑ jmp      19b
       │     for (size_t block = 0; block < numPointBlocks; block++)
  0.33 │1e7:   add      rcx,0x1
  0.74 │1eb:   cmp      rcx,rbp
       │     ↓ jae      1f7
       │     for (size_t point = 0; point < BLOCK_SIZE; point++)
  0.28 │       mov      eax,0x0
  0.24 │     ↑ jmp      19f
<snip>
```

### g++ -O3
Interestingly, similar to `StructOfVector`, we are not seeing performant vectorization here as scalar versions of instructions are still being used.

```
$ perf record -D 100 ./out/gcc/O3/default/Hybrid
$ perf report -Mintel
<snip>
       │     → call     std::chrono::_V2::steady_clock::now()@plt
       │       mov      r12,rax
       │       test     rbp,rbp
       │     ↓ je       1e8
       │       test     r13,r13
       │     ↓ je       1e8
       │       shl      r13,0x7
       │       lea      rdi,[rbx+0x40]
       │       pxor     xmm2,xmm2
       │       xor      esi,esi
       │       movss    xmm3,DWORD PTR [rip+0xd4d]
       │       lea      rcx,[rbx+r13*1+0x40]
       │       movss    xmm4,DWORD PTR [rip+0xd98]
       │178:   mov      rdx,rdi
       │       nop
  0.96 │180:   lea      rax,[rdx-0x40]
  1.07 │     ↓ jmp      1af
       │       cs       nop WORD PTR [rax+rax*1+0x0]
 20.21 │190:   comiss   xmm0,DWORD PTR [rip+0xd25]        # 200c <_IO_stdin_used+0xc>
 10.09 │     ↓ jbe      1a6
  0.03 │       comiss   xmm1,xmm2
  0.00 │     ↓ jbe      1a6
       │19e:   xorps    xmm1,xmm4
       │       movss    DWORD PTR [rax+0x40],xmm1
  6.39 │1a6:   add      rax,0x4
  6.43 │       cmp      rdx,rax
       │     ↓ je       1d6
  5.20 │1af:   movss    xmm1,DWORD PTR [rax+0x40]
  5.21 │       movaps   xmm0,xmm1
  5.36 │       mulss    xmm0,xmm3
  5.61 │       addss    xmm0,DWORD PTR [rax]
 15.13 │       comiss   xmm2,xmm0
 10.42 │       movss    DWORD PTR [rax],xmm0
  5.63 │     ↑ jbe      190
       │       comiss   xmm2,xmm1
       │     ↑ ja       19e
       │       add      rax,0x4
       │       cmp      rdx,rax
       │     ↑ jne      1af
  1.09 │1d6:   sub      rdx,0xffffffffffffff80
  1.16 │       cmp      rdx,rcx
       │     ↑ jne      180
       │       add      rsi,0x1
       │       cmp      rsi,rbp
       │     ↑ jne      178
       │1e8: → call     std::chrono::_V2::steady_clock::now()@plt
<snip>
```

### g++ -O3 -march=native
Once we start compiling for the native architecture (on a Zen 4 test system) we even start to see "true" vectorization, including FMA commands. Notably there is no shuffling of data required, as there was with the AoS solutions. There are also no "tail" calculations, resulting in a smaller section of the binary!

```
$ perf record -D 100 ./out/gcc/O3/native/Hybrid
$ perf report -Mintel
<snip>
       │     → call         std::chrono::_V2::steady_clock::now()@plt
       │       mov          r13,rax
       │       test         r12,r12
       │     ↓ je           210
       │       vbroadcastss zmm4,DWORD PTR [rip+0xd5b]        # 2008 <_IO_stdin_used+0x8>
       │       vbroadcastss zmm3,DWORD PTR [rip+0xd55]        # 200c <_IO_stdin_used+0xc>
       │       vbroadcastss zmm5,DWORD PTR [rip+0xd4f]        # 2010 <_IO_stdin_used+0x10>
       │       xor          esi,esi
       │       vxorps       xmm2,xmm2,xmm2
       │       test         r14,r14
       │     ↓ je           20d
       │180:   mov          rdx,rbx
       │       xor          ecx,ecx
       │     ↓ jmp          19c
       │       nop
  6.39 │190:   inc          rcx
  6.65 │       sub          rdx,0xffffffffffffff80
  6.67 │       cmp          r14,rcx
       │     ↓ je           201
  6.96 │19c:   vmovaps      zmm0,ZMMWORD PTR [rdx+0x40]
  6.84 │       vcmpgtps     k1,zmm0,zmm2
  8.33 │       vmovaps      zmm1,zmm0
  6.56 │       vfmadd213ps  zmm1,zmm4,ZMMWORD PTR [rdx]
 11.24 │       vmovaps      ZMMWORD PTR [rdx],zmm1
  5.57 │       vcmpgtps     k0{k1},zmm1,zmm3
  5.58 │       vcmpltps     k1,zmm0,zmm2
  5.51 │       kxorw        k1,k1,k0
  4.74 │       kmovw        edi,k0
  5.50 │       vcmpltps     k3{k1},zmm1,zmm2
  5.69 │       kmovw        eax,k3
  6.89 │       cmp          di,ax
  0.41 │     ↑ je           190
  0.19 │       xor          edi,eax
  0.15 │       vxorps       zmm0,zmm0,zmm5
  0.04 │       inc          rcx
  0.04 │       kmovw        k2,edi
  0.04 │       vmovaps      ZMMWORD PTR [rdx+0x40]{k2},zmm0
       │       sub          rdx,0xffffffffffffff80
       │       cmp          r14,rcx
       │     ↑ jne          19c
       │201:   inc          rsi
       │       cmp          rsi,r12
       │     ↑ jne          180
       │20d:   vzeroupper
       │210: → call         std::chrono::_V2::steady_clock::now()@plt
<snip>
```

### clang++ -O3 -march=native
There don't appear to be any tail calculations, but plenty of what looks like loop unrolling.

```
$ perf record -D 100 ./out/clang/O3/native/Hybrid
$ perf report -Mintel
<snip>
  1.30 │       vmovups      ZMMWORD PTR [r13-0x380]{k1},zmm6
  0.74 │       vmovaps      zmm4,ZMMWORD PTR [r13-0x300]
  0.63 │       vmovaps      zmm5,ZMMWORD PTR [r13-0x340]
  0.84 │       vfmadd231ps  zmm5,zmm4,zmm0
  0.12 │       vxorps       zmm6,zmm4,zmm2
  0.40 │       vcmpltps     k1,zmm5,zmm1
  0.66 │       vmovaps      ZMMWORD PTR [r13-0x340],zmm5
  0.31 │       vcmpltps     k0{k1},zmm4,zmm6
  0.22 │       knotw        k1,k0
  0.15 │       vcmpltps     k1{k1},zmm3,zmm5
  0.35 │       vcmpltps     k1{k1},zmm6,zmm4
  0.31 │       korw         k1,k1,k0
  0.90 │       vmovups      ZMMWORD PTR [r13-0x300]{k1},zmm6
  0.41 │       vmovaps      zmm4,ZMMWORD PTR [r13-0x280]
  0.62 │       vmovaps      zmm5,ZMMWORD PTR [r13-0x2c0]
  0.27 │       vfmadd231ps  zmm5,zmm4,zmm0
<snip>
```

## Additional Data

### g++ -O3 -march=native
```
$ perf stat -D100 -ddd ./out/gcc/O3/native/Hybrid
            606.61 msec task-clock                       #    1.000 CPUs utilized
                15      context-switches                 #   24.727 /sec
                 0      cpu-migrations                   #    0.000 /sec
                 7      page-faults                      #   11.539 /sec
     3,345,803,786      cycles                           #    5.516 GHz                         (35.54%)
        24,255,834      stalled-cycles-frontend          #    0.72% frontend cycles idle        (35.70%)
     9,244,807,246      instructions                     #    2.76  insn per cycle
                                                         #    0.00  stalled cycles per insn     (35.87%)
     1,089,085,385      branches                         #    1.795 G/sec                       (36.04%)
           846,490      branch-misses                    #    0.08% of all branches             (36.22%)
     3,283,200,091      L1-dcache-loads                  #    5.412 G/sec                       (36.14%)
     1,086,391,884      L1-dcache-load-misses            #   33.09% of all L1-dcache accesses   (35.96%)
        13,443,599      L1-icache-loads                  #   22.162 M/sec                       (35.81%)
             7,477      L1-icache-load-misses            #    0.06% of all L1-icache accesses   (35.63%)
        17,107,725      dTLB-loads                       #   28.202 M/sec                       (35.45%)
            70,779      dTLB-load-misses                 #    0.41% of all dTLB cache accesses  (35.42%)
               104      iTLB-loads                       #  171.443 /sec                        (35.42%)
                33      iTLB-load-misses                 #   31.73% of all iTLB cache accesses  (35.41%)
     1,052,727,464      L1-dcache-prefetches             #    1.735 G/sec                       (35.42%)

$ perf stat -D100 -MPipelineL2 ./out/gcc/O3/native/Hybrid
         1,045,820      ex_ret_brn_misp                  #      0.3 %  bad_speculation_mispredicts
                                                         #      0.1 %  bad_speculation_pipeline_restarts  (25.09%)
     8,827,290,663      de_src_op_disp.all                                                      (25.09%)
           220,427      resyncs_or_nc_redirects                                                 (25.09%)
     3,516,307,536      ls_not_halted_cyc                                                       (25.09%)
     8,747,401,040      ex_ret_ops                                                              (25.09%)
        26,692,265      ex_no_retire.load_not_complete   #     52.2 %  backend_bound_cpu
                                                         #      0.7 %  backend_bound_memory     (25.02%)
    11,163,196,528      de_no_dispatch_per_slot.backend_stalls                                  (25.02%)
     1,915,516,125      ex_no_retire.not_complete                                               (25.02%)
     3,515,685,647      ls_not_halted_cyc                                                       (25.02%)
         2,434,519      ex_ret_ucode_ops                 #     41.4 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (24.97%)
     3,517,013,890      ls_not_halted_cyc                                                       (24.97%)
     8,742,310,491      ex_ret_ops                                                              (24.97%)
       710,470,075      de_no_dispatch_per_slot.no_ops_from_frontend                #      1.6 %  frontend_bound_bandwidth  (24.92%)
        61,746,475      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      1.8 %  frontend_bound_latency   (24.92%)
     3,517,777,171      ls_not_halted_cyc                                                       (24.92%)
```

### clang++ -O3 -march=native
```
$ perf stat -D100 -ddd ./out/clang/O3/native/Hybrid
            812.16 msec task-clock                       #    1.000 CPUs utilized
                 3      context-switches                 #    3.694 /sec
                 2      cpu-migrations                   #    2.463 /sec
                 7      page-faults                      #    8.619 /sec
     4,479,259,883      cycles                           #    5.515 GHz                         (35.74%)
        67,259,687      stalled-cycles-frontend          #    1.50% frontend cycles idle        (35.73%)
     7,035,608,603      instructions                     #    1.57  insn per cycle
                                                         #    0.01  stalled cycles per insn     (35.72%)
        74,332,733      branches                         #   91.524 M/sec                       (35.73%)
           376,437      branch-misses                    #    0.51% of all branches             (35.73%)
     4,538,068,706      L1-dcache-loads                  #    5.588 G/sec                       (35.70%)
     1,128,724,862      L1-dcache-load-misses            #   24.87% of all L1-dcache accesses   (35.71%)
        28,271,423      L1-icache-loads                  #   34.810 M/sec                       (35.71%)
           201,429      L1-icache-load-misses            #    0.71% of all L1-icache accesses   (35.71%)
        17,727,163      dTLB-loads                       #   21.827 M/sec                       (35.70%)
            27,234      dTLB-load-misses                 #    0.15% of all dTLB cache accesses  (35.71%)
               207      iTLB-loads                       #  254.875 /sec                        (35.71%)
               184      iTLB-load-misses                 #   88.89% of all iTLB cache accesses  (35.71%)
     1,109,155,131      L1-dcache-prefetches             #    1.366 G/sec                       (35.70%)

$ perf stat -D100 -MPipelineL2 ./out/clang/O3/native/Hybrid
           283,521      ex_ret_brn_misp                  #      0.1 %  bad_speculation_mispredicts
                                                         #      0.2 %  bad_speculation_pipeline_restarts  (24.96%)
     8,142,432,198      de_src_op_disp.all                                                      (24.96%)
           745,057      resyncs_or_nc_redirects                                                 (24.96%)
     4,454,473,731      ls_not_halted_cyc                                                       (24.96%)
     8,084,138,928      ex_ret_ops                                                              (24.96%)
        20,811,495      ex_no_retire.load_not_complete   #     66.9 %  backend_bound_cpu
                                                         #      0.6 %  backend_bound_memory     (25.11%)
    18,058,375,933      de_no_dispatch_per_slot.backend_stalls                                  (25.11%)
     2,190,246,909      ex_no_retire.not_complete                                               (25.11%)
     4,453,611,706      ls_not_halted_cyc                                                       (25.11%)
         2,060,429      ex_ret_ucode_ops                 #     30.2 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (24.96%)
     4,455,278,730      ls_not_halted_cyc                                                       (24.96%)
     8,075,857,722      ex_ret_ops                                                              (24.96%)
       494,133,509      de_no_dispatch_per_slot.no_ops_from_frontend                #      0.6 %  frontend_bound_bandwidth  (24.97%)
        54,656,238      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      1.2 %  frontend_bound_latency   (24.97%)
     4,456,080,178      ls_not_halted_cyc                                                       (24.97%)
```
