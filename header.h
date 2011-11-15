#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_CHARS 1024
#define MAX_COMMANDS 10
#define MAX_ARGS 10
#define PIPETO 16
#define PIPEFROM 18
#define READ_END 0
#define WRITE_END 1


typedef struct stage stage;
struct stage{
  char* line;
  char* input;
  char* output;
  int argc;
  char* argv[MAX_ARGS + 1];
};

stage* parseline(FILE*);
