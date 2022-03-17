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
#include <iostream>
using namespace std;

#define localhost "127.0.0.1"
#define port_TCP_B "26308"

int main(int argc,char *argv[])
{
  if (argc!=2 && argc!=4)
  {
    printf("Plese input appropriate arguments!\n");
    return -1;
  }

  // Create client socket
  int sockfd;
  if ( (sockfd = socket(AF_INET,SOCK_STREAM,0))==-1)
  {
    perror("Failed to create socket!");
    return -1;
  }

  struct addrinfo hints, *res;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;  // use IPv4 or IPv6, whichever
  hints.ai_socktype = SOCK_STREAM;  //TCP Socket

  //load up server's address information with getaddrinfo()
  getaddrinfo(localhost, port_TCP_B, &hints, &res);

  printf("The client B is up and running.\n");

  // Initiate a connection with server。
  if (connect(sockfd, res->ai_addr, res->ai_addrlen) != 0)
  {
    perror("Connection failed!");
    close(sockfd);
    return -1;
  }

  //Communicate with Server
  char buffer[1024];

  if (argc==2)
  {
    string s(argv[1]);
    if(s=="TXLIST")     //TXLIST
    {
      sprintf(buffer,"3");
      if (send(sockfd,buffer,strlen(buffer),0)<=0) // send message
      {
         perror("Failed to send!");
         return -1;
      }
      
      memset(buffer,0,sizeof(buffer));
      if (recv(sockfd,buffer,sizeof(buffer),0)<=0) // receive message from serverM
      {
         perror("Receiving error!");
         return -1;
      }
      printf("Received: %s\n", buffer);
    }
    else    //CHECK WALLET: ./client <username1>
    {
      sprintf(buffer,"1 %s",argv[1]);
      if (send(sockfd,buffer,strlen(buffer),0)<=0) // send message
      {
         perror("Failed to send!");
         return -1;
      }
      printf("%s sent a balance enquiry request to the main server.\n",argv[1]);
    
      memset(buffer,0,sizeof(buffer));
      if (recv(sockfd,buffer,sizeof(buffer),0)<=0) // receive message from serverM
      {
         perror("Receiving error!");
         return -1;
      }
      printf("Received: %s\n", buffer);
    }
  }
  else if (argc==4) //TXCOINS: ./client <username2> <username1> <amount>
  {
    sprintf(buffer,"2 %s %s %s",argv[1],argv[2],argv[3]);
    if (send(sockfd,buffer,strlen(buffer),0)<=0) // send message
    {
      perror("Failed to send!");
      return -1;
    }
    printf("%s has requested to transfer %s coins to %s.\n",argv[1],argv[3],argv[2]);
    
    memset(buffer,0,sizeof(buffer));
    if (recv(sockfd,buffer,sizeof(buffer),0)<=0) // receive message from serverM
    {
      perror("Receiving error!");
      return -1;
    }
    printf("Received: %s\n", buffer);
  }
  // Close socket。
  close(sockfd);
}
