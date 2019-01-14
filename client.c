#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include "library.h"
#include "library.c"

#define PORT 9000

#define CASE_LOGIN 1
#define CASE_REGISTER 2
#define CASE_EXIT 3

#define CAN_WRITE 1
#define CAN_NOT_WRITE 0

#define HAS_LOG 1
#define HAS_NOT_LOG 0

#define MAX_COUNT_QUESTION 5

void login(char username[],char *passwd,int sock); // Ham dang nhap tai khoan
void signup(int sock);  //Ham dang ky tai khoan
void sendanswer(int sock); // Gui cau tra loi len phia Server
int answerinput(char s[]); // Tra loi cac cau hoi,moi cau gioi han trong 15s

struct question{
  char quest[1024];
};

struct question q[6];

int main()
{

  int sock, bytes_recieved;
  char send_data[1024],*recv_data;
  struct hostent *host;
  struct sockaddr_in server_addr;

  host = gethostbyname("localhost");
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror("Socket() error!!");
      exit(1);
    }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr = *((struct in_addr *)host->h_addr);

  bzero(&(server_addr.sin_zero),8);

  if (connect(sock, (struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1){
        perror("Connect() error!!");
        exit(1);
      }


  char *answer;
  int count_question,i,j,m;
  int state,log;
  int write;
  int choose;
  choose = 0;
  while(choose == 0)
    {
      printf("\nCHUONG TRINH THI TRAC NGHIEM\n");
      printf("**************************************************\n");
      printf("		1.Dang nhap\n");
      printf("		2.Dang ky\n");
      printf("		3.Thoat\n");
      printf("**************************************************\n");
      printf("Moi ban lua chon : ");scanf("%d",&choose);

      char username[40];
      char *passwd;
      switch(choose)
        {
        case CASE_LOGIN :
          m = 0;
          write = CAN_NOT_WRITE;
          count_question = 0;
          log = HAS_NOT_LOG;
          while(1)
            {
              if(log == HAS_NOT_LOG)  // Thao tac dang nhap
                {
                  login(username, passwd, sock);
                  log = HAS_LOG;
                }
              char *receive;
              receive = (char *) malloc( 1024 * sizeof(char) );
              bytes_recieved = recv(sock, receive, 1024, 0);
              receive[bytes_recieved] = '\0';

              char *nhanda;
              nhanda = (char *)malloc(9*sizeof(char));
              strncpy(nhanda,receive,9);

              //kiểm tra xem cờ write được bật chưa nếu rồi thì tiến hành lựa chọn tải đề thi và đáp
              if(write == CAN_WRITE)
                {
                  m++;
                  int a;

                  printf("\n%s\n",receive); //In ra dap an
                  printf("Ban co tiep tuc muon tai ve de thi va dap an ?\n");
                  printf("1.Co\n");
                  printf("2.Khong\n");
                  printf("Hay lua chon : ");scanf("%d",&a);
                  if(a == 1)
                    {
                      send(sock,"D&A", strlen("D&A"),0);
                      printf("%s", receive);
                      free(receive);
                    }
                  if(a == 2)
                    {
                      exit(1);
                      break;
                    }
                  if(m == 5)
                    {
                      exit(1);
                      break;
                    }
                }
              free(nhanda);

              //kiểm tra nếu nhận về là lỗi password thì exit
              if(strcmp(receive,"Username or password is incorrect") == 0)
                {
                  printf("%s\n\n", receive);
                  free(receive);
                  choose = 0;
                  exit(1);
                  break;
                }

               //kiểm tra nếu khác mã lỗi và cờ write chưa bật thì tiến hành load câu hỏi vào mảng q
              if(strcmp(receive,"Username or password is incorrect") != 0 && write == CAN_NOT_WRITE)
                {
                  strcpy(q[count_question].quest, receive);
                  free(receive);
                  count_question++;
                }

              //nếu lượng câu hỏi bằng MAX và cờ write chưa bật thì tiến hành hiện câu hỏi bắt đầu thi
              if(count_question == MAX_COUNT_QUESTION && write == CAN_NOT_WRITE)
                {
                  printf("\nBAN DA DANG NHAP THANH CONG !\n");
                  printf("Chon cac phuong an A,B,C,D de tra loi. Chon N de bo qua di toi cau tiep theo\n");
                  printf("Moi cau hoi chi duoc tra loi trong vong 15s\n");
                  printf("Tra loi dung duoc 20d, sai tru 10d, khong tra loi khong co diem.\n");
                  printf("Nhan phim bat ki de bat dau thi.\n");
                  while (getchar()!='\n');
                  for(i = 0; i < MAX_COUNT_QUESTION; i++)
                    {
                      answer = (char *) malloc(10 * sizeof(char));
                      printf("\n%s\n", q[i].quest);
                      state = answerinput(answer);
                      if(state == 0)
                        strcpy(answer,"N");
                      send(sock,answer, strlen(answer),0);
                      free(answer);
                    }
                  write = CAN_WRITE;
                }
            }

          close(sock);
          exit(0);

          break;
        case CASE_REGISTER : log = HAS_NOT_LOG;
          while(1)
            {
              if(log == HAS_NOT_LOG)
                {
                  signup(sock);
                  log = HAS_LOG;
                }
              char *receive;
              receive = (char *) malloc( 1024 * sizeof(char) );
              bytes_recieved = recv(sock, receive, 1024, 0);
              receive[bytes_recieved] = '\0';
              if(strcmp(receive,"Username is existed") == 0)
                {
                  printf("%s\n",receive);
                  printf("Ban can phai dang ky lai\n");
                  choose = 0;
                  break;
                }
              if(strcmp(receive,"Your user is ready now") == 0)
                {
                  printf("%s\n",receive);
                  choose = 0;
                  break;
                }

            }
          break;
        case CASE_EXIT : break;
        default : choose = 0;break;

        }

    }

}

void sigHandleSigalrm(int signo)
{
  return;
}
int answerinput(char s[])
{
  struct sigaction sa;

  sa.sa_handler = sigHandleSigalrm;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGALRM, &sa, NULL);

  alarm(15);
  strcpy(s,"");
  printf("Nhap cau tra loi :");

  if (scanf("%s", s) == 1)
    {
      alarm(0); // Huy bo alarm
    }

  /* Cac lenh sau khi terminate scanf */

  if(strcmp(s, "") == 0)
    return 0;
  else
    return 1;

}

void login(char username[],char *passwd,int sock)
{
  char dangnhap[1024];
  while(getchar()!='\n');
  printf("Username :");gets(username);
  passwd = getpass("Password :");
  strcpy(dangnhap, "1");
  strcat(dangnhap, username);
  strcat(dangnhap, " ");
  strcat(dangnhap, passwd);


  send(sock, dangnhap, strlen(dangnhap), 0);

}

void signup(int sock)
{
  char username[30];
  char *passwd;
  char pass1[30],pass2[30];
  int flag;
  flag =0;
  while(getchar()!='\n');
  do{
    printf("Username :");gets(username);
    passwd = getpass("Password :");strcpy(pass1, passwd);
    passwd = getpass("Confirm Password :");strcpy(pass2, passwd);
    if(strcmp(pass1, pass2) == 0)
      flag = 1;
    else
      printf("Hai mat khau chua trung khop, hay nhap lai !\n");
  }while(flag == 0);
  char dangnhap[1024];
  strcpy(dangnhap, "2");
  strcat(dangnhap, username);
  strcat(dangnhap, " ");
  strcat(dangnhap, pass1);
  send(sock, dangnhap, strlen(dangnhap), 0);
}



