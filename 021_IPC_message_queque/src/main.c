#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#define MAX_TEXT 512

struct msg_buffer {
    long msg_type;
    char text[MAX_TEXT];
};

int main() {
    key_t key = ftok("progfile", 65); // 创建唯一键
    int msgid = msgget(key, 0666 | IPC_CREAT); // 创建消息队列

    struct msg_buffer message;
    message.msg_type = 1;
    strcpy(message.text, "Hello, World!");

    msgsnd(msgid, &message, sizeof(message), 0); // 发送消息
    printf("Message sent: %s\n", message.text);

    msgrcv(msgid, &message, sizeof(message), 1, 0); // 接收消息
    printf("Message received: %s\n", message.text);

    msgctl(msgid, IPC_RMID, NULL); // 删除消息队列
    return 0;
}
