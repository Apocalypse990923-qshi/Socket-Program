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

void Load_All()
{
   record.clear();
   infile.open(file_path,ios::in);
   int index=0;
   while(!infile.eof())
   {
      switch(index % 4)
      {
         case 0:
            infile >> t.num;
            break;
         case 1:
            infile >> t.sender;
            break;
         case 2:
            infile >> t.recver;
            break;
         case 3:
            infile >> t.amount;
            record.push_back(t);
      }
      index++;
   }
   infile.close();
}

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

void Serial_Number()
{
   int latest=0;
   infile.open(file_path,ios::in);
   int index=0;
   while(!infile.eof())
   {
      switch(index % 4)
      {
         case 0:
            infile >> t.num;
            break;
         case 1:
            infile >> t.sender;
            break;
         case 2:
            infile >> t.recver;
            break;
         case 3:
            infile >> t.amount;
            if(t.num>latest) latest=t.num;
      }
      index++;
   }
   infile.close();
   
   sprintf(buffer,"%d",latest);
   if(sendto(sockfd,buffer,strlen(buffer),0,res->ai_addr,res->ai_addrlen)<=0) perror("Failed to send!");
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

void Save(string msg)
{
   int i=0;
   for(;i<msg.length();i++)
   {
      if(msg[i]==' ') break;
   }
   int new_num=atoi((msg.substr(0,i)).c_str());
   int len_num=i;
   for(i=len_num+1;i<msg.length();i++)
   {
      if(msg[i]==' ') break;
   }
   string new_sender = msg.substr(len_num+1,i-len_num-1);
   for(i=len_num+new_sender.length()+2;i<msg.length();i++)
   {
      if(msg[i]==' ') break;
   }
   string new_recver = msg.substr(len_num+new_sender.length()+2,i-len_num-new_sender.length()-2);
   int new_amount=atoi((msg.substr(len_num+new_sender.length()+new_recver.length()+3,msg.length()-len_num-new_sender.length()-new_recver.length()-3)).c_str());
   
   Load_All();
   outfile.open(file_path, ios::out);
   outfile<<new_num<<" "<<new_sender<<" "<<new_recver<<" "<<new_amount<<endl;
   while(record.size()>0)
   {
      t=record[record.size()-1];
      outfile<<t.num<<" "<<t.sender<<" "<<t.recver<<" "<<t.amount<<endl;
      record.pop_back();
   }
   outfile.close();
   
   sprintf(buffer, "Log Saved");
   if(sendto(sockfd,buffer,strlen(buffer),0,res->ai_addr,res->ai_addrlen)<=0) perror("Failed to send!");
}

void TXLIST()
{
   Load_All();
   sprintf(buffer,"%d",record.size());
   if(sendto(sockfd,buffer,strlen(buffer),0,res->ai_addr,res->ai_addrlen)<=0) perror("Failed to send!");
   else
   {
      for(int i=0;i<record.size();i++)
      {
         t=record[i];
         sprintf(buffer,"%d %s %s %d",t.num,t.sender.c_str(),t.recver.c_str(),t.amount);
         if(sendto(sockfd,buffer,strlen(buffer),0,res->ai_addr,res->ai_addrlen)<=0) perror("Failed to send!");
      }
   }
}

void Statistics(string user)
{
   Load(user);
   sprintf(buffer,"%d",record.size());
   if(sendto(sockfd,buffer,strlen(buffer),0,res->ai_addr,res->ai_addrlen)<=0) perror("Failed to send!");
   else
   {
      for(int i=0;i<record.size();i++)
      {
         t=record[i];
         sprintf(buffer,"%d %s %s %d",t.num,t.sender.c_str(),t.recver.c_str(),t.amount);
         if(sendto(sockfd,buffer,strlen(buffer),0,res->ai_addr,res->ai_addrlen)<=0) perror("Failed to send!");
      }
   }
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
  
  printf("The ServerA is up and running using UDP on port %d.\n",atoi(port_server_A));
  
  while (1)
  {
      memset(buffer,0,sizeof(buffer));
      if(recvfrom(sockfd,buffer,sizeof(buffer),0,res->ai_addr,&(res->ai_addrlen))<=0) perror("Receiving serverM error!");
      else
      {
         printf("The ServerA received a request from the Main Server.\n");
         int operation=int(buffer[0]-'0');
         switch(operation)
         {
            case 1:     //CHECK WALLET
            {
               string name(buffer, 2, strlen(buffer)-2);
               Check_Wallet(name);
               break;
            }
            case 2:     //latest serial number request
               Serial_Number();
               break;
            case 3:     //log entry
            {
               string data(buffer, 2, strlen(buffer)-2);
               Save(data);
               break;
            }
            case 4:     //TXLIST
               TXLIST();
               break;
            case 5:
            {
               string name(buffer, 2, strlen(buffer)-2);
               Statistics(name);
               break;
            }
            default:
               printf("Invalid operation!\n");
         }
         printf("The ServerA finished sending the response to the Main Server.\n");
      }
  }
  
  close(sockfd);
}
