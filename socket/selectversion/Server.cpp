//
// Created by LCX on 2020/8/21.
//

#include <netinet/in.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstdlib>
#include <stdio.h>
int main()
{
    int server_socket,client_socket;
    int server_len,client_len;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    int result;
    fd_set readfds,testfds;

    server_socket = socket(AF_INET,SOCK_STREAM,0);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htons(INADDR_ANY);
    server_address.sin_port = htons(9735);

    bind(server_socket,(struct sockaddr *)&server_address, sizeof(server_address));
    listen(server_socket,5);
    FD_ZERO(&readfds);
    FD_SET(server_socket,&readfds);//将服务器socket加入到集合中
    while (1)
    {
        char ch;
        int fd;
        int nread;
        testfds = readfds;
        printf("server waitting\n");
        /*无限期阻塞,并测试文件描述符变动*/
        result = select(FD_SETSIZE,&testfds,(fd_set *)0,(fd_set *)0,(struct timeval *)0);
        if (result < 1)
        {
            perror("server5");
            exit(1);
        }
        /*扫描所有的文件描述符*/
        for (fd = 0; fd < FD_SETSIZE; fd++) {
            /*找到相关文件描述符*/
            if(FD_ISSET(fd,&testfds))
            {
                if(fd == server_socket)
                {
                    client_len = sizeof(client_address);
                    client_socket = accept(server_socket,
                                           (struct sockaddr *)&client_address,
                                           reinterpret_cast<socklen_t *>(&client_len));
                    printf("before set readfds, value ====>%x\n",readfds.fds_bits);
                    FD_SET(client_socket,&readfds);
                    printf("adding client on fd %d,raedfds is %x\n",client_socket,readfds.fds_bits);
                    fflush(stdout);
                }
                else
                {
                    ioctl(fd,FIONREAD,&nread);
                    if (nread == 0)
                    {
                        close(fd);
                        FD_CLR(fd,&readfds);
                        printf("removing client on fd %d\n",fd);
                    }
                    else
                    {
                        read(fd,&ch,1);
                        sleep(1);
                        printf("serving client on fd %d\n",fd);
                        ch++;
                        write(fd,&ch,1);
                    }
                }

            }


        }
    }

}