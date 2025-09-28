#include <fuse.h>
#include <iostream>
#include <stdlib.h>

namespace memfs {
void usage(char const *name) {
  std::cerr << "usage:  " << name
            << " [FUSE and mount options] rootDir mountPoint\n";
  exit(-1);
}

fuse_operations operation = {

};

struct internal {
  FILE *logfile;
  char *rootdir;
};

int getattr(const char *path, struct stat *statbuf) { return 0; }

}; // namespace memfs
