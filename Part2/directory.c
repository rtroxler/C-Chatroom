#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/* port we're listening on */
#define PORT 5050
 
int main(int argc, char *argv[])
{
    fd_set master;
    fd_set read_fds;
    struct sockaddr_in diraddr;
    struct sockaddr_in clientaddr;
    int fdmax;
    int listener;
    int newfd;
    char buf[1024];
    char names[15][50];
    int ports[15];
    int nbytes;
    int yes = 1;
    int addrlen;
    int i, j;


    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    
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
    
    diraddr.sin_family = AF_INET;
    diraddr.sin_addr.s_addr = INADDR_ANY;
    diraddr.sin_port = htons(PORT);
    memset(&(diraddr.sin_zero), '\0', 8);
    
    if(bind(listener, (struct sockaddr *)&diraddr, sizeof(diraddr)) == -1)
    {
	perror("Error binding server.");
	exit(1);
    }
    
    if(listen(listener, 10) == -1)
    {
	perror("error on listener");
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
                        printf("Connection accepted by directory...\n");
                    
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
                    // handle data from chat server
                    if((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0)
                    {
                        // got error or connection closed by chat server
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
                        for(j = 0; j <= fdmax; j++)
                        {
                            if(FD_ISSET(j, &master))
                            {
                                if(names[j] != NULL) {
                                    ports[j] = atoi(buf);
                                } else {
                                    strcpy(names[j], buf);
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
