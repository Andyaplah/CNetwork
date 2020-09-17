//
// Created by LCX on 2020/9/17.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(){
    int sock = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("39.99.178.131");
    serv_addr.sin_port = htons(21);
    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    char buffer[40];
    read(sock, buffer, sizeof(buffer)-1);
    printf("Message from server: %s\n",buffer);
    //返回220响应 连接上服务器

    char send_buf1[60] = "USER ftptest\r\n";
    char send_buf2[40] = "PASS qwe12345\r\n";

    write(sock, send_buf1, strlen(send_buf1));
    read(sock, buffer, sizeof(buffer)-1);
    printf("Message from server: %s\n",buffer);

    write(sock, send_buf2, strlen(send_buf2));
    read(sock, buffer, sizeof(buffer)-1);
    printf("Message from server: %s\n",buffer);
    //返回230响应 连接上服务器 登录成功

    char send_buf3[80] = "PASV\r\n";
    write(sock, send_buf3, strlen(send_buf3));
    read(sock, buffer, sizeof(buffer)-1);
    printf("Message from server: %s\n",buffer);
    //227进入被动模式

//    connect(sock,(struct sockaddr*)&serv_addr, sizeof(serv_addr));
//    char send_buf4[80] = "CWD /home/test1\r\n";
//    write(sock, send_buf4, strlen(send_buf4));
//    read(sock, buffer, sizeof(buffer)-1);
//    printf("Message from server: %s\n",buffer);

    char send_buf5[80] = "SIZE test.txt\r\n";
    write(sock, send_buf5, strlen(send_buf5));
    read(sock, buffer, sizeof(buffer)-1);
    printf("Message from server: %s\n",buffer);

    //213 48   成功  文件大小48kb
    //下载
    char send_buf6[80] = "RETR test.txt\r\n";
    write(sock, send_buf6, strlen(send_buf6));
    read(sock, buffer, sizeof(buffer)-1);
    printf("Message from server: %s\n",buffer);

    int fd=open("/tmp/test.txt",O_WRONLY|O_CREAT);
    for(;;){
        read(sock, buffer, sizeof(buffer)-1);
        write(fd, buffer, sizeof(buffer)-1);
    }
    close(fd);
    //close(sock);
    return 0;
}