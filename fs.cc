#include <fuse.h>
#include <iostream>
#include <stdlib.h>

void usage(char const *name) {
  std::cerr << "usage:  " << name
            << " [FUSE and mount options] rootDir mountPoint\n";
  exit(-1);
}

fuse_operations memfs_operation = {

};
