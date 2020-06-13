#ifndef UTILITY
#define UTILITY

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

/**
* Http Requeest Header 구조체
*
* sd : client의 소켓 디스크립터
* ip : clinet ip
*/
typedef struct myClient
{
    int sd;
    char *ip;
} Client;

/**
* Http Requeest Header 구조체
*
* path : client가 요청한 파일 경로
* parameters : client가 요청한 파라미터
*/
typedef struct getRequest
{
    char path[256], parameters[256];
} GetRequest;

/**
* 파일 이름에 맞는 MIME를 리턴한다
*
* @param name : file name
*/
char *getMIME(char *name);

/**
* Get 요청을 파싱하여 경로와 파라미터를 저장하는 GetRequest 구조체를 초기화한다
*
* @param path : 파일 경로
* @param req : path를 파일경로와 파라미터로 분리한 결과를 저장할 변수
*/
void parseGetRequest(char *path, GetRequest *req);

/**
* total.cgi의 파라미터를 파싱하여 두 자연수 사이의 합을 리턴한다
*
* @param parm : total.cgi의 파라미터
*/
long long getSum(char *parm);

/**
* client가 요청한 경로에 따라 결과를 리턴한다.
*
* @param path : 요청 경로
* @param found : 요청 경로에 대해 탐색한 결과; return 참고
*
* return
0 : NOTFOUND
1 : FOUND; found에 파일 경로 저장
2 : cgi; found에 total.cgi의 파라미터 저장
*/
int findFile(char *path, char *found);

/**
* sd 에 filename data를 보낸다
*
* @param filename : 전송할 파일의 이름
* @param sd : client socket descriptor
*/
int sendFileData(char *filename, int sd);

/**
* 로그를 작성한다
*
* @param fd : log를 작성할 파일의 디스크립터
* @param ip : 클라이언트 ip
* @param path : 전송한 파일명
* @param len : 전송한 크기
* @param lock : 뮤텍스 변수
*/
void writeLog(int fd, char *ip, char *path, int len, pthread_mutex_t *lock);

#endif