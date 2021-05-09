#include "csapp.h"
#include "sbuf.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_CACHE_OBJ_NUM 10

#define NTHREADS 4
#define SBUFSIZE 16

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";


typedef struct {
    int readcnt;                /* Initially = 0 */
    int LRU;
    int is_empty;
    sem_t mutex;                /* protects accesses to readcnt */
    sem_t w;                    /* protects accesses to cache */
    char uri[MAXLINE];         /* store uri */
    char obj[MAX_OBJECT_SIZE]; /* store object from server */

} cache_block;
/* $end cache */

typedef struct {
    cache_block cacheobjs[MAX_CACHE_OBJ_NUM];    /* 10 cache blocks */
} cache_t;

void doit(int fd);
void parse_uri(char *uri, char *host, char *filepath, int *port);
void *thread(void *vargp);
void clienterror(int fd, char *cause, char *errnum, char *shotrmsg, char *longmsg);
void build_header(char *header, char *hostname, char *path, rio_t *client_rio);
void cache_init(cache_t *cache);
int  cache_find(cache_t *cache, char *uri);
int  cache_eviction(cache_t *cache);
void cache_store(cache_t *cache, char *uri, char *buf);
void update_lru(cache_t *cache, int i);
void read_pre(cache_t *cache, int i);
void read_after(cache_t *cache, int i);
void write_pre(cache_t *cache, int i);
void write_after(cache_t *cache, int i);

sbuf_t sbuf;
cache_t cache;

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    printf("%s", user_agent_hdr);

    listenfd = Open_listenfd(argv[1]);

    sbuf_init(&sbuf, SBUFSIZE);
    cache_init(&cache);
    for (int i = 0; i < NTHREADS; i++)
        Pthread_create(&tid, NULL, thread, NULL);

    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        sbuf_insert(&sbuf, connfd);
    }
    return 0;
}

void *thread(void *vargp) {
    Pthread_detach(pthread_self());
    while (1) {
        int connfd = sbuf_remove(&sbuf);
        doit(connfd);
        Close(connfd);
    }
}

int connect_server(char *hostname, int port) {
    char buf[100];
    sprintf(buf, "%d", port);
    return Open_clientfd(hostname, buf);
}

void doit(int fd) {
    int port = 80;
    char hostname[MAXLINE], filepath[MAXLINE];
    char buf[MAXLINE];
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char forward_hdr[MAXLINE];
    char uri_store[MAXLINE];

    rio_t client_io, server_io;    

    Rio_readinitb(&client_io, fd);
    rio_readlineb(&client_io, buf, MAXLINE);
    printf("Request headers:\n");
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);

    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not implemented", "Proxy does not implement this method");
        return;
    }

    strcpy(uri_store, uri);
    int cache_index;
    if ((cache_index = cache_find(&cache, uri)) != -1) {
        update_lru(&cache, cache_index);
        read_pre(&cache, cache_index);
        Rio_writen(fd, cache.cacheobjs[cache_index].obj, strlen(cache.cacheobjs[cache_index].obj));
        read_after(&cache, cache_index);
        return;
    }

    parse_uri(uri, hostname, filepath, &port);
    build_header(forward_hdr, hostname, filepath, &client_io);

    int server_fd = connect_server(hostname, port);
    if (server_fd < 0) {
        printf("cannot connect to server\n");
        return;
    }

    Rio_readinitb(&server_io, server_fd);
    Rio_writen(server_fd, forward_hdr, strlen(forward_hdr));

    size_t n;
    char cache_buf[MAX_OBJECT_SIZE];
    size_t buf_size = 0;
    while ((n = Rio_readlineb(&server_io, buf, MAXLINE)) != 0) {
        printf("proxy received %ld bytes to send to client %d\n", n, fd);

        buf_size += n;
        if (buf_size < MAX_OBJECT_SIZE) {
            strcat(cache_buf, buf);
        }

        Rio_writen(fd, buf, n);
    }
    Close(server_fd);

    if (buf_size < MAX_OBJECT_SIZE) {
        cache_store(&cache, uri_store, cache_buf);
    }
}

void parse_uri(char *uri, char *host, char *filepath, int *port) {
    char *str = strstr(uri, "//");
    if (str == NULL)
        str = uri;
    else 
        str += 2;
    char *delimiter = strstr(str, ":");
    if (delimiter != NULL) {
        *delimiter = '\0';
        sscanf(str, "%s", host);
        sscanf(delimiter + 1, "%d%s", port, filepath);
    } else {
        delimiter = strstr(str, "/");
        *delimiter = '\0';
        sscanf(str, "%s", host);
        *delimiter = '/';
        sscanf(delimiter, "%s", filepath);
    }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXLINE];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "<hr><em>The Tiny Web Server</em>\r\n");

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void build_header(char *header, char *hostname, char *path, rio_t *client_rio) {
    char buf[MAXLINE], request_hdr[MAXLINE], other_hdr[MAXLINE], host_hdr[MAXLINE];
    static const char *connection_key = "Connection";
    static const char *user_agent_key= "User-Agent";
    static const char *proxy_connection_key = "Proxy-Connection";

    /* request line */
    sprintf(request_hdr, "GET %s HTTP/1.0\r\n", path);
    /* get other request header for client rio and change it */
    while (Rio_readlineb(client_rio, buf, MAXLINE) > 0) {
        if (strcmp(buf, "\r\n") == 0) {
            break;
        }

        if (!strncasecmp(buf, "Host", strlen("Host"))) {
            strcpy(host_hdr, buf);
            continue;
        }

        if (!strncasecmp(buf, connection_key, strlen(connection_key))
                && !strncasecmp(buf, proxy_connection_key, strlen(proxy_connection_key))
                && !strncasecmp(buf, user_agent_key, strlen(user_agent_key))) {
            strcat(other_hdr, buf);
        }
    }

    if (strlen(host_hdr) == 0) {
        sprintf(host_hdr, "GET %s HTTP/1.0\r\n", hostname);
    }

    sprintf(header, "%s%s%s%s%s%s%s",
        request_hdr,
        host_hdr,
        "Connection: close\r\n",
        "Proxy-Connection: close\r\n",
        user_agent_hdr,
        other_hdr,
        "\r\n");
}

void cache_init(cache_t *cache) {
    for (int i = 0; i < MAX_CACHE_OBJ_NUM; i++) {
        cache->cacheobjs[i].readcnt = 0;
        cache->cacheobjs[i].LRU = 0;
        cache->cacheobjs[i].is_empty = 1;
        sem_init(&(cache->cacheobjs[i].mutex), 0, 1);
        sem_init(&(cache->cacheobjs[i].w), 0, 1);
    }
}

/* find uri is in the cache or not */
int cache_find(cache_t *cache, char *uri) {
    int i;
    for (i = 0; i < MAX_CACHE_OBJ_NUM; i++) {
        read_pre(cache, i);
        if ((cache->cacheobjs[i].is_empty == 0) && (strcmp(uri, cache->cacheobjs[i].uri) == 0)) {
            break;
        }
        read_after(cache, i);
    }

    if (i == MAX_CACHE_OBJ_NUM) return -1;    /* can not find url in the cache */
    return i;
}

/* find an available cache */
int cache_eviction(cache_t *cache) {
    int maxx = 0;
    int index = 0;
    for (int i = 0; i < MAX_CACHE_OBJ_NUM; i++) {
        read_pre(cache, i);
        if (cache->cacheobjs[i].is_empty == 1) {
            index = i;
            read_after(cache, i);
            break;
        }

        if (cache->cacheobjs[i].LRU > maxx) {
            maxx = cache->cacheobjs[i].LRU;
            index = i;
        }
        read_after(cache, i);
    }
    return index;
}

void cache_store(cache_t *cache, char *uri, char *buf) {
    int i = cache_eviction(cache);

    write_pre(cache, i);

    strcpy(cache->cacheobjs[i].uri, uri);
    strcpy(cache->cacheobjs[i].obj, buf);
    cache->cacheobjs[i].is_empty = 0;
    cache->cacheobjs[i].LRU = 0;

    write_after(cache, i);
    update_lru(cache, i);

}

/* update the LRU number except the new cache one */
void update_lru(cache_t *cache, int index) {
    for(int i = 0; i< MAX_CACHE_OBJ_NUM; i++)    {
        if (i == index) 
            continue;
        write_pre(cache, i);
        if(cache->cacheobjs[i].is_empty == 0){
            cache->cacheobjs[i].LRU++;
        }
        write_after(cache, i);
    }
}

void read_pre(cache_t *cache, int i) {
    P(&cache->cacheobjs[i].mutex);
    cache->cacheobjs[i].readcnt++;
    if (cache->cacheobjs[i].readcnt == 1) {   /* first in */
        P(&cache->cacheobjs[i].w);
    }
    V(&cache->cacheobjs[i].mutex);
}

void read_after(cache_t *cache, int i) {
    P(&cache->cacheobjs[i].mutex);
    cache->cacheobjs[i].readcnt--;
    if (cache->cacheobjs[i].readcnt == 0) {   /* Last out */
        V(&cache->cacheobjs[i].w);
    }
    V(&cache->cacheobjs[i].mutex);
}

void write_pre(cache_t *cache, int i) {
    P(&cache->cacheobjs[i].w);
}

void write_after(cache_t *cache, int i) {
    V(&cache->cacheobjs[i].w);
}