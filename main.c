#include <assert.h>   /* for assert */
#include <stddef.h>   /* for NULL */
#include <string.h>   /* for memcpy */
#include <sys/mman.h> /* for mmap and friends */
#include <stdio.h>    /* for fprintf */
#include <stdlib.h>   /* for EXIT_SUCCESS */
#include <stdint.h>   /* for uint64_t, uint32_t */
#include <sys/types.h> /* for pid_t */
#include <unistd.h>    /* for getpid */

const unsigned char program[] = {
    // mov eax, 42 (0x2a)
    0x48, 0xc7, 0xc0, 0x2a, 0x00, 0x00, 0x00,
    // ret
    0xc3,
};

const int kProgramSize = sizeof program;

typedef int (*JitFunction)();

void register_with_perf(const char *code_name, void *code_addr, size_t code_size) {
  pid_t pid = getpid();
  char filename[256];
  snprintf(filename, sizeof(filename), "/tmp/perf-%d.map", pid);
  FILE *f = fopen(filename, "a+");
  assert(f && "fopen failed");
  fprintf(f, "%lx %zx %s\n", (uintptr_t)code_addr, code_size, code_name);
  fclose(f);
}

int main() {
  fprintf(stderr, "my pid is %d\n", getpid());
  void *memory = mmap(/*addr=*/NULL, /*length=*/kProgramSize,
                      /*prot=*/PROT_READ | PROT_WRITE,
                      /*flags=*/MAP_ANONYMOUS | MAP_PRIVATE,
                      /*filedes=*/-1, /*offset=*/0);
  assert(memory != MAP_FAILED && "mmap failed");
  memcpy(memory, program, kProgramSize);
  int result = mprotect(memory, kProgramSize, PROT_EXEC);
  register_with_perf("my_jit_function", memory, kProgramSize);
  assert(result == 0 && "mprotect failed");
  JitFunction function = *(JitFunction*)&memory;
  int return_code = function();
  fprintf(stderr, "returned %d\n", return_code);
  assert(return_code == 42 && "the assembly was wrong");
  result = munmap(memory, kProgramSize);
  assert(result == 0 && "munmap failed");
  return EXIT_SUCCESS;
}
