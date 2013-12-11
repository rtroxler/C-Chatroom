/*
 ** server.c -- a multi-threaded chat server
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <wchar.h>
 
#define PORT 1234        // port we're listening on
#define MAX_SESSION 100  // max session number
#define BUFFER_SIZE 253  // buffer size
#define NAME_SIZE 30     // name size
 
// buffer mutual exclusion
pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;        
 
struct sockaddr_in myaddr;     // server address
struct sockaddr_in remoteaddr; // client address
socklen_t       addrlen;
 
int             listener;      //listener socket descriptor
 
typedef struct session {
    int       index;
    pthread_t thread;
    int       fd;
    char      name[NAME_SIZE+1];//+1 for '\0'
    int       length;
    char      buffer[BUFFER_SIZE+1];//+1 for '\0'
    // session* next; //for linked list, not implemented
} session_t;
 
session_t sessions[MAX_SESSION];
 
void  initial_sessions();
int   search_availiable();
void *session_thread(void *session_index);
void  send_to_all(session_t * sender);
void  close_and_release(session_t * session);
int   recvall(int fd, char *buf, int *len);
int   sendall(int fd, char *buf, int len);
 
int
main(void)
{
    initial_sessions();
    int cur_session_index;
    // get the listener
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    // lose the pesky "address already in use" error message
    int yes = 1; // for setsockopt() SO_REUSEADDR
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
        == -1) {
        perror("setsockopt");
        exit(1);
    }
    // bind
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = INADDR_ANY;
    myaddr.sin_port = htons(PORT);
    memset(myaddr.sin_zero, '\0', sizeof myaddr.sin_zero);
    if (bind(listener, (struct sockaddr *) &myaddr, sizeof myaddr) == -1) {
        perror("bind");
        exit(1);
    }
    printf("bind to port %d.\n", PORT);
 
    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(1);
    }
    printf("listen to port %d on socket %d\n", PORT, listener);
 
    // main loop
    while (1) {
        // handle new connections
        cur_session_index = search_availiable();
        addrlen = sizeof remoteaddr;
        if ((sessions[cur_session_index].fd =
             accept(listener, (struct sockaddr *) &remoteaddr, &addrlen))
             == -1) {
            perror("accept");
        } else {
            printf("accepted %d, assign to session %d\n",
                   sessions[cur_session_index].fd, cur_session_index);
            pthread_create(&(sessions[cur_session_index].thread), NULL,&session_thread,
                           &(sessions[cur_session_index].index));
            printf("server: new connection from %s on socket %d, session %d\n",
                   inet_ntoa(remoteaddr.sin_addr),
                   sessions[cur_session_index].fd, cur_session_index);
        }
    }
    return 0;
}
 
void
initial_sessions()
{
    int i;
    for (i = 0; i < MAX_SESSION; i++) {
        sessions[i].index = i;
        sessions[i].fd = -1;
    }
}
 
int
search_availiable()
{
    int i;
    for (i = 0; i < MAX_SESSION; i++)
        if (sessions[i].fd == -1)
            return i;
}
 
void
close_and_release(session_t * session)
{
    close(session->fd);
    session->fd = -1; // set invalid
}
 
void*
session_thread(void *session_index)
{
    int  my_index = *((int *) session_index);
    int  test;
    int  read;
    char length;
    // handle data from a client
    printf("session %d's fd %d\n", my_index, sessions[my_index].fd);
 
    //get clients' name
    //receive first char; length of name
    if ((test = recv(sessions[my_index].fd, &length, sizeof(char), 0)) <= 0) {
        // got error or connection closed by client
        if (test == 0)
            // remote connection closed
            printf("selectserver: socket %d hung up\n", sessions[my_index].fd);        
        else
            perror("recv");
    } else {
        // we got some data from a client
        // length includes '\0' tail and 1 byte header
        printf("get name, length = %d\n", length);
        read = (int) length - 1;
        if (recvall(sessions[my_index].fd, sessions[my_index].name , &read) == -1) {
            printf("connection closed or failed to receive");
        }else{
            while (1) {
                //receive data
                //receive first char; length of data
                if ((test = recv(sessions[my_index].fd, &length, sizeof(char), 0)) <= 0) {
                    // got error or connection closed by client
                    if (test == 0)
                        // remote connection closed
                        printf("selectserver: socket %d hung up\n", sessions[my_index].fd);
                    else
                        perror("recv");
                    break;
                } else {
                    // we got some data from a client
                    // length includes '\0' tail and 1 byte header
                    printf("get data, length = %d\n", length);
 
                    read = (int) length - 1;
                    if (recvall(sessions[my_index].fd, sessions[my_index].buffer, &read) == -1) {
                        printf("connection closed or failed to receive");
                        break;
                    }
                    //send message to all clients, with mutual exclusion
                    pthread_mutex_lock(&send_mutex);
                        send_to_all(&sessions[my_index]);
                    pthread_mutex_unlock(&send_mutex);
                }
                usleep(1000);
            }
        }
    }
    close_and_release(&sessions[my_index]);
}
 
void
send_to_all(session_t * sender)
{
    int i;
    for (i = 0; i < MAX_SESSION; i++)
        if (sessions[i].fd != -1){
            printf("send:%s\n",sender->name);
            sendall(sessions[i].fd, sender->name, strlen(sender->name));
            sendall(sessions[i].fd, ":\n    ", 6*sizeof(char));
            printf("send:%s\n",sender->buffer);
            sendall(sessions[i].fd, sender->buffer, strlen(sender->buffer));
            sendall(sessions[i].fd, "\n", sizeof(char));
        }
}
 
int
recvall(int fd, char *buf, int *len)
{
    int             total = 0;       // how many bytes we've sent
    int             bytesleft = *len;// how many we have left to send
    int             read;
    while (total < *len) {
        if ((read = recv(fd, buf + total, bytesleft, 0)) <= 0) {
            if (read == 0) {
                printf("selectserver: socket %d hung up\n", fd);
            } else {                 // read == -1
                perror("recv");
            }
            break;
        } else {
            total += read;
            bytesleft -= read;
        }
    }
    *len = total;                    // return number actually sent here
    return (read <= 0) ? -1 : 0;     // return -1 on failure, 0 on success
}
 
int
sendall(int fd, char *buf, int len)
{
    int total = 0;       // how many bytes we've sent
    int bytesleft = len;// how many we have left to send
    int n;
    while (total < len) {
        n = send(fd, buf + total, bytesleft, 0);
        if (n == -1) {
            break;
        }
        total += n;
        bytesleft -= n;
    }
    return n == -1 ? -1 : 0;         // return -1 on failure, 0 on success
}
