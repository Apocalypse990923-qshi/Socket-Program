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

void Statistics(string msg)
{
   if(msg=="0") return;
   
   int i=0;
   for(;i<msg.length();i++)
   {
      if(msg[i]==' ') break;
   }
   int rank=atoi((msg.substr(0,i)).c_str());
   int len_rank=i;
   for(i=len_rank+1;i<msg.length();i++)
   {
      if(msg[i]==' ') break;
   }
   string recver = msg.substr(len_rank+1,i-len_rank-1);
   for(i=len_rank+recver.length()+2;i<msg.length();i++)
   {
      if(msg[i]==' ') break;
   }
   int num = atoi(msg.substr(len_rank+recver.length()+2,i-len_rank-recver.length()-2).c_str());
   int len_num=i-len_rank-recver.length()-2;
   for(i=len_rank+recver.length()+len_num+3;i<msg.length();i++)
   {
      if(msg[i]==' ') break;
   }
   int amount=atoi(msg.substr(len_rank+recver.length()+len_num+3,i-len_rank-recver.length()-len_num-3).c_str());
   printf("%d %s %d %d\n",rank,recver.c_str(),num,amount);
   
   Statistics(msg.substr(i+1,msg.length()-i-1));
   return;
}

int main(int argc,char *argv[])
{
  if (argc!=2 && argc!=3 && argc!=4)
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
      printf("clientB sent a sorted list request to the main server.\n");
    }
    else    //CHECK WALLET: ./client <username1>
    {
      sprintf(buffer,"1 %s",argv[1]);
      if (send(sockfd,buffer,strlen(buffer),0)<=0)
      {
         perror("Failed to send!");
         return -1;
      }
      printf("%s sent a balance enquiry request to the main server.\n",argv[1]);
    
      memset(buffer,0,sizeof(buffer));
      if (recv(sockfd,buffer,sizeof(buffer),0)<=0)
      {
         perror("Receiving error!");
         return -1;
      }
      int n=atoi(buffer);
      if(n<0) printf("Unable to proceed with the request as %s is not part of the network.\n",argv[1]);
      else printf("The current balance of %s is : %d alicoins.\n",argv[1],n);
    }
  }
  else if (argc==3)  //./client <username> stats
  {
      sprintf(buffer,"4 %s",argv[1]);
      if (send(sockfd,buffer,strlen(buffer),0)<=0)
      {
         perror("Failed to send!");
         return -1;
      }
      printf("%s sent a statistics enquiry request to the main server.\n",argv[1]);
      
      memset(buffer,0,sizeof(buffer));
      if(recv(sockfd,buffer,sizeof(buffer),0)<=0)
      {
         perror("Receiving error!");
         return -1;
      }
      else
      {
         string data(buffer);
         if(data=="-1") printf("Unable to proceed with the request as %s is not part of the network.\n",argv[1]);
         else
         {
            printf("%s statistics are the following.:\n",argv[1]);
            printf("Rank--Username--NumofTransacions--Total\n");
            Statistics(data);
         }
      }
  }
  else if (argc==4) //TXCOINS: ./client <username2> <username1> <amount>
  {
    sprintf(buffer,"2 %s %s %s",argv[1],argv[2],argv[3]);
    if (send(sockfd,buffer,strlen(buffer),0)<=0)
    {
      perror("Failed to send!");
      return -1;
    }
    printf("%s has requested to transfer %s coins to %s.\n",argv[1],argv[3],argv[2]);
    
    memset(buffer,0,sizeof(buffer));
    if (recv(sockfd,buffer,sizeof(buffer),0)<=0)
    {
      perror("Receiving error!");
      return -1;
    }
    int n=atoi(buffer);
    if(n>=0) printf("%s successfully transferred %s alicoins to %s. The current balance of %s is : %d alicoins.\n",argv[1],argv[3],argv[2],argv[1],n);
    else
    {
      switch(n)
      {
         case -1:
            printf("Unable to proceed with the transaction as %s is not part of the network.\n",argv[1]);
            break;
         case -2:
         {
            string s(buffer, 3, strlen(buffer)-3);
            printf("%s was unable to transfer %s alicoins to %s because of insufficient balance. The current balance of %s is : %s alicoins.\n",argv[1],argv[3],argv[2],argv[1],s.c_str());
            break;
         }
         case -3:
            printf("Unable to proceed with the transaction as %s is not part of the network.\n",argv[2]);
            break;
         case -4:
            printf("Unable to proceed with the transaction as %s and %s are not part of the network.\n",argv[1],argv[2]);
      }
    }
  }
  
  // Close socket。
  close(sockfd);
}
