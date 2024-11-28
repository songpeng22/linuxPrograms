// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <time.h>

#define PORT 8080
#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

#if 1
// 精确到毫秒
void get_current_time(char *buffer, size_t size) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *tm_info = localtime(&tv.tv_sec);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
    snprintf(buffer + strlen(buffer), size - strlen(buffer), ".%03ld", tv.tv_usec / 1000); // 添加毫秒
}
#else
void get_current_time(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t); // 格式化日期和时间
}
#endif

// 设置 socket 为非阻塞
int set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl get flags");
        return -1;
    }
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int server_fd, new_socket, epoll_fd;
    struct sockaddr_in address;
    int opt = 1, addrlen = sizeof(address);
    struct epoll_event ev, events[MAX_EVENTS];
    // message
    char timestamp[30]; // 用于存储格式化的日期和时间
    // job
    int job_count = 0;

    // 创建 socket 并绑定
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    // 设置服务器 socket 为非阻塞
    set_nonblocking(server_fd);

    // 创建 epoll 实例
    epoll_fd = epoll_create1(0);
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

    printf("Server is listening on port %d...\n", PORT);

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        printf("epoll_wait ret nfds:%d\n",nfds);
        for (int i = 0; i < nfds; i++) {

            if (events[i].data.fd == server_fd) {
                // 有新连接
#if 1
                while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) > 0) {
                    ev.events = EPOLLIN | EPOLLET; // 使用边缘触发
                    ev.data.fd = new_socket;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &ev);
                    // 获取当前时间
                    get_current_time(timestamp, sizeof(timestamp));

                    printf("%s New connection accepted\n",timestamp);
                }
#else
                new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
                if (new_socket >= 0) {
                    ev.events = EPOLLIN;
                    ev.data.fd = new_socket;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &ev);

                    // 获取当前时间
                    get_current_time(timestamp, sizeof(timestamp));

                    printf("%s New connection accepted\n",timestamp);
                }
#endif
            } else {
                // 处理客户端消息
                char buffer[BUFFER_SIZE];
                int valread = read(events[i].data.fd, buffer, BUFFER_SIZE);
                //
                if (valread <= 0) {
                    // 获取当前时间
                    get_current_time(timestamp, sizeof(timestamp));
                    // 客户端关闭连接
                    printf("%s Client disconnected\n", timestamp);
                    close(events[i].data.fd);
                } else {
                    buffer[valread] = '\0';
                    // 获取当前时间
                    get_current_time(timestamp, sizeof(timestamp));
                    printf("%s Received: %s\n", timestamp, buffer);
                    job_count = job_count + 1;
                    //sleep(1);
                    // 获取当前时间
                    //get_current_time(timestamp, sizeof(timestamp));
                    //printf("%s job done.\n", timestamp);
                }
            }
        }
        printf("processing job list:\n");
        while(job_count){
            get_current_time(timestamp, sizeof(timestamp));
            printf("+%s job_count:%d is processing\n", timestamp, job_count);
            sleep(1);
            get_current_time(timestamp, sizeof(timestamp));
            printf("-%s job_count:%d is processing\n", timestamp, job_count);
            job_count = job_count - 1;
            if(0 == job_count){
                // 获取当前时间
                get_current_time(timestamp, sizeof(timestamp));
                printf("%s job done.\n", timestamp);
            }
        }
    }

    close(server_fd);
    return 0;
}
