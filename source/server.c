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

int main(int argc, char **argv)
{
    int port;      // server port
    char dir[255]; // service directory

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

    return 0;
}