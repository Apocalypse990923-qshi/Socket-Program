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
#include <math.h>
#include <algorithm>
#include <iostream>
using namespace std;

#define localhost "127.0.0.1"
#define port_server_A "21308"
#define port_server_B "22308"
#define port_server_C "23308"
#define port_UDP "24308"
#define port_TCP_A "25308"
#define port_TCP_B "26308"

int listenfd_A, listenfd_B, childfd_A, childfd_B, sockfd;
struct addrinfo hints, *res, *A_addr, *B_addr, *C_addr;
char buffer[1024];  //store message

int Check_Wallet(string user)
{
   bool user_exist=false;
   int balance=1000;  //initial balance
   
   sprintf(buffer,"1 %s",user.c_str());
   if(sendto(sockfd,buffer,strlen(buffer),0,A_addr->ai_addr,A_addr->ai_addrlen)<=0) perror("Failed to send!");
   else
   {
      memset(buffer,0,sizeof(buffer));
      if(recvfrom(sockfd,buffer,sizeof(buffer),0,A_addr->ai_addr,&(A_addr->ai_addrlen))<=0) perror("Receiving serverA error!");
      else
      {
         if(buffer[0]!='!') //user exists in server
         {
            user_exist=true;
            balance+=atoi(buffer);
         }
      }
   }
   
   sprintf(buffer,"1 %s",user.c_str());
   if(sendto(sockfd,buffer,strlen(buffer),0,B_addr->ai_addr,B_addr->ai_addrlen)<=0) perror("Failed to send!");
   else
   {
      memset(buffer,0,sizeof(buffer));
      if(recvfrom(sockfd,buffer,sizeof(buffer),0,B_addr->ai_addr,&(B_addr->ai_addrlen))<=0) perror("Receiving serverB error!");
      else
      {
         if(buffer[0]!='!') //user exists in server
         {
            user_exist=true;
            balance+=atoi(buffer);
         }
      }
   }
   
   sprintf(buffer,"1 %s",user.c_str());
   if(sendto(sockfd,buffer,strlen(buffer),0,C_addr->ai_addr,C_addr->ai_addrlen)<=0) perror("Failed to send!");
   else
   {
      memset(buffer,0,sizeof(buffer));
      if(recvfrom(sockfd,buffer,sizeof(buffer),0,C_addr->ai_addr,&(C_addr->ai_addrlen))<=0) perror("Receiving serverC error!");
      else
      {
         if(buffer[0]!='!') //user exists in server
         {
            user_exist=true;
            balance+=atoi(buffer);
         }
      }
   }
   
   if(user_exist==false) //user does not exist in any server
   {
      return -1;
   }
   return balance;
}

int TXCoins(string msg)
{
   int i=0;
   for(;i<msg.length();i++)
   {
      if(msg[i]==' ') break;
   }
   string sender = msg.substr(0,i);
   for(i=sender.length()+1;i<msg.length();i++)
   {
      if(msg[i]==' ') break;
   }
   string recver = msg.substr(sender.length()+1,i-sender.length()-1);
   int amount=atoi((msg.substr(sender.length()+recver.length()+2,msg.length()-sender.length()-recver.length()-2)).c_str());
   
   int sender_balance = Check_Wallet(sender);
   int recver_balance = Check_Wallet(recver);
   if(sender_balance < 0) return -1; //sender does not exist
   else if(sender_balance - amount < 0) return -2; //insufficient balance
   if(recver_balance < 0) return -3; //recver does not exist
   
   //request serverA,B,C for latest serial number
   int latest=0;
   sprintf(buffer,"2");
   if(sendto(sockfd,buffer,strlen(buffer),0,A_addr->ai_addr,A_addr->ai_addrlen)<=0) perror("Failed to send!");
   else
   {
      memset(buffer,0,sizeof(buffer));
      if(recvfrom(sockfd,buffer,sizeof(buffer),0,A_addr->ai_addr,&(A_addr->ai_addrlen))<=0) perror("Receiving serverA error!");
      else
      {
         latest=max(latest,atoi(buffer));
      }
   }
   
   sprintf(buffer,"2");
   if(sendto(sockfd,buffer,strlen(buffer),0,B_addr->ai_addr,B_addr->ai_addrlen)<=0) perror("Failed to send!");
   else
   {
      memset(buffer,0,sizeof(buffer));
      if(recvfrom(sockfd,buffer,sizeof(buffer),0,B_addr->ai_addr,&(B_addr->ai_addrlen))<=0) perror("Receiving serverB error!");
      else
      {
         latest=max(latest,atoi(buffer));
      }
   }
   
   sprintf(buffer,"2");
   if(sendto(sockfd,buffer,strlen(buffer),0,C_addr->ai_addr,C_addr->ai_addrlen)<=0) perror("Failed to send!");
   else
   {
      memset(buffer,0,sizeof(buffer));
      if(recvfrom(sockfd,buffer,sizeof(buffer),0,C_addr->ai_addr,&(C_addr->ai_addrlen))<=0) perror("Receiving serverC error!");
      else
      {
         latest=max(latest,atoi(buffer));
      }
   }
   
   srand((int)time(NULL));
   int r=rand()%3;  //randomly choose a server to store this transaction
   struct addrinfo *rand_addr;
   switch(r)
   {
      case 0:
         rand_addr=A_addr;
         break;
      case 1:
         rand_addr=B_addr;
         break;
      case 2:
         rand_addr=C_addr;
   }
   
   sprintf(buffer,"3 %d %s %s %d",latest+1,sender.c_str(),recver.c_str(),amount);
   if(sendto(sockfd,buffer,strlen(buffer),0,rand_addr->ai_addr,rand_addr->ai_addrlen)<=0) perror("Failed to send!");
   else
   {  //receive confirmation
      memset(buffer,0,sizeof(buffer));
      if(recvfrom(sockfd,buffer,sizeof(buffer),0,rand_addr->ai_addr,&(rand_addr->ai_addrlen))<=0) perror("Receiving random server error!");
      else sender_balance=Check_Wallet(sender);
   }
      
   return sender_balance;
}

void Backend(char* data)
{
  //printf("Received: %s\n",data);
  //sprintf(data,"ServerM received message successfully");
  
  int operation=int(data[0]-'0');
  switch(operation)
  {
      case 1:     //CHECK WALLET
      {
         string name(data, 2, strlen(data)-2);
         sprintf(data,"%d",Check_Wallet(name));
         break;
      }
      case 2:     //TXCOINS
      {
         string message(data, 2, strlen(data)-2);
         sprintf(data,"%d",TXCoins(message));
         break;
      }
      default:
         sprintf(data,"Invalid operation!");
         printf("Invalid operation!\n");
  }
  
  
  
  
  return;
}

int main(int argc,char *argv[])
{
  // Create lisen sockets and child sockets with Client_A and Client_B
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
  
  if ( (sockfd = socket(AF_INET,SOCK_DGRAM,0))==-1) //Create UDP Socket
  {
   perror("Failed to create UDP socket!");
   return -1;
  }

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
  getaddrinfo(localhost, port_TCP_B, &hints, &res);

  if (bind(listenfd_B, res->ai_addr, res->ai_addrlen) != 0 )
  {
    perror("Binding B failed!");
    close(listenfd_B);
    return -1;
  }

  //Bind address and port number to UDP socket
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  
  getaddrinfo(localhost, port_UDP, &hints, &res);
  
  if (bind(sockfd, res->ai_addr, res->ai_addrlen) != 0 )
  {
   perror("Binding UDP failed!");
   close(sockfd);
   return -1;
  }
  
  //get serverA,B,C address information
  getaddrinfo(localhost, port_server_A, &hints, &A_addr);
  getaddrinfo(localhost, port_server_B, &hints, &B_addr);
  getaddrinfo(localhost, port_server_C, &hints, &C_addr);

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

  while(1)
  {
    read_fds=master;
    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
    {
            perror("Failed to select!");
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
          }
          close(childfd_A);
        }
      }
    }
    if(FD_ISSET(listenfd_B, &read_fds)) //B has an incoming connection
    {
      childfd_B=accept(listenfd_B,(struct sockaddr *)&clientB_addr,(socklen_t*)&socklen);
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
          }
          close(childfd_B);
        }
      }
    }
  }

  close(listenfd_A);close(listenfd_B);close(sockfd);
}
