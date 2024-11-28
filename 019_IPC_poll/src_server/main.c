// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1, addrlen = sizeof(address);
    struct pollfd fds[MAX_CLIENTS];
    int nfds = 1; // 初始只有服务器 socket
    int msg_count = 0;

    // 创建 socket 并绑定
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    // 将服务器 socket 添加到 pollfd
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    printf("Server is listening on port %d...\n", PORT);

    while (1) {
        // 调用 poll 等待事件
        int poll_count = poll(fds, nfds, -1);
        printf("poll_count:%d\n",poll_count);
        if (poll_count < 0) {
            perror("poll error");
            exit(EXIT_FAILURE);
        }

        // 检查是否有新连接
        if (fds[0].revents & POLLIN) {
            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (new_socket >= 0) {
                fds[nfds].fd = new_socket;
                fds[nfds].events = POLLIN;
                nfds++;
                printf("New connection accepted, nfds:%d\n", nfds);
            }
        }

        // 处理客户端消息
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                char buffer[BUFFER_SIZE];
                int valread = read(fds[i].fd, buffer, BUFFER_SIZE);
                if (valread <= 0) {
                    // 客户端关闭连接
                    printf("[%d] Client disconnected\n",i);
                    close(fds[i].fd);
                    // 将已关闭的 socket 替换为最后一个 socket
                    fds[i] = fds[nfds - 1];
                    nfds--;
                } else {
                    buffer[valread] = '\0';
                    msg_count++;
                    printf("[%d] Received: %s, count:%d\n", i, buffer, msg_count);
                    // misson is conducting
                    sleep(2);
                    // 这里可以处理收到的消息，向客户端发送响应等
                    //char * message = "message received.";
                    //send(fds[i].fd, message, strlen(message), 0);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
