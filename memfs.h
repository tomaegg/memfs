#pragma once

#include <atomic>
#include <fuse/fuse.h>
#include <memory>
#include <optional>
#include <string>
#include <sys/stat.h>
#include <unordered_map>

namespace memfs {

struct MemIdx;

struct MemNode {
  friend struct MemIdx;

private:
  std::string name;
  std::unordered_map<std::string, std::unique_ptr<MemNode>> files;
  struct stat status;
};

struct MemIdx {
  MemIdx();
  std::optional<struct stat> status(std::string const &);

private:
  std::optional<MemNode *> get_node(std::string const &);
  size_t alloc_ino() { return this->next_ino.fetch_add(1); };

  std::unique_ptr<MemNode> root;
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
