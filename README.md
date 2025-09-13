## x86v
Display the supported x86-64 micro-architecture version.

## Levels of micro-architecture
x86-64 v1: cmov, cmpxchg8bm fld, fxsave, emms, fxsave, syscall,
	       cvtss2si, cvtpi2pd \
x86-64 v2: cmpxchg16b popcnt sse3 sse4.1 sse4.2 ssse3 \
x86-64 v3: avx f16c fma movbe osxsave avx2 bmi1 bmi2 lzcnt \
x86-64 v4: avx512f avx512bw avx512cd avx512dq avx512vl

Taken from [Wikipedia](https://en.wikipedia.org/wiki/X86-64#Microarchitecture_levels).

**Note**
Most x86-64 CPUs partially support next version of micro-architecture extensions,
for example Intel SandyBridge has support for AVX but doesn't support AVX2.

There's also a header only library for C++ to check supported features
by a x86-64 CPU (Intel and AMD) at runtime. It's not complete yet.
