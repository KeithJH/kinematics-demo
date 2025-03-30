# VectorOfStructSim
Conventional Array of Structures (AoS) layout using a `std::vector`. This means data for various fields is interleaved in memory, which can present a challenge for vectorization but generally makes for good cache usage. Logic can be straightforward, even using for-each loops that map closely to how a person may think of the problem space.

```
struct Body
{
	float x, y;
	float horizontalSpeed, verticalSpeed;
	Color color;
};
std::vector<Body> _bodies;

void Update(const float deltaTime)
{
	for (auto &body : _bodies)
	{
		// Update position based on speed
		body.x += body.horizontalSpeed * deltaTime;
		body.y += body.verticalSpeed * deltaTime;

		// Bounce horizontally
		if ((body.x - BODY_RADIUS < 0 && body.horizontalSpeed < 0) ||
			(body.x + BODY_RADIUS > _width && body.horizontalSpeed > 0))
		{
			body.horizontalSpeed *= -1;
		}

		// Bounce vertically
		if ((body.y - BODY_RADIUS < 0 && body.verticalSpeed < 0) ||
			(body.y + BODY_RADIUS > _height && body.verticalSpeed > 0))
		{
			body.verticalSpeed *= -1;
		}
	}
}
```

## Benchmark Comparison
Compiling the same sources with both `g++` (version 13.3.0) and `clang++` (version 18.1.3) results in fairly different approaches (see next section) but results in comparable benchmarks. On the tested system `g++` consistently performs marginally better across all dataset sizes.

### gcc
```
$ g++ --version | head -1
g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0

$ ./out/build/release/bench/kinematics-demo-bench --benchmark-no-analysis
benchmark name                            samples    iterations          mean
-------------------------------------------------------------------------------
Update VectorOfStructSim: 1000000              100             1     928.59 us
Update VectorOfStructSim: 5000000              100             1    5.03115 ms
```

### clang
```
$ clang++ --version | head -1
Ubuntu clang version 18.1.3 (1ubuntu1)

$ ./out/build/clang/bench/kinematics-demo-bench --benchmark-no-analysis
benchmark name                            samples    iterations          mean
-------------------------------------------------------------------------------
Update VectorOfStructSim: 1000000              100             1    1.29502 ms
Update VectorOfStructSim: 5000000              100             1    6.49957 ms
```

## Assembly Analysis
Both compilers manage to see an opportunity to vectorize the update math even in this form, likely as relevant data is still close enough in memory. This would likely be even more apparent if the `Color` data was not also stored in the same struct, though that then would increase complexity especially when it comes to rendering the scene.

### gcc
Here the compiler is able to use "packed-single" forms of math on `xmm` registers (128-bit), such as the very import `vfmadd231ps` (fused-multiply-add). 

```
$ perf record -D 1000 ./out/build/release/gui/kinematics-demo-gui 5000000
<wait some time, then close>

$ perf annotate -Mintel
kinematics::VectorOfStructSim::Update(float)
<snip>
  3.49 │40:   vaddss      xmm3,xmm3,xmm4
  8.69 │      vcomiss     xmm3,xmm9
  4.65 │    ↓ jbe         5b
  0.11 │      vcomiss     xmm0,xmm5
  0.05 │    ↓ jbe         5b
  0.05 │51:   vxorps      xmm0,xmm0,xmm10
  0.03 │      vmovss      DWORD PTR [rax+0x8],xmm0
  3.86 │5b:   vsubss      xmm0,xmm2,xmm4
  6.95 │      vcomiss     xmm5,xmm0
  4.19 │    ↓ ja          c0
  3.41 │65:   vaddss      xmm2,xmm2,xmm4
  7.43 │      vcomiss     xmm2,xmm8
  3.70 │    ↓ jbe         80
  0.13 │      vcomiss     xmm6,xmm5
  0.05 │    ↓ jbe         80
  0.05 │76:   vxorps      xmm6,xmm6,xmm11
  0.06 │      vmovss      DWORD PTR [rax+0xc],xmm6
  3.73 │80:   add         rax,0x14
  3.87 │      cmp         rdx,rax
       │    ↓ je          d0
  3.73 │89:   vmovq       xmm0,QWORD PTR [rax+0x8]
  4.38 │      vmovq       xmm1,QWORD PTR [rax]
  4.26 │      vfmadd231ps xmm1,xmm0,xmm7
  4.11 │      vmovshdup   xmm6,xmm0
  3.79 │      vmovaps     xmm3,xmm1
  4.14 │      vmovlps     QWORD PTR [rax],xmm1
  4.91 │      vmovshdup   xmm2,xmm1
  3.42 │      vsubss      xmm1,xmm1,xmm4
  7.38 │      vcomiss     xmm5,xmm1
  4.74 │    ↑ jbe         40
<snip>
```

### clang
Here the compiler decides to leverage the full 512-bits of `zmm` registers available on the test system which requires the use of scattering (`vscatterdps`). The main position update is still able to use `vfmadd231ps` for operations on packed-single-precision floats.

```
$ perf record -D 1000 ./out/build/clang/gui/kinematics-demo-gui 5000000
<wait some time, then close>

$ perf annotate -Mintel
kinematics::VectorOfStructSim::Update(float)
<snip>
  0.17 │       vfmadd231ps     zmm27,zmm29,zmm3
  0.28 │       vfmadd231ps     zmm28,zmm30,zmm3
  0.16 │       vxorps          zmm1,zmm29,zmm23
 14.50 │       vscatterdps     DWORD PTR [rbx+zmm18*1]{k3},zmm27
  0.20 │       kxnorw          k3,k0,k0
  0.32 │       vaddps          zmm0,zmm27,zmm20
  0.27 │       vaddps          zmm8,zmm27,zmm21
  0.14 │       vaddps          zmm7,zmm28,zmm21
 20.77 │       vscatterdps     DWORD PTR [rbx+zmm19*1]{k3},zmm28
  0.15 │       vcmpltps        k3,zmm29,zmm1
  0.15 │       vcmpltps        k0{k3},zmm0,zmm22
  0.17 │       vcmpltps        k3,zmm1,zmm29
  0.13 │       vaddps          zmm0,zmm28,zmm20
  0.14 │       vcmpltps        k3{k3},zmm4,zmm8
  0.15 │       vxorps          zmm8,zmm30,zmm23
  0.88 │       korw            k3,k3,k0
 31.19 │       vscatterdps     DWORD PTR [rbx+zmm24*1]{k3},zmm1
  0.18 │       vcmpltps        k3,zmm30,zmm8
  0.16 │       vcmpltps        k0{k3},zmm0,zmm22
  0.17 │       vcmpltps        k3,zmm8,zmm30
  0.26 │       vcmpltps        k3{k3},zmm5,zmm7
  0.15 │       korw            k3,k3,k0
 21.41 │       vscatterdps     DWORD PTR [rbx+zmm25*1]{k3},zmm8
  0.53 │       add             rbx,0x140
  0.48 │       cmp             r9,r11
  0.00 │     ↑ jne             160
<snip>
```

## Additional Data
Not meaning to fully analyze the rest of the `perf` results, it is interesting to note two observations:
* Using 128-bit vector instructions gave ~4 instructions per cycle (IPC) while 512-bit vector instructions gave ~0.5 IPC.
* Stalls due to the memory subsystem (`backend_bound_memory`) also goes from ~8% to ~27% using 512-bits instead of 128-bit.

### gcc
```
$ ./out/build/release/gui/kinematics-demo-gui 5000000 &

$ perf stat -ddd --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '18608':

          5,143.90 msec task-clock                       #    1.000 CPUs utilized
             4,117      context-switches                 #  800.365 /sec
                25      cpu-migrations                   #    4.860 /sec
                 0      page-faults                      #    0.000 /sec
    28,245,680,887      cycles                           #    5.491 GHz                         (35.86%)
       457,239,227      stalled-cycles-frontend          #    1.62% frontend cycles idle        (35.81%)
   113,264,981,227      instructions                     #    4.01  insn per cycle
                                                         #    0.00  stalled cycles per insn     (35.83%)
    25,720,068,802      branches                         #    5.000 G/sec                       (35.98%)
        16,222,869      branch-misses                    #    0.06% of all branches             (35.96%)
    16,757,997,342      L1-dcache-loads                  #    3.258 G/sec                       (35.53%)
     1,611,640,715      L1-dcache-load-misses            #    9.62% of all L1-dcache accesses   (35.63%)
       107,418,518      L1-icache-loads                  #   20.883 M/sec                       (35.69%)
           535,342      L1-icache-load-misses            #    0.50% of all L1-icache accesses   (35.68%)
        26,714,413      dTLB-loads                       #    5.193 M/sec                       (35.69%)
        25,918,022      dTLB-load-misses                 #   97.02% of all dTLB cache accesses  (35.77%)
           466,229      iTLB-loads                       #   90.637 K/sec                       (35.65%)
           212,568      iTLB-load-misses                 #   45.59% of all iTLB cache accesses  (35.51%)
     1,601,607,961      L1-dcache-prefetches             #  311.360 M/sec                       (35.43%)

       5.142923752 seconds time elapsed

$ perf stat -MPipelineL2 --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '18608':

        22,167,150      ex_ret_brn_misp                  #      1.3 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (24.92%)
   182,732,631,266      de_src_op_disp.all                                                      (24.92%)
           508,416      resyncs_or_nc_redirects                                                 (24.92%)
    39,409,036,970      ls_not_halted_cyc                                                       (24.92%)
   179,618,060,878      ex_ret_ops                                                              (24.92%)
     4,438,530,678      ex_no_retire.load_not_complete   #     11.9 %  backend_bound_cpu
                                                         #      7.8 %  backend_bound_memory     (25.08%)
    46,610,032,477      de_no_dispatch_per_slot.backend_stalls                                        (25.08%)
    11,233,710,720      ex_no_retire.not_complete                                               (25.08%)
    39,397,592,372      ls_not_halted_cyc                                                       (25.08%)
        54,133,857      ex_ret_ucode_ops                 #     75.9 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (25.11%)
    39,424,546,035      ls_not_halted_cyc                                                       (25.11%)
   179,626,958,478      ex_ret_ops                                                              (25.11%)
     6,767,421,338      de_no_dispatch_per_slot.no_ops_from_frontend                #      1.4 %  frontend_bound_bandwidth  (24.90%)
       584,180,124      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      1.5 %  frontend_bound_latency   (24.90%)
    39,437,034,984      ls_not_halted_cyc                                                       (24.90%)

       7.154330654 seconds time elapsed
```

### clang
```
$ ./out/build/clang/gui/kinematics-demo-gui 5000000 &

$ perf stat -ddd --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '19140':

          7,132.97 msec task-clock                       #    1.000 CPUs utilized
             4,345      context-switches                 #  609.143 /sec
                57      cpu-migrations                   #    7.991 /sec
                 0      page-faults                      #    0.000 /sec
    39,207,299,084      cycles                           #    5.497 GHz                         (35.61%)
       504,755,213      stalled-cycles-frontend          #    1.29% frontend cycles idle        (35.78%)
    19,610,765,660      instructions                     #    0.50  insn per cycle
                                                         #    0.03  stalled cycles per insn     (35.82%)
       431,252,312      branches                         #   60.459 M/sec                       (35.87%)
         7,402,963      branch-misses                    #    1.72% of all branches             (35.86%)
    15,364,305,002      L1-dcache-loads                  #    2.154 G/sec                       (35.89%)
     1,685,348,397      L1-dcache-load-misses            #   10.97% of all L1-dcache accesses   (35.78%)
       151,586,661      L1-icache-loads                  #   21.252 M/sec                       (35.75%)
           637,749      L1-icache-load-misses            #    0.42% of all L1-icache accesses   (35.72%)
        28,015,144      dTLB-loads                       #    3.928 M/sec                       (35.69%)
        27,028,792      dTLB-load-misses                 #   96.48% of all dTLB cache accesses  (35.57%)
           499,886      iTLB-loads                       #   70.081 K/sec                       (35.58%)
           315,607      iTLB-load-misses                 #   63.14% of all iTLB cache accesses  (35.53%)
     1,672,876,597      L1-dcache-prefetches             #  234.527 M/sec                       (35.57%)

       7.131066805 seconds time elapsed

$ perf stat -MPipelineL2 --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '19140':

         6,287,849      ex_ret_brn_misp                  #      0.3 %  bad_speculation_mispredicts
                                                         #      0.2 %  bad_speculation_pipeline_restarts  (25.16%)
   116,715,115,622      de_src_op_disp.all                                                      (25.16%)
         3,659,931      resyncs_or_nc_redirects                                                 (25.16%)
    33,114,274,379      ls_not_halted_cyc                                                       (25.16%)
   115,801,340,018      ex_ret_ops                                                              (25.16%)
     9,138,694,174      ex_no_retire.load_not_complete   #     11.5 %  backend_bound_cpu
                                                         #     26.8 %  backend_bound_memory     (24.87%)
    76,059,384,540      de_no_dispatch_per_slot.backend_stalls                                        (24.87%)
    13,083,221,512      ex_no_retire.not_complete                                               (24.87%)
    33,100,041,542      ls_not_halted_cyc                                                       (24.87%)
   100,710,479,026      ex_ret_ucode_ops                 #      7.6 %  retiring_fastpath
                                                         #     50.7 %  retiring_microcode       (25.00%)
    33,124,607,739      ls_not_halted_cyc                                                       (25.00%)
   115,851,693,965      ex_ret_ops                                                              (25.00%)
     5,825,847,708      de_no_dispatch_per_slot.no_ops_from_frontend                #      1.6 %  frontend_bound_bandwidth  (24.97%)
       447,521,692      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      1.4 %  frontend_bound_latency   (24.97%)
    33,137,360,665      ls_not_halted_cyc                                                       (24.97%)

       6.008543417 seconds time elapsed
```
