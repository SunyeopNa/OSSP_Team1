#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#define MAXLINE  511
#define MAX_SOCK 1024 // 솔라리스의 경우 64
char *EXIT_STRING = "exit";
char *START_STRING = "OSSP 그림채팅\n";
char *MAIN_MENU = "\n\t서비스 번호를 선택하세요.\n 1.방 개설 \n 2.방 참여\n\n\n\n<번호/명령(go,exit)> ";
char *CREAT_ROOM = "enter chatting room subject ";
int maxfdp1;                // 최대 소켓번호 +1
int num_chat = 0;          // 채팅 참가자 수
int clisock_list[MAX_SOCK]; // 채팅에 참가자 소켓번호 목록
int listen_sock;      //server socket
socklen_t* addrlen = sizeof(struct sockaddr_in);
int serv_port = 2402;
int rooms_num;
pthread_mutex_t mutx;   //use thread with mutex
enum flag_state { F_Init = 0, F_Geust, F_Reqlist, F_Newroom, F_Host };
FILE *chat_log;
struct chat_room {
	// room structure
	char room_name[MAXLINE];
	int user_cnt;    // entry num
	int user_list[MAX_SOCK]; // entry name list   // {5, 8, 7, 0}
	int user_list_exist[MAX_SOCK];            // {1, 1, 1, 0}
	int user_max;
	int is_open;
	int pic_array[400][400][3];   //400x400 픽셀, RGB
};
struct chat_room chat_rooms[MAXLINE];
void removeClient(int s);    // 채팅 탈퇴 처리 함수
int tcp_listen(int host, int port, int backlog); // 소켓 생성 및 listen
void errquit(char *mesg) { perror(mesg); exit(1); }
void creat_room(int sock, char *buf);
int out_room_list(int sock);
void *client_main(void *arg);
void removeClinetFromRoom(int s);
void close_room(int room_no);

int main(int argc, char *argv[])
{
	printf("그림 배열 초기화...\n");
	for (int k = 0; k < MAXLINE; k++)
		for (int i = 0; i < 400; i++)
			for (int j = 0; j < 400; j++)
				for (int l = 0; l < 3; l++)
				{
					chat_rooms[k].pic_array[i][j][l] = 0;
				}
	printf("그림 배열 초기화 완료\n");
	int clnt_addr_size;
	struct sockaddr_in cliaddr;
	char buf[MAXLINE + 1];
	int accp_sock;
	pthread_t thread;
	//fd_set read_fds;  // 읽기를 감지할 fd_set 구조체
	if (argc != 1)
	{
		printf("사용법 :%s \n", argv[0]);
		exit(0);
	}
	//sig_pipe();
	if (pthread_mutex_init(&mutx, NULL))
	{
		printf("mutex init error\n"); exit(0);
	}
	listen_sock = tcp_listen(INADDR_ANY, serv_port, 5);
	while (1)
	{
		clnt_addr_size = sizeof(cliaddr);
		//FD_ZERO(&read_fds);
		puts("wait for client\n");
		/***** listen socket : 새로운 사용자가 연결, 2402 Port *****/
		// Listen_sock은 2402와 연결된 연결요청 전용 소켓
		accp_sock = accept(listen_sock, (struct sockaddr *)&cliaddr, &addrlen);
		if (accp_sock == -1)
			errquit("accept fail");
		pthread_mutex_lock(&mutx);
		clisock_list[num_chat++] = accp_sock;
		pthread_mutex_unlock(&mutx);
		pthread_create(&thread, NULL, client_main, (void*)accp_sock);
		send(accp_sock, START_STRING, strlen(START_STRING), 0);
		send(accp_sock, MAIN_MENU, strlen(MAIN_MENU), 0);
		printf("%d번째 사용자 추가.\n", num_chat);
		/**********************************************************/
	}  // end of while
	return 0;
}

/***** 각각의 소켓에 대한 스레드****/
// 클라이언트가 보낸 메시지를 모든 클라이언트에게 방송
void *client_main(void *arg)
{
	int cli_sock = (int)arg;
	int nbyte = 0;
	char buf[MAXLINE + 1];
	int flag = F_Init;   //client의 채팅 참여 여부 판단
	int coor_x = 0, coor_y = 0, width = 10, height = 10;
	int c_red = 0, c_green = 0, c_blue = 0, brush_size = 0;
	char *ptr;
	while ((nbyte = recv(cli_sock, buf, MAXLINE, 0)) > 0)
	{
		printf("flag %d\n", flag);
		//flag 0 : 클라이언트 최초 접속한 후의 기본 상태. 초기 상태
		//이미 무엇을 할 것인지 메세지를 받음
		if (flag == F_Init)
		{
			//채팅방 생성 요청 ==>
			if (strstr(buf, "1") != NULL)
			{
				send(cli_sock, "방 제목을 입력하세요 :", sizeof("방 제목을 입력하세요 :"), 0);
				flag = 3;
				continue;
			}
			//채팅방 목록 요청
			if (strstr(buf, "2") != NULL)
			{
				if (out_room_list(cli_sock)) flag = 2;
				else
				{
					send(cli_sock, MAIN_MENU, strlen(MAIN_MENU), 0);
				}
				continue;
			}
		}
		//flag 1 : 채팅에 정상적으로 참여된 상태. 방에 참여한 상태, 1 : 일반 참가 4 :  host 참가
		if (flag == F_Geust || flag == F_Host)
		{
			//프로그램 종료 신호 (현재는 exit) ..
			if ((strstr(buf, EXIT_STRING)))
			{
				printf("exit\n");
				pthread_mutex_lock(&mutx);
				removeClinetFromRoom(cli_sock);
				removeClient(cli_sock);
				flag = 0;
				num_chat--;
				pthread_mutex_unlock(&mutx);
				return 0;
			}
			printf("recv msg\n");
			for (int rn = 0; rn < rooms_num; rn++)
			{
				for (int un = 0; un < chat_rooms[rn].user_max; un++)
				{
					//참여한 방을 찾아냄 --> 해당 방 배열에 적용 후 같은 방 인원 모두에게 내용 send
					if (cli_sock == chat_rooms[rn].user_list[un])
					{
						//호스트가 나가서 방이 종료된 경우 -> 같은 방에 있는 다른 클라이언트 모두 종료
						if (!chat_rooms[rn].is_open)
						{
							pthread_mutex_lock(&mutx);
							removeClinetFromRoom(cli_sock);
							removeClient(cli_sock);
							pthread_mutex_unlock(&mutx);
							flag = 0;
							//clinet gtk 종료 send 필요
						}
						//배열에 적용 시작부분
						//000 000 00000 00000 00000 00
						//X   Y   R     G     B     Px
						printf("%s\n", buf);
						ptr = strtok(buf, " ");
						coor_x = atoi(ptr);
						ptr = strtok(NULL, " ");
						coor_y = atoi(ptr);
						ptr = strtok(NULL, " ");
						c_red = atoi(ptr);
						ptr = strtok(NULL, " ");
						c_green = atoi(ptr);
						ptr = strtok(NULL, " ");
						c_blue = atoi(ptr);
						ptr = strtok(NULL, " ");
						brush_size = atoi(ptr);
						pthread_mutex_lock(&mutx);
						/////////////브러시 사이즈 적용 필요
						int x_ul = coor_x - (brush_size / 2);
						int y_ul = coor_y - (brush_size / 2);
						int x_dr = coor_x + (brush_size / 2);
						int y_dr = coor_y + (brush_size / 2);
						if (x_ul < 0) x_ul = 0; if (y_ul < 0) y_ul = 0;

						printf("[%d,%d]\n", coor_x + (brush_size / 2), coor_y + (brush_size / 2));

						for (int x = x_ul; x < x_dr; x++)
						{
							for (int y = y_ul; y < y_dr; y++)
							{
								chat_rooms[rn].pic_array[x][y][0] = c_red;
								chat_rooms[rn].pic_array[x][y][1] = c_green;
								chat_rooms[rn].pic_array[x][y][2] = c_blue;
							}
						}
						sprintf(buf, "%d %d %d %d %d %d", coor_x, coor_y, c_red, c_green, c_blue, brush_size);
						//배열에 적용 끝부분
						for (int rm = 0; rm < chat_rooms[rn].user_max; rm++)
						{
							printf("chat_rooms[rn].user_list_exist[rm] = %d\n", chat_rooms[rn].user_list_exist[rm]);
							if (chat_rooms[rn].user_list_exist[rm])   //사람이 있을때만 메세지 전송
																	  //if (chat_rooms[rn].user_list[rm] != cli_sock)
								send(chat_rooms[rn].user_list[rm], buf, 40, 0);
						}
						pthread_mutex_unlock(&mutx);
						rn = 1000;   //다음 loop로 건너감
						break;
					}
				}
			}
		}
		//flag 2 : 채팅방 목록 요청한 상태. 방 참여를 하기로 한 상태.
		if (flag == F_Reqlist)
		{
			int room_no;
			room_no = atoi(buf);
			printf("test : room_no : %d\n", room_no);
			//닫힌방인가
			if (chat_rooms[room_no].is_open == 0)
			{
				send(cli_sock, "unavailable room no.", strlen("unavailable room no."), 0);
				send(cli_sock, MAIN_MENU, strlen(MAIN_MENU), 0);
				flag = 0;
				pthread_mutex_unlock(&mutx);
				continue;
			}
			// 방 인원 여유가 있는가
			if (chat_rooms[room_no].user_cnt >= chat_rooms[room_no].user_max)
			{
				// 방 인원수를 초과했을경우
				send(cli_sock, "인원 초과\\n", strlen("인원 초과\n"), 0);
				send(cli_sock, MAIN_MENU, strlen(MAIN_MENU), 0);
				flag = 0;
				pthread_mutex_unlock(&mutx);
				continue;
			}
			for (int i = 0; i < chat_rooms[room_no].user_max; i++)
			{
				if (chat_rooms[room_no].user_list_exist[i] == 0)
				{
					//방의 비어있는 공간에 새 클라이언트 입력
					chat_rooms[room_no].user_list[i] = cli_sock;
					chat_rooms[room_no].user_list_exist[i] = 1;
					//인원수 증가
					chat_rooms[room_no].user_cnt++;
					printf("%d번 방에 %d번째 자리 ON \n", room_no, i);
					//X : client 시작 루프 종료 문자. 뒤의 정수형은 사용자의 방 번호
					//현재 client 가 방에서 몇번째 공간에 있는지 send
					sprintf(buf, "X %02d\0", i);
					send(cli_sock, buf, strlen(buf), 0);
					break;
				}
			}
			//현재 방의 상태 새로연결한 클라이언트에게 모두 전송
			for (int i = 0; i < 400; i++)
			{
				for (int j = 0; j < 400; j++)
				{

					sprintf(buf, "%03d %03d %05d %05d %05d 01\n\0", i, j,
						chat_rooms[room_no].pic_array[i][j][0],
						chat_rooms[room_no].pic_array[i][j][1],
						chat_rooms[room_no].pic_array[i][j][2]);

					send(cli_sock, buf, strlen(buf), 0);


				}
			}
			pthread_mutex_unlock(&mutx);
			printf("초기상태 전송 완료\n");
			flag = 1;
			printf("flag is 1\n");
		}
		//flag 3 : 채팅방 새로 생성 상태. 채팅방 제목을 입력받을 상태.
		if (flag == F_Newroom)
		{
			pthread_mutex_lock(&mutx);
			creat_room(cli_sock, buf);
			send(cli_sock, "X 00\0", strlen("X 00\0"), 0);
			pthread_mutex_unlock(&mutx);
			flag = 4;
			printf("flag is 4\n");
		}
	}
	//이미 방에 참여한 사람이면 방에서도 제거
	pthread_mutex_lock(&mutx);
	if (flag == F_Geust || flag == F_Host)removeClinetFromRoom(cli_sock);
	removeClient(cli_sock);    // 클라이언트의 종료
	pthread_mutex_unlock(&mutx);
	flag = 0;
}

// 채팅 탈퇴 처리
void removeClient(int cli_sock) {
	close(cli_sock);
	for (int i = 0; i<num_chat; i++)
	{
		if (cli_sock == clisock_list[i])
		{
			for (; i<num_chat - 1; i++)
				clisock_list[i] = clisock_list[i + 1];
			break;
		}
	}
	num_chat--;
	printf("채팅 참가자 1명 탈퇴. 현재 참가자 수 = %d\n", num_chat);
	return;
}

// 채팅 방에서 소켓 번호 제거 및 자리 비우기
void removeClinetFromRoom(int s)
{
	for (int i = 0; i < MAXLINE; i++)
	{
		for (int j = 0; j < chat_rooms[i].user_max; j++)
		{
			if (chat_rooms[i].user_list[j] == s)
			{
				chat_rooms[i].user_list[j] = MAX_SOCK;   //소켓 -> 의미없는 값
				chat_rooms[i].user_list_exist[j] = 0;   //사람 없음
				chat_rooms[i].user_cnt--;
				//호스트가 방에서 나갈 경우
				if (j == 0)
				{
					close_room(i);
					chat_rooms[i].user_cnt = 0;
				}
				return;
			}
		}
	}
	return;
}

// listen 소켓 생성 및 listen
int  tcp_listen(int host, int port, int backlog) {
	int sd;
	int opt_yes = 1;       // 소켓의 옵션값
	struct sockaddr_in servaddr;
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("socket fail");
		exit(1);
	}
	// 소켓의 옵션 변경 TIME-WAIT 시에도 재실행 가능
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt_yes, sizeof(opt_yes))<0)
		printf("error : reuse setsockopt\n");
	//SO_KEEPALIVE 지정 .상대방과 연결상태 점검
	if (setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, &opt_yes, sizeof(opt_yes))<0)
		printf("error ; keepalive \n");
	// servaddr 구조체의 내용 세팅
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(host);
	servaddr.sin_port = htons(port);
	if (bind(sd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("bind fail");  exit(1);
	}
	// 클라이언트로부터 연결요청을 기다림
	listen(sd, backlog);
	return sd;
}

//방 종료 함수
void creat_room(int sock, char *buf)   //채팅방을 개설해 주는 함수
{
	int room_no = MAXLINE;
	//빈 방 구조체 탐색
	for (int i = 0; i < MAXLINE; i++)
	{
		if (chat_rooms[i].is_open == 0)
		{
			room_no = i;
			break;
		}
	}
	//방 최대값 도달. 추가 생성 불가
	if (room_no == MAXLINE)
	{
		send(sock, "cannot open new chat room", sizeof("cannot open new chat room"), 0);
		return;
	}
	//빈 방 구조체 초기화.   (사용자 인원 = 0, 방 제목 입력)
	chat_rooms[room_no].user_cnt = 0;
	printf("creat room %d %d %s\n", sock, chat_rooms[room_no].user_cnt, buf);
	strcpy(chat_rooms[room_no].room_name, buf);
	//chat_room에 사용자 추가. 무조건 [0]일 것. 0은 호스트
	chat_rooms[room_no].user_list[chat_rooms[room_no].user_cnt] = sock;
	chat_rooms[room_no].user_list_exist[chat_rooms[room_no].user_cnt] = 1;
	//사용자 인원 추가. 이 함수에서는 무조건 1일 것
	chat_rooms[room_no].user_cnt++;
	//user_max 제한. 최대 인원수는 4명으로 고정
	chat_rooms[room_no].user_max = 4;
	//open 상태로 변경
	chat_rooms[room_no].is_open = 1;
	rooms_num++;
	//열린 방 번호 출력
	printf("create room success\n ", room_no);
}

//0 : 채팅방 없음, 1: 채팅방 있음
int out_room_list(int sock) { //채팅방 목록을 보여주는 함수
	int j;
	char room_list[10][MAXLINE];

	int chk_room = rooms_num;

	if (rooms_num < 1)
	{
		send(sock, "no exist room", sizeof("no exist room"), 0);
		return 0;
	}
	for (j = 0; j<chk_room; j++) {
		if (chat_rooms[j].is_open == 1)
		{
			sprintf(room_list[j], "%d. %s (%d/%d)\n", j, chat_rooms[j].room_name, chat_rooms[j].user_cnt, chat_rooms[j].user_max);
			send(sock, room_list[j], 10 * MAXLINE, 0);
			printf("%s\n", room_list[j]);
		}
		else chk_room++;
	}
	return 1;
}

//방 종료 함수
void close_room(int room_no)
{
	time_t current_time;
	time(&current_time);
	char file_name[20];
	//그림 저장
	sprintf(file_name, "%10d_%3d.txt\0", current_time, room_no);
	FILE *of = fopen(file_name, "w");
	for (int i = 0; i < 400; i++)
	{
		for (int j = 0; j < 400; j++)
		{
			for (int k = 0; k < 3; k++)
			{
				fprintf(of, "%d ", chat_rooms[room_no].pic_array[i][j][k]);
				chat_rooms[room_no].pic_array[i][j][k] = 0;
			}
		}
		printf("\n");
	}
	//방 종료
	chat_rooms[room_no].is_open = 0;
	rooms_num--;
	return;
}