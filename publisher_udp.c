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
    if (argc<5) {
        fprintf(stderr, "Uso: %s <broker_ip> <puerto> <tema> <num_mensajes>\n", argv[0]);
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    const char* topic = argv[3];
    int nmsgs = atoi(argv[4]);

    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s<0) { perror("socket"); return 1; }

    struct sockaddr_in broker; memset(&broker,0,sizeof(broker));
    broker.sin_family = AF_INET; broker.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &broker.sin_addr)<=0) { perror("inet_pton"); return 1; }

    for (int i=1;i<=nmsgs;i++) {
        char line[1024];
        snprintf(line, sizeof(line), "PUB %s Gol #%d al minuto %d", topic, i, 30+i);
        ssize_t sent = sendto(s, line, strlen(line), 0, (struct sockaddr*)&broker, sizeof(broker));
        if (sent<0) perror("sendto");
        printf("[PUB/UDP] enviado: %s\n", line);
        usleep(800000); // 0.8 s
    }
    close(s);
    return 0;
}
