#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#define MAXARGS 31
#define MAXFILENAME 1024
#define MAXPATHSTR 2048

extern char **environ;
int myexeclp(const char *file, const char *args, ...){
  
  // 파일 존재 및 실행가능 여부 테스트
  chdir(getenv("PATH")); //PATH에서 경로명 가져옴
  char *cur = getcwd(NULL, 0);
  while(1){
    cur = getcwd(NULL, 0);
    if(access(file, X_OK) < 0){
      chdir(".."); // 존재하지 않거나 실행불가이면 상위 디렉토리로 이동
      if(strcmp(getcwd(NULL, 0),cur)) return -1;
    }
    else{
      break;
    }
  }
  
  char path[MAXPATHSTR] = {};
  sprintf(path, "PATH=%s", cur);
  char *envp[] = {path, NULL};
  
  va_list ap;
  va_start(ap, args);
  char *v[MAXARGS] = {};
  v[0] = (char *)args;
  int i=1;
  while(1){
    v[i] = va_arg(ap, char *);
    if(v[i] == NULL) break;
    i++;
    args++;
  }
  va_end(ap);
  
  if(execve(file, v, envp)<0){
    fprintf(stderr,"error");
    return -1;
  }
  return 0;
  
}

int main(void) {
  char path[MAXPATHSTR];
  sprintf(path, "PATH=%s:%s", getcwd(NULL, 0), getenv("PATH"));
  putenv(path);
  // prepare the executable file named "hello"
  system("cp hello.tmp hello.c"); // copy hello.tmp to hello.c
  system("clang-7 -pthread -lm -o hello hello.c"); // build hello.c
  system("rm hello.c"); // remove hello.c
  // execlp("ls", "ls", "-a", NULL);
  // execlp("hello", "hello", "-a", "-b", "-c", (char *) 0);
  myexeclp("hello", "hello", "-a", "-b", "-c", (char *) 0);
  return 0;
}
