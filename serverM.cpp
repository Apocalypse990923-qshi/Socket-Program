#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <cmath>
#include <algorithm>

#define localhost "127.0.0.1"
#define port_UDP "24308"
#define port_TCP_A "25308"
#define port_TCP_B "26308"

void Backend(char* buf)
{
  print("Received: %s\n",buf);
  sprintf(buf,"ServerM received message successfully");
  return;
}

int main(int argc,char *argv[])
{
  // Create lisen sockets and child sockets with Client_A and Client_B
  int listenfd_A, listenfd_B, childfd_A, childfd_B;
  if ( (listenfd_A = socket(AF_INET,SOCK_STREAM,0))==-1)
  {
    perror("Failed to create socket A!");
    return -1;
  }

  if ( (listenfd_B = socket(AF_INET,SOCK_STREAM,0))==-1)
  {
    perror("Failed to create socket B!");
    return -1;
  }

  struct addrinfo hints, *res;

  // Bind address and port number to socket for A
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  getaddrinfo(localhost, port_TCP_A, &hints, &res);

  if (bind(listenfd_A, res->ai_addr, res->ai_addrlen) != 0 )
  {
    perror("Binding A failed!");
    close(listenfd_A);
    return -1;
  }

  // Bind address and port number to socket for B
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  getaddrinfo(localhost, port_TCP_B, &hints, &res);

  if (bind(listenfd_B, res->ai_addr, res->ai_addrlen) != 0 )
  {
    perror("Binding B failed!");
    close(listenfd_B);
    return -1;
  }

  //listen
  if (listen(listenfd_A,5) != 0 )
  {
    perror("Failed to listen A!");
    close(listenfd_A);
    return -1;
  }

  if (listen(listenfd_B,5) != 0 )
  {
    perror("Failed to listen B!");
    close(listenfd_B);
    return -1;
  }

  printf("The main server is up and running.\n");

  //select() function to implement multi-client server(adapted from Beej's Guide to Network Programming)
  fd_set master;    // master file descriptor list
  fd_set read_fds;  // temp file descriptor list for select()
  int fdmax;        // maximum file descriptor number

  FD_ZERO(&master);    // clear the master and temp sets
  FD_ZERO(&read_fds);

  FD_SET(listenfd_A, &master); // add the listener to the master set
  FD_SET(listenfd_B, &master);

  fdmax = max(listenfd_A, listenfd_B); // keep track of the biggest file descriptor

  int  socklen=sizeof(struct sockaddr_in);
  struct sockaddr_in clientA_addr, clientB_addr;  // address information of client socket
  char buffer[1024];  //store message

  while(1)
  {
    read_fds=master;
    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
    {
            perror("Failed to select!");
            return -1;
    }
    if(FD_ISSET(listenfd_A, &read_fds)) //A has an incoming connection
    {
      childfd_A=accept(listenfd_A,(struct sockaddr *)&clientA_addr,(socklen_t*)&socklen);
      if(childfd_A==-1) perror("Failed to accept A!");
      else
      {
        memset(buffer,0,sizeof(buffer));
        if(recv(childfd_A,buffer,sizeof(buffer),0)<=0) perror("Receiving A error!");
        else
        {
          Backend(buffer);
          if (send(childfd_A,buffer,strlen(buffer),0)<=0) // send message
          {
            perror("Failed to send!");
            return -1;
          }
          close(childfd_A);
        }
      }
    }
    if(FD_ISSET(listenfd_B, &read_fds)) //B has an incoming connection
    {
      childfd_B=accept(listenfd_B,(struct sockaddr *)&clientA_addr,(socklen_t*)&socklen);
      if(childfd_B==-1) perror("Failed to accept B!");
      else
      {
        memset(buffer,0,sizeof(buffer));
        if(recv(childfd_B,buffer,sizeof(buffer),0)<=0) perror("Receiving B error!");
        else
        {
          Backend(buffer);
          if (send(childfd_B,buffer,strlen(buffer),0)<=0) // send message
          {
            perror("Failed to send!");
            return -1;
          }
          close(childfd_B);
        }
      }
    }
  }

  close(listenfd_A);close(listenfd_B);
}
