#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>

char *argv[1024] = {NULL,};
char *argc[1024] = {NULL,};
char *argp[1024] = {NULL,};
char *redirection[1024] = {NULL,};
char linecom[4096];
char command[4096];
char path[4096];
int filenum, ptmp=0;
int noclobber = 0;
int background =0; // &실행될때마다 -1
int backflag=0; // 현재 명령어가 background 실행인지 알려줌
int fd[2];
int pflag=0;

void history_save();
void history(int n);
void shell(int d, int a);
void rdandpipe(int n);

void history_save(){
    FILE *f = NULL;
    f = fopen(path, "a+");
    // fseek(f, -1, SEEK_END);
    fputs(command, f);
    fputs("\n",f);
    fclose(f);
}

void history(int n){
    FILE *f = NULL;
    f = fopen(path, "r");
    if(f != NULL){
        char tmp[256];
        int i=1;
        if(n==-1){
            while(!feof(f)){
                bzero(tmp, 256);
                fgets(tmp, sizeof(tmp), f);
                if(strlen(tmp) > 0 && backflag == 0) printf("%d %s", i++, tmp);
            }
        }
        else{
            bzero(linecom, 256);
            while(i<=n){
                bzero(tmp, 256);
                if(fgets(tmp, sizeof(tmp), f) == NULL){
                  perror("no number");
                  command[0] = '\0';
                  break;
                }
                if(i == n){
                    bzero(command,256);
                    strncpy(command, tmp, strlen(tmp)-1); // \n 제거
                    // printf("%d %s", i++, tmp);
                    break;
                }
                i++;
            }
        }
    }
    else{
        printf("file does not exist");
    }
    fclose(f);
}

void shell(int d, int a){ // d=1 -> redirection, d=2 -> redirection <, a: 1: append인 경우
    if(backflag == 1) printf("----------background----------\n");
    //history
    // printf("%d %d\n", d, a);
    // printf("argv[0]: %s %s\n", argv[0], argv[2]);
    int k=0;
    while(argv[k] != NULL){
        // printf("%s ", argv[k]);
        k++; // 길이 확인
    }
    // printf("\n");
    int pid, fd1, fd2;
    int status; 
        if(strcmp(argv[0], "cd") == 0){ // cd
                chdir(argv[1]);
        }
        else if(strcmp(argv[0], "history") == 0){ // history
            if(d==1 && a==0){
                if((fd1 = open(argv[filenum], O_WRONLY|O_CREAT|O_TRUNC, 0644)) <0){
                    perror("file open error");
                    exit(-1);
                }
                int n, fhis;
                char r[2048];
                if((fhis = open("history.txt", O_RDONLY)) < 0){
                    perror("file open error");
                    return ;
                }
                while((n = read(fhis, r, 2048))>0){
                    if(write(fd1, r, n) <0){
                        perror("file write error");
                        return ;
                    }
                }
                close(fhis);
                close(fd1);
            }
            else if(d==1 && a==1){
                if((fd1 = open(argv[filenum], O_WRONLY|O_APPEND)) < 0){
                    if((fd1 = open(argv[filenum], O_WRONLY|O_CREAT|O_APPEND, 0644)) <0){
                        perror("file open error");
                        exit(-1);
                    }
                }
                int n, fhis;
                char r[2048];
                if((fhis = open("history.txt", O_RDONLY)) < 0){
                    perror("file open error");
                    return ;
                }
                while((n = read(fhis, r, 2048))>0){
                    if(write(fd1, r, n) <0){
                        perror("file write error");
                        return ;
                    }
                }
                close(fhis);
                close(fd1);

            }
            else{
                history(-1);
            }
        }
        // 단일 명령어
        else{
            if((pid = fork())<0){
                status = -1;
            }
            else if(pid==0){
                if(d==30){
                    dup2(ptmp, STDIN_FILENO); //표준입력-명령어의 입력을 파일로 받음 --> 이전 명령어에 대해(첫명령어는 0)
                    dup2(fd[1], STDOUT_FILENO); // 표준출력-명령어의 결과를 파일로 받음
                    close(ptmp);
                    execvp(argv[0], argv);
                    perror("error");
                }
                else if(d==31){
                    dup2(ptmp, STDIN_FILENO); //표준입력-명령어의 입력을 파일로 받음
                    close(ptmp);
                    if(a==0){
                        if((fd1 = open(argv[filenum], O_WRONLY|O_CREAT|O_TRUNC, 0644)) <0){
                            perror("file open error");
                            exit(-1);
                        }
                        argv[filenum-1] = NULL;
                        dup2(fd1, STDOUT_FILENO);
                        close(fd1);
                    }
                    else if(a==1){
                        if((fd1 = open(argv[filenum], O_WRONLY|O_APPEND)) < 0){
                            if((fd1 = open(argv[filenum], O_WRONLY|O_CREAT|O_APPEND, 0644)) <0){
                                perror("file open error");
                                exit(-1);
                            }
                        }
                        argv[filenum-1] = NULL;
                        dup2(fd1, STDOUT_FILENO);
                        close(fd1);
                    }
                    execvp(argv[0], argv);
                    perror("error");
                }
                else if(d==1 && a==0){
                    if((fd1 = open(argv[filenum], O_WRONLY|O_CREAT|O_TRUNC, 0644)) <0){
                        perror("file open error");
                        exit(-1);
                    }
                    argv[filenum-1] = NULL;
                    dup2(fd1, STDOUT_FILENO);
                    close(fd1);
                    execvp(argv[0], argv);
                }
                else if(d==1 && a==1){
                    if((fd1 = open(argv[filenum], O_WRONLY|O_APPEND)) < 0){
                        if((fd1 = open(argv[filenum], O_WRONLY|O_CREAT|O_APPEND, 0644)) <0){
                            perror("file open error");
                            exit(-1);
                        }
                    }
                    argv[filenum-1] = NULL;
                    dup2(fd1, STDOUT_FILENO);
                    close(fd1);
                    execvp(argv[0], argv);
                    perror("error");
                }
                else if(d==2 && a==0){
                    if((fd1=open(argv[filenum], O_RDONLY))<0){
                        perror("file open error");
                        exit(-1);
                    }
                    argv[filenum -1] = NULL;
                    dup2(fd1, STDIN_FILENO);
                    close(fd1);
                    execvp(argv[0], argv);
                    perror("error");
                }
                else {
                    execvp(argv[0], argv);
                    perror("error");
                }
                _exit(127);
                }
            else{
                if(!backflag) waitpid(pid, &status, 0); // background 실행이 아닐때만 waitpid
                else printf("background: %d\n", pid);
            }   
        }
        if(backflag == 1) printf("-------------------------------\n");
}

void rdandpipe(int n){ // argv 사용, 명령어 하나만 들어옴, n=()에서 넘어온 변수인가(0: ()안의 명령어 아님, 1: (를 포함한 명령어 변수, 2: ()안의 명령어 )
    int k=0, pip=0;
    // printf("rdandpipe\n");
    // printf("%s ", argv[0]);
    while(argv[k] != NULL){
        if(!strcmp(argv[k], "|")) pip =1; 
        // printf("%s ", argv[k]);
        k++; // 길이 확인
    }
    // printf("\n");
    filenum = k-1;
    if((strstr(argv[k-2], ">") || strstr(argv[k-2], ">|"))&& pip == 0){
        // ****> 일 때 noclober on인 경우 덮어쓰기 x 변수 1로 바꿔주기, ()에서 넘어온 변수가 아니어야 함****
        if(!strcmp(argv[0], "cat")){ // cat을 포함하고 있는 경우
            if(!strcmp(argv[1], ">")){ // cat > file1
                // FILE *f = NULL;
                // f = fopen(argv[2], "w");
                // while(fputs("",f) != EOF) continue;
                // fclose(f);
                int fd = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 0644);
                char tmp[1024];
                printf("===input===\n");
                while(scanf("%s", tmp) != EOF){
                    write(fd, tmp, strlen(tmp));
                    write(fd, "\n", 1);
                }
                printf("===finish===\n");
                close(fd);
            }
            else{ // cat file1 file2 .. > file 3
                int i=1;
                int floc=0;
                while(argv[floc] != NULL) floc++;
                floc -= 1;
                int cp, base; // 쓸 파일, 읽을 파일
                cp = open(argv[floc], O_WRONLY|O_CREAT|O_TRUNC, 0644);
                while(i < floc-1){
                    int n;
                    char tmp[2048];
                    base = open(argv[i], O_RDONLY);
                    while((n=read(base, tmp, 2048)) > 0){
                        if(write(cp, tmp, n) < 0) perror("write error");
                    }
                    close(base);
                    if(write(cp, "\n", 1) < 0) perror("write error");
                    i++;
                }
                close(cp);
            }
        }
        else{ // cat을 포함하고 있지 않은 경우
            // printf("%s %s %s",argv[0], argv[1], argv[2]);
            // printf("argv[k-1]:%s\n", argv[k-1]);
            if(n==1 || !strcmp(argv[k-2], ">>")){ // noclobber 상관없음, append 가능, ()명령어 이미 파일 덮어쓰기 가능 판별하고 들어옴
                // printf("()안의 command\n");
                shell(1,1);
            }
            else if(n==0 && noclobber==1 && !strcmp(argv[k-2],">") && (access(argv[k-1], F_OK) != -1)){ // 덮어쓰기 불가(noclobber=1), >, 파일이 있는 경우
                printf("덮어쓰기 불가\n");
            }
            else{ // 덮어쓰기 가능
                // printf("덮어쓰기 가능");
                shell(1,0);
            }
        }
    }
    else if(strcmp(argv[1], ">>") == 0){ // command >> file
        shell(1,1); // append 모드로 파일 열기 옵션 추가
    }
    else if(strcmp(argv[filenum-1], "<") == 0){ // command << file
        shell(2,0);
    }
    else{ // command | command | command --- 통으로 들어옴
        // 마지막 명령어 제외 모두 결과를 파일에다 받음 -> STDOUT_FILENO 30

        int m=0;
        pid_t pid;
        while(argv[m]!=NULL){
            argp[m] = argv[m];
            m++;
        } 
        bzero(argv, sizeof(argv));
        int i=0, j=0, fir=0;
        while(argp[i] != NULL){
            // printf("line 293: %s\n", argp[i]);
            if(strstr(argp[i],">")||strstr(argp[i],">>")||strstr(argp[i],">|")){
                filenum = j+1;
                argv[j++] = argp[i++];
                argv[j++] = argp[i++];
                if(n==1 || !strcmp(argp[i-2], ">>")){ // noclobber 상관없음, append 가능, ()명령어 이미 파일 덮어쓰기 가능 판별하고 들어옴
                    shell(31,1);
                }
                else if(n==0 && noclobber==1 && !strcmp(argp[i-2],">") && (access(argp[i-2], F_OK) != -1)){ // 덮어쓰기 불가(noclobber=1), >, 파일이 있는 경우
                    printf("덮어쓰기 불가\n");
                }
                else{ // 덮어쓰기 가능
                    shell(31,0);
                }
                close(fd[1]); // write close
            }
            else if(!strcmp(argp[i],"|")){
                pipe(fd);
                shell(30,0);
                close(fd[1]); // write close
                ptmp = fd[0];
                bzero(argv, sizeof(argv));
                j=0;
                i++;
            }
            else if(argp[i+1] == NULL){ // 마지막 명령어
                argv[j] = argp[i];
                pipe(fd);
                shell(31,3);
                j=0;
                i++;
            }
            else{
                argv[j] = argp[i];
                i++;
                j++;
            }

        }
    }
}


int main(){
    char tmp[1024];
    bzero(path, 1024);
    getcwd(tmp, 1024);
    strcat(path, tmp);
    strcat(path, "/history.txt");
    while(1){
        pflag = 0;
        bzero(command, 256);
        bzero(argv, 256);
        bzero(argc, 256);
        bzero(argp, 256);
        printf("\n$ ");
        fgets(command, sizeof(command), stdin);
        command[strlen(command)-1] = '\0'; 
        history_save();
        backflag = 0;
        background = 0;
        // !일때 !가 아닌 정상 command가 나올 때까지 반복
        while(strstr(command,"!")){ 
            int line;
            int i=0;
            char *ptr = strtok(command, "!");
            line = atoi(ptr);
            history(line);
        }
        if(strlen(command) == 0) goto EXIT;
        // command space로 분리해서 argv에 저장
        int i=0;
        char commands[256];
        strcpy(commands,command); // 분리할 경우 command 망가지므로 copy
        char *ptr = strtok(command, " ");
        while(ptr != NULL){
            argv[i++] = ptr;
            ptr = strtok(NULL, " ");
        }

        if(strstr(commands, "&")){ // & 개수 count
            int i=0;
            while(argv[i] != NULL){
                if(!strcmp(argv[i++],"&")){
                    background++;
                }
            }
        }
        // printf("background: %d\n", background); 

        if(!strcmp(argv[0], "set") && !strcmp(argv[1],"+o") && !strcmp(argv[2], "noclobber")){ // off noclobber
            printf("off noclobber\n");
            noclobber = 0;
            goto EXIT;
        }
        else if(!strcmp(argv[0], "set") && !strcmp(argv[1],"-o") && !strcmp(argv[2], "noclobber")){ // on noclobber
            printf("on noclobber\n");
            noclobber = 1;
            goto EXIT;
        }
        else if(!strcmp(argv[0], "set") && !strcmp(argv[1],"+C")){ // off noclobber
            printf("off noclobber\n");
            noclobber = 0;
            goto EXIT;
        }
        else if(!strcmp(argv[0], "set") && !strcmp(argv[1],"-C")){ // on noclobber
            printf("on noclobber\n");
            noclobber = 1;
            goto EXIT;
        }

        // redirection이 있는 경우
        if(strstr(commands,">")||strstr(commands,">>")||strstr(commands,">|")||strstr(commands,"<")||strstr(commands,"|")){
            // printf("redirection exist\n");
            // date; cd ../; ls > out.txt
            // (date; cd ../; ls) > out.txt
            // (date; cd ../; ls) > out.txt; pwd
            // (date; cd ../; ls) > out.txt; pwd > out2.txt 
            // pwd; (date; cd ../; ls) > out.txt
            // pwd > out.txt ; pwd > out2.txt
            // cd ../ > out.txt
            // ls | sort > test.txt
            // ls | sort | head -5 > test.txt
            // (ls | sort; ls) > test.txt
            // ()여부
            if(strstr(commands, "(")){ // case 2, 3, 4, 5
                // 처음부터 돌면서 ; 탐색
                // 분리한 문자열에 (있으면 )탐색해서 그 위치 + 2해서 파일 이름 파악
                // () 안에 실행하면서 각각의 실행 내용 저장
                // ()로 묶여진 명령어 아닐 경우 ; 나올때 까지 저장
                int k=0;
                bzero(argc, sizeof(argc));
                while(argv[k] != NULL){ // 분리한 명령어 argv에 저장위해 argc에 argv 복사
                    argc[k] = argv[k];
                    k++;
                }
                int rd = 0; // redirection 명령어 포함 여부
                int i=0, j=0;
                char *ptr; // argc위치
                bzero(argv, sizeof(argv)); // initialize argv
                while(argc[i] != NULL){
                    if(strstr(argc[i], "(")){ // ()부분, 전부 실행한후 break
                        int tmp=i, fidx;
                        char fname[256] = {"\0",};
                        while(!strstr(argc[tmp], ")")) {
                            tmp++;
                        }
                        if(argc[tmp+1] != NULL){
                            if(strstr(argc[tmp+1],">")||strstr(argc[tmp+1],">>")||strstr(argc[tmp+1],">|")||strstr(argc[tmp+1],"<")||strstr(argc[tmp+1],"|")){
                                fidx = tmp+2; // 파일명이 들어간 idx 저장
                                rd = 1;
                            }
                        }
                        // printf("rd: %d\n", rd);
                        // char file[256] = argc[fidx];
                        // ; 기준으로
                        if(rd==1){
                            strcpy(fname, argc[fidx]);
                            if(strstr(argc[fidx],"&")) backflag = 1;
                            if(argc[fidx+1] != NULL){
                                if(strstr(argc[fidx+1],"&")) backflag = 1;  
                            }
                            while(i <= tmp){
                                if(noclobber==1 && !strcmp(argc[fidx-1], ">") && access(argc[fidx], F_OK) != -1){
                                    printf("덮어쓰기 불가");
                                    if( backflag == 1 ){
                                        if(strstr(fname, ";")) i=fidx+1; // file&;
                                        else if(argc[fidx+1] != NULL){ // file &; || file& ;
                                            if(strstr(argc[fidx+1],";")) i=fidx+2;
                                            else if(argc[fidx+2] == NULL) i=fidx+2;
                                        }
                                        else if(argc[fidx+2] != NULL){ // file & ;
                                            if(strstr(argc[fidx+2], ";")) i=fidx+3;  
                                            else i=fidx+2;
                                        }
                                        else i=fidx+1;
                                    }
                                    else{ //backflag = 0
                                        if(strstr(fname, ";")) i=fidx+1; // file;
                                        else if(argc[fidx+1] != NULL){ // file ;
                                            if(strstr(argc[fidx+1],";")) i=fidx+2;
                                        }
                                        else i=fidx+1;
                                    }
                                    j=0;
                                    bzero(argv, sizeof(argv));
                                    rd=0;
                                }
                                else if(strstr(argc[i], "(")){ // ( 포함이면 떼고 저장
                                    ptr = strtok(argc[i], "(");
                                    argc[i] = ptr;
                                    if(strstr(argc[i], ";")){
                                        ptr = strtok(argc[i], ";");
                                        argv[j] = ptr;
                                        j++;
                                        argv[j++] = argc[tmp+1];
                                        if(strstr(argc[fidx], "&")){ // filename&;
                                            ptr = strtok(argc[fidx], "&");
                                            argv[j++] = ptr;
                                        }
                                        else if(strstr(argc[fidx], ";")){ //filename;
                                            ptr = strtok(argc[fidx], ";");
                                            argv[j++] = ptr;
                                        }
                                        else argv[j++] = argc[fidx]; // filename
                                        if(!strstr(argv[0], "cd")) pflag = 1;
                                        rdandpipe(0);
                                        bzero(argv, sizeof(argv));
                                        i++;
                                        j=0;
                                        
                                    }
                                    else{                                
                                        argv[j] = ptr;
                                        i++;
                                        j++;
                                    }
                                }
                                // flename;
                                // filename ;
                                // filename&;
                                // filename& ;
                                // filename & ;
                                // filename &;
                                else if(strstr(argc[i], ")")){ // ) 포함이면 떼고 저장, rdandpipe 실행
                                    ptr = strtok(argc[i], ")");
                                    argv[j] = ptr;
                                    j++;
                                    argv[j++] = argc[tmp+1]; // redirection 저장
                                    if(strstr(argc[fidx], ";")){
                                        ptr = strtok(argc[fidx], ";");
                                        argv[j++] = ptr;
                                    }
                                    else if(strstr(argc[fidx], "&")){
                                        ptr = strtok(argc[fidx], "&");
                                        argv[j++] = ptr;
                                    }
                                    else argv[j++] = argc[fidx];
                                    if(pflag) rdandpipe(1);
                                    else {
                                        rdandpipe(0);
                                        if(!strstr(argv[0], "cd")) pflag = 1;
                                    }
                                    if(backflag == 1){
                                        if(strstr(fname, ";")) i=fidx+1;
                                        else if(argc[fidx+1] != NULL){
                                            if(strstr(argc[fidx+1], ";")) i=fidx+2; 
                                            else if(strstr(argc[fidx+1],"&")) i=fidx+2;
                                        }
                                        else if(argc[fidx+2] != NULL){
                                            if(strstr(argc[fidx+2], ";")) i=fidx+3; 
                                        }
                                        else i=fidx+1;
                                    }
                                    else{ // backflag =0
                                        if(strstr(fname, ";")) i=fidx+1;
                                        else if(argc[fidx+1] != NULL){
                                            if(strstr(argc[fidx+1],";")) i=fidx+2;
                                        }
                                        else i=fidx+1;
                                    }
                                    bzero(argv, sizeof(argv));
                                    j=0;
                                    rd=0;
                                    pflag=0;
                                }
                                else if(strstr(argc[i], ";")){ // ;을 포함하고 있는 경우 
                                    ptr = strtok(argc[i], ";");
                                    argv[j] = ptr;
                                    j++;
                                    argv[j++] = argc[tmp+1];    
                                    if(strstr(argc[fidx], ";")){
                                        ptr = strtok(argc[fidx], ";");
                                        argv[j++] = ptr;
                                    }
                                    else argv[j++] = argc[fidx];
                                    if(pflag) rdandpipe(1);
                                    else{
                                        rdandpipe(0);
                                        if(!strstr(argv[0], "cd")) pflag = 1;
                                    }
                                    bzero(argv, sizeof(argv));
                                    i++;
                                    j=0;
                                }
                                else{ // ; 미포함 () 미포함
                                    argv[j++] = argc[i++];
                                }
                            }
                        }
                        else if(rd==0){
                            int tmp=i;
                            while(!strstr(argc[tmp], ")")) tmp++;
                            if(argc[tmp] != NULL && strstr(argc[tmp], "&")) backflag = 1;
                            if(argc[tmp+1] != NULL){
                                if(strstr(argc[tmp+1], "&")) backflag = 1;
                            }
                            int pip=0;
                            while(i<=tmp){
                                if(strstr(argc[i], "|")) pip = 1;
                                if(strstr(argc[i], "(")){
                                    ptr = strtok(argc[i], "(");
                                    argc[i] = ptr;
                                }
                                if(strstr(argc[i], ")")){ //)일때
                                    ptr = strtok(argc[i], ")");
                                    argv[j] = ptr;
                                    if(pip==1){
                                        rdandpipe(0); // redirection 없음
                                        pip=0;
                                    }
                                    else shell(0,0);
                                    bzero(argv, sizeof(argv));
                                    j=0;
                                    if(argc[i+1] != NULL){
                                        if(strstr(argc[i+1],"&")) i = i+2;
                                        else i++;
                                    }
                                    else if(backflag == 1) i++;
                                    else i++;
                                    if(background > 0){
                                        background--;
                                    }
                                    backflag=0;
                                }
                                else if(strstr(argc[i], ";")){
                                    ptr = strtok(argc[i], ";");
                                    argv[j] = ptr;
                                    if(pip==1){
                                        rdandpipe(0); // redirection 없음
                                        pip=0;
                                    }
                                    else shell(0,0);
                                    bzero(argv, sizeof(argv));
                                    j=0;
                                    i++;
                                }
                                else{
                                    if(strlen(argc[i])>0) argv[j] = argc[i];
                                    j++;
                                    i++;
                                }
                            }
                            bzero(argv, sizeof(argv));
                        }
                    }
                    else if(strstr(argc[i], ";") || argc[i+1] == NULL || i>=sizeof(argc)){ // ;이거나 배열 끝이거나 명령어 끝일떄 (pwd; test) > txt; f &
                        if(strstr(argc[i], "&")) backflag = 1;
                        else if(argc[i+1] != NULL){
                            if(strstr(argc[i+1], "&")) backflag = 1;
                        }
                        // printf("%s,,,,", argc[i]);
                        if(strstr(argc[i], ";")){
                            ptr = strtok(argc[i], ";");
                            argv[j] = ptr;
                        }
                        if(strstr(argc[i], "&")){
                            ptr = strtok(argc[i], "&");
                            argv[j] = ptr;
                        }
                        else argv[j] = argc[i];
                        if(rd == 1){
                            rdandpipe(0);
                            if(backflag == 1){
                                if(strstr(argc[i], "&")) i++;
                                else if(argc[i+1] != NULL){
                                    if(strstr(argc[i+1], "&")) i = i+2;
                                    else i++;
                                }
                            }
                            else i++;
                        }
                        else{ // redirection없는 경우
                            shell(0,0);
                            if(strstr(argc[i], "&")) i++;
                            else if(argc[i+1] != NULL){
                                if(strstr(argc[i+1], "&")) i = i+2;
                                else i++;
                            }
                            else i++;
                        }
                        bzero(argv, sizeof(argv));
                        j=0;
                        backflag=0;
                        rd=0;
                    }
                    else{ // ()가 아닌부분
                        argv[j] = argc[i];
                        if(strstr(argv[j],">")||strstr(argv[j],">>")||strstr(argv[j],">|")||strstr(argv[j],"<")||strstr(argv[j],"|")){
                                rd = 1;
                            }
                        i++;
                        j++;
                    }
                }

            }
            else{ // commands안에 ()가 없음 redirection은 있음 
                // ; 있으면 분리 후 redirection 검사
                if(strstr(commands, ";")){ // case 1
                // ls &; pwd > test.txt 
                // pwd > test.txt; ls &  
                    int i=0;
                    int j=0;
                    int k=0;
                    while(argv[k] != NULL){ // argc에 argv저장
                        argc[k] = argv[k];
                        k++;
                    } // k는 NULL위치
                    bzero(argv, sizeof(argv));
                    int rd = 0;
                    while(argc[i] != NULL){ // argc가지고 ; 찾아내서 argv 변경
                        // printf("argc[i]: %s\n", argc[i]);
                        if(strstr(argc[i], ";") || argc[i+1] == NULL || i>=sizeof(argc)){ // ;이거나 배열 끝이거나 명령어 끝일때
                            if(strstr(argc[i], "&")) backflag = 1;
                            if(strstr(argc[i], ";")){
                                if(!strstr(argc[i], "&")){
                                    ptr = strtok(argc[i], ";");
                                    argv[j] = ptr;
                                }
                                else argv[j] = NULL;
                            }
                            else {
                                if(!strstr(argc[i], "&")) argv[j] = argc[i];
                                else argv[j] = NULL;
                            }
                            if(rd == 1){
                                rdandpipe(0);
                            }
                            else{ // redirection없는 경우
                                shell(0,0);
                            }
                            bzero(argv, sizeof(argv)); //다음 명령 실행 위해 argv초기화
                            i++;
                            j=0;
                            rd=0;
                            backflag = 0;
                        }
                        
                        else{
                            argv[j] = argc[i];
                            if(strstr(argv[j],">")||strstr(argv[j],">>")||strstr(argv[j],">|")||strstr(argv[j],"<")||strstr(argv[j],"|")){
                                rd = 1;
                            }
                            j++;
                            i++;
                        }
                    }
                }
                // ; 없으면 redirection 실행
                // pwd > test.txt &  else
                else{ // case 7
                    int k=0;
                    while(argv[k] != NULL){ // argc에 argv저장
                        k++;
                    } // k는 NULL위치
                    if(strstr(argv[k-1], "&")) {
                        backflag = 1;
                        if(!strcmp(argv[k-1], "&")) argv[k-1] = NULL;
                        else{
                            ptr = strtok(argv[k-1], "&");
                        }
                    }
                    // if(!strcmp(argv[k-1], "|")){ // |을 포함하고 있는 경우 
                    //     printf("pipe");
                    //     argv[k-1] = "\0";
                    // }
                    rdandpipe(0);
                    backflag = 0;
                }
            }
        }
        // redirection이 없는 경우
        // ()처리 추가
        else{
            int k=0; 
            bzero(argc, sizeof(argc));
            while(argv[k] != NULL){ // argc에 argv저장
                argc[k] = argv[k];
                k++;
            }
            bzero(argv, sizeof(argv));
            int i=0, j=0;
            while(argc[i] != NULL){ // argc가지고 ; 찾아내서 argv 변경
                // if(argc[i] != NULL) printf("argc[i]: %s\n", argc[i]);
                // (ls; pwd)&
                // find ./ -name history.txt -print &
                // pwd; (ls; pwd)&; ls -al
                // ls &; pwd
                // ls; (ls -al; pwd); pwd
                // (를 포함하고 있으면
                    // ) 위치 파악 후 strstr(argc[i], "&") || strstr(argc[i+1], "&") 
                        // backflag = 1
                    // ) 까지 명령어 실행, background 상 실행이면 )에서 background -1, backflag =0
                    // ) 일때 )이후로 떼고 저장
                        // shell(0,0) 실행 후
                        // backflag = 0
                        // i+1이 &나 | 포함이면 i += 2
                        // 아니면 i++
                if(strstr(argc[i], "(")){
                    int tmp=i;
                    while(!strstr(argc[tmp], ")")) tmp++;
                    if(argc[tmp] != NULL && strstr(argc[tmp], "&")) backflag = 1;
                    if(argc[tmp+1] != NULL){
                        if(strstr(argc[tmp+1], "&")) backflag = 1;
                    }
                    // printf("backflag:%d\n", backflag);
                    while(i<=tmp){
                        if(strstr(argc[i], "(")){
                            ptr = strtok(argc[i], "(");
                            argc[i] = ptr;
                        }
                        if(strstr(argc[i], ")")){ //)일때
                            ptr = strtok(argc[i], ")");
                            argv[j] = ptr;
                            shell(0,0);
                            bzero(argv, sizeof(argv));
                            j=0;
                            if(argc[i+1] != NULL){
                                if(strstr(argc[i+1],"&")) i = i+2;
                                else i++;
                            }
                            else if(backflag == 1) i++;
                            else i++;
                            if(background > 0){
                                background--;
                            }
                            backflag=0;
                        }
                        else if(strstr(argc[i], ";")){
                            ptr = strtok(argc[i], ";");
                            argv[j] = ptr;
                            shell(0,0);
                            bzero(argv, sizeof(argv));
                            j=0;
                            i++;
                        }
                        else{
                            if(strlen(argc[i])>0) argv[j] = argc[i];
                            j++;
                            i++;
                        }
                    }
                    bzero(argv, sizeof(argv));
                }
                else if(strstr(argc[i], ";") || argc[i+1] == NULL || i>=sizeof(argc)){ // ;이거나 명령어 끝인 경우 
                    if(strstr(argc[i], ";")){
                        if(strstr(argc[i], "&")){
                            ptr = strtok(argc[i], "&");
                            background--;
                            backflag =1;
                            if(strcmp(ptr,";")) argv[j] = ptr;                            
                        }
                        else{
                            ptr = strtok(argc[i], ";");
                            if(ptr != NULL) argv[j] = ptr;
                        }
                    }
                    else {
                        if(strstr(argc[i], "&")){
                            ptr = strtok(argc[i], "&");
                            background--;
                            backflag =1;
                            if(ptr != NULL) argv[j] = ptr;                            
                        }
                        else{
                            if(strlen(argc[i])>0) argv[j] = argc[i];
                        }
                    }
                    shell(0,0);
                    backflag=0;
                    bzero(argv, sizeof(argv));
                    j=0;
                    i++;
                }
                else{ // ls -l& ; ls -l & ;
                    if(strstr(argc[i], "&")){
                        ptr = strtok(argc[i], "&");
                        background--;
                        backflag =1;
                        if(ptr != NULL){
                            argv[j] = ptr;
                            j++;
                        }                            
                    }
                    else{
                        argv[j] = argc[i];
                        j++;
                    }
                    i++;
                }
            }
        }
        EXIT: ;
    }
}
