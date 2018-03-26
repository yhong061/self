#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

#define SERVER_PORT  (12345)


int GetServerAddr(char * addrname)
{
    printf("please input server addr:");
    scanf("%s",addrname);
    return 1;
}

int main(int argc,char **argv)
{
    int cli_sockfd;
    int len;
    socklen_t addrlen;
    struct sockaddr_in cli_addr;
    char buffer[256];

    //Step1. 建立UDP类型的socket
    cli_sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if(cli_sockfd<0)
    {
        printf("I cannot socket success\n");
        return 1;
    }

    //Step2. 绑定服务端地址
    char seraddr[14];
    GetServerAddr(seraddr);
    addrlen=sizeof(struct sockaddr_in);
    bzero(&cli_addr,addrlen);
    cli_addr.sin_family=AF_INET;
    cli_addr.sin_addr.s_addr=inet_addr(seraddr);
    cli_addr.sin_port=htons(SERVER_PORT);

    //Step3. 发送消息给服务端
    printf("please input send contexts:\n");
    bzero(buffer,sizeof(buffer));
    /* 从标准输入设备取得字符串*/
    len=read(STDIN_FILENO,buffer,sizeof(buffer));
    /* 将字符串传送给server端*/
    sendto(cli_sockfd,buffer,len,0,(struct sockaddr*)&cli_addr,addrlen);

    //Step4.接收server端返回的字符串
    len=recvfrom(cli_sockfd,buffer,sizeof(buffer),0,(struct sockaddr*)&cli_addr,&addrlen);
    printf("receive from %s\n",inet_ntoa(cli_addr.sin_addr));

    printf("receive: %s",buffer);
    close(cli_sockfd);
}

