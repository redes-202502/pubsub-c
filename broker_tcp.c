#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>

#define MAX_CLIENTS 128
#define MAX_TOPICS 128
#define MAX_TOPIC_LEN 64
#define BUF_SIZE 4096

typedef struct {
    int fd;
    int is_active;
    int is_publisher; // 1=PUB, 0=SUB
    char buf[BUF_SIZE];
    size_t buf_len;
    // para SUB, lista de temas suscritos
    char topics[16][MAX_TOPIC_LEN];
    int topic_count;
} Client;

typedef struct {
    char topic[MAX_TOPIC_LEN];
    int subs_fd[MAX_CLIENTS];
    int sub_count;
} Topic;

static Topic topics[MAX_TOPICS];
static int topic_count = 0;

static int find_or_create_topic(const char* t) {
    for (int i=0;i<topic_count;i++) {
        if (strncmp(topics[i].topic, t, MAX_TOPIC_LEN)==0) return i;
    }
    if (topic_count >= MAX_TOPICS) return -1;
    strncpy(topics[topic_count].topic, t, MAX_TOPIC_LEN-1);
    topics[topic_count].topic[MAX_TOPIC_LEN-1] = '\0';
    topics[topic_count].sub_count = 0;
    return topic_count++;
}

static void trim_newline(char* s) {
    size_t n = strlen(s);
    while (n>0 && (s[n-1]=='\n' || s[n-1]=='\r')) { s[n-1]='\0'; n--; }
}

static void to_upper(char* s) {
    for (;*s;s++) *s = (char)toupper((unsigned char)*s);
}

static void add_subscriber_to_topic(int topic_idx, int fd) {
    Topic* tp = &topics[topic_idx];
    for (int i=0;i<tp->sub_count;i++) if (tp->subs_fd[i]==fd) return;
    if (tp->sub_count < MAX_CLIENTS) tp->subs_fd[tp->sub_count++] = fd;
}

static void remove_fd_everywhere(int fd) {
    for (int i=0;i<topic_count;i++) {
        Topic* tp = &topics[i];
        int w=0;
        for (int j=0;j<tp->sub_count;j++) if (tp->subs_fd[j]!=fd) tp->subs_fd[w++]=tp->subs_fd[j];
        tp->sub_count = w;
    }
}

static void send_line(int fd, const char* line) {
    size_t len = strlen(line);
    send(fd, line, len, 0);
}

static void broadcast_to_topic(int topic_idx, const char* payload) {
    Topic* tp = &topics[topic_idx];
    for (int i=0;i<tp->sub_count;i++) {
        char out[2048];
        snprintf(out, sizeof(out), "%s\n", payload);
        send_line(tp->subs_fd[i], out);
    }
}

static void handle_command(Client* c, char* line) {
    //"SUB topic"  |  "PUB topic mensaje..."
    trim_newline(line);
    if (!*line) return;

    char* cmd = strtok(line, " ");
    if (!cmd) return;

    char upper[16]; strncpy(upper, cmd, sizeof(upper)-1); upper[sizeof(upper)-1]='\0'; to_upper(upper);

    if (strcmp(upper, "SUB")==0) {
        c->is_publisher = 0;
        char* topic = strtok(NULL, ""); // resto
        if (!topic) return;
        while (*topic==' ') topic++;
        if (!*topic) return;
        if (c->topic_count < 16) {
            strncpy(c->topics[c->topic_count], topic, MAX_TOPIC_LEN-1);
            c->topics[c->topic_count][MAX_TOPIC_LEN-1]='\0';
            int idx = find_or_create_topic(topic);
            if (idx>=0) add_subscriber_to_topic(idx, c->fd);
            c->topic_count++;
            char ack[256]; snprintf(ack, sizeof(ack), "[BROKER] SUB OK %s\n", topic);
            send_line(c->fd, ack);
        }
    } else if (strcmp(upper, "PUB")==0) {
        c->is_publisher = 1;
        char* topic = strtok(NULL, " ");
        char* rest = strtok(NULL, "");
        if (!topic || !rest) return;
        while (*rest==' ') rest++;
        int idx = find_or_create_topic(topic);
        if (idx<0) return;
        char msg[2048];
        snprintf(msg, sizeof(msg), "[%s] %s", topic, rest);
        broadcast_to_topic(idx, msg);
    } else {
        char err[256]; snprintf(err, sizeof(err), "[BROKER] ERR comando desconocido\n");
        send_line(c->fd, err);
    }
}

static void handle_rx(Client* c) {
    // leer disponible y procesar por líneas
    char tmp[1024];
    ssize_t r = recv(c->fd, tmp, sizeof(tmp), 0);
    if (r<=0) {
        // desconexión
        c->is_active = 0;
        close(c->fd);
        remove_fd_everywhere(c->fd);
        return;
    }
    if (c->buf_len + (size_t)r > sizeof(c->buf)) r = (ssize_t)(sizeof(c->buf) - c->buf_len);
    memcpy(c->buf + c->buf_len, tmp, (size_t)r);
    c->buf_len += (size_t)r;

    // procesar líneas
    size_t start = 0;
    for (size_t i=0;i<c->buf_len;i++) {
        if (c->buf[i]=='\n') {
            char line[1024];
            size_t len = i - start + 1;
            if (len >= sizeof(line)) len = sizeof(line)-1;
            memcpy(line, c->buf+start, len);
            line[len]='\0';
            handle_command(c, line);
            start = i+1;
        }
    }
    if (start>0) {
        memmove(c->buf, c->buf+start, c->buf_len - start);
        c->buf_len -= start;
    }
}

int main(int argc, char** argv) {
    int port = 5000;
    if (argc>=2) port = atoi(argv[1]);
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s<0) { perror("socket"); return 1; }
    int opt=1; setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr; memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr))<0) { perror("bind"); return 1; }
    if (listen(s, 64)<0) { perror("listen"); return 1; }

    printf("[BROKER TCP] Escuchando en puerto %d\n", port);
    Client clients[MAX_CLIENTS]; memset(clients, 0, sizeof(clients));

    while (1) {
        fd_set rfds; FD_ZERO(&rfds);
        FD_SET(s, &rfds);
        int maxfd = s;
        for (int i=0;i<MAX_CLIENTS;i++) if (clients[i].is_active) {
            FD_SET(clients[i].fd, &rfds);
            if (clients[i].fd>maxfd) maxfd=clients[i].fd;
        }
        int r = select(maxfd+1, &rfds, NULL, NULL, NULL);
        if (r<0) { if (errno==EINTR) continue; perror("select"); break; }

        if (FD_ISSET(s, &rfds)) {
            // aceptar
            struct sockaddr_in caddr; socklen_t clen=sizeof(caddr);
            int cfd = accept(s, (struct sockaddr*)&caddr, &clen);
            if (cfd>=0) {
                int placed = 0;
                for (int i=0;i<MAX_CLIENTS;i++) if (!clients[i].is_active) {
                    clients[i].fd = cfd;
                    clients[i].is_active = 1;
                    clients[i].is_publisher = -1;
                    clients[i].buf_len = 0;
                    clients[i].topic_count = 0;
                    placed = 1; break;
                }
                if (!placed) { close(cfd); }
                else {
                    char welcome[256];
                    snprintf(welcome, sizeof(welcome),
                             "[BROKER] Conectado. Use 'SUB <tema>' o 'PUB <tema> <mensaje>'\n");
                    send_line(cfd, welcome);
                }
            }
        }
        for (int i=0;i<MAX_CLIENTS;i++) if (clients[i].is_active && FD_ISSET(clients[i].fd, &rfds)) {
            handle_rx(&clients[i]);
        }
    }
    close(s);
    return 0;
}
