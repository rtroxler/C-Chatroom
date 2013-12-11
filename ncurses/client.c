/*
 ** client.c -- a stream socket client demo
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <wchar.h>
#include <curses.h>
#include <locale.h>
 
#define PORT 1234              // the server's port
#define BUFFER_SIZE 253        // max number of bytes we can send at once
#define NAME_SIZE 30
 
int sockfd;
WINDOW* type_window;
WINDOW* message_window;
pthread_mutex_t scr_mutex = PTHREAD_MUTEX_INITIALIZER;
 
void *receiver();
void *window_resizer();
int sendall(int fd, char *buf, int *len);
 
int
main(int argc, char *argv[])
{
    if (!setlocale(LC_CTYPE, "")) {
      fprintf(stderr, "Can't set the specified locale! "
              "Check LANG, LC_CTYPE, LC_ALL.\n");
      return 1;
    }
 
    pthread_t receiver_thread;
    pthread_t resizer_thread;
    // buffer format:
    // length(1 byte) + name(no null terminated) + ":\n\t" + sentense(null terminated)
    char buf[BUFFER_SIZE+2];
    char name[NAME_SIZE+2];
    int length;
    struct hostent *he;
    struct sockaddr_in their_addr; // connector's address information 
 
    if (argc != 2) {
        fprintf(stderr, "usage: client hostname\n");
        exit(1);
    }
 
    //ncurses initialization
    initscr();
    cbreak();
    echo();
 
    type_window = derwin(stdscr,1,getmaxx(stdscr)-1,getmaxy(stdscr)-1,0);
    message_window = derwin(stdscr,getmaxy(stdscr)-3,getmaxx(stdscr)-1,0,0);
    scrollok(message_window,TRUE);
    keypad(stdscr, TRUE);
    keypad(type_window, TRUE);
    wmove(message_window,0,0);
 
    // get the host info
    if ((he = gethostbyname(argv[1])) == NULL) {
        herror("gethostbyname");
        endwin();
        exit(1);
    }
 
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        endwin();
        exit(1);
    }
 
    their_addr.sin_family = AF_INET;        // host byte order 
    their_addr.sin_port = htons(PORT);        // short, network byte order 
    their_addr.sin_addr = *((struct in_addr *) he->h_addr);
    memset(their_addr.sin_zero, 0, sizeof their_addr.sin_zero);
 
    if (connect(sockfd, (struct sockaddr *) &their_addr, sizeof their_addr)
        == -1) {
        perror("connect");
        endwin();
        exit(1);
    }
 
    //start auto resizer thread
    pthread_create(&resizer_thread, NULL, &window_resizer, NULL);
 
    //start receiver thread
    pthread_create(&receiver_thread, NULL, &receiver, NULL);
 
    //get user's name
    mvwprintw(type_window,0,0,"Enter your name: ");
    wgetnstr(type_window,name+1,NAME_SIZE);
    werase(type_window);
    length = strlen(name+1)+2;
    name[0]=(char)length;
 
    //send user's name to server
    sendall(sockfd, name, &length);
 
    /* another version
    char c;
    int buf_index;
    */
    while(1){
        //user input
        //mutual exclusion to aviod that both thread write buffer of screen
        //   at the same time
        pthread_mutex_lock(&scr_mutex);
            wprintw(type_window,">");
        pthread_mutex_unlock(&scr_mutex);
 
        /* another version
        for(buf_index=strlen(name)+4; buf_index < BUFFER_SIZE-buf_index; buf_index++){
            char c = wgetch(type_window);
            if(c=='\n')
                break;
            pthread_mutex_lock(&scr_mutex);
                waddch(type_window,c);
            pthread_mutex_unlock(&scr_mutex);
            buf[buf_index]=c;
        }
        buf[buf_index]='\0';
        */
 
        wgetnstr(type_window,buf+1,BUFFER_SIZE);
 
        //check quit command
        if( strcmp( buf+1,":quit")==0 ){
            delwin(type_window);
            endwin();
            exit(1);
        }
 
        //calculate length
        length = strlen(buf+1)+2;
        buf[0]=(char)length;
        //send buffer to server
        sendall(sockfd, buf, &length);
 
        //mutual exclusion to aviod that both thread write buffer of screen
        //   at the same time
        pthread_mutex_lock(&scr_mutex);
            //clear input area
            wmove(type_window,0,0);
            werase(type_window);
        pthread_mutex_unlock(&scr_mutex);
        //refresh screen
        wrefresh(type_window);
 
        usleep(1000);
    }
    close(sockfd);
    delwin(type_window);
    endwin();
    return 0;
}
 
void*
window_resizer(){
    int max_y = getmaxy(stdscr);
    int max_x = getmaxx(stdscr);
    while(1){
        if(max_y != getmaxy(stdscr) || max_x != getmaxx(stdscr)){
            //note: type_window = derwin(stdscr,1,getmaxx(stdscr)-1,getmaxy(stdscr)-1,0);
            max_y = getmaxy(stdscr);
            max_x = getmaxx(stdscr);
            pthread_mutex_lock(&scr_mutex);
                //erase, the screen won't be a mess
                werase(type_window);
                wrefresh(type_window);
                mvwin(type_window,getmaxy(stdscr)-1,0);
                wresize(type_window,1,getmaxx(stdscr)-1);
                wrefresh(type_window);
                werase(message_window);
                wrefresh(message_window);
                wresize(message_window, getmaxy(stdscr)-1, getmaxx(stdscr));
                wrefresh(message_window);
            pthread_mutex_unlock(&scr_mutex);
        }
        usleep(1000);
    }
 
}
 
void*
receiver()
{
    char buf;
    int test;
    while(1){
        // handle data from a server
        if ((test = recv(sockfd, &buf, sizeof(buf), 0)) <= 0) {
            // got error or connection closed by client
            if (test == 0)        // connection closed
                printf("server: socket %d hung up\n", sockfd);        
            else
                perror("recv");
            break;
        } else {
            pthread_mutex_lock(&scr_mutex);
                wprintw(message_window,"%c", buf);
            pthread_mutex_unlock(&scr_mutex);
            wrefresh(message_window);
        }
        usleep(1000);
    }
    close(sockfd);
    pthread_exit(NULL);
}
 
int
sendall(int fd, char *buf, int *len)
{
    int total = 0;               // how many bytes we've sent
    int bytesleft = *len;        // how many we have left to send
    int n;
    while (total < *len) {
        n = send(fd, buf + total, bytesleft, 0);
        if (n == -1) {
            break;
        }
        total += n;
        bytesleft -= n;
    }
    *len = total;                // return number actually sent here
    return n == -1 ? -1 : 0;     // return -1 on failure, 0 on success
}
