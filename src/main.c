#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <time.h>
#include <math.h>

#if defined(__linux__)
#include <bsd/stdlib.h>
#endif

#include <pthread.h>
#include <assert.h>
#include <signal.h>

#include "base64/include/libbase64.h"

#define BENCHMARK_ITERATIONS 10000

#define crypto_box_PUBLICKEYBYTES 32
#define crypto_box_SECRETKEYBYTES 32
#define sodium_base64_VARIANT_ORIGINAL BASE64_FORCE_NEON64

#define randombytes_buf(buf, nbytes) arc4random_buf(buf, nbytes)

#include "config.h"

#ifdef HAVE_AVX
  #include "mx25519/src/amd64/scalarmult.h"
  static const uint8_t p[] = {9};
  #define crypto_scalarmult_base(publickey, secretkey) \
    mx25519_scalarmult_amd64x((uint8_t *) publickey, (uint8_t *) secretkey, p)
#endif

#ifdef HAVE_NEON64
  #include "X25519-AArch64/X25519-AArch64.h"
  #define crypto_scalarmult_base(publickey, secretkey) \
    X25519_calc_public_key((unsigned char *) publickey, (unsigned char *) secretkey)
#endif

void sodium_bin2base64(char * const b64, const size_t b64_maxlen,
                         const unsigned char * const bin, const size_t bin_len,
                         const int variant)
{
/*
  void base64_encode(const char *src, size_t srclen,
                     char *out, size_t *outlen,
                     int flags);
*/
  size_t outlen;
  base64_encode((const char *) bin, bin_len,
               b64, &outlen,
               variant);
  b64[outlen] = '\0';
}

struct control_data
{
  bool done;
  pthread_mutex_t mutex;
};

static struct control_data *control = NULL;

void mine_keys(char const *filters[], size_t filters_len, size_t iterations, bool stop_on_find)
{
  unsigned char publickey[crypto_box_PUBLICKEYBYTES];
  unsigned char secretkey[crypto_box_SECRETKEYBYTES];
  char b64publickey[45];
  char b64privatekey[45];
  size_t filters_lengths[filters_len];

  for (size_t i = 0; i < filters_len; i++)
  {
    filters_lengths[i] = strlen(filters[i]);
  }

  if (iterations)
  {
    for (size_t c = 0; c < iterations; c++)
    {
      randombytes_buf(secretkey, sizeof secretkey);
      crypto_scalarmult_base(publickey, secretkey);
      sodium_bin2base64(b64publickey, sizeof b64publickey, publickey, sizeof publickey, sodium_base64_VARIANT_ORIGINAL);

      for (size_t i = 0; i < filters_len; i++)
      {
        if (strncmp(filters[i], b64publickey, filters_lengths[i]) == 0)
        {
          pthread_mutex_lock(&control->mutex);
          if (control->done)
          {
            pthread_mutex_unlock(&control->mutex);
            // discard solution to guarantee single result to output
            return;
          }
          sodium_bin2base64(b64privatekey, sizeof b64privatekey, secretkey, sizeof publickey, sodium_base64_VARIANT_ORIGINAL);
          fprintf(stdout, "%s %s\n", b64publickey, b64privatekey);
          fflush(stdout);
          if (stop_on_find)
          {
            control->done = true;
            pthread_mutex_unlock(&control->mutex);
            return;
          }
          pthread_mutex_unlock(&control->mutex);
          break;
        }
      }
    }
  }
  else
  {
    for (;;)
    {
      randombytes_buf(secretkey, sizeof secretkey);
      crypto_scalarmult_base(publickey, secretkey);
      sodium_bin2base64(b64publickey, sizeof b64publickey, publickey, sizeof publickey, sodium_base64_VARIANT_ORIGINAL);

      for (size_t i = 0; i < filters_len; i++)
      {
        if (strncmp(filters[i], b64publickey, filters_lengths[i]) == 0)
        {
          pthread_mutex_lock(&control->mutex);
          if (control->done)
          {
            // discard solution to guarantee single result to output
            pthread_mutex_unlock(&control->mutex);
            return;
          }
          sodium_bin2base64(b64privatekey, sizeof b64privatekey, secretkey, sizeof publickey, sodium_base64_VARIANT_ORIGINAL);
          fprintf(stdout, "%s %s\n", b64publickey, b64privatekey);
          fflush(stdout);
          if (stop_on_find)
          {
            control->done = true;
            pthread_mutex_unlock(&control->mutex);
            return;
          }
          pthread_mutex_unlock(&control->mutex);
          break;
        }
      }
    }
  }
}

float benchmark(unsigned cpus, unsigned filters_len, unsigned iterations)
{
  unsigned pid, pids[cpus];
  struct timespec start, end;
  const char *test[] = {"########"}; // will never match

  clock_gettime(CLOCK_MONOTONIC_RAW, &start);

  for (size_t i = 0; i < cpus; i++)
  {
    pid = fork();
    if (pid == 0)
    {
      break;
    }
    else
    {
      pids[i] = pid;
    }
  }

  if (pid == 0)
  {
    mine_keys(test, filters_len || 1, iterations, false);
    exit(0);
  }
  else
  {
    for (size_t i = 0; i < cpus; i++)
    {
      waitpid(pids[i], NULL, 0);
    }
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);

    float delta_s = (float)(end.tv_sec - start.tv_sec) + (float)(end.tv_nsec - start.tv_nsec) / 1000000000;

    float keys_per_second = (iterations * cpus) / delta_s;
    fprintf(stderr, "Benchmark result: %.0f keys/second\n", keys_per_second);
    return keys_per_second;
  }
}

void sigchld_rcv(int x) {}

int main(int argc, char const *argv[])
{
  int cpus = sysconf(_SC_NPROCESSORS_ONLN);
  bool batch_mode_enabled = false;
  pid_t pid;
  pid_t pids[cpus];

  fprintf(stderr, "Multiprocessing: %d\n", cpus);
  fprintf(stderr, "Benchmarking...\n");

  float kps = benchmark(cpus, argc - 1, BENCHMARK_ITERATIONS * cpus);

  if (argc < 2)
  {
    printf("No argument passed, stopping.\n");
    return 0;
  }

  if (strcmp(argv[1], "-b") == 0)
  {
    batch_mode_enabled = true;
    fprintf(stderr, "Running batch mode with %d prefixes\n", argc - 2);
  }
  else if (argc > 2)
  {
    fprintf(stderr, "Too many prefixes. Did you mean to run batch mode? (use flag -b)\n");
    return -1;
  }

  for (size_t i = 1 + batch_mode_enabled; i < argc; i++)
  {
    size_t len = strlen(argv[i]);
    if (len > 8)
    {
      fprintf(stderr, "Your values **may** be too log. Good luck.\n");
    }

    for (size_t c = 0; c < len; c++)
    {
      char chr = argv[i][c];
      if (chr != '+' && (chr < '/' || chr > '9') && (chr < 'A' || chr > 'Z') && (chr < 'a' || chr > 'z'))
      {
        fprintf(stderr, "Invalid value: '%s'. Only base64 characters are allowed.\n", argv[i]);
        return -1;
      }
    }

    float expected_wait_s = (float)pow(64, len) / kps;
    float expected_throuhput_s = 1 / expected_wait_s;
    if (batch_mode_enabled)
    {
      fprintf(stderr,
              "Expecting to find a key for '%s' once every %.1f seconds. [%.3f keys per hour]\n",
              argv[i], expected_wait_s, expected_throuhput_s * 60 * 60);
    }
    else
    {
      fprintf(stderr, "Estimated wait time: %.1f seconds.\n", expected_wait_s);
    }
  }

  // initialize shared memory
  int prot = PROT_READ | PROT_WRITE;
  int flags = MAP_SHARED | MAP_ANONYMOUS;
  control = mmap(NULL, sizeof(struct control_data), prot, flags, -1, 0);
  assert(control);

  control->done = false;

  // initialise mutex so it works properly in shared memory
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&control->mutex, &attr);

  for (size_t i = 0; i < cpus; i++)
  {
    pid = fork();
    if (pid == 0)
    {
      break;
    }
    else
    {
      pids[i] = pid;
    }
  }
  if (pid)
  {
    if (!batch_mode_enabled)
    {
      signal(SIGCHLD, sigchld_rcv);
    }
    pause();
    for (size_t i = 0; i < cpus; i++)
    {
      killpg(pids[i], SIGKILL);
    }
    return 0;
  }

  mine_keys(&argv[1 + batch_mode_enabled], argc - 1 - batch_mode_enabled, 0, !batch_mode_enabled);
  return 0;
}
