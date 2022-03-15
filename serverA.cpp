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
#include <fstream>
#include <iostream>
#include <vector>
using namespace std;

#define localhost "127.0.0.1"
#define port_server_A "21308"
#define port_UDP "24308"
#define file_path "block1.txt"

struct transaction
{
   int num;
   string sender;
   string recver;
   int amount;
};

int sockfd;
struct addrinfo hints, *res;
char buffer[1024];

vector <struct transaction> record;
struct transaction t;
ifstream infile;
ofstream outfile;

void Load(string user)
{
   record.clear();
   infile.open(file_path,ios::in);
   int index=0;
   bool is_user=false;
   while(!infile.eof())
   {
      switch(index % 4)
      {
         case 0:
            infile >> t.num;
            break;
         case 1:
         {
            infile >> t.sender;
            if(t.sender == user) is_user=true;
            break;
         }
         case 2:
         {
            infile >> t.recver;
            if(t.recver == user) is_user=true;
            break;
         }
         case 3:
         {
            infile >> t.amount;
            if(is_user)
            {
               record.push_back(t);
               is_user=false;
            }
         }
      }
      index++;
   }
   infile.close();
}

void Check_Wallet(string user)
{
   int balance=0;
   Load(user);
   if(record.size()==0)
   {
      sprintf(buffer, "!");
   }
   else
   {
      for(int i=0;i<record.size();i++)
      {
         t=record[i];
         if(user==t.sender) balance-=t.amount;
         else if(user==t.recver) balance+=t.amount;
      }
      sprintf(buffer,"%d",balance);
   }
   if(sendto(sockfd,buffer,strlen(buffer),0,res->ai_addr,res->ai_addrlen)<=0) perror("Failed to send!");
}

int main(int argc,char *argv[])
{
  if ( (sockfd = socket(AF_INET,SOCK_DGRAM,0))==-1) //Create UDP Socket
  {
   perror("Failed to create UDP socket!");
   return -1;
  }
  
  //Bind address and port number to UDP socket
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  
  getaddrinfo(localhost, port_server_A, &hints, &res);
  
  if (bind(sockfd, res->ai_addr, res->ai_addrlen) != 0 )
  {
   perror("Binding UDP failed!");
   close(sockfd);
   return -1;
  }
  
  //get serverM address information
  memset(&hints, 0, sizeof(hints));
  
  getaddrinfo(localhost, port_UDP, &hints, &res);
  
  while (1)
  {
      memset(buffer,0,sizeof(buffer));
      if(recvfrom(sockfd,buffer,sizeof(buffer),0,res->ai_addr,&(res->ai_addrlen))<=0) perror("Receiving serverM error!");
      else
      {
         int operation=int(buffer[0]-'0');
         switch(operation)
         {
            case 1:     //CHECK WALLET
            {
               string name(buffer, 2, strlen(buffer)-2);
               Check_Wallet(name);
               break;
            }
            default:
               printf("Invalid operation!\n");
         }
         
         
      }
   
  }
  
  close(sockfd);
}
