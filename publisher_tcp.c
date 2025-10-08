#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

static void send_all(int fd, const char* s) {
    size_t n = strlen(s), off=0;
    while (off<n) {
        ssize_t r = send(fd, s+off, n-off, 0);
        if (r<=0) return;
        off += (size_t)r;
    }
}

int main(int argc, char** argv) {
    if (argc<5) {
        fprintf(stderr, "Uso: %s <broker_ip> <puerto> <tema> <num_mensajes>\n", argv[0]);
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    const char* topic = argv[3];
    int nmsgs = atoi(argv[4]);
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s<0) { perror("socket"); return 1; }
    struct sockaddr_in addr; memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr)<=0) { perror("inet_pton"); return 1; }
    if (connect(s, (struct sockaddr*)&addr, sizeof(addr))<0) { perror("connect"); return 1; }

    char recvbuf[512]; recv(s, recvbuf, sizeof(recvbuf), 0); // bienvenida 

    for (int i=1;i<=nmsgs;i++) {
        char line[1024];
        snprintf(line, sizeof(line), "PUB %s Gol #%d al minuto %d", topic, i, 30+i);
        send_all(s, line);
        send_all(s, "\n");
        printf("[PUB] enviado: %s\n", line);
        sleep(1);
    }
    close(s);
    return 0;
}
