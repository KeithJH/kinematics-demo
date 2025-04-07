# Aligned
SoA layout that uses `float*` fields manually managed with `new[]` and `delete[]` while specifying alignment, in the hopes that instructions that rely on alignment are used and improve performance.

In order to remain fairly generic a constant 64-byte alignment is used, which is the largest requirement on systems tested (for 512-byte `zmm` registers). This could technically be decreased for some systems, and may need to be increased for others.

```
struct Points
{
	float *position;
	float *speed;
};

Points points;
points.position = new (std::align_val_t(64)) float[numPoints];
points.speed    = new (std::align_val_t(64)) float[numPoints];
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
| gcc      | O0           | default      |     19219 |
| gcc      | debug        | default      |      5310 |
| gcc      | O1           | default      |      4294 |
| gcc      | O2           | default      |      4261 |
| gcc      | O3           | default      |      3888 |
| gcc      | O3           | native       |       623 |
| clang    | debug        | default      |      4236 |
| clang    | O3           | default      |      2634 |
| clang    | O3           | native       |       760 |

The test system (Zen 4) may not be sensitive to alignment as the results appear just about the same as in `StructOfPointer`

## Assembly Analysis
### g++ -O0
Identical to `StructOfPointer`

```
$ diff -s ./out/gcc/O0/default/{StructOfPointer,Aligned}
Files ./out/gcc/O0/default/StructOfPointer and ./out/gcc/O0/default/Aligned are identical
```

### g++ -Og
Identical to `StructOfPointer` (using scalar "vector" instructions like `mulss`)

```
$ diff -s ./out/gcc/debug/default/{StructOfPointer,Aligned}
Files ./out/gcc/debug/default/StructOfPointer and ./out/gcc/debug/default/Aligned are identical
```

### g++ -O3
Identical to `StructOfPointer` (using scalar "vector" instructions like `mulss`)

```
$ diff -s ./out/gcc/O3/default/{StructOfPointer,Aligned}
Files ./out/gcc/O3/default/StructOfPointer and ./out/gcc/O3/default/Aligned are identical
```

### g++ -O3 -march=native
Oddly it doesn't seem that aligned instructions are generated

```
$ make OUT=native CXX=g++ CXX_FLAGS="-O3 -march=native -S -masm=intel"
$ diff -y --suppress-common-lines native/{StructOfPointer,Aligned}
	.file	"StructOfPointer.cpp"			      |		.file	"Aligned.cpp"
.LFB371:						      |	.LFB385:
							      >		mov	esi, 64
	call	_Znam@PLT				      |		call	_ZnamSt11align_val_t@PLT
							      >		mov	esi, 64
	call	_Znam@PLT				      |		call	_ZnamSt11align_val_t@PLT
							      >		mov	esi, 64
	call	_Znam@PLT				      |		call	_ZnamSt11align_val_t@PLT
							      >		mov	esi, 64
	call	_Znam@PLT				      |		call	_ZnamSt11align_val_t@PLT
	call	_ZdaPv@PLT				      |		mov	esi, 64
							      >		call	_ZdaPvSt11align_val_t@PLT
	call	_ZdaPv@PLT				      |		mov	esi, 64
							      >		call	_ZdaPvSt11align_val_t@PLT
.LFSB371:						      |	.LFSB385:
.LFE371:						      |	.LFE385:
```

Using [Compiler Explorer](https://godbolt.org/), it does appear that "x86-64 gcc (trunk)" does generate the instructions (checked April 5th, 2025).

It's also possible to use `std::assume_aligned<>()` (as of C++ 20) to help nudge things along even with version 13.3.

```
auto position = std::assume_aligned<64>(points.position);
auto speed = std::assume_aligned<64>(points.speed);
for (size_t point = 0; point < numPoints; point++)
{
	position[point] += points.speed[point] * DELTA_TIME;

	if ((position[point] < 0 && speed[point] < 0) ||
	    (position[point] > POSITION_LIMIT && speed[point] > 0))
	{
		speed[point] *= -1;
	}
}
```

### clang++ -O3 -march=native
While it doesn't seem to change the results much on the test system (Zen 4), we at least do see a bunch of `vmovaps` (aligned) instead of `vmovups` (unaligned) when loading values from memory

```
$ make OUT=native CXX=clang++ CXX_FLAGS="-O3 -march=native -S -masm=intel"
$ diff -y --suppress-common-lines native/{StructOfPointer,Aligned}
	.file	"StructOfPointer.cpp"			      |		.file	"Aligned.cpp"
							      >		mov	esi, 64
	call	_Znam@PLT				      |		call	_ZnamSt11align_val_t@PLT
							      >		mov	esi, 64
	call	_Znam@PLT				      |		call	_ZnamSt11align_val_t@PLT
	vmovups	zmm12, zmmword ptr [rsi + 4*r8 - 64]	      |		vmovaps	zmm12, zmmword ptr [rsi + 4*r8 - 64]
	vmovups	zmm14, zmmword ptr [rbx + 4*r8]		      |		vmovaps	zmm14, zmmword ptr [rbx + 4*r8]
	vmovups	zmm13, zmmword ptr [rsi + 4*r8]		      |		vmovaps	zmm13, zmmword ptr [rsi + 4*r8]
	vmovups	zmm15, zmmword ptr [rbx + 4*r8 + 64]	      |		vmovaps	zmm15, zmmword ptr [rbx + 4*r8 + 64]
	vmovups	zmmword ptr [rbx + 4*r8], zmm14		      |		vmovaps	zmmword ptr [rbx + 4*r8], zmm14
	vmovups	zmmword ptr [rbx + 4*r8 + 64], zmm15	      |		vmovaps	zmmword ptr [rbx + 4*r8 + 64], zmm15
	vmovups	ymm12, ymmword ptr [r14 + 4*r9]		      |		vmovaps	ymm12, ymmword ptr [r14 + 4*r9]
	vmovups	ymm13, ymmword ptr [rbx + 4*r9]		      |		vmovaps	ymm13, ymmword ptr [rbx + 4*r9]
	vmovups	ymmword ptr [rbx + 4*r9], ymm13		      |		vmovaps	ymmword ptr [rbx + 4*r9], ymm13
							      >		mov	esi, 64
	call	_ZdaPv@PLT				      |		call	_ZdaPvSt11align_val_t@PLT
							      >		mov	esi, 64
	call	_ZdaPv@PLT				      |		call	_ZdaPvSt11align_val_t@PLT
	vmovups	zmm12, zmmword ptr [rsi + 4*r8 - 64]	      |		vmovaps	zmm12, zmmword ptr [rsi + 4*r8 - 64]
	vmovups	zmm14, zmmword ptr [rbx + 4*r8]		      |		vmovaps	zmm14, zmmword ptr [rbx + 4*r8]
	vmovups	zmm13, zmmword ptr [rsi + 4*r8]		      |		vmovaps	zmm13, zmmword ptr [rsi + 4*r8]
	vmovups	zmm15, zmmword ptr [rbx + 4*r8 + 64]	      |		vmovaps	zmm15, zmmword ptr [rbx + 4*r8 + 64]
	vmovups	zmmword ptr [rbx + 4*r8], zmm14		      |		vmovaps	zmmword ptr [rbx + 4*r8], zmm14
	vmovups	zmmword ptr [rbx + 4*r8 + 64], zmm15	      |		vmovaps	zmmword ptr [rbx + 4*r8 + 64], zmm15
	vmovups	ymm12, ymmword ptr [r14 + 4*r9]		      |		vmovaps	ymm12, ymmword ptr [r14 + 4*r9]
	vmovups	ymm13, ymmword ptr [rbx + 4*r9]		      |		vmovaps	ymm13, ymmword ptr [rbx + 4*r9]
	vmovups	ymmword ptr [rbx + 4*r9], ymm13		      |		vmovaps	ymmword ptr [rbx + 4*r9], ymm13
	vmovups	zmm12, zmmword ptr [rsi + 4*r8 - 64]	      |		vmovaps	zmm12, zmmword ptr [rsi + 4*r8 - 64]
	vmovups	zmm14, zmmword ptr [rbx + 4*r8]		      |		vmovaps	zmm14, zmmword ptr [rbx + 4*r8]
	vmovups	zmm13, zmmword ptr [rsi + 4*r8]		      |		vmovaps	zmm13, zmmword ptr [rsi + 4*r8]
	vmovups	zmm15, zmmword ptr [rbx + 4*r8 + 64]	      |		vmovaps	zmm15, zmmword ptr [rbx + 4*r8 + 64]
	vmovups	zmmword ptr [rbx + 4*r8], zmm14		      |		vmovaps	zmmword ptr [rbx + 4*r8], zmm14
	vmovups	zmmword ptr [rbx + 4*r8 + 64], zmm15	      |		vmovaps	zmmword ptr [rbx + 4*r8 + 64], zmm15
	vmovups	ymm12, ymmword ptr [r14 + 4*r9]		      |		vmovaps	ymm12, ymmword ptr [r14 + 4*r9]
	vmovups	ymm13, ymmword ptr [rbx + 4*r9]		      |		vmovaps	ymm13, ymmword ptr [rbx + 4*r9]
	vmovups	ymmword ptr [rbx + 4*r9], ymm13		      |		vmovaps	ymmword ptr [rbx + 4*r9], ymm13
	vmovups	zmm12, zmmword ptr [rsi + 4*r8 - 64]	      |		vmovaps	zmm12, zmmword ptr [rsi + 4*r8 - 64]
	vmovups	zmm14, zmmword ptr [rbx + 4*r8]		      |		vmovaps	zmm14, zmmword ptr [rbx + 4*r8]
	vmovups	zmm13, zmmword ptr [rsi + 4*r8]		      |		vmovaps	zmm13, zmmword ptr [rsi + 4*r8]
	vmovups	zmm15, zmmword ptr [rbx + 4*r8 + 64]	      |		vmovaps	zmm15, zmmword ptr [rbx + 4*r8 + 64]
	vmovups	zmmword ptr [rbx + 4*r8], zmm14		      |		vmovaps	zmmword ptr [rbx + 4*r8], zmm14
	vmovups	zmmword ptr [rbx + 4*r8 + 64], zmm15	      |		vmovaps	zmmword ptr [rbx + 4*r8 + 64], zmm15
	vmovups	ymm12, ymmword ptr [r14 + 4*r9]		      |		vmovaps	ymm12, ymmword ptr [r14 + 4*r9]
	vmovups	ymm13, ymmword ptr [rbx + 4*r9]		      |		vmovaps	ymm13, ymmword ptr [rbx + 4*r9]
	vmovups	ymmword ptr [rbx + 4*r9], ymm13		      |		vmovaps	ymmword ptr [rbx + 4*r9], ymm13
	vmovups	zmm12, zmmword ptr [rsi + 4*r8 - 64]	      |		vmovaps	zmm12, zmmword ptr [rsi + 4*r8 - 64]
	vmovups	zmm14, zmmword ptr [rbx + 4*r8]		      |		vmovaps	zmm14, zmmword ptr [rbx + 4*r8]
	vmovups	zmm13, zmmword ptr [rsi + 4*r8]		      |		vmovaps	zmm13, zmmword ptr [rsi + 4*r8]
	vmovups	zmm15, zmmword ptr [rbx + 4*r8 + 64]	      |		vmovaps	zmm15, zmmword ptr [rbx + 4*r8 + 64]
	vmovups	zmmword ptr [rbx + 4*r8], zmm14		      |		vmovaps	zmmword ptr [rbx + 4*r8], zmm14
	vmovups	zmmword ptr [rbx + 4*r8 + 64], zmm15	      |		vmovaps	zmmword ptr [rbx + 4*r8 + 64], zmm15
	vmovups	ymm12, ymmword ptr [r14 + 4*r9]		      |		vmovaps	ymm12, ymmword ptr [r14 + 4*r9]
	vmovups	ymm13, ymmword ptr [rbx + 4*r9]		      |		vmovaps	ymm13, ymmword ptr [rbx + 4*r9]
	vmovups	ymmword ptr [rbx + 4*r9], ymm13		      |		vmovaps	ymmword ptr [rbx + 4*r9], ymm13
```

## Additional Data

### clang++ -O3 -march=native
```
$ perf stat -D100 -ddd ./out/clang/O3/native/Aligned
            646.66 msec task-clock                       #    1.000 CPUs utilized
                32      context-switches                 #   49.485 /sec
                 0      cpu-migrations                   #    0.000 /sec
                 7      page-faults                      #   10.825 /sec
     3,566,046,013      cycles                           #    5.515 GHz                         (35.66%)
        26,258,745      stalled-cycles-frontend          #    0.74% frontend cycles idle        (35.80%)
     7,340,608,512      instructions                     #    2.06  insn per cycle
                                                         #    0.00  stalled cycles per insn     (35.96%)
       275,007,222      branches                         #  425.277 M/sec                       (35.99%)
           423,550      branch-misses                    #    0.15% of all branches             (36.01%)
     4,361,020,887      L1-dcache-loads                  #    6.744 G/sec                       (35.90%)
     1,087,697,968      L1-dcache-load-misses            #   24.94% of all L1-dcache accesses   (35.76%)
        14,497,702      L1-icache-loads                  #   22.420 M/sec                       (35.60%)
            36,838      L1-icache-load-misses            #    0.25% of all L1-icache accesses   (35.56%)
        17,115,095      dTLB-loads                       #   26.467 M/sec                       (35.56%)
            62,291      dTLB-load-misses                 #    0.36% of all dTLB cache accesses  (35.56%)
                 0      iTLB-loads                       #    0.000 /sec                        (35.55%)
                47      iTLB-load-misses                                                        (35.55%)
     1,087,672,970      L1-dcache-prefetches             #    1.682 G/sec                       (35.55%)


$ perf stat -D100 -MPipelineL2 ./out/clang/O3/native/Aligned
           255,351      ex_ret_brn_misp                  #      0.0 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (24.98%)
     8,301,606,833      de_src_op_disp.all                                                      (24.98%)
           165,351      resyncs_or_nc_redirects                                                 (24.98%)
     3,767,739,089      ls_not_halted_cyc                                                       (24.98%)
     8,285,798,867      ex_ret_ops                                                              (24.98%)
        23,571,734      ex_no_retire.load_not_complete   #     60.2 %  backend_bound_cpu
                                                         #      0.8 %  backend_bound_memory     (25.10%)
    13,792,791,963      de_no_dispatch_per_slot.backend_stalls                                  (25.10%)
     1,725,795,995      ex_no_retire.not_complete                                               (25.10%)
     3,766,984,056      ls_not_halted_cyc                                                       (25.10%)
         2,071,458      ex_ret_ucode_ops                 #     36.6 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (24.99%)
     3,768,854,081      ls_not_halted_cyc                                                       (24.99%)
     8,286,607,899      ex_ret_ops                                                              (24.99%)
       365,878,154      de_no_dispatch_per_slot.no_ops_from_frontend                #      0.7 %  frontend_bound_bandwidth  (24.93%)
        33,876,893      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      0.9 %  frontend_bound_latency   (24.93%)
     3,769,198,058      ls_not_halted_cyc                                                       (24.93%)
```
