#include "memfs.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fuse.h>
#include <iostream>
#include <memory>
#include <stdlib.h>
#include <unistd.h>
#include <variant>

namespace memfs {
void usage(char const *name) {
  std::cerr << "usage:  " << name << " [FUSE and mount options] mount_point\n";
  exit(-1);
}

int getattr(const char *path, struct stat *statbuf) {
  auto inner = internal_data();
  inner->index.log("getattr: %s\n", path);
  auto s = inner->index.status(path);
  if (s.has_value()) {
    *statbuf = s.value();
    return 0;
  }
  return -ENOENT;
}

int open(const char *path, struct fuse_file_info *fi) {
  auto inner = internal_data();
  inner->index.log("open: %s\n", path);
  auto s = inner->index.node(path);
  if (!s.get()) {
    return -ENOENT;
  }

  auto fd = new Fd(s);
  fi->fh = reinterpret_cast<uintptr_t>(fd);
  return 0;
}

int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
            struct fuse_file_info *fi) {

  auto inner = internal_data();
  inner->index.log("readdir: %s, off=%d\n", path, offset);
  auto n = inner->index.node(path);
  if (!n.get()) {
    return -ENOENT;
  }
  if (filler(buf, ".", nullptr, 0) != 0) {
    return -ENOMEM;
  }
  if (filler(buf, "..", nullptr, 0) != 0) {
    return -ENOMEM;
  }
  std::shared_lock guard(n->lock);
  for (auto const &iter : n->files) {
    if (filler(buf, iter.first.c_str(), nullptr, 0) != 0) {
      return -ENOMEM; // buffer full
    };
  }
  return 0;
}

int mknod(const char *path, mode_t mode, dev_t dev) {
  Internal *inner = internal_data();
  inner->index.log("mknod: %s, mode=%d, dev=%d\n", path, mode, dev);
  auto new_node = inner->index.mknode(path, mode, dev);
  if (std::holds_alternative<int>(new_node)) {
    return -std::get<int>(new_node);
  }
  return 0;
}

int read(const char *path, char *buf, size_t size, off_t offset,
         struct fuse_file_info *fi) {
  internal_data()->index.log("read: %s, size=%d, off=%d\n", path, size, offset);

  auto node = reinterpret_cast<Fd *>(fi->fh)->node;
  std::shared_lock guard(node->lock);

  store_type const &store = node->store;

  if (offset > store.size()) {
    return 0;
  }

  size_t ret = std::min(store.size() - offset, size);
  memcpy(buf, store.data() + offset, ret);
  return ret;
}

int write(const char *path, const char *buf, size_t size, off_t offset,
          struct fuse_file_info *fi) {

  internal_data()->index.log("write: %s, size=%d, off=%d\n", path, size,
                             offset);

  auto node = reinterpret_cast<Fd *>(fi->fh)->node;
  std::unique_lock guard(node->lock);

  store_type &store = node->store;

  if (offset + size >= store.size()) {
    store.resize(offset + size);
  }

  memcpy(store.data() + offset, buf, size);
  node->status.st_size = store.size();

  return size;
}

void destroy(void *user_data) {
  auto data = static_cast<Internal *>(user_data);
  data->index.log("destroying user_data\n");
}

}; // namespace memfs

static fuse_operations fuse_operation = {
    .getattr = memfs::getattr,
    .mknod = memfs::mknod,
    .unlink = nullptr, // TODO:
    .open = memfs::open,
    .read = memfs::read,
    .write = memfs::write,
    .readdir = memfs::readdir,
    .destroy = memfs::destroy,
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

  FILE *f = fopen("logger.out", "a");
  static memfs::Internal user_data{
      memfs::MemIdx(f),
  };
  fuse_main(argc, argv, &fuse_operation, static_cast<void *>(&user_data));
}
