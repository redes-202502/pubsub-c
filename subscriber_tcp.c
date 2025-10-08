#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char** argv) {
    if (argc<4) {
        fprintf(stderr, "Uso: %s <broker_ip> <puerto> <tema1> [tema2 ...]\n", argv[0]);
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s<0) { perror("socket"); return 1; }
    struct sockaddr_in addr; memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr)<=0) { perror("inet_pton"); return 1; }
    if (connect(s, (struct sockaddr*)&addr, sizeof(addr))<0) { perror("connect"); return 1; }

    // recibir bienvenida
    char wb[512]; recv(s, wb, sizeof(wb), 0);

    // enviar SUB por cada tema
    for (int i=3;i<argc;i++) {
        char line[256];
        snprintf(line, sizeof(line), "SUB %s\n", argv[i]);
        send(s, line, strlen(line), 0);
    }

    // recibir indefinidamente
    char buf[2048];
    while (1) {
        ssize_t r = recv(s, buf, sizeof(buf)-1, 0);
        if (r<=0) break;
        buf[r]='\0';
        fputs(buf, stdout);
        fflush(stdout);
    }
    close(s);
    return 0;
}
