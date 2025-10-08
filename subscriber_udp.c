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

    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s<0) { perror("socket"); return 1; }

    // bind en puerto efímero para poder recibir
    struct sockaddr_in me; memset(&me,0,sizeof(me));
    me.sin_family = AF_INET; me.sin_addr.s_addr = htonl(INADDR_ANY);
    me.sin_port = htons(0);
    if (bind(s, (struct sockaddr*)&me, sizeof(me))<0) { perror("bind"); return 1; }

    struct sockaddr_in broker; memset(&broker,0,sizeof(broker));
    broker.sin_family = AF_INET; broker.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &broker.sin_addr)<=0) { perror("inet_pton"); return 1; }

    // enviar SUB <tema> por cada argumento
    for (int i=3;i<argc;i++) {
        char line[256];
        snprintf(line, sizeof(line), "SUB %s", argv[i]);
        sendto(s, line, strlen(line), 0, (struct sockaddr*)&broker, sizeof(broker));
    }

    char buf[2048];
    while (1) {
        ssize_t r = recv(s, buf, sizeof(buf)-1, 0);
        if (r<=0) continue;
        buf[r]='\0';
        printf("%s\n", buf);
        fflush(stdout);
    }
    close(s);
    return 0;
}
