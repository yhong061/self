#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdio.h>
#include <netinet/in.h>

int hy_getIp_gethostbyname(void)
{
    char  **pptr;
    struct hostent *hptr;
    char   str[32];

    char hostname[20] = {0};
    gethostname(hostname,sizeof(hostname));
    printf(" gethostbyname for host:%s\n", hostname);
	
    if((hptr = gethostbyname(hostname)) == NULL)
    {
        printf(" gethostbyname error for host:%s\n", hostname);
        return 0;
    }

    printf("official hostname:%s\n",hptr->h_name);
    for(pptr = hptr->h_aliases; *pptr != NULL; pptr++)
        printf(" alias:%s\n",*pptr);

    switch(hptr->h_addrtype)
    {
        case AF_INET:
        case AF_INET6:
            pptr=hptr->h_addr_list;
            for(; *pptr!=NULL; pptr++)
                printf(" address:%s\n",inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
            printf(" first address: %s\n",inet_ntop(hptr->h_addrtype, hptr->h_addr, str, sizeof(str)));
            //printf("%s\n",inet_ntoa(*(struct in_addr*)(hptr->h_addr)));
        break;
        default:
            printf("unknown address type\n");
        break;
    }

    return 0;
}

int hy_getIp_SIOCGIFADDR(void)
{
        int inet_sock;
        struct ifreq ifr;
        inet_sock = socket(AF_INET, SOCK_DGRAM, 0);

        strcpy(ifr.ifr_name, "eth0");
        if (ioctl(inet_sock, SIOCGIFADDR, &ifr) <  0)
                perror("ioctl");
        printf("%s\n", inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));
        return 0;
}

int main(void)
{
	//hy_getIp_gethostbyname();
	hy_getIp_SIOCGIFADDR();

	return 0;
}
