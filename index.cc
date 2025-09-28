#include <atomic>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <optional>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

// return values ignore empty string
std::vector<std::string> split(std::string const &s,
                               std::string const &delimiter) {
  std::vector<std::string> ret;

  if (delimiter.empty()) {
    ret.emplace_back(s);
    return ret;
  }

  size_t start = 0;
  size_t end = s.find(delimiter, start);

  while (end != std::string::npos) {
    auto part = s.substr(start, end - start);
    if (part.size()) {
      ret.emplace_back(part);
    }

    start = end + delimiter.size();
    end = s.find(delimiter, start);
  }

  auto last = s.substr(start);
  if (last.size())
    ret.emplace_back(last);

  return ret;
}

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

MemIdx::MemIdx() : root(std::make_unique<MemNode>()), next_ino(1) {
  std::memset(&this->root->status, 0, sizeof(this->root->status));

  this->root->name = "/";

  struct stat &s = this->root->status;

  s.st_ino = this->alloc_ino();
  s.st_mode = S_IFDIR | S_IREAD | S_IWRITE | S_IEXEC;
  s.st_uid = getuid();
  s.st_gid = getgid();
  s.st_size = 4096;
  s.st_nlink = 2;
  s.st_atime = time(nullptr);
  s.st_mtime = time(nullptr);
  s.st_ctime = time(nullptr);
}

std::optional<MemNode *> MemIdx::get_node(std::string const &path) {
  auto parts = split(path, "/");

  MemNode *p = root.get();
  for (auto const &name : parts) {
    auto f = p->files.find(name);
    if (f == p->files.end()) {
      return std::nullopt;
    }
    p = f->second.get();
  }
  return p;
}

std::optional<struct stat> MemIdx::status(std::string const &path) {
  auto n = this->get_node(path);
  if (n.has_value()) {
    return n.value()->status;
  }
  return std::nullopt;
}
