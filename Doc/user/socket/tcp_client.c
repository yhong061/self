#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <errno.h>

#define SERVER_PORT  (12345)
#define MAX_MSG_SIZE  (1024)

int GetServerAddr(char * addrname)
{
    printf("please input server addr:\n");
    scanf("%s",addrname);
    printf("addrname = %s\n", addrname);
    return 1;
}

int main()
{
    struct sockaddr_in cli_addr;/* 客户端的地址*/
    char msg[MAX_MSG_SIZE];/* 缓冲区*/
    char seraddr[14];
    struct sockaddr_in ser_addr;

    //Step1. 创建TCP的SOCKET
    int cli_sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(cli_sockfd<0)
    {/*创建失败 */
        fprintf(stderr,"socker Error:%s\n",strerror(errno));
        exit(1);
    }
    //Step2. 绑定客户端地址
    bzero(&ser_addr,  sizeof(ser_addr));
    cli_addr.sin_family=AF_INET;
    cli_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    cli_addr.sin_port=0;
    if(bind(cli_sockfd,(struct sockaddr*)&cli_addr, sizeof(ser_addr))<0)
    { 
        fprintf(stderr,"Bind Error:%s\n",strerror(errno));
        exit(1);
    }

    //Step3. 连接到服务器地址
    GetServerAddr(seraddr);  /* 串口输入服务器的地址*/
    bzero(&ser_addr, sizeof(ser_addr));
    ser_addr.sin_family=AF_INET;
    ser_addr.sin_addr.s_addr=inet_addr(seraddr);
    ser_addr.sin_port=htons(SERVER_PORT);
    if(connect(cli_sockfd,(struct sockaddr*)&ser_addr,  sizeof(ser_addr))!=0)/*请求连接*/
    {
        /*连接失败 */
        fprintf(stderr,"Connect Error:%s\n",strerror(errno));
        close(cli_sockfd);
        exit(1);
    }

    //Step4. 发送消息到服务器
    strcpy(msg,"hi,I am client!");
    send(cli_sockfd,msg,sizeof(msg),0);/*发送数据*/

    //Step5. 从服务器接收消息
    recv(cli_sockfd,msg,MAX_MSG_SIZE,0); /* 接受数据*/
    printf("%s\n",msg);/*在屏幕上打印出来 */

    //Step6. 关闭客户端本地连接
    close(cli_sockfd);
}

