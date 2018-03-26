#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <errno.h>

#define SERVER_PORT  (12345)
#define MAX_MSG_SIZE  (1024)

int main()
{ 
    int sock_fd, cli_sockfd; /*sock_fd：监听socket；client_fd：数据传输socket */ 
    struct sockaddr_in cli_addr; /* 客户端地址信息 */ 
    char msg[MAX_MSG_SIZE];/* 缓冲区*/

    //Step1. 创建TCP的SOCKET
    int ser_sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(ser_sockfd<0)
    {
        fprintf(stderr,"socker Error:%s\n",strerror(errno));
        exit(1);
    } 

    //Step2. 将本机的Ip和port绑定到这个ser_sockfd中
    struct sockaddr_in ser_addr;
    int addrlen=sizeof(struct sockaddr_in);
    bzero(&ser_addr,addrlen);
    ser_addr.sin_family=AF_INET;
    ser_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    ser_addr.sin_port=htons(SERVER_PORT);
    if(bind(ser_sockfd,(struct sockaddr*)&ser_addr,sizeof(struct sockaddr_in))<0)
    { /*绑定失败 */
        fprintf(stderr,"Bind Error:%s\n",strerror(errno));
        exit(1);
    } 

    //Step3. 启动ser_sockfd的监听功能
    if(listen(ser_sockfd, 20)<0)  //20表示最大允许20个用户请求排队
    {
        fprintf(stderr,"Listen Error:%s\n",strerror(errno));
        close(ser_sockfd);
        exit(1);
    }
    while(1)
    {
        //Step4. 等待接收客户连接请求*/
        cli_sockfd=accept(ser_sockfd,(struct sockaddr*) &cli_addr,&addrlen);
        if(cli_sockfd<=0)
        {
            fprintf(stderr,"Accept Error:%s\n",strerror(errno));
        }
        else
        {
            //Step5. 有客户端连接，接收客户端信i息
            //ssize_t recv(int sockfd, void *buf, size_t len, int flags);
            recv(cli_sockfd,msg,MAX_MSG_SIZE,0); 
            printf("received a connection from %sn", inet_ntoa(cli_addr.sin_addr));
            printf("%s\n",msg);/*在屏幕上打印出来 */ 
            //Step6. 发送信息给客户端
            strcpy(msg,"hi,I am server!");
            send(cli_sockfd,msg,sizeof(msg),0); 
            //Step7. 关闭客户端的连接
            close(cli_sockfd); 
        }
    }
    //Step8. 关闭服务端的连接
    close(ser_sockfd);
}



