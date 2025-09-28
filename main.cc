#include "memfs.h"
#include <iostream>
#include <unistd.h>

bool isroot() { return (getuid() == 0) || (geteuid() == 0); }

int main(int argc, char *argv[]) {

  if (isroot()) {
    std::cerr << "Running BBFS as root opens unnacceptable security holes\n";
    return 1;
  }

  if ((argc < 2) || (argv[argc - 1][0] == '-'))
    memfs::usage(argv[0]);
}
