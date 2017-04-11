
multi_udp_server:

文件编译：gcc -Wall multi_udp_server.c -o server 

运行：./server 230.1.1.1 7838

================================================================

multi_udp_clinet:

编译：gcc -Wall multi_udp_clinet.c -o clinet

运行：./clinet 230.1.1.1 7838 192.168.1.121 5500