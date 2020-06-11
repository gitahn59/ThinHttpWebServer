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

void parseHeader(char *header, struct Header *hd)
{
    char temp[10];
    sscanf(header, "%s %s %s %s %s", hd->type, hd->path, hd->version, temp, hd->host);
}

long long getSum(char *parm)
{
    long long l, r, temp;
    sscanf(parm, "from=%lld&to=%lld", &l, &r);

    return ((r * (r + 1)) / 2) - (((l - 1) * l) / 2);
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