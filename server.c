#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>
#include "library.h"
#include "library.c"


#define true 1
#define false 0
#define PORT 9000
#define BACKLOG 10


//MOT SO HAM CHINH SU DUNG TREN PHIA SERVER
void readfile(); //Doc file
void sendquestion();  // Gui cau hoi toi Client
int checklogin(char *username,int sock_client); // Kiem tra tai khoan dang nhao
int checksignup(char *username,int sock_client);// Kiem tra tai khoan dang ky
void chamdiem(char *dapan); // Cham diem khi Client gui dap an len Server
void sendsubject(int sock_client); // Gui dap an va cau hoi toi Client neu co yeu cau


struct question{
  char question[1024];
  char a[1024];
  char b[1024];
  char c[1024];
  char d[1024];
  char answer[10];
};
struct user{
  char name[50];
  int state;
};

typedef struct
{
  char name[50];
  int point;
}rank;

int num_question;
int num_line;
int socaudung,socausai,socauboqua;
struct question q[10];
rank r;

void handle_child(int x)
{
  pid_t rc;
  int stat;
  while((rc=waitpid(-1,&stat,WNOHANG))>0)
    { 
      printf(" Child terminate with PID : %d \n ",rc);
      if(WIFEXITED(stat))
	printf(" Child exited with code %d \n", WEXITSTATUS(stat));
      else
	printf(" Child terminated abnormally \n");
    }
  signal(SIGCHLD,handle_child);
  return ;
}

main()
{

  get_numline();
  readfile(num_line);


  socaudung = 0; socausai =0,socauboqua=0;
  int state;
  struct sockaddr_in server;
  struct sockaddr_in client;
  pid_t pid;
  int flag;
  int sock;
  int sock_client;
  int sin_size;
  int byte_recv;
  char send_data[1024];
  char recv_data[1024];
  struct hostent *host;
  int stat_val;
  pid_t child_pid;

  // Tao kiem soat tin hieu
  signal(SIGCHLD, handle_child);

  if((sock = socket(AF_INET,SOCK_STREAM,0)) == -1) {
    perror("Init socket error!!");
    exit(0);
  }
  server.sin_family = AF_INET;
  server.sin_port  = htons(PORT);
  server.sin_addr.s_addr = INADDR_ANY;


  bzero(&(server.sin_zero),8);
  if(bind(sock,(struct sockaddr*)&server,sizeof(struct sockaddr)) == -1){
    perror("bind() error!!");
    exit(0);
  }
  if(listen(sock,BACKLOG) == -1){
    perror("Listen() error!!");
    exit(0);
  }
  readfile();
  int byte_recv1;
  int i,j,d;

  int check;
  check = 0;
  d=0;
  while(1)
    {
      sin_size = sizeof(struct sockaddr_in);
      sock_client = accept(sock,(struct sockaddr*)&client,&sin_size);
      if( sock_client == -1)
	{
	  break;
	}

      // Forking Server
      if((pid = fork()) == 0)
	{
	  close(sock);       /* Child Socket Listening ; */
	  printf("Have a connection from :%s\n\n",inet_ntoa(client.sin_addr));

	  while(1)
	    {
	      char *receive;
	      receive = (char *) malloc(1024 * sizeof(char));
	      byte_recv = recv(sock_client, receive, 1024, 0);
	      receive[byte_recv]='\0';
	      if(strcmp(receive,"D&A") == 0)
            {
              sendsubject(sock_client);
              close(sock_client);
              break;
            }
	      if(check == 1 && strcmp(receive,"D&A") != 0)
            {
              d++;
              char dapan[10];
              if(strcmp(receive, "N") == 0)
                socauboqua++;
              sprintf(dapan, "%d", d);
              strcat(dapan,receive);
              //puts(dapan);
              chamdiem(dapan);
              if(d == 5)
                {
                  char guikq[1024];
                  char caudung[3];
                  char causai[3];
                  char boqua[3];
                  char diemso[7];
                  socausai = socausai - socauboqua;
                  int diemsoD;
                  diemsoD = socaudung * 20 - socausai * 10;

                  r.point=diemsoD;
                  FILE *p;
                  p=fopen("rank.txt","ab");
                  fwrite(&r,sizeof(r),1,p);
                  fclose(p);


                  sprintf(caudung, "%d", socaudung);
                  sprintf(causai, "%d", socausai);
                  sprintf(boqua, "%d", socauboqua);
                  sprintf(diemso, "%d", diemsoD);
                  strcat(diemso, "/100");
                  strcpy(guikq, "Ket Qua :\n");
                  strcat(guikq, "So cau dung   : ");
                  strcat(guikq, caudung);
                  strcat(guikq, "\n");
                  strcat(guikq, "So cau sai    : ");
                  strcat(guikq, causai);
                  strcat(guikq, "\n");

                  strcat(guikq, "So cau bo qua : ");
                  strcat(guikq, boqua);
                  strcat(guikq, "\n");
                  strcat(guikq, "Diem so : ");
                  strcat(guikq, diemso);strcat(guikq, "\n");
                  send(sock_client, guikq, strlen(guikq), 0);  // Gui tra lai phia Client ket qua  bai thi
                }
            }

          //kiem tra check = 0 va do dai nhan
	      if(check == 0 && strlen(receive) > 0 )
            {
              if(receive[0] == '1')
                {
                  //Xu ly thao tac login
                  state = checklogin(receive, sock_client);
                  if(state == 1)
                    {
                      sendquestion(sock_client);
                      check = 1;
                    }
                }
              if(receive[0] == '2')
                {
                  //Xu ly thao tac signup
                  state = checksignup(receive, sock_client);
                  if(state == 0)
                    {
                      char *username2;
                      username2 = (char *) malloc(strlen(receive - 1) * sizeof(char));
                      for(j = 1; j < strlen(receive) ; j++)
                        username2[j-1] = receive[j];
                      username2[strlen(receive) - 1] = '\0';
                      FILE *f;
                      f = fopen("user.txt","a+");
                      fprintf(f,"%s\n", username2);
                      fclose(f);

                    }
                }

            }
        if (strcmp(receive,"get_rank") == 0)
        {
          get_rank(sock_client);
        }
	      free(receive);
	    }
	}

      close(sock_client); /* Parent Close Connect Socket */
    }
  close(sock);
  exit(0);

}
int checksignup(char *username,int sock_client)
{
  int check;
  char *username1;
  int i,j;
  i = 0;
  while(username[i] != ' ')
    {
      i++;
    }
  username1 = (char *) malloc(i*sizeof(char));
  strncpy(username1, username,  i);

  IS is;
  int flag;
  check = 0;
  is = new_inputstruct("user.txt");
  flag = 0;
  char user[1024];
  FILE *f;
  while(get_line(is)>=0)
    {
      strcpy(user,"2");
      strcat(user,is->fields[0]);
      if(strcmp(username1, user) == 0)
	flag = 1;
    }

  if(flag ==  0)
    {
      check = 0;
      send(sock_client, "Your user is ready now", strlen("Your user is ready now"),0);
    }
  if(flag == 1)
    {
      check = 1;
      send(sock_client, "Username is existed", strlen("Username is existed"),0);
    }
  return check;

}
int checklogin(char *username,int sock_client)
{
  int check;
  IS is;
  int flag;
  check = 0;
  is = new_inputstruct("user.txt");
  flag = 0;
  char user[1024];
  while(get_line(is)>=0)
    {
      strcpy(user, "1");
      strcat(user, is->fields[0]);
      strcat(user, " ");
      strcat(user, is->fields[1]);
      if(strcmp( username, user ) == 0)
      {
        strcpy(r.name,is->fields[0]);
      }
	    flag = 1;
    }

  if(flag ==  0)
    {
      check = 0;
      send(sock_client,"Username or password is incorrect", strlen("Username or password is incorrect"),0);
    }
  if(flag == 1)
    {
      check = 1;
    }
  return check;

}
int get_numline()
{
  IS is;
  int num_line=0;
  is = new_inputstruct("dethi.txt");
  while(get_line(is)>=0)
    {
      num_line++;
    }
  num_question = num_line/6;
  return num_line;
}
void readfile(int num_line)
{


  struct question qu[num_question];
  IS is;
  is = new_inputstruct("dethi.txt");
  int i,j,k;
  num_line = 0;
  while(get_line(is)>=0)
    {
      num_line++;
      char line[1024];
      strcpy(line,"");
      for(i=0; i<is->NF;i++)
        {
          strcat(line,is->fields[i]);
          strcat(line," ");
        }
      if(num_line%6 == 1)
        {
          strcpy(qu[num_line/6].question, line);
        }
      if(num_line%6 == 2)
        {
          strcpy(qu[num_line/6].a, line);
        }
      if(num_line%6 == 3)
        {
          strcpy(qu[num_line/6].b, line);
        }
      if(num_line%6 == 4)
        {
          strcpy(qu[num_line/6].c, line);
        }
      if(num_line%6 == 5)
        {
          strcpy(qu[num_line/6].d, line);
        }
      if(num_line%6 == 0)
        {
          strcpy(qu[num_line/6].answer, line);
        }
    }

  for(i=0;i<num_question;i++)
    {
      strcpy(q[i].question, qu[i].question);
      strcpy(q[i].a, qu[i].a);
      strcpy(q[i].b, qu[i].b);
      strcpy(q[i].c, qu[i].c);
      strcpy(q[i].d, qu[i].d);
      strcpy(q[i].answer, qu[i].answer);
    }


}

void sendquestion(int sock_client)
{
  int i;
  char quest[1024];
  for(i = 0 ; i<num_question; i++)
    {

      strcpy(quest, "");
      strcat(quest, q[i].question);
      strcat(quest, "\n");
      strcat(quest, q[i].a);
      strcat(quest, "\n");
      strcat(quest, q[i].b);
      strcat(quest, "\n");
      strcat(quest, q[i].c);
      strcat(quest, "\n");
      strcat(quest, q[i].d);
      send(sock_client, quest,strlen(quest),0);
      sleep(1);
    }
}

void sendsubject(int sock_client)
{
  char str[MAXLEN];
  //char str2[MAXLEN];
  char *result;
  result = (char *)calloc(MAXLEN,sizeof(char));
  FILE *p1;
  p1=fopen("dethi.txt","r");
  while(!feof(p1))
    {
       if(fgets(str,MAXLEN-1,p1)!=NULL)
	  strcat(result,str);
    }
  send(sock_client, result, strlen(result),0);
  free(result);
  fclose(p1);
}

void chamdiem(char *dapan)
{
  IS is;
  is = new_inputstruct("dapan.txt");
  int flag;
  int i;
  flag = 0;

  while(get_line(is)>=0)
    {
      char *line;
      line = (char *) malloc( 2 * sizeof(char) );

      for(i=0; i<is->NF; i++)
        {
          strcat(line, is->fields[i]);
        }
      if(strcmp(line, dapan) == 0)
        {
          socaudung++;
          flag = 1;
        }
      free(line);
    }
  if(flag == 0)
    socausai++;

}

void get_rank(int sock_client){
  rank d[50];
  rank k;
  FILE *p1;
  int i=0,m=0,y=0;
  char name1[50];
  char diem[5];
  char stt[5];
  char string[MAXLEN];
  p1=fopen("rank.txt","rb");
        while(1)
    {
      fread(&k,sizeof(k),1,p1);
      d[++m]=k;
      if(feof(p1))
        break;
    }
    fclose(p1);
        int o;
        for(i=1;i<=m-1;i++)
      for(y=i+1;y<=m;y++)
      {
        if(d[i].point<d[y].point)
          {
            d[o]=d[i];
            d[i]=d[y];
            d[y]=d[o];
          }
      }
      strcpy(string,"\n----------------TOP 5--------------------\n");
      strcat(string,"RANK   HO TEN          DIEM\n");
        for(i=1;i<5;i++)
    {
      sprintf(stt,"%d",i);      
      sprintf(name1, "%s", d[i].name);
      sprintf(diem, "%d", d[i].point);
      strcat(string,stt);
      strcat(string,"\t");
      strcat(string,name1);
      strcat(string,"\t\t");
      strcat(string,diem);
      strcat(string,"\n");
    }
    strcat(string,"\n--------------------------------------------\n");
    send(sock_client, string, strlen(string), 0); 
}
