#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h> // definitions of structures needed for sockets, e.g. sockaddr
#include <sys/types.h> // definitions of a number of data types used in socket.h and netinet/in.h
#include <netinet/in.h> // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h> // eixt(), atoi()
#include <strings.h> 
#include <string.h> // bzero(), strncpy(), strstr()
#include <fcntl.h> // open()

int main(int argc, char *argv[]){

    int sockfd, newsockfd; // server socket, client socket의 socket descriptor 값 저장
    int portno; // 포트 번호
    socklen_t clilen; // struct sockaddr_in의 크기 저장

    char buffer[1024]; // request message 저장
    int fd; // 요청한 파일의 file descriptor 저장
    char text[4096]; // 요청한 파일에서 읽은 값 저장
    int m; // 요청한 파일에서 읽어온 바이트 수 저장

    struct sockaddr_in server_addr, client_addr; // sockaddr_in : 인터넷 주소를 포함하는 구조체

    if(argc < 2){ // ./server [portnumber] 형식으로 파일을 실행했는지 확인
        fprintf(stderr, "ERROR, no port provided\n"); // 올바른 형식이 아닐경우 에러 출력
        exit(1);// 종료
    }

    // socket 생성
    sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); // PF_INET: IPv4 인터넷 프로토콜 체계, SOCK_STREAM: TCP, IPPROTO_TCP: TCP 소켓 생성
    if(sockfd < 0){ // socket 생성 실패할 경우
        perror("ERROR opening socket"); // 에러메세지 출력
        exit(1); // 종료
    }

    // 주소 정보에 해당하는 IP, port번호를 socket에 할당
    bzero((char *) &server_addr, sizeof(server_addr)); // server_addr을 size(server_addr)만큼 0으로 채움
    portno = atoi(argv[1]); // 입력한 포트 번호 portno에 저장
    server_addr.sin_family = AF_INET; // 프로토콜에 따라 주소 체계 선택 -> IPv4 인터넷 프로토콜에 적용하는 주소 체계
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 서버의 IP주소를 찾아 자동으로 대입(INADDR_ANY)한 후 네트워크 바이트 순서로 변경해서 저장
    server_addr.sin_port = htons(portno); //포트번호를 네트워크 바이트 순서로 변경
    if(bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr))<0){ // 소켓에 IP번호와 포트번호를 할당함으로써 통신에 사용할 수 있도록 준비
        perror("ERROR on binding"); //에러 발생시 에러메세지 출력
        exit(1); // 종료
    }

    if(listen(sockfd, 5) < 0){ // 클라이언트 연결 요청 가능 상태로 변경, 최대 5개까지 대기시킬 수 있음
        perror("EROOR on listen"); //에러 발생시 에러메세지 출력
        exit(1); // 종료
    }

    while(1){
        clilen = sizeof(client_addr); //clien_addr구조체의 크기 저장

        newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, &clilen); // 클라이언트 연결 요청 수락
        if(newsockfd < 0){ // accept()에러 발생 시
            perror("ERROR on accept"); // 에러 메세지 출력
            exit(1); // 종료
        }

        int n; // newsockfd에서 읽어온 바이트 수
        bzero(buffer, 1024); // buffer를 0으로 채움
        n = read(newsockfd, buffer, 1024); // newsockfd에서 1024만큼 읽어서 buffer에 씀
        if(n<0){ // read()에러 발생시
            perror("ERROR reading"); // 에러 메세지 출력
            exit(1);// 종료
        }
        
        int i=5; // 클라이언트가 요청한 파일명의 시작 인덱스
        while(buffer[i] > 32) i++; // 파일명이 끝나는 지점까지 i 위치 변경

        printf("%s", buffer); // request message 출력

        char tmp[1024] = {'\0',}; // 파일 이름 저장할 변수
        strncpy(tmp, &buffer[5], i-5); // buffer에서 파일이름 추출
        
        fd = open(tmp, O_RDONLY); // 파일 열기
        if(fd >= 0){ // 요청한 파일이 있는 경우

            // 요청한 파일의 타입에 따라 response-header 구분
            if(strstr(tmp, ".html")){ //html 파일인 경우
                if(write(newsockfd, "HTTP/1.1 200 OK\r\nContent-Type: text/html;charset=UTF-8\r\n\r\n", 58) == -1){ // response-header 작성 실패 시
                    perror("write error: "); // 에러 메세지 출력
                    exit(1); // 종료
                }
            }
            else if(strstr(tmp, ".pdf")){ //pdf 파일인 경우
                if(write(newsockfd, "HTTP/1.1 200 OK\r\nContent-Type: application/pdf\r\n\r\n", 50) == -1){
                    perror("write error: ");
                    exit(1);
                }
                
            }
            else if(strstr(tmp, ".gif")){ //gif파일인 경우
                if(write(newsockfd, "HTTP/1.1 200 OK\r\nContent-Type: image/gif\r\n\r\n", 44) == -1){
                    perror("write error: ");
                    exit(1);
                }
            }
            else if(strstr(tmp, ".jpeg")||strstr(tmp,".jpg")){ //jpeg || jpg
                if(write(newsockfd, "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\n\r\n", 45) == -1){
                    perror("write error: ");
                    exit(1);
                }
            }
            else if(strstr(tmp, ".mp3")){ //mp3
                if(write(newsockfd, "HTTP/1.1 200 OK\r\nContent-Type: audio/mpeg\r\n\r\n", 46) == -1){
                    perror("write error: ");
                    exit(1);
                }
            }
            else{ // 파일 형식이 pdf, jpg, jpeg, html, mp3가 아닌 경우
                if(write(newsockfd, "HTTP/1.1 404 NOT FOUND\r\nContent-Type: text/html\r\n\r\n", 52) == -1){ // request-header 404 작성
                perror("write error: "); // write()에러 발생 시 에러 메세지 출력 후
                exit(1); // 종료
                }
                else{
                    if(write(newsockfd, "<HTML><BODY><H1><pre></pre><pre></pre>404 NOT FOUND</H1></BODY></HTML>\n",72) == -1){ // request-body 작성
                        perror("write error: "); // write()에러 발생 시 에러 메세지 출력 후
                        exit(1); // 종료
                    }
                }
            }
            m = read(fd,text,4095); // 요청한 파일을 4095바이트만큼 읽어 text에 저장 후 읽은 바이트 수 m에 저장
            if(m>=0){ // 파일 읽기 성공
                if(write(newsockfd, text, m) == -1){ // 읽은 파일 newsockfd에 작성, 에러 발생시
                    perror("write error: "); // 에러 메세지 출력
                    exit(1); // 종료
                }
                while(1){ // 요청한 파일 내용 읽고 쓰기 반복
                    m=read(fd,text,4095); // 요청한 파일을 4095바이트만큼 읽어 text에 저장 후 읽은 바이트 수 m에 저장
                    if(m>0){ // 읽어온 내용이 있을 때
                        if(write(newsockfd, text, m) == -1){ // 읽어온 내용 newsockfd에 작성, 에러 발생 시
                            perror("read error: "); // 에러 메세지 출력
                            exit(1); // 종료
                        }
                    }
                    else if(m==-1){ // read()에러 발생시
                        perror("read error: "); // 에러 메세지 출력
                        exit(1); // 종료
                    }
                    else { // 더이상 읽을 내용이 없는 경우
                        break; // while문 빠져나감
                    } 
                } 
            }
            else{ // read() 에러 발생시
                perror("read error: "); // 에러 메세지 출력 후
                exit(1); // 종료
            }
            close(fd); // 요청한 파일 닫음
        }
        else{ // 요청한 파일이 없는 경우 
            if(write(newsockfd, "HTTP/1.1 404 NOT FOUND\r\nContent-Type: text/html\r\n\r\n", 52) == -1){ // request-header 404 작성
                perror("write error: "); // write()에러 발생 시 에러 메세지 출력 후
                exit(1); // 종료
            }
            else{
                if(write(newsockfd, "<HTML><BODY><H1><pre></pre><pre></pre>404 NOT FOUND</H1></BODY></HTML>\n",72) == -1){ // request-body 작성
                    perror("write error: "); // write()에러 발생 시 에러 메세지 출력 후
                    exit(1); // 종료
                }
            }
        }
        close(newsockfd); // 클라이언트 소켓 닫음
    }
    close(sockfd); // 서버 소켓 닫음
    
}
