
// recieve name from client and broadcast to others
if((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0)
{
    perror("recv() name error.");
} else {
    if (fdmax > 1) {
        strncpy(message, buf, MSG_SIZE - 1);
        message[MSG_SIZE - 1] = '\0';
        strncat(message, " has joined the chat.", MSG_SIZE - strlen(message) - 1);
        for(j = 0; j <= fdmax; j++)
        {
            if(FD_ISSET(j, &master))
            {
                  if(j != listener && j != i)
                  {
                          if(send(j, message, nbytes, 0) == -1)
                                perror("send() error.");
                          printf("%s\n", message);
                          message[0] = '\0';
                          memset(message, 0, MSG_SIZE);
                  }
            }
        }
    } else {
          if(send(i, "You are the first user to join the chat.", nbytes, 0) == -1)
                perror("send() error.");
    }
}
