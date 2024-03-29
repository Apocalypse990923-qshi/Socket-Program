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
#include <fstream>
#include <vector>
#include <map>
using namespace std;

#define localhost "127.0.0.1"
#define port_server_A "21308"
#define port_server_B "22308"
#define port_server_C "23308"
#define port_UDP "24308"
#define port_TCP_A "25308"
#define port_TCP_B "26308"

struct transaction
{
   int num;    //serial number
   string sender;
   string recver;
   int amount;
};

struct stat
{
   int num;    //number of transactions made with user
   int amount;
};

bool compare(struct transaction a,struct transaction b)
{
   return a.num>b.num;
}

int listenfd_A, listenfd_B, childfd_A, childfd_B, sockfd, childfd;
struct addrinfo hints, *res, *A_addr, *B_addr, *C_addr;
int  socklen=sizeof(struct sockaddr_in);
struct sockaddr_in clientA_addr, clientB_addr;  // address information of client socket
char buffer[1024];  //store message

vector <struct transaction> record;
struct transaction t;
ifstream infile;
ofstream outfile;

int Check_Wallet(string user,bool flag)
{
   bool user_exist=false;
   int balance=1000;  //initial balance
   
   sprintf(buffer,"1 %s",user.c_str());
   if(sendto(sockfd,buffer,strlen(buffer),0,A_addr->ai_addr,A_addr->ai_addrlen)<=0) perror("Failed to send!");
   else
   {
      if(flag) printf("The main server sent a request to server A.\n");
      memset(buffer,0,sizeof(buffer));
      if(recvfrom(sockfd,buffer,sizeof(buffer),0,A_addr->ai_addr,&(A_addr->ai_addrlen))<=0) perror("Receiving serverA error!");
      else
      {
         if(flag) printf("The main server received transactions from Server A using UDP over port %d.\n",atoi(port_server_A));
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
      if(flag) printf("The main server sent a request to server B.\n");
      memset(buffer,0,sizeof(buffer));
      if(recvfrom(sockfd,buffer,sizeof(buffer),0,B_addr->ai_addr,&(B_addr->ai_addrlen))<=0) perror("Receiving serverB error!");
      else
      {
         if(flag) printf("The main server received transactions from Server B using UDP over port %d.\n",atoi(port_server_B));
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
      if(flag) printf("The main server sent a request to server C.\n");
      memset(buffer,0,sizeof(buffer));
      if(recvfrom(sockfd,buffer,sizeof(buffer),0,C_addr->ai_addr,&(C_addr->ai_addrlen))<=0) perror("Receiving serverC error!");
      else
      {
         if(flag) printf("The main server received transactions from Server C using UDP over port %d.\n",atoi(port_server_C));
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

pair<int,int> TXCoins(string msg,int port)
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
   printf("The main server received from %s to transfer %d coins to %s using TCP over port %d.\n",sender.c_str(),amount,recver.c_str(),port);
   
   int sender_balance = Check_Wallet(sender,false);
   int recver_balance = Check_Wallet(recver,false);
   if(sender_balance<0 && recver_balance<0) return make_pair(-4,0); //neither exists
   if(sender_balance < 0) return make_pair(-1,0); //sender does not exist
   if(recver_balance < 0) return make_pair(-3,0); //recver does not exist
   if(sender_balance - amount < 0) return make_pair(-2,sender_balance); //insufficient balance
   
   //request serverA,B,C for latest serial number
   int latest=0;
   sprintf(buffer,"2");
   if(sendto(sockfd,buffer,strlen(buffer),0,A_addr->ai_addr,A_addr->ai_addrlen)<=0) perror("Failed to send!");
   else
   {
      printf("The main server sent a request to server A.\n");
      memset(buffer,0,sizeof(buffer));
      if(recvfrom(sockfd,buffer,sizeof(buffer),0,A_addr->ai_addr,&(A_addr->ai_addrlen))<=0) perror("Receiving serverA error!");
      else
      {
         printf("The main server received the feedback from server A using UDP over port %d.\n",atoi(port_server_A));
         latest=max(latest,atoi(buffer));
      }
   }
   
   sprintf(buffer,"2");
   if(sendto(sockfd,buffer,strlen(buffer),0,B_addr->ai_addr,B_addr->ai_addrlen)<=0) perror("Failed to send!");
   else
   {
      printf("The main server sent a request to server B.\n");
      memset(buffer,0,sizeof(buffer));
      if(recvfrom(sockfd,buffer,sizeof(buffer),0,B_addr->ai_addr,&(B_addr->ai_addrlen))<=0) perror("Receiving serverB error!");
      else
      {
         printf("The main server received the feedback from server B using UDP over port %d.\n",atoi(port_server_B));
         latest=max(latest,atoi(buffer));
      }
   }
   
   sprintf(buffer,"2");
   if(sendto(sockfd,buffer,strlen(buffer),0,C_addr->ai_addr,C_addr->ai_addrlen)<=0) perror("Failed to send!");
   else
   {
      printf("The main server sent a request to server C.\n");
      memset(buffer,0,sizeof(buffer));
      if(recvfrom(sockfd,buffer,sizeof(buffer),0,C_addr->ai_addr,&(C_addr->ai_addrlen))<=0) perror("Receiving serverC error!");
      else
      {
         printf("The main server received the feedback from server C using UDP over port %d.\n",atoi(port_server_C));
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
   
   sender_balance-=amount;
   return make_pair(sender_balance,0);
}

void Add_to_record(char* data)
{
   string msg(data);
   int i=0;
   for(;i<msg.length();i++)
   {
      if(msg[i]==' ') break;
   }
   t.num=atoi((msg.substr(0,i)).c_str());
   int len_num=i;
   for(i=len_num+1;i<msg.length();i++)
   {
      if(msg[i]==' ') break;
   }
   t.sender = msg.substr(len_num+1,i-len_num-1);
   for(i=len_num+t.sender.length()+2;i<msg.length();i++)
   {
      if(msg[i]==' ') break;
   }
   t.recver = msg.substr(len_num+t.sender.length()+2,i-len_num-t.sender.length()-2);
   t.amount=atoi((msg.substr(len_num+t.sender.length()+t.recver.length()+3,msg.length()-len_num-t.sender.length()-t.recver.length()-3)).c_str());
   record.push_back(t);
}

void Receive_list(struct addrinfo *server_addr)
{
   int n;      //how many transactions stored in server
   sprintf(buffer,"4");
   if(sendto(sockfd,buffer,strlen(buffer),0,server_addr->ai_addr,server_addr->ai_addrlen)<=0) perror("Failed to send!");
   else
   {
      memset(buffer,0,sizeof(buffer));
      if(recvfrom(sockfd,buffer,sizeof(buffer),0,server_addr->ai_addr,&(server_addr->ai_addrlen))<=0) perror("Receiving error!");
      else
      {
         n=atoi(buffer);
         for(int i=0;i<n;i++)
         {
            memset(buffer,0,sizeof(buffer));
            if(recvfrom(sockfd,buffer,sizeof(buffer),0,server_addr->ai_addr,&(server_addr->ai_addrlen))<=0) perror("Receiving error!");
            else Add_to_record(buffer);
         }
      }
   }
}

void TXLIST()
{
   printf("A TXLIST request has been received.\n");
   record.clear();
   Receive_list(A_addr);
   Receive_list(B_addr);
   Receive_list(C_addr);
   
   sort(record.begin(),record.end(),compare);   //sort list
   outfile.open("alichain.txt", ios::out);   //save in "alichain.txt"
   while(record.size()>0)
   {
      t=record[record.size()-1];
      outfile<<t.num<<" "<<t.sender<<" "<<t.recver<<" "<<t.amount<<endl;
      record.pop_back();
   }
   outfile.close();
   printf("The sorted file is up and ready.\n");
}

void Receive_stats(struct addrinfo *server_addr, string username)
{
   int n;      //how many transactions of user stored in server
   sprintf(buffer,"5 %s",username.c_str());
   if(sendto(sockfd,buffer,strlen(buffer),0,server_addr->ai_addr,server_addr->ai_addrlen)<=0) perror("Failed to send!");
   else
   {
      memset(buffer,0,sizeof(buffer));
      if(recvfrom(sockfd,buffer,sizeof(buffer),0,server_addr->ai_addr,&(server_addr->ai_addrlen))<=0) perror("Receiving error!");
      else
      {
         n=atoi(buffer);
         for(int i=0;i<n;i++)
         {
            memset(buffer,0,sizeof(buffer));
            if(recvfrom(sockfd,buffer,sizeof(buffer),0,server_addr->ai_addr,&(server_addr->ai_addrlen))<=0) perror("Receiving error!");
            else Add_to_record(buffer);
         }
      }
   }
}

int Statistics(string user)
{
   if(Check_Wallet(user,false)<0) return -1; //user does not exist
   
   record.clear();
   Receive_stats(A_addr,user);
   Receive_stats(B_addr,user);
   Receive_stats(C_addr,user);

   map<string, struct stat> stats_map;
   while(record.size()>0)
   {
      t=record[record.size()-1];
      string other;
      if(t.sender==user) other=t.recver;
      else if(t.recver==user)
      {
         other=t.sender;
         t.amount*=(-1);
      }
      
      if(stats_map.count(other))
      {
         stats_map[other].amount+=t.amount;
         stats_map[other].num++;
      }
      else
      {
         struct stat s;
         s.num=1;
         s.amount=t.amount;
         stats_map.insert(pair<string, struct stat>(other,s));
      }
      record.pop_back();
   }
   map<string, struct stat>::iterator iter;
   for(iter = stats_map.begin(); iter != stats_map.end(); iter++)
   {
      t.num=(iter->second).num;
      t.sender=user;
      t.recver=iter->first;
      t.amount=(iter->second).amount;
      record.push_back(t);
   }
   sort(record.begin(),record.end(),compare);

   for(int i=0;i<record.size();i++)
   {
      t=record[i];
      sprintf(buffer,"%d %s %d %d ",i+1,t.recver.c_str(),t.num,t.amount);
      if(send(childfd,buffer,strlen(buffer),0)<=0) perror("Failed to send!");
   }  
   
   return 0; //success
}

void Backend(char* data, struct sockaddr_in* client_addr)
{
  string client_name=(client_addr==&clientA_addr)?"A":"B";
  int operation=int(data[0]-'0');
  switch(operation)
  {
      case 1:     //CHECK WALLET
      {
         string name(data, 2, strlen(data)-2);
         printf("The main server received input=%s from the client using TCP over port %d.\n",name.c_str(),client_addr->sin_port);
         int n=Check_Wallet(name,true);
         sprintf(data,"%d",n);
         if(n<0) printf("Username was not found on database.\n");
         else printf("The main server sent the current balance to client %s.\n",client_name.c_str());
         break;
      }
      case 2:     //TXCOINS
      {
         string message(data, 2, strlen(data)-2);
         pair<int,int> p=TXCoins(message,client_addr->sin_port);
         if(p.first==-2) sprintf(data,"%d %d",p.first,p.second);
         else sprintf(data,"%d",p.first);
         printf("The main server sent the result of the transaction to client %s.\n",client_name.c_str());
         break;
      }
      case 3:     //TXLIST
         TXLIST();
         break;
      case 4:     //stats
      {
         printf("A STATS request has been received.\n");
         string name(data, 2, strlen(data)-2);
         sprintf(data,"%d",Statistics(name));
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
          childfd=childfd_A;
          char c=buffer[0];
          Backend(buffer,&clientA_addr);	//process the request	
          if(c=='3') continue;
          if (send(childfd_A,buffer,strlen(buffer),0)<=0)
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
          childfd=childfd_B;
          char c=buffer[0];
          Backend(buffer,&clientB_addr);
          if(c=='3') continue;
          if (send(childfd_B,buffer,strlen(buffer),0)<=0)
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
