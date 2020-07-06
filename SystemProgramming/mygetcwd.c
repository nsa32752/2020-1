#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>

void creatEnv();

/**
 * 파일을 복사하여 사본을 생성한다. 
 * @param buf   경로명을 저장할 버퍼 변수.
 * @param size  버퍼 변수의 크기
 * @return 버퍼 변수의 시작 주소, 즉 buf
 */
char *mygetcwd(char *buf, size_t size) {
    char tmp[256] = "\0";
    char *res;
    DIR *curr_info;
    DIR *par_info;
    struct dirent *curr;
    struct dirent *par;
    long cur_ino;
    long par_ino;
    
    
    while(1){
      // printf("\n\n");
      char buf2[256] = "/";
      char buf3[256] = "\0";
      curr_info = opendir(".");
      par_info = opendir(".."); // parent directory
      curr = readdir(curr_info);
      par = readdir(par_info);
      if(curr->d_ino == par->d_ino) break;
      while(curr != NULL){ // find current directory inode
        // printf("curr: %d %s %d %ld\n",(int)(*curr->d_name), curr->d_name,curr->d_reclen, curr->d_ino);
        cur_ino = curr->d_ino;
        if((int)(*curr->d_name) == 46 && strlen(curr->d_name)==1) {
        	break;
        }
        curr = readdir(curr_info);
      }
      while(par != NULL){ // find current directory name from parent directory
        // printf("par: %s %ld\n", par->d_name, par->d_ino);
        if(par->d_ino == cur_ino){
            res = par->d_name;
        }
        par = readdir(par_info);
      }
      strcat(buf2, res);
      strcat(buf3, buf2);
      strcat(buf3, tmp);
      strcpy(buf, buf3);
      strcpy(tmp, buf);

      chdir("..");
    }
    closedir(curr_info);
    closedir(par_info);
  return buf;
}

int main(void) {
  pid_t pid;
  int status;
  char buf[255];

  creatEnv();
  chdir("dir/sub");
  printf("original func: %s\n", getcwd(NULL, 0));
  printf("mygetcwd func: %s\n", mygetcwd(buf, 255));

  return 0;
}

void creatEnv(){
  mkdir("dir", 0755);
  mkdir("dir/sub", 0755);
  mkdir("dir/sub2", 0);
  
  creat("dir/a", 0755);
  creat("dir/b", 0755);
  creat("dir/sub/x", 0755);
  symlink("dir/a", "dir/sl");
  symlink("dir/x", "dir/dsl");
}
