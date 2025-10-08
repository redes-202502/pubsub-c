#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_TOPIC_LEN 64
#define MAX_SUBS 1024
#define BUF_SZ 2048

typedef struct {
    struct sockaddr_in addr;
    char topic[MAX_TOPIC_LEN];
    int active;
} Subscriber;

static Subscriber subs[MAX_SUBS];

static int sockaddr_equal(const struct sockaddr_in* a, const struct sockaddr_in* b) {
    return a->sin_addr.s_addr == b->sin_addr.s_addr && a->sin_port == b->sin_port;
}

static void add_subscriber(const struct sockaddr_in* who, const char* topic) {
    // si ya existe con el mismo topic y addr, no duplicar
    for (int i=0;i<MAX_SUBS;i++) if (subs[i].active) {
        if (sockaddr_equal(&subs[i].addr, who) && strncmp(subs[i].topic, topic, MAX_TOPIC_LEN)==0)
            return;
    }
    for (int i=0;i<MAX_SUBS;i++) if (!subs[i].active) {
        subs[i].active = 1;
        subs[i].addr = *who;
        strncpy(subs[i].topic, topic, MAX_TOPIC_LEN-1);
        subs[i].topic[MAX_TOPIC_LEN-1] = '\0';
        return;
    }
}

int main(int argc, char** argv) {
    if (argc<2) {
        fprintf(stderr, "Uso: %s <puerto>\n", argv[0]);
        return 1;
    }
    int port = atoi(argv[1]);
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s<0) { perror("socket"); return 1; }
    int opt=1; setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr; memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr))<0) { perror("bind"); return 1; }

    printf("[BROKER UDP] Escuchando en puerto %d\n", port);
    char buf[BUF_SZ];

    while (1) {
        struct sockaddr_in cli; socklen_t clen = sizeof(cli);
        ssize_t r = recvfrom(s, buf, sizeof(buf)-1, 0, (struct sockaddr*)&cli, &clen);
        if (r<=0) continue;
        buf[r]='\0';

        //"SUB tema" o "PUB tema mensaje..."
        if (strncmp(buf, "SUB ", 4)==0) {
            const char* topic = buf+4;
            while (*topic==' ') topic++;
            if (*topic) {
                add_subscriber(&cli, topic);
                char ack[256];
                snprintf(ack, sizeof(ack), "[BROKER] SUB OK %s", topic);
                sendto(s, ack, strlen(ack), 0, (struct sockaddr*)&cli, clen);
            }
        } else if (strncmp(buf, "PUB ", 4)==0) {
            const char* p = buf+4;
            while (*p==' ') p++;
            // topic hasta el primer espacio
            char topic[MAX_TOPIC_LEN]; int ti=0;
            while (*p && *p!=' ' && ti<MAX_TOPIC_LEN-1) topic[ti++]=*p++;
            topic[ti]='\0';
            while (*p==' ') p++;
            const char* msg = p;
            if (topic[0] && *msg) {
                char out[BUF_SZ];
                snprintf(out, sizeof(out), "[%s] %s", topic, msg);
                for (int i=0;i<MAX_SUBS;i++) if (subs[i].active) {
                    if (strncmp(subs[i].topic, topic, MAX_TOPIC_LEN)==0) {
                        sendto(s, out, strlen(out), 0, (struct sockaddr*)&subs[i].addr, sizeof(subs[i].addr));
                    }
                }
            }
        } else {
            const char* err = "[BROKER] ERR";
            sendto(s, err, strlen(err), 0, (struct sockaddr*)&cli, clen);
        }
    }
    close(s);
    return 0;
}
