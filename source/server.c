#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

#define CLISIZE 3
#define MAXHTTPHEADSIZE 4096

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
* stderr에 eroor message를 출력하고 프로그램을 종료한다.
*
* @param message : error message
*/
void error_handling(char *message);

// count of client
int clientCnt = 0;

// client socket descriptor list
int clientList[CLISIZE];

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

    // wait all chat thread
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
    // num of client
    int sd = *(int *)nsd;

    char header[MAXHTTPHEADSIZE];
    int str_len; // length of request

    // mutex lock
    pthread_mutex_lock(&m_lock);

    // client로 부터 message 수신
    str_len = recv(sd, header, MAXHTTPHEADSIZE - 1, 0);

    // message 끝 표시
    header[str_len] = 0;
    // stdout에 message 출력
    fputs(header, stdout);
    send(sd, header, strlen(header), 0);
    // mutex unlock
    pthread_mutex_unlock(&m_lock);
    // close client descriptor
    close(sd);
    // free
    free(nsd);
}

void error_handling(char *message)
{
    // stderr에 error message 출력
    fputs(message, stderr);
    fputc('\n', stderr);
    // exit
    exit(1);
}