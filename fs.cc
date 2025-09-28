#include "memfs.h"
#include <cerrno>
#include <cstring>
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

int open(const char *path, struct fuse_file_info *fi) {
  auto inner = internal_data();
  auto s = inner->index.node(path);
  if (!s.has_value()) {
    return -ENOENT;
  }

  auto fd = new Fd(s.value()->store);
  fi->fh = reinterpret_cast<uintptr_t>(fd);
  return 0;
}

int read(const char *path, char *buf, size_t size, off_t offset,
         struct fuse_file_info *fi) {
  auto store = reinterpret_cast<Fd *>(fi->fh)->store;
  if (offset > store->size()) {
    return 0;
  }

  size_t ret = std::min(store->size() - offset, size);
  memcpy(buf, store->data(), ret);
  return ret;
}

int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
            struct fuse_file_info *fi) {

  auto inner = internal_data();
  auto n = inner->index.node(path);
  if (!n.has_value()) {
    return -ENOENT;
  }
  if (filler(buf, ".", nullptr, 0) != 0) {
    return -ENOMEM;
  }
  if (filler(buf, "..", nullptr, 0) != 0) {
    return -ENOMEM;
  }
  for (auto const &iter : n.value()->files) {
    if (filler(buf, iter.first.c_str(), nullptr, 0) != 0) {
      return -ENOMEM; // buffer full
    };
  }
  return 0;
}

}; // namespace memfs

static fuse_operations fuse_operation = {
    .getattr = memfs::getattr,
    .unlink = nullptr, // TODO:
    .open = memfs::open,
    .read = memfs::read,
    .write = nullptr, // TODO:
    .readdir = memfs::readdir,
    .create = nullptr, // TODO:
};

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
  fuse_main(argc, argv, &fuse_operation, static_cast<void *>(&user_data));
}
