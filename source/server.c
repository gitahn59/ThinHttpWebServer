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
#include "trie.h"

#define MAXHTTPHEADSIZE 4096
#define MAXCLIENTSIZE 200

/**
* 파일 요청에 대한 캐시 구조체
*
* type : 캐시 타입; 0: Not Found, 1: 파일 존재함
* response_header_size : 응답헤더의 크기
* sended : 헤더를 제외한 전송 용량
* path : 파일 경로
* response_header : 응답헤더
*/
typedef struct myinfo
{
    int type, response_header_size, sended;
    char path[256];
    char response_header[200];
} Info;

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
void *accept_thread(void *sd);

/**
* http request를 받으면 요청에 대한
* 응답을 리턴하는 쓰레드
*
* @param cli : client 정보
*/
void *response_thread(void *cli);

/**
* 캐시 정보를 이용해 파일을 송신하한다
*
* @param path : 파일 경로
* @param num : 캐시 등록번호
* @param ㅊ : 클라이언트 정보
*/
void send_reserved_file(char *path, int num, Client c);

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
int logfd;     // log file descriptor

const char *NOTFOUND = "HTTP/1.1 200 OK\r\nContent-Type: text/plain \r\n\r\nNot found";
int NOTFOUNDLEN = 55;
const char *TOTAL = "HTTP/1.1 200 OK\r\nContent-Type: text/plain \r\n\r\n";
int TOTALLEN = 46;

// mutex
pthread_mutex_t m_lock;

trie *t;
int id;             // trie의 다음 등록번호
Info regist[10000]; // 파일 요청 정보에 대한 캐시

int main(int argc, char **argv)
{
    int i;    // loop variable
    int sock; // server socket descriptor

    struct sockaddr_in sin; // server and client socket
    int optvalue = 1;
    pthread_t tid;
    char command[BUFSIZ]; // 사용자 command

    signal(SIGPIPE, SIG_IGN); // 클라이언트 오류로 소켓이 끊어지면 발생하는 Signal 처리

    // port, service dir 초기화
    init(argc, argv);

    // 파일 오픈
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

    // 캐시 초기화
    t = makeTrie();
    id = 0;
    regist[0].type = 0;

    // accept thread 시작
    if (pthread_create(&tid, NULL, accept_thread, (void *)&sock) < 0)
    {
        perror("thread create error");
        exit(1);
    }

    while (1)
    {
        scanf("%s", command);
        if (strcmp(command, "q") == 0) //종료 command가 들어오면
        {
            break;
        }
    }

    // accept thread를 종료
    pthread_cancel(tid);
    pthread_join(tid, NULL);

    // 프로그램 정리
    close(sock);
    close(logfd);
    trie_free(t);

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
    if (port == 0)
    {
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

void *accept_thread(void *sd)
{
    struct sockaddr_in cli; // client socket
    int clientlen = sizeof(cli);
    Client *c; // new client
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
        if (pthread_create(&tid, NULL, response_thread, (void *)c) < 0)
        {
            perror("thread create error");
            exit(1);
        }
        pthread_detach(tid);
    }
    // while  loop : e
}

void *response_thread(void *cli)
{
    Client c = *(Client *)cli; // 클라이언트 정보
    int num, rst;
    char path[256], found[256];
    char type[20], filename[256];
    char header[MAXHTTPHEADSIZE];
    int str_len; // length of request
    int sended = 0;
    char res[200];

    // Http Request 수신
    str_len = recv(c.sd, header, MAXHTTPHEADSIZE - 1, 0);
    header[str_len] = 0; // header 끝 표시
    strcpy(type, "NONE");
    // header 파싱
    sscanf(header, "%s %s", type, filename);

    if (strcmp(type, "GET") == 0)
    {
        // 파일 경로 생성
        strcpy(path, dir);
        strcat(path, filename);

        // 요청 파일 등록정보 탐색
        num = trie_find(t, path);
        if (num >= 0) // 요청 정보가 기록되어있으면
        {
            send_reserved_file(path, num, c);
        }
        else // 처음 들어오는 요청인 경우
        {
            rst = findFile(path, found); // 요청에 해당하는 파일 탐색
            if (rst == 0)                // 파일이 존재하지 않는 경우
            {
                send(c.sd, NOTFOUND, strlen(NOTFOUND), 0);
                sended = 9;
                pthread_mutex_lock(&m_lock);
                trie_insert(t, 0, path); // trie에 문자열 등록
                pthread_mutex_unlock(&m_lock);
            }
            else if (rst == 1) // 파일이 존재하는 경우
            {
                // Http Response header 생성
                sprintf(res, "HTTP/1.1 200 OK\r\nContent-Type: %s \r\n\r\n", getMIME(found));
                // Http Response header 송신
                send(c.sd, res, strlen(res), 0);
                // Http Response body 송신
                sended += sendFileData(found, c.sd);
                pthread_mutex_lock(&m_lock);
                num = ++id;
                // 캐시 정보 등록
                regist[num].type = 1;
                strcpy(regist[num].path, found);
                strcpy(regist[num].response_header, res);
                regist[num].response_header_size = strlen(res);
                regist[num].sended = sended;
                trie_insert(t, num, path); // trie에 문자열 등록
                pthread_mutex_unlock(&m_lock);
            }
            else if (rst == 2) // total.cgi를 요청한 경우
            {
                send(c.sd, TOTAL, TOTALLEN, 0);
                sprintf(res, "%lld", getSum(found));
                sended += send(c.sd, res, strlen(res), 0);
            }
            writeLog(logfd, c.ip, path + strlen(dir), sended, &m_lock);
        }
    }

    // close client descriptor
    close(c.sd);
    // free
    free(cli);
}

void send_reserved_file(char *path, int num, Client c)
{
    if (regist[num].type == 0)
    { // 파일이 없는 경우
        send(c.sd, NOTFOUND, strlen(NOTFOUND), 0);
        writeLog(logfd, c.ip, path + strlen(dir), strlen(NOTFOUND), &m_lock);
    }
    else if (regist[num].type == 1)
    { // 파일이 있는 경우
        // Http Response header 송신
        send(c.sd, regist[num].response_header, regist[num].response_header_size, 0);
        // Http Response body 송신
        sendFileData(regist[num].path, c.sd);
        writeLog(logfd, c.ip, path + strlen(dir), regist[num].sended, &m_lock);
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
