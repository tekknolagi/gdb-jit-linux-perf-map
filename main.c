#include <assert.h>   /* for assert */
#include <stddef.h>   /* for NULL */
#include <string.h>   /* for memcpy */
#include <sys/mman.h> /* for mmap and friends */
#include <stdio.h>    /* for fprintf */
#include <stdlib.h>   /* for EXIT_SUCCESS */
#include <stdint.h>   /* for uint64_t, uint32_t */
#include <sys/types.h> /* for pid_t */
#include <unistd.h>    /* for getpid */

// BEGIN copied from GDB docs

typedef enum
{
  JIT_NOACTION = 0,
  JIT_REGISTER_FN,
  JIT_UNREGISTER_FN
} jit_actions_t;

struct jit_code_entry
{
  struct jit_code_entry *next_entry;
  struct jit_code_entry *prev_entry;
  const char *symfile_addr;
  uint64_t symfile_size;
};

struct jit_descriptor
{
  uint32_t version;
  /* This type should be jit_actions_t, but we use uint32_t
     to be explicit about the bitwidth.  */
  uint32_t action_flag;
  struct jit_code_entry *relevant_entry;
  struct jit_code_entry *first_entry;
};

/* GDB puts a breakpoint in this function.  */
void __attribute__((noinline)) __jit_debug_register_code() {
  __asm__ __volatile__("" : : : "memory");
};

/* Make sure to specify the version statically, because the
   debugger may check the version before we can set it.  */
struct jit_descriptor __jit_debug_descriptor = {
  .version = 1,
  .action_flag = JIT_NOACTION,
  .first_entry = NULL,
  .relevant_entry = NULL,
};

// END copied from GDB docs

const unsigned char program[] = {
    // int3
    0xcc,
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

void register_with_gdb(void *code_addr, size_t code_size) {
  // The object is the perf map filename
  pid_t pid = getpid();
  char filename[256];
  int filename_len = snprintf(filename, sizeof(filename), "/tmp/perf-%d.map", pid);

  struct jit_code_entry *entry = malloc(sizeof *entry);
  entry->symfile_addr = filename;
  entry->symfile_size = filename_len + 1; // +1 for NUL
  entry->prev_entry = NULL;
  // This is where you would lock mutex if multithreaded
  entry->next_entry = __jit_debug_descriptor.first_entry;
  if (__jit_debug_descriptor.first_entry) {
    __jit_debug_descriptor.first_entry->prev_entry = entry;
  }
  __jit_debug_descriptor.first_entry = entry;
  __jit_debug_descriptor.relevant_entry = entry;
  __jit_debug_descriptor.action_flag = JIT_REGISTER_FN;
  __jit_debug_register_code();
  __jit_debug_descriptor.action_flag = JIT_NOACTION;
  // This is where you would unlock mutex if multithreaded
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
  register_with_gdb(memory, kProgramSize);
  assert(result == 0 && "mprotect failed");
  JitFunction function = *(JitFunction*)&memory;
  int return_code = function();
  fprintf(stderr, "returned %d\n", return_code);
  assert(return_code == 42 && "the assembly was wrong");
  result = munmap(memory, kProgramSize);
  assert(result == 0 && "munmap failed");
  return EXIT_SUCCESS;
}
