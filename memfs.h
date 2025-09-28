#pragma once

#include <atomic>
#include <fuse/fuse.h>
#include <memory>
#include <optional>
#include <string>
#include <sys/stat.h>
#include <unordered_map>
#include <vector>

namespace memfs {

struct MemIdx;

using store_type = std::vector<std::byte>;

struct MemNode {
  friend struct MemIdx;

  MemNode() : store(std::make_shared<store_type>()) {}

  std::shared_ptr<store_type> store;
  std::unordered_map<std::string, std::unique_ptr<MemNode>> files;
  std::string name;
  struct stat status;
};

struct Fd {
  Fd(std::shared_ptr<store_type> ptr) : store(ptr) {};

  std::shared_ptr<store_type> store;
};

struct MemIdx {
  MemIdx();
  std::optional<struct stat> status(std::string const &);
  std::optional<struct MemNode *> node(std::string const &);

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
