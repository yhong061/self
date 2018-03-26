#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <errno.h>

#define SERVER_PORT  (12345)

int main(int argc,char **argv)
{
    int ser_sockfd;
    int len;

    socklen_t addrlen;
    char seraddr[100];
    struct sockaddr_in ser_addr;

    //Step1. 建立UDP类型的socket
    ser_sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if(ser_sockfd<0)
    {
        printf("I cannot socket success\n");
        return 1;
    }
    
    //Step2. 绑定本机的IP和port到socket中
    addrlen=sizeof(struct sockaddr_in);
    bzero(&ser_addr,addrlen);
    ser_addr.sin_family=AF_INET;
    ser_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    ser_addr.sin_port=htons(SERVER_PORT);
    /*绑定客户端*/
    if(bind(ser_sockfd,(struct sockaddr *)&ser_addr,addrlen)<0)
    {
        printf("connect");
        return 1;
    }

    while(1)
    {
        //Step3. 等待客户端连接并接收客户端数据
        bzero(seraddr,sizeof(seraddr));
        len=recvfrom(ser_sockfd,seraddr,sizeof(seraddr),0,(struct sockaddr*)&ser_addr,&addrlen);
        /*显示client端的网络地址*/
        printf("receive from %s\n",inet_ntoa(ser_addr.sin_addr));
        /*显示客户端发来的字串*/ 
        printf("recevce:%s",seraddr);

        //Step4. 将字串返回给client端*/
        sendto(ser_sockfd,seraddr,len,0,(struct sockaddr*)&ser_addr,addrlen);
    }

    return 0;
}



