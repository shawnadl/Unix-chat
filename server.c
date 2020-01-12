#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<pthread.h>
#include<time.h>
#include<ncurses.h>
#include<fcntl.h>
 
#define BUF_SIZE 100
#define MAX_CLNT 100
#define MAX_IP 30

void *handle_clnt(void *arg);
void send_msg(char *msg, int len);
void error_handling(char *msg);
char *serverState(int count);
void status_box(char port[]);
char *itoa(int, int);
int  menu(void);
int  server_init(void); 
 
int		clnt_cnt=0;
int	 	clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;
 
//========================================================================	
//main function
//========================================================================	
	int main(void)
	{
	    int 	serv_sock, clnt_sock, clnt_adr_sz;
	    int 	numeric_port, choice = 0;
	    char	char_port[5];
	    struct 	sockaddr_in serv_adr, clnt_adr;
	    pthread_t 	t_id;
	    int		fd;
	    char 	logtext[128];
	    fd = open("log.txt", O_CREAT | O_WRONLY, 0644);
	    if(fd == -1) error_handling("can not open file log");


	    /** time log **/
	    struct tm 	*t;
	    time_t timer = time(NULL);
	    t = localtime(&timer);
	    
	    /** init interface **/
	    initscr();
	    start_color();
	    use_default_colors();
	    noecho();

		init_pair(1, COLOR_BLACK, COLOR_WHITE);
		init_pair(2, COLOR_GREEN, COLOR_WHITE);
		init_pair(3, COLOR_BLUE, COLOR_WHITE);
		init_pair(4, COLOR_RED, COLOR_WHITE);
		init_pair(5, COLOR_BLACK, COLOR_YELLOW);
		init_pair(6, -1, -1);
		init_pair(7, COLOR_RED, COLOR_YELLOW);
START:		 choice = menu();
		 if(choice == 1){
			 endwin();
			 return 0;
		 }


	    /*** init server ***/
	    numeric_port = server_init();
	    strcpy(char_port, itoa(numeric_port, 10));
	    status_box(char_port);

	    /*** init LOG window ***/
	    WINDOW *lwin = newwin(40,80,15,0);
	    int maxy, maxx, y = 2, x; getmaxyx(lwin, maxy, maxx);
	    char buf[64];

	    wbkgd(lwin, COLOR_PAIR(1));
	    box(lwin, 0,0);
	    mvwprintw(lwin, 1, maxx/2-1, "LOG");
	    refresh(); wrefresh(lwin);

	    /***creating socket***/
	    pthread_mutex_init(&mutx, NULL);
	    serv_sock=socket(PF_INET, SOCK_STREAM, 0);
	    
	    memset(&serv_adr, 0, sizeof(serv_adr));
	    serv_adr.sin_family=AF_INET;
	    serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	    serv_adr.sin_port=htons(numeric_port);
	    
	    /***binding address to the socket***/
	    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1){
		error_handling("bind() error, PORT is used!");
		werase(lwin); wbkgd(lwin, COLOR_PAIR(6));
		wrefresh(lwin);
		goto START;
	    }

	    /***Listening to a connection on a socket***/
	    if (listen(serv_sock, 5)==-1){
		error_handling("listen() error, program is stopped!");
		werase(lwin);
		wbkgd(lwin, COLOR_PAIR(6)); wrefresh(lwin);
		goto START;
	    }
	    /***While connection is found***/
	    while(1)
	    {
		/***log the time***/
		t=localtime(&timer);
		clnt_adr_sz=sizeof(clnt_adr);
		/***accept connection request clnt_sock = socket's descriptor***/
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,(socklen_t*)&clnt_adr_sz);
	 
		/***clnt_sock is refered to the newest connection***/
		pthread_mutex_lock(&mutx);	  //this is critical segment, just can be accessed
		clnt_socks[clnt_cnt++]=clnt_sock; //one time by one thread to prevent editing data
		pthread_mutex_unlock(&mutx);	  //simultanously
	 
		/***create a thread to handle client***/
		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
		/***detach the thread, don't care about it anymore***/
		pthread_detach(t_id);
		
		/***write down to log window the history of incoming connection***/
		if(y>=38){
			werase(lwin); box(lwin,0,0); wrefresh(lwin); y = 2;
		}
		mvwprintw(lwin,y,1, "connected client IP: "); strcpy(logtext, "connected client IP: ");
		getyx(lwin, y,x);
		wattron(lwin, COLOR_PAIR(3));
		mvwprintw(lwin,y,x+1, inet_ntoa(clnt_adr.sin_addr));
		strcat(logtext, inet_ntoa(clnt_adr.sin_addr));
		wattroff(lwin, COLOR_PAIR(3));

		strcpy(buf,itoa(t->tm_year+1900, 10)); strcat(buf, "-");
		strcat(buf,itoa(t->tm_mon+1, 10)); strcat(buf, "-");
		strcat(buf, itoa(t->tm_mday, 10)); strcat(buf, " ");
		strcat(buf, itoa(t->tm_hour, 10)); strcat(buf, ":");
		strcat(buf, itoa(t->tm_min, 10));

		getyx(lwin, y,x);
		mvwprintw(lwin, y, x+1, buf); strcat(logtext, buf);

		getyx(lwin, y,x);
		mvwprintw(lwin,y,x+1, "USER: "); strcat(logtext, "USER: ");
		strcpy(buf, itoa(clnt_cnt, 10));
		wattron(lwin, COLOR_PAIR(2));
		mvwprintw(lwin, y, x+7, buf); strcat(logtext, buf); strcat( logtext, "\n");
		wattroff(lwin, COLOR_PAIR(2));
		lseek(fd, 0, SEEK_END);
		write(fd, logtext, strlen(logtext));
		y++;
		/***End writting log***/
	       wrefresh(lwin);
	    }
	    close(serv_sock);
	    return 0;
	}
	 
//========================================================================	
//function for thread, each thread will read from a fixed socket descriptor
//========================================================================	
void *handle_clnt(void *arg)
	{
	    int clnt_sock=*((int*)arg);
	    int str_len=0, i, close_status = 0;
	    char msg[BUF_SIZE];
	    
	    /***read data from client socket and pass it to send_msg***/
	    while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0)
		send_msg(msg, str_len);
	 
	    /***if read = -1 remove the socket***/
	    pthread_mutex_lock(&mutx);
	    for (i=0; i<clnt_cnt; i++)
	    {
		if (clnt_sock==clnt_socks[i])
		{
		    while(i++<clnt_cnt-1)
			clnt_socks[i]=clnt_socks[i+1];
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutx);
    close(clnt_sock);
    pthread_exit((void *)0);
}
 
//========================================================================	
//Send message function, this function will write message to all sockets
//========================================================================	
void send_msg(char* msg, int len)
{
    int i;
    pthread_mutex_lock(&mutx);		//critical section
    for (i=0; i<clnt_cnt; i++)
        write(clnt_socks[i], msg, len);
    pthread_mutex_unlock(&mutx);
}
 
//========================================================================	
//Create a pop-up window to print out error
//========================================================================	
void error_handling(char *msg)
{
    WINDOW *ewin = newwin(5,60,15,10);
    wbkgd(ewin, COLOR_PAIR(7));
    box(ewin, 0, 0);

    mvwprintw(ewin,1,1,msg);
    wrefresh(ewin);
    getch();

    werase(ewin);
    wbkgd(ewin, COLOR_PAIR(6));
    wrefresh(ewin);
    return;
}
 
//========================================================================	
//print out server state
//========================================================================	
char* serverState(int count)
{
    char* stateMsg = malloc(sizeof(char) * 20);
    strcpy(stateMsg ,"None");
    
    if (count < 5)
        strcpy(stateMsg, "Good");
    else
        strcpy(stateMsg, "Bad");
    
    return stateMsg;
}        
 
//========================================================================	
//create a window print out server's status like port, state, number of client
//========================================================================	
void status_box(char port[])
{
	char buf[256];
	int maxy, maxx, y,x;


	WINDOW *swin = newwin(6,80,8,0);
	refresh();
	getmaxyx(swin, maxy, maxx);


	box(swin, 0, 0);
	wbkgd(swin, COLOR_PAIR(1));
	mvwprintw(swin, 1, maxx/3, "UNIX-PROG CHAT SERVER(C)");
	mvwprintw(swin, 2, 1, "SERVER PORT     ");
	getyx(swin, y, x);
	mvwprintw(swin, y, x+1, port);

	mvwprintw(swin, 3, 1, "SERVER STATE    ");
	getyx(swin, y, x);
	
	wattron(swin, COLOR_PAIR(2));
	mvwprintw(swin, y, x+1, serverState(clnt_cnt));
	wattroff(swin, COLOR_PAIR(2));

	mvwprintw(swin, 4, 1, "MAX CONNECTION  ");
	getyx(swin, y, x);
	mvwprintw(swin, y, x+1, "100");

	wrefresh(swin);
	//getch();
	//endwin();
}
//========================================================================	
//convert integer to string
//========================================================================	
char* itoa(int val, int base){

	static char buf[32] = {0};

	int i = 30;

	for(; val && i ; --i, val /= base)

		buf[i] = "0123456789abcdef"[val % base];

	return &buf[i+1];

}

//========================================================================	
//create menu window
//========================================================================	
int menu(void){
	/***Banner window contains team and project's information***/
	WINDOW *infoWin = newwin(7,80,0,0);
	WINDOW *mwin    = newwin(10,80,8,0);
	int maxy, maxx, y,x;
	keypad(mwin, true);
	getmaxyx(infoWin, maxy, maxx);

	wbkgd(infoWin, COLOR_PAIR(1));
	wbkgd(mwin, COLOR_PAIR(1));
	refresh();
	box(infoWin, 0,0); box(mwin, 0, 0);

	wattron(infoWin, COLOR_PAIR(3));
	mvwprintw(infoWin, 1, maxx/2 - 10, "SANGMYUNG UNIVERSITY");
	mvwprintw(infoWin, 2, maxx/2 - 8, "UNIX PROGRAMMING");
	mvwprintw(infoWin, 3, maxx/2 - 5, "TEAM Unix.");
	wattroff(infoWin, COLOR_PAIR(3));
	wattron(infoWin, COLOR_PAIR(4));
	mvwprintw(infoWin, 4, maxx/2 - 8, "CHATTING PROJECT");
	wattroff(infoWin, COLOR_PAIR(4));
	mvwprintw(infoWin, 5, 11, "Baek Yun Cheol Prof. Students: JeongHyeon, JeongSik, Shawn");
	wrefresh(infoWin);

	/***Select menu***/
	int i,j, choice, highlight = 0;
	getmaxyx(mwin, maxy, maxx);
	mvwprintw(mwin, 1, maxx/2 - 2, "MENU");
	char str[2][16] = {"1. START", "2. EXIT "};
	
	while(1){
	   for(i = 0; i < 2; i++){
		if(i == highlight) wattron(mwin, A_REVERSE);
		mvwprintw(mwin, 3+i, maxx/2 - 4, str[i]);
		wattroff(mwin, A_REVERSE);
	   }
	wrefresh(mwin);

	choice = wgetch(mwin);
	switch(choice){
	    case KEY_UP:
		highlight = highlight - 1;
		if(highlight < 0) highlight = 0;
		break;
	    case KEY_DOWN:
		highlight = highlight + 1;
		if(highlight > 1) highlight = 1;
		break;
	    default:
		break;
	}
	if(choice == 10) {
		wclear(mwin);
		wbkgd(mwin, COLOR_PAIR(6));
		wrefresh(infoWin); wrefresh(mwin);
		return highlight;
	}
	wrefresh(mwin);
	}
}

//========================================================================	
//Asking user input port number to initialize server
//========================================================================	
int server_init(void){
	char c, buf[5];
	int y, col, i = 0;
	WINDOW *initWin = newwin(5, 80, 8, 0);
	wbkgd(initWin, COLOR_PAIR(1));
	box(initWin,0,0);
	mvwprintw(initWin, 2, 1, "Enter PORT number: ");

	wrefresh(initWin);
	getyx(initWin, y, col);

	wattron(initWin, COLOR_PAIR(4));
	while(1){
		c = getch();
		if( c == 10) break;
		buf[i] = c;
		mvwaddch(initWin, 2, col, c);
		col++; i++;
		wrefresh(initWin);
	}
	wattroff(initWin, COLOR_PAIR(4));
	return atoi(buf);
}

