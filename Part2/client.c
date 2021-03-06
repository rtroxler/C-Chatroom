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
#define MSG_SIZE 1200

void clearprompt();

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
    struct sockaddr_in server;
    char msg[1000] , server_reply[2000], name[80], message[MSG_SIZE];
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

    //start curses
    initscr();
    getmaxyx(stdscr, row, col);

    //get user name
    mvprintw(LINES - 1, 0, "Enter your name >> ");
    getstr(name);
    clearprompt();
    strtok(name, "\n");

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

        //activity on socket, incoming msg
        if (FD_ISSET(sock, &readset))
        {
            //clear extra junk 
            memset(server_reply, 0, 2000);
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
        //activity from stdin, writing msg
        if (FD_ISSET(fileno(stdin), &readset))
        {
            memset(msg, 0, 1000);
            mvprintw(LINES - 1, 0, ">> ");
            getstr(msg);
            mvprintw(cur_row++, 0, "You: %s", msg);
            strtok(msg, "\n");

            //format message to include username to send to server
            strncpy(message, name, MSG_SIZE - 1);
            message[MSG_SIZE - 1] = '\0';
            strncat(message, ": ", MSG_SIZE - strlen(message) - 1);
            strncat(message, msg, MSG_SIZE - strlen(message) - 1);

            //Send message to server
            if( send(sock , message, strlen(message) , 0) < 0)
            {
                puts("Send failed");
                return 1;
            }
            clearprompt();
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

