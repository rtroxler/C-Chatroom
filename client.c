#include <stdio.h> //printf
#include <ncurses.h> //ncurses
#include <string.h>    //strlen
#include <sys/socket.h>    //socket
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h> //select
#include <arpa/inet.h> //inet_addr
#include <fcntl.h> //fcntl

#define TRUE 1
#define FALSE 0
#define PORT 2020 

void clearprompt();
char* concat(char *s1, char *s2);

void setnonblocking(sock)
int sock;
{
    int opts;
    opts = fcntl(sock,F_GETFL);
    if (opts < 0) {
        perror("fcntl(F_GETFL)");
        exit(1);
    }
    opts = (opts | O_NONBLOCK);
    if (fcntl(sock,F_SETFL,opts) < 0) {
        perror("fcntl(F_SETFL)");
        exit(1);
    }
    return;
}

int main(int argc , char *argv[])
{
    int sock, row, col, cur_row, max_fd;
    char nickname[80];
    struct sockaddr_in server;
    char msg[1000] , server_reply[2000];
    char *message, *username;
    fd_set readset;
    struct timeval tv;

    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons( PORT );

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }

    if (sock > fileno(stdin))
        max_fd = sock;

    setnonblocking(sock);

    cur_row = 0;

    initscr();
    getmaxyx(stdscr, row, col);

    mvprintw(LINES - 1, 0, "Enter your name >> ");
    getstr(nickname);
    clearprompt();

    //keep communicating with server
    for(;;)
    {
        // set time value to 1 second
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        FD_ZERO(&readset);
        FD_SET(sock, &readset);
        FD_SET(fileno(stdin), &readset);

        select(max_fd+1, &readset, NULL, NULL, &tv);

        if (FD_ISSET(sock, &readset))
        {
            if( recv(sock , server_reply , 2000 , 0) < 0)
            {
                puts("recv failed");
                break;
            } else {
                mvprintw(cur_row++, 0, "%s\n", server_reply);
                memset(server_reply, 0, 2000);
                clearprompt();
            }
        }
        if (FD_ISSET(fileno(stdin), &readset))
        {
            mvprintw(LINES - 1, 0, ">> ");
            getstr(msg);
            mvprintw(cur_row++, 0, "You: %s", msg);

            if( send(sock , msg, strlen(msg) , 0) < 0)
            {
                puts("Send failed");
                return 1;
            }
            clearprompt();
            //Send some data
        }


    }
    endwin();

    close(sock);
    return 0;
}

void clearprompt() {
    move(LINES - 1, 0);
    clrtoeol();
    mvprintw(LINES - 1, 0, ">> ");
    refresh();
}

char* concat(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}
