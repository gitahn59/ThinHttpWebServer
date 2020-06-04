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
#include <fcntl.h>
#include <signal.h>

#define CLISIZE 3
#define MAXHTTPHEADSIZE 4096
#define MAXCLIENTSIZE 50

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

// mutex
pthread_mutex_t m_lock;

char *MIME[5][2] = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".gif", "image/gif"}};

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
* 프로그램이 Usage에 맞게 실행되었는지 확인한다
* 정상적으로 실행되었으면 port와 service direcotry를 설정하고 안내문을 출력한다
* 비정상적으로 실행되었으면 usage를 출력하고 종료한다
*
* @param argc : 명령행 인자의 개수
* @param argv : 명령행 인자
*/
void init(int argc, char **argv);

/**
* http request를 받으면 요청에 대한
* 응답을 리턴하는 쓰레드
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
* filename에 로직을 처리한다
*
* @param filename : 파일명
* @param sd : client socket descriptor
*/
void handleFileRequest(char *filename, Client c);

/**
* sd 에 파일의 data를 보낸다
*
* @param filename : 파일명
* @param sd : client socket descriptor
*/
int sendFileData(char *filename, int sd);

/**
* stderr에 eroor message를 출력하고 프로그램을 종료한다.
*
* @param message : error message
*/
void error_handling(char *message);

/**
* total.cgi의 파라메터를 이용해 사이의 합을 리턴한다
*
* @param parm : total.cgi의 파라메터
*/
long long getSum(char *parm);

/**
* 로그를 작성한다
*
* @param path : 전송한 파일명
* @param len : 전송한 크기
*/
void writeLog(char *ip, char *path, int len);

// count of client
int clientCnt = 0;

int port;      // server port
char dir[255]; // service directory
int dirlen;    // len of dir
int logfile;
char logdata[200];

int main(int argc, char **argv)
{
    int i;                       // loop variable
    int sock;                    // server socket descriptor
    Client *c;                   // new client socket descriptor
    struct sockaddr_in sin, cli; // server and client socket
    int clientlen = sizeof(cli);
    int optvalue = 1;
    pthread_t tid;

    signal(SIGPIPE, SIG_IGN);

    // port, service dir 초기화
    init(argc, argv);

    logfile = open("log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (logfile == -1)
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

    // multi client의 접속을 처리하기 위한 while loop
    // while loop : s
    while (1)
    {
        c = (Client *)malloc(sizeof(Client));
        // accept client
        c->sd = accept(sock, (struct sockaddr *)&cli, &clientlen);
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
        if (pthread_create(&tid, NULL, response, (void *)c) < 0)
        {
            perror("thread create error");
            exit(1);
        }
        pthread_detach(tid);
    }
    // while  loop : e

    // close sd
    close(sock);
    close(logfile);

    return 0;
}

char *getMIME(char *name)
{
    int i;
    for (i = 0; i < 5; i++)
    {
        if (strstr(name, MIME[i][0]))
        {
            return MIME[i][1];
        }
    }

    return NULL;
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

    // 프로그램 안내문 출력
    printf("================================\n");
    printf("Simple_Web_Server\n");
    printf("    Service directory : %s\n    port : %d\n", dir, port);
    printf("================================\n\n");
}

void *response(void *nsd)
{
    struct Header hd;
    // num of client
    Client c = *(Client *)nsd;
    char path[255];

    char header[MAXHTTPHEADSIZE];
    int str_len; // length of request

    pthread_mutex_lock(&m_lock); // mutex lock
    clientCnt++;
    pthread_mutex_unlock(&m_lock); // mutex unlock

    // Http Request 수신
    str_len = recv(c.sd, header, MAXHTTPHEADSIZE - 1, 0);
    header[str_len] = 0; // header 끝 표시

    // header 파싱
    parseHeader(header, &hd);

    // 파일 경로 생성
    strcpy(path, dir);
    strcat(path, hd.path);

    handleFileRequest(path, c);
    // close client descriptor
    close(c.sd);
    // free
    free(nsd);

    pthread_mutex_lock(&m_lock); // mutex lock
    clientCnt--;
    pthread_mutex_unlock(&m_lock); // mutex unlock
}

void parseGetRequest(char *path, GetRequest *req)
{
    char *c = strstr(path, "?");
    if (c == NULL) // 파라미터가 없으면
    {
        strcpy(req->path, path);
        req->parameters[0] = '\0';
    }
    else // 파라미터가 있으면
    {
        // 분리해서 저장
        strncpy(req->path, path, c - path);
        strcpy(req->parameters, c + 1);
    }
}

void parseHeader(char *header, struct Header *hd)
{
    char temp[10];
    sscanf(header, "%s %s %s %s %s", hd->type, hd->path, hd->version, temp, hd->host);
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
        sended += write(c.sd, rst, strlen(rst));
        writeLog(c.ip, path, sended);
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
        sended += write(c.sd, rst, strlen(rst));
        // Http Response body 송신
        sended += sendFileData(req.path, c.sd);
        writeLog(c.ip, req.path, sended);
    }
    else
    { // index.html도 존재 하지 않으면
        sprintf(rst, "HTTP/1.1 200 OK\r\nContent-Type: %s \r\n\r\n <html><body>Not found</body></html>", "text/html");
        sended += send(c.sd, rst, strlen(rst), 0);
        writeLog(c.ip, path, sended);
    }
}

int sendFileData(char *filename, int sd)
{
    struct stat buf; // 파일 정보
    int rfd, size, n, sended = 0;
    char data[BUFSIZ];
    stat(filename, &buf);

    rfd = open(filename, O_RDONLY);
    if (rfd == -1)
    {
        perror("Open");
        exit(1);
    }

    size = (int)buf.st_size;
    while ((n = read(rfd, data, BUFSIZ)) > 0)
    {
        sended += write(sd, data, n);
    }
    if (n == -1)
    {
        perror("Read");
        exit(1);
    }
    close(rfd);
    return sended;
}

void error_handling(char *message)
{
    // stderr에 error message 출력
    fputs(message, stderr);
    fputc('\n', stderr);
    // exit
    exit(1);
}

/**
* total.cgi의 파라메터를 이용해 사이의 합을 리턴한다
*
* @param parm : total.cgi의 파라메터
*/
long long getSum(char *parm)
{
    long long l, r, temp;
    sscanf(parm, "from=%lld&to=%lld", &l, &r);

    return ((r * (r + 1)) / 2) - (((l - 1) * l) / 2);
}

void writeLog(char *ip, char *path, int len)
{
    pthread_mutex_lock(&m_lock); // mutex lock
    sprintf(logdata, "%s %s %d\n", ip, path, len);
    write(logfile, logdata, strlen(logdata));
    pthread_mutex_unlock(&m_lock); // mutex unlock
}