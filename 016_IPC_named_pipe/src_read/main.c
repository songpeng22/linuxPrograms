#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <limits.h> //defines PIPE_BUF

//#define FIFO_NAME "/tmp/fifo_01"
#define FIFO_NAME "./fifo_01"

int main() {
    int count_w = 0;
    int count_r = 0;
    int open_mode = O_RDONLY;
    char buffer[PIPE_BUF + 1];
    bool remote_child = false;
    int ret = 0;

#if 1

#elif 1
    // 创建命名管道
    int ret = mkfifo(FIFO_NAME, 0666); // 创建FIFO，权限为0666
    if (ret != 0)
    {
        fprintf(stderr, "Could not create fifo %s\n", FIFO_NAME);
        exit(EXIT_FAILURE);
    }
    else{
        printf("mkdifo success.\n");
    }
#else
    int fd[2];
    pipe(fd); // 创建管道
#endif
#if 1
    // 尝试打开 FIFO 进行写入，以检查是否已经有孤儿进程在使用
    printf("+open().\n");
    int pipe_fd = open(FIFO_NAME, open_mode);
    if (pipe_fd == -1) {
        printf("Failed to open FIFO for reading\n");
        remote_child = false;
    }
    else{
        printf("Succeed to open FIFO for reading\n");
        remote_child = true;
    }
    while(true){
        ret = read(pipe_fd,buffer,PIPE_BUF);
        if(ret > 0){
            printf("Received:%d:%s\n",ret,buffer);
        }
        sleep(1);
    }
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
