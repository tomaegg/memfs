#include "memfs.h"
#include <cerrno>
#include <fuse.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>

namespace memfs {
void usage(char const *name) {
  std::cerr << "usage:  " << name << " [FUSE and mount options] mount_point\n";
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

static fuse_operations operation = {
    .getattr = getattr,
    .unlink = nullptr,  // TODO:
    .open = nullptr,    // TODO:
    .write = nullptr,   // TODO:
    .readdir = nullptr, // TODO:
    .create = nullptr,  // TODO:
};

}; // namespace memfs
//

static bool isroot() { return (getuid() == 0) || (geteuid() == 0); }

int main(int argc, char *argv[]) {

  if (isroot()) {
    std::cerr << "running " << argv[0]
              << " as root opens unnacceptable security holes\n";
    return 1;
  }

  if ((argc < 2) || (argv[argc - 1][0] == '-'))
    memfs::usage(argv[0]);

  static memfs::Internal user_data;
  fuse_main(argc, argv, &memfs::operation, static_cast<void *>(&user_data));
}
