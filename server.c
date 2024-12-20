#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 3334
#define MAX_CLIENTS 10

typedef struct {
    int sock;
    struct sockaddr_in addr;
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast_message(const char *message) {
    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < client_count; i++) {
        send(clients[i].sock, message, strlen(message), 0);
    }
    pthread_mutex_unlock(&client_mutex);
}

void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    char buffer[1024];
    while (1) {
        int len = recv(client_sock, buffer, sizeof(buffer), 0);
        if (len <= 0) break;

        buffer[len] = '\0';
        printf("Received: %s\n", buffer);
        broadcast_message(buffer);
    }

    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].sock == client_sock) {
            clients[i] = clients[--client_count];
            break;
        }
    }
    pthread_mutex_unlock(&client_mutex);

    close(client_sock);
    printf("Client disconnected.\n");
    return NULL;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, MAX_CLIENTS) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock == -1) {
            perror("Accept failed");
            continue;
        }

        pthread_mutex_lock(&client_mutex);
        if (client_count >= MAX_CLIENTS) {
            close(client_sock);
            pthread_mutex_unlock(&client_mutex);
            continue;
        }
        clients[client_count].sock = client_sock;
        clients[client_count].addr = client_addr;
        client_count++;
        pthread_mutex_unlock(&client_mutex);

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, &client_sock);
        pthread_detach(tid);
        printf("Client connected.\n");
    }

    close(server_sock);
    return 0;
}
