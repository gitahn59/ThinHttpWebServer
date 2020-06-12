#include "utility.h"

char *getMIME(char *name)
{
    char *MIME[5][2] = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".gif", "image/gif"}};

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

long long getSum(char *parm)
{
    long long l, r, temp;
    sscanf(parm, "from=%lld&to=%lld", &l, &r);

    return ((r * (r + 1)) / 2) - (((l - 1) * l) / 2);
}

int findFile(char *path, char *found)
{
    struct stat buf; // 파일정보
    int sended = 0;
    GetRequest req;
    char rst[100];
    int idx;

    parseGetRequest(path, &req); // 경로에서 파일경로와 parameter를 추출
    // total.cgi 인 경우
    if (strstr(req.path, "total.cgi") != NULL)
    {
        strcpy(found, req.parameters);
        return 2;
    }

    // 파일명 조정
    if (access(req.path, F_OK) != 0) // 파일이 존재하지 않으면
    {
        if (getMIME(req.path) == NULL) // 요청 경로가 디렉토리인 경우
        {
            //index.html로 설정
            strcat(req.path, "/index.html");
        }
        else // 요청 경로가 파일인 경우
        {
            idx = strrchr(req.path, '/') - req.path;
            // index.html로 파일명 교체
            strcpy(req.path + idx + 1, "index.html");
        }
    }
    else
    {
        stat(req.path, &buf);
        if (S_ISDIR(buf.st_mode))            // 파일은 존재하지만 디렉토리인 경우
            strcat(req.path, "/index.html"); // index.html로 설정
    }

    // 파일이 존재하면
    if (access(req.path, F_OK) == 0)
    {
        strcpy(found, req.path);
        return 1;
    }
    else
    { // index.html도 존재 하지 않으면
        return 0;
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
        sended += send(sd, data, n, 0);
    }
    if (n == -1)
    {
        perror("Read");
        exit(1);
    }
    close(rfd);
    return sended;
}

void writeLog(int fd, char *ip, char *path, int len, pthread_mutex_t* lock)
{
    char logdata[200];
    pthread_mutex_lock(lock); // mutex lock
    sprintf(logdata, "%s %s %d\n", ip, path, len);
    write(fd, logdata, strlen(logdata));
    pthread_mutex_unlock(lock); // mutex unlock
}