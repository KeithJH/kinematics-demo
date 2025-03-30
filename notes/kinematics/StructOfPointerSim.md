# StructOfPointerSim
SoA layout that uses `float*` fields manually managed with `new[]` and `delete[]`. In order to get the best performance a common `UpdateHelper` method is used.

```
struct Bodies
{
        float *x, *y;
        float *horizontalSpeed, *verticalSpeed;
        Color *color;
};
Bodies _bodies;

void Update(const float deltaTime)
{
	UpdateHelper(deltaTime, _bodies.x, _bodies.y, _bodies.horizontalSpeed, _bodies.verticalSpeed);
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
Update StructOfPointerSim: 1000000             100             1     225.21 us
Update StructOfPointerSim: 5000000             100             1    2.30152 ms
```

### clang
```
$ clang++ --version | head -1
Ubuntu clang version 18.1.3 (1ubuntu1)

$ ./out/build/clang/bench/kinematics-demo-bench --benchmark-no-analysis --rng-seed 433152058
benchmark name                            samples    iterations          mean
-------------------------------------------------------------------------------
<BENCH>
Update StructOfPointerSim: 1000000             100             1     164.29 us
Update StructOfPointerSim: 5000000             100             1     2.6562 ms
```

## Assembly Analysis
### gcc
```
$ perf record -D 1000 ./out/build/release/gui/kinematics-demo-gui 5000000
<wait some time, then close>

$ perf annotate -Mintel
main
<snip>
  0.44 │457:   vmovups      zmm2,ZMMWORD PTR [rdi+rdx*1]
  3.35 │       vmovups      zmm1,ZMMWORD PTR [rsi+rdx*1]
  3.13 │       vmovaps      zmm3,zmm8
  3.98 │       vfmadd213ps  zmm3,zmm1,ZMMWORD PTR [rcx+rdx*1]
  9.41 │       vmovups      ZMMWORD PTR [rcx+rdx*1],zmm3
  5.23 │       vcmpltps     k2,zmm2,zmm0
  2.79 │       vcmpgtps     k1,zmm2,zmm0
  2.77 │       vmovaps      zmm4,zmm2
  2.85 │       vfmadd213ps  zmm4,zmm8,ZMMWORD PTR [r11+rdx*1]
  1.63 │       vaddps       zmm9,zmm4,zmm7
  2.98 │       vmovups      ZMMWORD PTR [r11+rdx*1],zmm4
  1.60 │       vcmpltps     k2{k2},zmm9,zmm0
  1.49 │       knotw        k3,k2
  1.45 │       kmovw        r14d,k2
  1.46 │       vaddps       zmm9{k3}{z},zmm4,zmm6
  1.35 │       vcmpltps     k0{k1},zmm11,zmm9
  1.24 │       kandw        k0,k0,k3
  1.36 │       kmovw        eax,k0
  1.53 │       or           ax,r14w
  0.03 │     ↓ jne          9f0
  1.35 │4cf:   vcmpltps     k1,zmm1,zmm0
  1.42 │       vaddps       zmm2,zmm3,zmm7
  1.43 │       vcmpltps     k0{k1},zmm2,zmm0
  1.54 │       knotw        k1,k0
  3.07 │       kmovw        r14d,k0
  3.28 │       vaddps       zmm2{k1}{z},zmm3,zmm6
  4.92 │       vcmpgtps     k1{k1},zmm1,zmm0
  6.96 │       vcmpgtps     k1{k1},zmm2,zmm10
  6.55 │       kmovw        eax,k1
  6.68 │       or           ax,r14w
  0.00 │     ↓ jne          9d0
  4.65 │50d:   inc          r9
  4.21 │       add          rdx,0x40
  2.54 │       cmp          r9,r10
       │     ↑ jne          457
<snip>
```

### clang
```
$ perf record -D 1000 ./out/build/clang/gui/kinematics-demo-gui 5000000
<wait some time, then close>

$ perf annotate -Mintel
kinematics::StructOfPointerSim::Update(float)
<snip>
  1.39 │190:   vmovups      zmm9,ZMMWORD PTR [r12+rdx*4]
  1.50 │       vmovups      zmm10,ZMMWORD PTR [r14+rdx*4]
  3.06 │       vfmadd231ps  zmm10,zmm9,zmm2
  4.26 │       vxorps       zmm14,zmm9,zmm7
  2.93 │       vcmpltps     k1,zmm9,zmm14
  3.76 │       vmovups      ZMMWORD PTR [r14+rdx*4],zmm10
  1.82 │       vaddps       zmm13,zmm10,zmm5
  2.38 │       vaddps       zmm10,zmm10,zmm8
  2.30 │       vmovups      zmm11,ZMMWORD PTR [r13+rdx*4+0x0]
  2.59 │       vmovups      zmm12,ZMMWORD PTR [r15+rdx*4]
  4.31 │       vcmpltps     k0{k1},zmm13,zmm6
  4.45 │       vcmpltps     k1,zmm14,zmm9
  4.45 │       vcmpltps     k1{k1},zmm3,zmm10
  5.46 │       korw         k1,k1,k0
  5.52 │       vfmadd231ps  zmm12,zmm11,zmm2
  5.62 │       vxorps       zmm10,zmm11,zmm7
  7.00 │       vaddps       zmm9,zmm12,zmm5
 13.88 │       vmovups      ZMMWORD PTR [r15+rdx*4],zmm12
  8.27 │       vmovups      ZMMWORD PTR [r12+rdx*4]{k1},zmm14
  4.20 │       vcmpltps     k1,zmm11,zmm10
  1.01 │       vaddps       zmm12,zmm12,zmm8
  1.05 │       vcmpltps     k0{k1},zmm9,zmm6
  1.01 │       vcmpltps     k1,zmm10,zmm11
  2.15 │       vcmpltps     k1{k1},zmm4,zmm12
  1.37 │       korw         k1,k1,k0
  2.12 │       vmovups      ZMMWORD PTR [r13+rdx*4+0x0]{k1},zmm10
  1.07 │       add          rdx,0x10
  1.09 │       cmp          rcx,rdx
       │     ↑ jne          190
<snip>
```

## Additional Data
### gcc
```
$ ./out/build/release/gui/kinematics-demo-gui 5000000 &

$ perf stat -ddd --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '29422':

          7,212.26 msec task-clock                       #    1.001 CPUs utilized
            12,067      context-switches                 #    1.673 K/se
               108      cpu-migrations                   #   14.974 /sec
                 0      page-faults                      #    0.000 /sec
    39,418,140,682      cycles                           #    5.465 GHz                         (35.77%)
     1,066,854,134      stalled-cycles-frontend          #    2.71% frontend cycles idle        (35.68%)
    33,882,328,489      instructions                     #    0.86  insn per cycle
                                                         #    0.03  stalled cycles per insn     (35.72%)
     3,037,749,460      branches                         #  421.192 M/sec                       (35.66%)
        30,709,538      branch-misses                    #    1.01% of all branches             (35.94%)
    14,049,266,797      L1-dcache-loads                  #    1.948 G/sec                       (35.87%)
     3,812,144,127      L1-dcache-load-misses            #   27.13% of all L1-dcache accesses   (35.77%)
       363,235,342      L1-icache-loads                  #   50.364 M/sec                       (35.74%)
         1,442,456      L1-icache-load-misses            #    0.40% of all L1-icache accesses   (35.71%)
        62,679,560      dTLB-loads                       #    8.691 M/sec                       (35.52%)
        60,410,329      dTLB-load-misses                 #   96.38% of all dTLB cache accesses  (35.60%)
         1,297,169      iTLB-loads                       #  179.856 K/sec                       (35.63%)
           791,654      iTLB-load-misses                 #   61.03% of all iTLB cache accesses  (35.70%)
     3,361,267,850      L1-dcache-prefetches             #  466.049 M/sec                       (35.67%)

       7.204674840 seconds time elapsed

$ perf stat -MPipelineL2 --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '29422':

        33,756,624      ex_ret_brn_misp                  #      0.6 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (24.95%)
    40,255,727,109      de_src_op_disp.all                                                      (24.95%)
         1,836,413      resyncs_or_nc_redirects                                                 (24.95%)
    45,535,296,824      ls_not_halted_cyc                                                       (24.95%)
    38,398,494,362      ex_ret_ops                                                              (24.95%)
    24,886,000,565      ex_no_retire.load_not_complete   #     24.3 %  backend_bound_cpu
                                                         #     58.0 %  backend_bound_memory     (25.19%)
   224,849,027,992      de_no_dispatch_per_slot.backend_stalls                                  (25.19%)
    35,333,377,051      ex_no_retire.not_complete                                               (25.19%)
    45,522,649,730      ls_not_halted_cyc                                                       (25.19%)
       117,876,116      ex_ret_ucode_ops                 #     14.0 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (24.89%)
    45,556,855,373      ls_not_halted_cyc                                                       (24.89%)
    38,412,233,582      ex_ret_ops                                                              (24.89%)
     7,789,830,598      de_no_dispatch_per_slot.no_ops_from_frontend                #      0.5 %  frontend_bound_bandwidth  (24.97%)
     1,075,229,713      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      2.4 %  frontend_bound_latency   (24.97%)
    45,576,278,997      ls_not_halted_cyc                                                       (24.97%)

       8.278323506 seconds time elapsed
```

### clang
```
$ ./out/build/clang/gui/kinematics-demo-gui 5000000 &

$ perf stat -ddd --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '29553':

          6,489.54 msec task-clock                       #    1.001 CPUs utilized
             9,068      context-switches                 #    1.397 K/sec
               143      cpu-migrations                   #   22.035 /sec
                 0      page-faults                      #    0.000 /sec
    35,532,992,290      cycles                           #    5.475 GHz                         (35.50%)
       703,708,093      stalled-cycles-frontend          #    1.98% frontend cycles idle        (35.61%)
    21,364,152,044      instructions                     #    0.60  insn per cycle
                                                         #    0.03  stalled cycles per insn     (35.73%)
       875,029,031      branches                         #  134.837 M/sec                       (35.72%)
        12,203,773      branch-misses                    #    1.39% of all branches             (35.96%)
    14,623,491,045      L1-dcache-loads                  #    2.253 G/sec                       (36.01%)
     2,986,791,744      L1-dcache-load-misses            #   20.42% of all L1-dcache accesses   (35.96%)
       273,339,895      L1-icache-loads                  #   42.120 M/sec                       (35.83%)
         1,017,466      L1-icache-load-misses            #    0.37% of all L1-icache accesses   (35.81%)
        47,110,054      dTLB-loads                       #    7.259 M/sec                       (35.59%)
        45,551,791      dTLB-load-misses                 #   96.69% of all dTLB cache accesses  (35.58%)
           956,521      iTLB-loads                       #  147.394 K/sec                       (35.50%)
           574,717      iTLB-load-misses                 #   60.08% of all iTLB cache accesses  (35.60%)
     2,140,235,203      L1-dcache-prefetches             #  329.798 M/sec                       (35.59%)

       6.482174440 seconds time elapsed

$ perf stat -MPipelineL2 --pid=$(pgrep kinematics-demo)
<wait some time, then cancel>
 Performance counter stats for process id '29553':

        11,686,393      ex_ret_brn_misp                  #      0.1 %  bad_speculation_mispredicts
                                                         #      0.0 %  bad_speculation_pipeline_restarts  (25.05%)
    23,765,597,563      de_src_op_disp.all                                                      (25.05%)
         1,092,774      resyncs_or_nc_redirects                                                 (25.05%)
    35,422,324,006      ls_not_halted_cyc                                                       (25.05%)
    23,510,986,986      ex_ret_ops                                                              (25.05%)
    23,362,651,346      ex_no_retire.load_not_complete   #     14.6 %  backend_bound_cpu
                                                         #     71.9 %  backend_bound_memory     (24.93%)
   183,721,078,472      de_no_dispatch_per_slot.backend_stalls                                  (24.93%)
    28,110,650,879      ex_no_retire.not_complete                                               (24.93%)
    35,407,950,658      ls_not_halted_cyc                                                       (24.93%)
        78,369,131      ex_ret_ucode_ops                 #     11.0 %  retiring_fastpath
                                                         #      0.0 %  retiring_microcode       (25.11%)
    35,438,934,615      ls_not_halted_cyc                                                       (25.11%)
    23,524,118,322      ex_ret_ops                                                              (25.11%)
     4,912,692,374      de_no_dispatch_per_slot.no_ops_from_frontend                #      0.4 %  frontend_bound_bandwidth  (24.91%)
       662,728,342      cpu/de_no_dispatch_per_slot.no_ops_from_frontend,cmask=0x6/ #      1.9 %  frontend_bound_latency   (24.91%)
    35,454,944,898      ls_not_halted_cyc                                                       (24.91%)

       6.440373035 seconds time elapsed
```
