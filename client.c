/** chat_client **/
/*
*1. Client will create a loop keep generate socket for each time it runs.
*2. Then it requires users to input their name, server port number and ther ip address.
*3. Client will connect to server through the ip and the port number.
*4. If connect is success, it creates 2 thread, one send message and one receive message
*5. 2 threads run parallelly.
*6. Send thread creates a loop that keep reading input and send it to socket whereas receive
*   thread run a loop that keep reading socket and displaying it out.
*7. both send thread and receive thread will create a window for input purpose and output purpose
*8. Client will be terminated if 2 threads are terminated.
*9. The rest of program is about interface using ncurses library
*/
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<pthread.h>
#include<time.h>
#include<ncurses.h>
 
#define BUF_SIZE 100
#define NORMAL_SIZE 20
//===================================================================
//functions prototype
//===================================================================
void* send_msg(void* arg);
void* recv_msg(void* arg);
void  error_handling(char* msg);
void  changeName();
int   menu(void);
void  manual(void);
void  infoBox(void);
void  sideBar(void);
char* input(WINDOW *, int, int, int);
void  string_filter(char *,char**,char**);

char  name[BUF_SIZE]="[DEFALT]";     	// name
char  msg_form[NORMAL_SIZE];            // msg form
char  serv_time[NORMAL_SIZE];        	// server time
char  msg[BUF_SIZE];                    // msg
char  serv_port[NORMAL_SIZE];        	// server port number
char  clnt_ip[NORMAL_SIZE];            	// client ip address
 
//===================================================================
//MAIN
//===================================================================
int main(int argc, char *argv[])
{
    int 	sock;			//socket descriptor
    struct 	sockaddr_in serv_addr;  //socket address for server
    pthread_t 	snd_thread, rcv_thread;	//thread id variable
    void* 	thread_return;		//return value of thread
    int		choice = 0;		//control the loop

    initscr();				//init screen, allocate memory for windows
    curs_set(FALSE);			//make cursor disappear
    start_color();			//allow using color in windows
    use_default_colors();		//allow using default colors(current terminal color) to be used

START:					//START label for goto statement below
  while(1){				//this loop keep creating socket, binding address and creating threads
    choice = menu();			//call menu windows and read choice from user
    if(choice == 2){			//if choice = 2, clear windows, free allocated memory and end program
	    endwin();
	    return 0;
    }

    if (choice==1){			//if choice = 1, display manual windows
	    manual();
    }

  else{					//else of choice == 0, start the chat

    /** local time **/
    struct tm *t;			//struct time contains time information
    time_t timer = time(NULL);		//return current time since Jan 1 1970
    t=localtime(&timer);		//break up timer value(second) to tm structure
    sprintf(serv_time, "%d-%d-%d %d:%d", t->tm_year+1900, t->tm_mon+1, t->tm_mday,
		   			 t->tm_hour, t->tm_min);
    infoBox();				//display infobox, where user fills name, ip, port

    /***init information for socket***/
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(clnt_ip);
    serv_addr.sin_port=htons(atoi(serv_port));

    /***Create socket***/
    sock=socket(PF_INET, SOCK_STREAM, 0);
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1){
        error_handling(" connect() error");
	goto START;
    }

    sideBar();				//display side bar, where filled name, ip, port are showed.
 
    pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);//create a thread to send message
    pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);//create a thread to receive message
    pthread_join(snd_thread, &thread_return);		      //wait for send thread return
    pthread_join(rcv_thread, &thread_return);		      //wait for receive thread return
    close(sock);					      //close socket when done
    return 0;
  }
  }
}
//===================================================================
//send message
//===================================================================
void* send_msg(void* arg)
{
    int   sock	=*((int*)arg);		  //casting parameter to integer.
    char  name_msg[NORMAL_SIZE+BUF_SIZE]; //name_msg contains name and message content.

    WINDOW *twin = newwin(5, 80, 39, 0);  //create a windows to display input characters of user (tWin)
					  //window's size is 5x80, start at row 39, col 0
    wbkgd(twin, COLOR_PAIR(5));		  //set background color
    box(twin, 0, 0);			  //set border
    wrefresh(twin);			  //refresh this window to display all changed things

    while(1)
    {	/***echo input in the text box***/
 	strcpy(msg,input(twin, 1, 1, 5)); //input will return string that user entered, then copy it to msg
	werase(twin); box(twin,0,0); wrefresh(twin); //erase that windows - reset border - refresh win

        if (!strcmp(msg, "!q") || !strcmp(msg, "!Q")) //if user enter these input:
        {
            close(sock);		  //close socket
	    endwin();			  //free allocated memory
	    system("clear");		  //clear terminal screen
            exit(0);			  //exit right from function
        }
 
        /***send message***/
        sprintf(name_msg, "%s: %s", name,msg);  //concatenate user name with their message into the name_msg
        write(sock, name_msg, strlen(name_msg));//write it to the socket
    }
    return NULL;			
}
 
//===================================================================
//receive message
//===================================================================
void* recv_msg(void* arg)
{
    sleep(1);				//postpone this thread 1s so that send thread will run first	
    int sock=*((int*)arg);	        //cast parameter to integer.
    char name_msg[NORMAL_SIZE+BUF_SIZE];//contain name and message content.
    int str_len;			//string length.
    int maxy, maxx, y, x, row = 2;	//current window's information: maxy = max rows, maxx = max col...
    char *user, *message;		//store user's name and message contents separately.

    //cWin a.k.a "chat win" is main window where received messages and sending messages are displayed.
    WINDOW *cwin = newwin(30,59,8,0);	//create windows size = 30x59, start from row 8 col 0, named cWin
    getmaxyx(cwin, maxy, maxx);		//obtains max row, max column of cWin window
    wbkgd(cwin, COLOR_PAIR(1));		//set background color
    mvwprintw(cwin,1, maxx/2 - 4, "CHAT LOG");	//set title for window
    box(cwin, 0, 0);			//set border (0, 0 means default border)
    wrefresh(cwin);			//refresh cWin
    while(1)
    {	
	/***read message from socket***/
        str_len=read(sock, name_msg, NORMAL_SIZE+BUF_SIZE-1);
        if (str_len==-1)
            return (void*)-1;
        name_msg[str_len]= '\0'; //terminator character
	/***spilit string into user and message content***/
	string_filter(name_msg, &user, &message);
	
	/***user name will be colorized***/
L1:
	if(strcmp(user,name)){	//if message's from current user, user name will be blue
		wattron(cwin, COLOR_PAIR(2)); //turn attribute on, all next texts will be colorized by color
					      //pair no.2.
		mvwprintw(cwin,row,1,user); getyx(cwin, y,x); //print username in 'row' row, and first column
		mvwaddch(cwin, row,x+1, ':');		      //then get position of cursor row = y, col = x 
		wattroff(cwin, COLOR_PAIR(2));		      //turn off color
		mvwprintw(cwin, row, x+3, message);	      //print message's content right next to user name
	}
	else{	//if message's from other user, user name will be red
		wattron(cwin, COLOR_PAIR(3));
		mvwprintw(cwin,row,1,user); getyx(cwin, y,x);
		mvwaddch(cwin, row,x+1, ':');
		wattroff(cwin, COLOR_PAIR(3));
		mvwprintw(cwin, row, x+3, message);
	}
	
	/***I added this line to fix display bug, old code has bug if name + message
	length is longer than window's size then next line will overwrite old line. Delete this comment
	after reading ^_^***/
	if(strlen(user) + strlen(message) > 79) row++;

	row++;	//increment row so next line will not be overwritten.
	if(row > 29){  //if rows are more than window's size, clear it
		werase(cwin); box(cwin,0,0); wrefresh(cwin); //erase win, reset border, refresh display
		mvwprintw(cwin, 1, maxx/2-4, "CHAT LOG");    //reset title
		row = 2;				     //reset row to 2
		goto L1;				     //go to L1 to print the message
	}
	wrefresh(cwin);
    }
    return NULL;					     //exit thread.
}
//========================================================================
//Create a pop-up window to print out error
//========================================================================
void error_handling(char *msg)
{
    WINDOW *ewin = newwin(5,60,15,10);	//create eWin aka error window. Size 5x60, start at row 15, col 10
    wbkgd(ewin, COLOR_PAIR(6));		//set background is yellow, text is red
    box(ewin, 0, 0);			//set border

    mvwprintw(ewin,1,1,msg);		//print message to error window
    wrefresh(ewin);			//refresh window
    getch();				//wait user enter any character to exit window

    werase(ewin);			//erase window
    wbkgd(ewin, COLOR_PAIR(4));		//reset background to terminal color
    wrefresh(ewin);			//refresh
    return;
}
//===================================================================
//this function print out menu and information about team
//===================================================================
int menu(void){
	WINDOW *infoWin = newwin(7,80,0,0);  //inforWin is information Window which displays project information
	WINDOW *mwin    = newwin(10,80,8,0); //mwin is menu window, displays and performs menu
	int maxy, maxx, y,x;		     //maxy to store max row, maxx = max col, y = row, x = col
	keypad(mwin, true);		     //this function allows using arrow key
	getmaxyx(infoWin, maxy, maxx);	     //get max row and max col of infoWin
	
	/***initialize color pair: alias, foreground, background***/
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	init_pair(2, COLOR_BLUE, COLOR_WHITE);
	init_pair(3, COLOR_RED, COLOR_WHITE);
	init_pair(4, -1, -1);		     //-1, -1 will set color as terminal color
	init_pair(5, COLOR_BLACK, COLOR_YELLOW);
	init_pair(6, COLOR_RED, COLOR_YELLOW);

	wbkgd(infoWin, COLOR_PAIR(1));	//background for infoWin
	wbkgd(mwin, COLOR_PAIR(1));	//background for mwin
	refresh();			//refresh window
	box(infoWin, 0,0); box(mwin, 0, 0);  //set border for 2 windows.

	/***print out text with color to the infoWin***/
	wattron(infoWin, COLOR_PAIR(2));
	mvwprintw(infoWin, 1, maxx/2 - 10, "SANGMYUNG UNIVERSITY");
	mvwprintw(infoWin, 2, maxx/2 - 8, "UNIX PROGRAMMING");
	mvwprintw(infoWin, 3, maxx/2 - 5, "TEAM Unix.");
	wattroff(infoWin, COLOR_PAIR(2));

	wattron(infoWin, COLOR_PAIR(3));
	mvwprintw(infoWin, 4, maxx/2 - 8, "CHATTING PROJECT");
	wattroff(infoWin, COLOR_PAIR(3));
	mvwprintw(infoWin, 5, 11, "Baek Yun Cheol Prof. Students: JeongHyeon, Shawn, JeongSik");
	wrefresh(infoWin);
	//-----------------------------------------------------------
	//infoWin is done, now work with menu
	//-----------------------------------------------------------
	int i,j, choice, highlight = 0;		//i,j for loop control, choice is user choice, highlight is
						//row (it decides which row will be display as highlight)
	getmaxyx(mwin, maxy, maxx);
	mvwprintw(mwin, 1, maxx/2 - 2, "MENU");
	char str[3][16] = {"1. START", "2. MANUAL", "3. EXIT"};

	while(1){	//this loop run until user hit enter key
	   for(i = 0; i < 3; i++){		//print out menu options
		if(i == highlight)		//if current line = highlight, reverse its color
			wattron(mwin, A_REVERSE);
		mvwprintw(mwin, 3+i, maxx/2 - 4, str[i]);
		wattroff(mwin, A_REVERSE);
	   }
	wrefresh(mwin);				//refresh win

	choice = wgetch(mwin);			//get choice (arrow key)
	switch(choice){
	    case KEY_UP:			//key up: highlight - 1 (move up highlight)
		highlight = highlight - 1;
		if(highlight < 0) highlight = 0;
		break;
	    case KEY_DOWN:			//key down: highlight + 1(move down highlight)
		highlight = highlight + 1;
		if(highlight > 2) highlight = 2;
		break;
	    default:				//default: don't move
		break;
	}
	if(choice == 10) {			//if user hit enter key
		wclear(mwin);			//remove menu
		wbkgd(mwin, COLOR_PAIR(4));
		wrefresh(infoWin); wrefresh(mwin);
		return highlight;		//return highlight (user choice)
	}
	wrefresh(mwin);
	}
}

//===================================================================
//manual window, display detail and manual for user. It works similarly
//to above functions.
//===================================================================
void manual(void){
	int maxy, maxx, y, x;			
	WINDOW *manualWin = newwin(10,80,8,0);	//init window, set background, border...
	wbkgd(manualWin, COLOR_PAIR(1));
	box(manualWin,0,0);
	getmaxyx(manualWin, maxy, maxx);
	
	/***Print out content of manual***/
	mvwprintw(manualWin, 1, maxx/2 - 3, "MANUAL");
	mvwprintw(manualWin, 3, 1, "- This program is our practice-02 project");
	mvwprintw(manualWin, 4, 1, "- There is only 1 option you can choose:");
	mvwprintw(manualWin, 5, 1, "   + Enter !q or !Q to quit.");
	mvwprintw(manualWin, 6, 1, "Thank you, hope we'll get 100/100 for this project ^_^");
	wrefresh(manualWin);
	/***Wait user hit a key then exit***/
	getch();
	werase(manualWin);
	wbkgd(manualWin, COLOR_PAIR(4));
	wrefresh(manualWin); refresh();
}

//===================================================================
//info box is a window which merely display user name, client ip,
//server port number. It works similarly to above functions
//===================================================================
void infoBox(void){
	int maxy, maxx, y, x;
	WINDOW *iwin = newwin(8,80,8,0); //init win, background, border...
	getmaxyx(iwin, maxy, maxx);
	wbkgd(iwin, COLOR_PAIR(1));
	box(iwin,0,0);

	/*
	*First, input will be called to echo input to current window.
	*Second, input will return input string
	*Finally, copy that returned string to name, ip, port respectively
	*/
	mvwprintw(iwin, 1, maxx/2 - 11, "Fill below information.");
	mvwprintw(iwin, 2, 1, "Name:         "); getyx(iwin,y,x);
	strcpy(name, input(iwin,y,x,2));
	mvwprintw(iwin, 3, 1, "Client IP:    "); getyx(iwin,y,x);
	strcpy(clnt_ip,	input(iwin,y,x,2));
	mvwprintw(iwin, 4, 1, "Server Port:  "); getyx(iwin,y,x);
	strcpy(serv_port, input(iwin,y,x,2));
	mvwprintw(iwin, 5,1, "Connecting...");
	wrefresh(iwin);
	sleep(1);
	/***Clear this window after being done***/
	werase(iwin);
	wbkgd(iwin, COLOR_PAIR(4));
	wrefresh(iwin);
}

//===================================================================
//read input and echo input into a specified window
//y,x is where the first character is gonna be printed, num is color
//that is gonna be attributed.
//===================================================================
char *input(WINDOW *textWin, int y, int x, int num){
	char c;				//input character
	int i = 0;			//loop control
	char *string = malloc(256);	//store input string
	int maxy, maxx;			//max row and col of current window
	noecho();			//dont echo input to terminal window
	keypad(textWin, true);
	getmaxyx(textWin, maxy, maxx);
	wrefresh(textWin);

	wattron(textWin, COLOR_PAIR(num));//set color to text
	while(1){	//this loop keeps reading input character
	    if(x == maxx-1)//if current cursor > window's size, return string
		return string;
	    c = getch();
	    if(c == 10)	{		//if user hit enter key:
		string[i] = '\0';	//put terminator character
		wattroff(textWin, COLOR_PAIR(num)); //turn off color
		return string;		//return string
	    }
	    if(c == '-'){		//hit '-' to delete a character
		    if(i == 0) continue;
		    string[i-1] = '\0'; //remove last character in string too
		    mvwaddch(textWin, y, x-1, ' ');
		    i--; x--;		//decrement i and x
		    wrefresh(textWin);
	    }
	    /***read input and store it to string and display it in indicated window***/
	    else if((c>=32 && c<=90) || (c>=97 && c<=126)){
	    	string[i] = c;
	    	mvwaddch(textWin, y, x, c);
	    	x++, i++;
	    	wrefresh(textWin);
	    }
	}
}
//===================================================================
//side bar display current user name, port number and ip address
//===================================================================
void sideBar(void){
	int maxy,maxx, y,x;
	WINDOW *swin = newwin(30,20,8,60);
	wbkgd(swin, COLOR_PAIR(1));
	getmaxyx(swin, maxy, maxx);

	mvwprintw(swin, 1, maxx/2 - 2, "STATUS");
	mvwprintw(swin, 2, 1, "name: "); getyx(swin,y,x);
	wattron(swin, COLOR_PAIR(3));
	mvwprintw(swin, 2, x+1, name);
	wattroff(swin, COLOR_PAIR(3));

       	mvwprintw(swin, 3, 1, "port: ");getyx(swin,y,x);
	wattron(swin, COLOR_PAIR(3));
	mvwprintw(swin, 3, x+1, serv_port);
	wattroff(swin, COLOR_PAIR(3));

	mvwprintw(swin, 4, 1, "IP:"); getyx(swin, y, x);
	wattron(swin, COLOR_PAIR(3));
	mvwprintw(swin, 4, x+1, clnt_ip);
	wattroff(swin, COLOR_PAIR(3));

	box(swin, 0, 0);
	wrefresh(swin);
}

//===================================================================
//spilit string into name of user and message content.
//===================================================================
void string_filter(char *name_msg, char **user, char **message){
	*user = strtok(name_msg, ":");
	*message = strtok(NULL, "\n");
}
