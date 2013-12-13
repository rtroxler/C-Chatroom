#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/* port we're listening on */
#define PORT 2020
#define DIR_PORT 5050
 
int main(int argc, char *argv[])
{
    fd_set master, read_fds;
    struct sockaddr_in serveraddr, clientaddr, diraddr;
    int fdmax, listener, newfd, port_no, nbytes, addrlen, i,j, dir_sock;
    char buf[1024], message[2000];
    char *chatroom;
    int yes = 1;

    if(argc == 1) {
        chatroom = "Default Chatroom";
        port_no = PORT;
    } else if (argc == 2) {
        chatroom = argv[1];
        port_no = PORT;
    } else {
        chatroom = argv[1];
        port_no = atoi(argv[2]);
    }

    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    //create dir socket
    dir_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (dir_sock == -1)
    {
        printf("Could not create dir socket.");
    }

    //setup directory
    diraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    diraddr.sin_family = AF_INET;
    diraddr.sin_port = htons(5050);
    memset(&(diraddr.sin_zero), '\0', 8);

    printf("Before connect to directory.");
    //connect to directory and send info
    if (connect(dir_sock, (struct sockaddr*)&diraddr, sizeof(diraddr)) < 0)
    {
        perror("connect to dir failed. Error.");
        return 1;
    }
    printf("Connected to directory.");
    if (send(dir_sock, chatroom, strlen(chatroom), 0) < 0) 
    {
        puts("Send chatname failed.");
        return 1;
    }
    if (send(dir_sock, port_no, strlen(port_no), 0) < 0) 
    {
        puts("Send port number failed.");
        return 1;
    }

    
    /* get the listener */
    if((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
	perror("Error setting up listener.");
	exit(1);
    }

    if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
	perror("Address already in use");
	exit(1);
    }
    printf("Chatroom: %s\n", chatroom);
    printf("Port: %d\n", port_no);
    
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(port_no);
    memset(&(serveraddr.sin_zero), '\0', 8);
    
    if(bind(listener, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1)
    {
	perror("Error binding server.");
	exit(1);
    }
    
    if(listen(listener, 10) == -1)
    {
	perror("Server error listening.");
	exit(1);
    }
    
    FD_SET(listener, &master);
    fdmax = listener; 
    
    for(;;)
    {
        read_fds = master;
        
        if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("Error on select().");
            exit(1);
        }
        
        //run through the existing connections looking for data to be read
        for(i = 0; i <= fdmax; i++)
        {
            if(FD_ISSET(i, &read_fds))
            { 
                if(i == listener)
                {
                    // handle new connections
                    addrlen = sizeof(clientaddr);
                    if((newfd = accept(listener, (struct sockaddr *)&clientaddr, &addrlen)) == -1)
                    {
                        perror("Error accepting new connection.");
                    }
                    else
                    {
                        printf("Connection accepted by server...\n");
                    
                        FD_SET(newfd, &master); // add to master set 
                        if(newfd > fdmax)
                        { 
                            fdmax = newfd;
                        }
                        printf("%s: New connection from %s on socket %d\n", argv[0], inet_ntoa(clientaddr.sin_addr), newfd);

                    }
                }
                else
                {
                    // handle data from a client 
                    if((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0)
                    {
                        // got error or connection closed by client 
                        if(nbytes == 0)
                        // connection closed 
                        printf("%s: socket %d hung up\n", argv[0], i);
                        
                        else
                        perror("recv() error.");
                        
                        close(i);
                        // remove from master set 
                        FD_CLR(i, &master);
                    }
                    else
                    {
                        // we got message
                        for(j = 0; j <= fdmax; j++)
                        {
                            // send to everyone! 
                            if(FD_ISSET(j, &master))
                            {
                                  // except the listener and ourselves 
                                  if(j != listener && j != i)
                                  {
                                          if(send(j, buf, nbytes, 0) == -1)
                                                perror("send() error.");
                                          printf("Data from socket %d: %s\n", i, buf);
                                          buf[0] = '\0';
                                          memset(buf, 0, 1024);
                                  }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
