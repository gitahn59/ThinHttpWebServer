#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "utility.h"

#define MAXHTTPHEADSIZE 4096
#define MAXCLIENTSIZE 50

// mutex
pthread_mutex_t m_lock;

/**
* 프로그램이 Usage에 맞게 실행되었는지 확인한다
* 정상적으로 실행되었으면 port와 service direcotry를 설정하고 안내문을 출력한다
* 비정상적으로 실행되었으면 usage를 출력하고 종료한다
*
* @param argc : 명령행 인자의 개수
* @param argv : 명령행 인자
*/
void init(int argc, char **argv);

/**
* 다수의 client가 접속하기를 계속 기다리며
* client가 접속하면
* response thread를 생성하는 쓰레드
*
* @param sd : server socket 
*/
void* accept_thread(void *sd);

/**
* http request를 받으면 요청에 대한
* 응답을 리턴하는 쓰레드
*
* @param arg : client number
*/
void *response_thread(void *arg);

/**
* filename에 로직을 처리한다
*
* @param filename : 파일명
* @param c : Client info
*/
void handleFileRequest(char *filename, Client c);

/**
* stderr에 eroor message를 출력하고 프로그램을 종료한다.
*
* @param message : error message
*/
void error_handling(char *message);

// count of client
int clientCnt = 0;

int port;      // server port
char dir[255]; // service directory
int dirlen;    // len of dir
int logfd; // log file descriptor
const char* NOTFOUND = "HTTP/1.1 200 OK\r\nContent-Type: text/html \r\n\r\n <html><body>Not found</body></html>";
int NOTFOUNDLEN = 82;

int main(int argc, char **argv)
{
    int i;    // loop variable
    int sock; // server socket descriptor

    struct sockaddr_in sin; // server and client socket
    int optvalue = 1;
    pthread_t tid;
    char str[BUFSIZ];

    signal(SIGPIPE, SIG_IGN);

    // port, service dir 초기화
    init(argc, argv);

    logfd = open("log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (logfd == -1)
    {
        perror("Open");
        exit(1);
    }

    // init mutex
    if (pthread_mutex_init(&m_lock, NULL) != 0)
    {
        perror("Mutex Init failure");
        return 1;
    }

    // tcp socket 생성
    sock = socket(PF_INET, SOCK_STREAM, 0);
    // 생성에 실패하면
    if (sock == -1)
        // error message 출력
        error_handling("socket() error");

    // init socket
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;                   // internet protocal
    sin.sin_port = htons(port);                 // set port num
    sin.sin_addr.s_addr = inet_addr("0.0.0.0"); // "0.0.0.0" listen all ip

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optvalue, sizeof(optvalue));

    // bind socket
    if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)))
    {
        perror("bind");
        exit(1);
    }

    // listen
    if (listen(sock, MAXCLIENTSIZE))
    {
        perror("listen");
        exit(1);
    }

    if (pthread_create(&tid, NULL, accept_thread, (void *)&sock) < 0)
    {
        perror("thread create error");
        exit(1);
    }

    while (1)
    {
        scanf("%s", str);
        if (strcmp(str, "q") == 0) //종료 command가 들어오면
        {
            break;
        }
    }

    // accept thread를 종료
    pthread_cancel(tid);
    pthread_join(tid, NULL);

    // close sd
    close(sock);
    close(logfd);

    return 0;
}

void init(int argc, char **argv)
{
    // service directory와 port argument로 전달되지 않은 경우
    if (argc != 3)
    {
        // print 사용법
        printf("Usage : %s <service directory> <port>\n", argv[0]);
        exit(1); // 프로그램 종료
    }
    strcpy(dir, argv[1]);
    dirlen = strlen(dir);
    port = atoi(argv[2]);
    if(port==0){
        // print 사용법
        printf("Unvalid Port number\n");
        printf("Usage : %s <service directory> <port>\n", argv[0]);
        exit(1); // 프로그램 종료
    }

    // 프로그램 안내문 출력
    printf("================================\n");
    printf("Simple_Web_Server\n");
    printf("    Service directory : %s\n    port : %d\n", dir, port);
    printf("Exit : enter q \n");
    printf("================================\n\n");
}

void* accept_thread(void *sd)
{
    struct sockaddr_in cli; // server and client socket
    int clientlen = sizeof(cli);
    Client *c; // new client socket descriptor
    pthread_t tid;

    // multi client의 접속을 처리하기 위한 while loop
    // while loop : s
    while (1)
    {
        c = (Client *)malloc(sizeof(Client));
        // accept client
        c->sd = accept(*(int *)sd, (struct sockaddr *)&cli, &clientlen);
        if (c->sd == -1)
        {
            perror("accept");
            exit(1);
        }

        c->ip = inet_ntoa(cli.sin_addr);
        // create chat thread
        while (1)
        {
            if (clientCnt < MAXCLIENTSIZE)
            {
                break;
            }
            else
                sleep(10);
        }
        if (pthread_create(&tid, NULL, response_thread, (void *)c) < 0)
        {
            perror("thread create error");
            exit(1);
        }
        pthread_detach(tid);
    }
    // while  loop : e
}

void *response_thread(void *nsd)
{
    struct Header hd;
    strcpy(hd.type,"NONE");
    // num of client
    Client c = *(Client *)nsd;
    char path[255];

    char header[MAXHTTPHEADSIZE];
    int str_len; // length of request

    // Http Request 수신
    str_len = recv(c.sd, header, MAXHTTPHEADSIZE - 1, 0);
    header[str_len] = 0; // header 끝 표시

    // header 파싱
    parseHeader(header, &hd);
    if(strcmp(hd.type, "GET")==0){
        // 파일 경로 생성
        strcpy(path, dir);
        strcat(path, hd.path);
        handleFileRequest(path, c);
    }

    // close client descriptor
    close(c.sd);
    // free
    free(nsd);
}

void handleFileRequest(char *path, Client c)
{
    struct stat buf; // 파일정보
    int sended = 0;
    GetRequest req;
    parseGetRequest(path, &req); // 경로를 파일경로와 parameter로 분리
    char rst[100];
    int idx;

    // total.cgi 인 경우
    if (strstr(req.path, "total.cgi") != NULL)
    {
        sprintf(rst, "HTTP/1.1 200 OK\r\nContent-Type: text/plain \r\n\r\n%lld", getSum(req.parameters));
        sended += send(c.sd, rst, strlen(rst),0);
        writeLog(logfd, c.ip, path, sended, &m_lock);
        return;
    }

    // 파일명 조정
    if (access(req.path, F_OK) != 0) // 존재하지 않으면
    {
        if (getMIME(req.path) == NULL)
        { // 요청 경로가 디렉토리인 경우
            // index.html로 설정
            strcat(req.path, "/index.html");
        }
        else
        { // 요청 경로가 파일인 경우
            idx = strrchr(req.path, '/') - req.path;
            strcpy(req.path + idx + 1, "index.html"); // 파일명 교체
        }
    }
    else
    { // 파일은 존재하지만 디렉토리인 경우
        stat(req.path, &buf);
        if (S_ISDIR(buf.st_mode))
        {
            strcat(req.path, "/index.html"); // index.html로 설정
        }
    }

    // 파일이 존재하면
    if (access(req.path, F_OK) == 0)
    {
        // Http Response header 생성
        sprintf(rst, "HTTP/1.1 200 OK\r\nContent-Type: %s \r\n\r\n", getMIME(req.path));
        // Http Response header 송신
        sended += send(c.sd, rst, strlen(rst),0);
        // Http Response body 송신
        sended += sendFileData(req.path, c.sd);
        writeLog(logfd, c.ip, path, sended, &m_lock);
    }
    else
    { // index.html도 존재 하지 않으면
        sended += send(c.sd, NOTFOUND, NOTFOUNDLEN, 0);
        writeLog(logfd, c.ip, path, sended, &m_lock);
    }
}

void error_handling(char *message)
{
    // stderr에 error message 출력
    fputs(message, stderr);
    fputc('\n', stderr);
    // exit
    exit(1);
}

