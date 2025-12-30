#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "jit-reader.h"

GDB_DECLARE_GPL_COMPATIBLE_READER;

enum gdb_status read_debug_info(struct gdb_reader_funcs *self,
                                struct gdb_symbol_callbacks *cb,
                                void *memory, long memory_sz) {
  if (memory_sz < 4) {
    return GDB_FAIL;
  }
  static const char *perf_prefix = "/tmp/perf-";
  if (memcmp(memory, perf_prefix, strlen(perf_prefix)) != 0) {
    return GDB_FAIL;
  }
  struct gdb_object *obj = cb->object_open(cb);
  struct gdb_symtab *symtab = cb->symtab_open(cb, obj, /*filename=*/NULL);
  // Parse the Linux perf map file, adding a new block for each entry.
  FILE *f = fopen((char *)memory, "r");
  if (!f) {
    cb->symtab_close(cb, symtab);
    cb->object_close(cb, obj);
    return GDB_FAIL;
  }
  char *line = NULL;
  size_t line_len = 0;
  while (1) {
    // Use getline
    ssize_t nread = getline(&line, &line_len, f);
    if (nread == -1) {
      break;
    }
    uintptr_t addr = 0;
    size_t size = 0;
    char *name = malloc(1024);
    if (sscanf(line, "%lx %zx %1023s", &addr, &size, name) != 3) {
      continue;
    }
    cb->block_open(cb, symtab, /*parent=*/NULL, addr, addr+size, name);
  }
  fclose(f);
  cb->symtab_close(cb, symtab);
  cb->object_close(cb, obj);
  return GDB_FAIL;
}

enum gdb_status unwind_frame(struct gdb_reader_funcs *self,
                             struct gdb_unwind_callbacks *cb) {
  return GDB_SUCCESS;
}

struct gdb_frame_id get_frame_id(struct gdb_reader_funcs* self, struct gdb_unwind_callbacks* cbs) {
  struct gdb_frame_id frame = {0x1234000, 0};
  return frame;
}

void destroy_reader(struct gdb_reader_funcs* self) {}

extern struct gdb_reader_funcs *gdb_init_reader(void) {
  static struct gdb_reader_funcs reader_funcs = {
    .reader_version = GDB_READER_INTERFACE_VERSION,
    .priv_data = NULL,
    .read = read_debug_info,
    .unwind = unwind_frame,
    .get_frame_id = get_frame_id,
    .destroy = destroy_reader,
  };
  return &reader_funcs;
}
