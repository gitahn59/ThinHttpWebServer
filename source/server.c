#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

#define CLISIZE 3
#define MAXHTTPHEADSIZE 4096

/**
* Http Requeest Header 구조체
*/
struct Header
{
    char type[10], path[256], version[20], host[100];
};

// mutex
pthread_mutex_t m_lock;

/**
* http request를 받으면 요청에 대한
* html을 리턴하는 쓰레드
*
* @param arg : client number
*/
void *response(void *arg);

/**
* http request header 문자열로부터
* Header 구조체를 생성한다
*
* @param header : http request header
* @param hd : header
*/
void parseHeader(char *header, struct Header *hd);

/**
* file의 데이터를 가져온다
*
* @param filename : 파일명
*/
char *getFileData(char *filename);

/**
* stderr에 eroor message를 출력하고 프로그램을 종료한다.
*
* @param message : error message
*/
void error_handling(char *message);

// count of client
int clientCnt = 0;

int main(int argc, char **argv)
{
    int port;      // server port
    char dir[255]; // service directory

    int i;                       // loop variable
    int sock;                    // server socket descriptor
    int *nsd;                    // new client socket descriptor
    struct sockaddr_in sin, cli; // server and client socket
    int clientlen = sizeof(cli);
    pthread_t chat_thread[CLISIZE];
    void *thread_result;
    int optvalue = 1;

    // service directory와 port argument로 전달되지 않은 경우
    if (argc != 3)
    {
        // print 사용법
        printf("Usage : %s <service directory> <port>\n", argv[0]);
        exit(1); // 프로그램 종료
    }
    strcpy(dir, argv[1]);
    port = atoi(argv[2]);

    // 프로그램 안내문 출력
    printf("================================\n");
    printf("Simple_Web_Server\n");
    printf("    Service directory : %s\n    port : %d\n", dir, port);
    printf("================================\n\n");

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
    if (listen(sock, 5))
    {
        perror("listen");
        exit(1);
    }

    // multi client의 접속을 처리하기 위한 while loop
    // while loop : s
    while (clientCnt < CLISIZE)
    {
        nsd = (int *)malloc(sizeof(int));
        // accept client
        *nsd = accept(sock, (struct sockaddr *)&cli, &clientlen);
        if (*nsd == -1)
        {
            perror("accept");
            exit(1);
        }

        // create chat thread
        pthread_create(&chat_thread[clientCnt], NULL, response, (void *)nsd);
        clientCnt++;
    }
    // while  loop : e

    // wait all response thread
    for (i = 0; i < clientCnt; i++)
    {
        pthread_join(chat_thread[i], &thread_result);
    }

    // close sd
    close(sock);
    return 0;
}

void *response(void *nsd)
{
    struct Header hd;
    // num of client
    int sd = *(int *)nsd;
    char rst[6000];

    char header[MAXHTTPHEADSIZE];
    int str_len; // length of request

    // mutex lock
    pthread_mutex_lock(&m_lock);
    // mutex unlock
    pthread_mutex_unlock(&m_lock);

    // client로 부터 message 수신
    str_len = recv(sd, header, MAXHTTPHEADSIZE - 1, 0);

    // message 끝 표시
    header[str_len] = 0;
    // stdout에 message 출력
    // send(sd, header, strlen(header), 0);

    char *data = getFileData("html/index.html");
    sprintf(rst,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n %s", data);
    send(sd, rst, strlen(rst),0);
    free(data);

    parseHeader(header, &hd);

    // close client descriptor
    close(sd);
    // free
    free(nsd);
}

void parseHeader(char *header, struct Header *hd)
{
    char temp[10];
    sscanf(header, "%s %s %s %s %s", hd->type, hd->path, hd->version, temp, hd->host);
    fputs(hd->host, stdout);
}

char *getFileData(char *filename)
{
    struct stat buf;
    FILE *rfp;
    int size;
    char *data;

    stat(filename, &buf);

    if ((rfp = fopen(filename, "r")) == NULL){
        perror("fopen");
        exit(1);
    }

    size = (int)buf.st_size + 1;
    data = (char *)malloc(size * sizeof(char));
    fread(data, size-1, size-1,rfp);
    data[size]='\0';
    return data;
}

void error_handling(char *message)
{
    // stderr에 error message 출력
    fputs(message, stderr);
    fputc('\n', stderr);
    // exit
    exit(1);
}