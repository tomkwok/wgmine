#if defined(__x86_64__)
  // assume AVX is supported
  #define HAVE_AVX 1
#endif

#if defined(__aarch64__) && (defined(__ARM_NEON__) || defined(__ARM_NEON))
  #define HAVE_NEON64 1
#endif
