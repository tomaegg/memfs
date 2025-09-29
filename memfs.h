#pragma once

#include <atomic>
#include <cstdio>
#include <fuse/fuse.h>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unordered_map>
#include <variant>
#include <vector>

namespace memfs {

struct MemIdx;

using store_type = std::vector<std::byte>;

struct MemNode {
  friend struct MemIdx;

  void init(ino_t ino);
  void incre_link();
  template <typename... Args> std::shared_ptr<MemNode> emplace(Args &&...args) {
    std::unique_lock guard(this->lock);
    auto [it, _] = this->files.emplace(std::forward<Args>(args)...);
    return it->second;
  };

  store_type store;

  std::unordered_map<std::string, std::shared_ptr<MemNode>> files;
  std::string name;
  struct stat status;
  std::shared_mutex lock;
};

struct Fd {
  Fd(std::shared_ptr<MemNode> n) : node(n) {};
  std::shared_ptr<MemNode> node;
};

struct MemIdx {
  void log(const char *format, ...);

  MemIdx(FILE *f);
  ~MemIdx();
  std::optional<struct stat> status(std::string const &);
  std::shared_ptr<struct MemNode> node(std::string const &);
  std::variant<int, std::shared_ptr<struct MemNode>> mknode(std::string const &,
                                                            mode_t, dev_t);

private:
  FILE *f;
  size_t alloc_ino() { return this->next_ino.fetch_add(1); };
  std::shared_ptr<MemNode> get_node(std::string const &);

  std::shared_ptr<MemNode> root;
  mutable std::atomic<size_t> next_ino;
  std::unordered_map<size_t, MemNode *> inode_map;
};

struct Internal {
  MemIdx index;
};

void usage(char const *);

inline Internal *internal_data() {
  return static_cast<Internal *>(fuse_get_context()->private_data);
}

} // namespace memfs
