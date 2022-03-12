#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

int main(int argc,char *argv[])
{
  if (argc!=2)
  {
    printf("Using:./server port\nExample:./server 5005\n\n"); return -1;
  }

  // 第1步：创建服务端的socket。
  int listenfd;
  if ( (listenfd = socket(AF_INET,SOCK_STREAM,0))==-1) { perror("socket"); return -1; }

  // 第2步：把服务端用于通信的地址和端口绑定到socket上。
  struct addrinfo hints, *res;

  //load up address information with getaddrinfo()

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;  // use IPv4 or IPv6, whichever
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;   //fill in my IP for me

  getaddrinfo(NULL, argv[1], &hints, &res);

  /*
  struct sockaddr_in servaddr;    // 服务端地址信息的数据结构。
  memset(&servaddr,0,sizeof(servaddr));
  servaddr.sin_family = AF_INET;  // 协议族，在socket编程中只能是AF_INET。
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);          // 任意ip地址。
  //servaddr.sin_addr.s_addr = inet_addr("192.168.190.134"); // 指定ip地址。
  servaddr.sin_port = htons(atoi(argv[1]));  // 指定通信端口。

  if (bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr)) != 0 )
  { perror("bind"); close(listenfd); return -1; }
  */

  if (bind(listenfd, res->ai_addr, res->ai_addrlen) != 0 )
  { perror("bind"); close(listenfd); return -1; }

  // 第3步：把socket设置为监听模式。
  if (listen(listenfd,5) != 0 ) { perror("listen"); close(listenfd); return -1; }

  // 第4步：接受客户端的连接。
  int  new_fd;                  // new socket used for this connection

  int  socklen=sizeof(struct sockaddr_in); // struct sockaddr_in的大小
  struct sockaddr_in new_addr;  // new socket的地址信息。

  new_fd=accept(listenfd,(struct sockaddr *)&new_addr,(socklen_t*)&socklen);
  printf("客户端（%s）已连接。\n",inet_ntoa(new_addr.sin_addr));

  // 第5步：与客户端通信，接收客户端发过来的报文后，回复ok。
  char buffer[1024];
  while (1)
  {
    int iret;
    memset(buffer,0,sizeof(buffer));
    if ( (iret=recv(new_fd,buffer,sizeof(buffer),0))<=0) // 接收客户端的请求报文。
    {
       printf("iret=%d\n",iret); break;
    }
    printf("接收：%s\n",buffer);

    strcpy(buffer,"ok");
    if ( (iret=send(new_fd,buffer,strlen(buffer),0))<=0) // 向客户端发送响应结果。
    { perror("send"); break; }
    printf("发送：%s\n",buffer);
  }

  // 第6步：关闭socket，释放资源。
  close(listenfd); close(new_fd);
}
