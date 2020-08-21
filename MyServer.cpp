//
// Created by LCX on 2020/8/19.
//
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <zconf.h>

int main(){
    //创建套接字
    int serv_sock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    //将套接字和IP,端口绑定
    struct sockaddr_in serv_addr;
    //每个字节都用0填充
    memset(&serv_addr,0, sizeof(serv_addr));
    //使用ipv4地址
    serv_addr.sin_family = AF_INET;
    //具体ip地址
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(1234);
    bind(serv_sock,(struct sockaddr*)&serv_addr, sizeof(serv_addr));
    //进入监听状态 等待用户发起请求
    listen(serv_sock, 20);
    //接收客户端请求
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);
    //阻塞
    int clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr,&clnt_addr_size);
    //向客户端发送数据
    char str[] = "hello world";
    write(clnt_sock,str, sizeof(str));
    //关闭套接字
    close(clnt_sock);
    close(serv_sock);

    return 0;
}
