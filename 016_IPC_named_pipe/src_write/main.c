#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <limits.h> //defines PIPE_BUF
#include <time.h>   //add time to pipe message

//#define FIFO_NAME "/tmp/fifo_01"
#define FIFO_NAME "./fifo_01"

void get_current_time(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t); // 格式化日期和时间
}

int main() {
    int count_w = 0;
    int count_r = 0;
    // buffer
    char buffer[PIPE_BUF + 1];
    int bytes_write = 0;
    // message
    char message[256];
    char full_message[300];
    char timestamp[20]; // 用于存储格式化的日期和时间
    // pipe
    int pipe_fd = -1;
    // status    
    bool connected = false;
    bool remote_child = false;
    int ret = 0;
#if 1
    if (access(FIFO_NAME, F_OK) == -1)
    {
        // 管道文件不存在
        // 创建命名管道
        ret = mkfifo(FIFO_NAME, 0666); // 创建FIFO，权限为0666
        if (ret != 0)
        {
            fprintf(stderr, "Could not create fifo %s\n", FIFO_NAME);
            exit(EXIT_FAILURE);
        }
        else{
            printf("mkdifo success.\n");
        }
    }
    else{
        printf("fifo already exist.\n");
        // 检查是否有孤儿进程已经在使用这个pipe
        pipe_fd = open(FIFO_NAME, O_WRONLY | O_NONBLOCK);
        if (pipe_fd == -1) {
            printf("Failed to open FIFO on nonblock open, remote child not exist\n");
            remote_child = false;
        }
        else{
            printf("Connected to existing child process.\n");
            remote_child = true;
            connected = true;
        }
    }
#else
    int fd[2];
    pipe(fd); // 创建管道
#endif
#if 1
    if( !connected ){
        printf("+open().\n");
        pipe_fd = open(FIFO_NAME, O_WRONLY);
        if (pipe_fd == -1) {
            printf("Failed to open FIFO for writing\n");
            remote_child = false;
        }
        else{
            printf("Succeed to open FIFO for writing\n");
            remote_child = true;
        }
    }
    char * text = "message from write side";
    strcpy(message,text);
    // 获取当前时间
    get_current_time(timestamp, sizeof(timestamp));
    // 创建带时间戳的消息
    snprintf(full_message, sizeof(full_message), "[%s] %s", timestamp, message);
    int len = strlen(full_message); 
    ret = write(pipe_fd, full_message, len);
    printf("write \"%s\"\n",full_message);
#if 0
    pid_t pid = -1;
    if(!remote_child){
        pid = fork();
    }
    if( !remote_child && 0 == pid  ){
        //child process
        printf("child process\n");
    }

    if( !remote_child && pid > 0  ){
        //parent process
        printf("parent process\n");
    }
#endif
    close(pipe_fd); // 关闭写入端
#else
    if (fork() == 0) { // 子进程
        close(fd[1]); // 关闭写入端
        char buffer[100];
        while(true){
            read(fd[0], buffer, sizeof(buffer)); // 从管道读取
            printf("Received: %s\n", buffer);
            count_r = count_r + 1;
            if(count_r >= 5 )
                break;
        }
        printf("close pipe and return.\n");
        close(fd[0]);
    } else { // 父进程
        close(fd[0]); // 关闭读取端
        while(1){
            count_w = count_w + 1;
            if(count_w <=3)
                write(fd[1], "Hello, IPC!", 12); // 向管道写入

            sleep(1);
        }
        close(fd[1]);
    }
#endif
    return 0;
}
