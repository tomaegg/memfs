#include "memfs.h"
#include <cerrno>
#include <fuse.h>
#include <iostream>
#include <stdlib.h>

namespace memfs {
void usage(char const *name) {
  std::cerr << "usage:  " << name
            << " [FUSE and mount options] rootDir mountPoint\n";
  exit(-1);
}

int getattr(const char *path, struct stat *statbuf) {
  auto inner = internal_data();
  auto s = inner->index.status(path);
  if (s.has_value()) {
    *statbuf = s.value();
    return 0;
  }
  return -ENOENT;
}

static fuse_operations operation = {.getattr = getattr};

}; // namespace memfs
//
