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
*/
struct Header
{
    char type[10], path[256], version[20], host[100];
};

typedef struct myClient
{
    int sd;
    char *ip;
} Client;

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
*/
void parseGetRequest(char *path, GetRequest *req);

/**
* http request header 문자열로부터
* Header 구조체를 생성한다
*
* @param header : http request header
* @param hd : header
*/
void parseHeader(char *header, struct Header *hd);

/**
* total.cgi의 파라메터를 이용해 사이의 합을 리턴한다
*
* @param parm : total.cgi의 파라메터
*/
long long getSum(char *parm);

/**
* sd 에 파일의 data를 보낸다
*
* @param filename : 파일명
* @param sd : client socket descriptor
*/
int sendFileData(char *filename, int sd);

/**
* 로그를 작성한다
*
* @param fd : log를 작성할 파일의 디스크립터
* @param path : 전송한 파일명
* @param len : 전송한 크기
*/
void writeLog(int fd, char *ip, char *path, int len, pthread_mutex_t* lock);

#endif