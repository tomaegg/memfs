
#include <cstdarg>
#include <cstdio>

#include "memfs.h"
#include <cerrno>
#include <cstring>
#include <memory>
#include <mutex>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <variant>
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

namespace memfs {

void MemIdx::log(const char *format, ...) {
  va_list ap;
  va_start(ap, format);

  vfprintf(f, format, ap);
  fflush(f);
}

void MemNode::init(ino_t ino) {
  std::memset(&status, 0, sizeof(status));
  status.st_uid = getuid();
  status.st_gid = getgid();
  status.st_atime = time(nullptr);
  status.st_mtime = time(nullptr);
  status.st_ctime = time(nullptr);
  status.st_ino = ino;
  status.st_nlink = 1;
}

void MemNode::incre_link() {
  std::unique_lock guard(this->lock);
  this->status.st_nlink++;
}

MemIdx::~MemIdx() { fclose(this->f); }

MemIdx::MemIdx(FILE *f) : f(f), root(std::make_shared<MemNode>()), next_ino(1) {
  this->root->name = "/";
  this->root->init(this->alloc_ino());

  struct stat &s = this->root->status;
  s.st_mode = S_IFDIR | S_IREAD | S_IWRITE | S_IEXEC;
  s.st_size = 4096;
  s.st_nlink = 2;
}

std::shared_ptr<MemNode> MemIdx::get_node(std::string const &path) {
  if (path == "/") {
    return this->root;
  }

  auto parts = split(path, "/");

  auto p = root;
  for (auto const &name : parts) {
    std::shared_lock guard(p->lock);
    auto f = p->files.find(name);
    if (f == p->files.end()) {
      return nullptr;
    }
    p = f->second;
  }
  return p;
}

std::optional<struct stat> MemIdx::status(std::string const &path) {
  auto n = this->get_node(path);
  if (n.get()) {
    return n->status;
  }
  return std::nullopt;
}

std::shared_ptr<struct MemNode> MemIdx::node(std::string const &path) {
  auto n = this->get_node(path);
  return n;
}

std::variant<int, std::shared_ptr<struct MemNode>>
MemIdx::mknode(std::string const &path, mode_t mode, dev_t dev) {
  int pos = 0;
  for (int i = path.size() - 1; i >= 0; i--) {
    if (path[i] == '/') {
      pos = i;
      break;
    }
  }
  std::string const parent = path.substr(0, pos);
  std::string const name = path.substr(pos + 1); //
  //
  log("index: mknode: parent: %s, name: %s\n", parent.c_str(), name.c_str());

  if (!name.size()) {
    // root dir
    return EEXIST;
  }

  auto pnode = this->get_node(parent);
  if (!pnode.get()) {
    return ENOENT;
  }

  MemNode *raw = new MemNode;
  raw->init(this->alloc_ino());
  raw->name = name;
  raw->status.st_dev = dev;
  raw->status.st_mode = mode;
  if (mode & S_IFDIR) {
    raw->status.st_nlink = 2;
    pnode->incre_link();
  }

  return pnode->emplace(name, std::shared_ptr<MemNode>(raw));
}

}; // namespace memfs
