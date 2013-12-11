#include <stdio.h> //printf
#include <stdlib.h>
#include <string.h>    //strlen
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr
#include <sys/time.h> //FD_SET, FD_ISSET

#define TRUE 1
#define FALSE 0
#define PORT 8888

char* concat(char *s1, char *s2);

int main(int argc , char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char nick[30], message[1000], server_reply[2000];
    char *nickname, send_msg;
    fd_set readset;

    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons( PORT );

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }

    puts("Connected\n");
    printf("Enter nickname: ");
    fgets(nick, sizeof(nick),stdin);
    nickname = concat(nick, ": ");
    printf("> ");

    //keep communicating with server
    while(TRUE)
    {
        FD_ZERO(&readset);
        FD_SET(fileno(stdin), &readset);

        //need to add the activity thing to be consistent
        select(fileno(stdin)+1, &readset, NULL, NULL, NULL);

        if (FD_ISSET(fileno(stdin), &readset))
        {
            fgets(message, sizeof(message),stdin);
            puts(">");

            send_msg = concat(nickname, message); 

            //Send some data
            if( send(sock , &send_msg, strlen(&send_msg) , 0) < 0)
            {
                puts("Send failed");
                return 1;
            }
        }

        //Receive a reply from the server
        if( recv(sock , server_reply , 2000 , 0) < 0)
        {
            puts("recv failed");
            break;
        }
        else {
            printf("Server Reply: %s\n", server_reply);
            server_reply[0]='\0'; 
        }


    }

    close(sock);
    return 0;
}

char* concat(char *s1, char *s2) 
{
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    char *result = malloc(len1+len2+1);
    memcpy(result, s1, len1);
    memcpy(result+len1, s2, len2+1);
    return result;
}
