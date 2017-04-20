#include <stdio.h>
#include "server.h"

/* ./server <config file> */
int main(int argc, char **argv) {
  if(argc != 2) {
    fprintf(stderr,"Usage: %s <config file>\n",argv[0]);
    return 1;
  }

  return 0;
}
