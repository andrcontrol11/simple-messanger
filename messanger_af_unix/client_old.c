#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <wait.h>
#include <stdbool.h>
#include <errno.h>

void reset(fd_set *rfds,int socket){
    FD_ZERO(rfds);
    FD_SET(socket,rfds);
    FD_SET(0,rfds);
}

int main(int argc, char *argv[]) {
    int con = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK, 0);
    struct sockaddr_un addr_name;
    addr_name.sun_family = AF_UNIX;
    strcpy(addr_name.sun_path, "my server");
    int con2 = connect(con,(const struct sockaddr *) &addr_name, sizeof(addr_name));
    if(con2 == -1){
        printf("%s\n",strerror(errno));
        _exit(0);
    }
    printf("start chatting\n");
    char str[4096];
    fd_set rfds;
    int rmax = con;
    while(true){
        reset(&rfds, con);
        int rate = select(rmax + 1,&rfds,NULL,NULL,NULL);
        if(rate && FD_ISSET(con,&rfds)){
            char buff[4096];
            int cnt;
            if ((cnt = read(con, buff, 4096)) > 0){
                buff[cnt] = '\0';
                if(!strcmp(buff,"exit")){
                    write(con, buff, strlen(buff) + 1);
                    printf("other client leave chat\n");
                    break;
                }else{
                    printf("%s\n",buff);
                }
            }
        }
        if(rate && FD_ISSET(0,&rfds)){
            if(scanf("%[^\r\n]%*1[\r\n]",str) > 0){
                write(con, str, strlen(str) + 1);
                if(!strcmp(str,"exit")){
                    break;
                }
            }
        }
    }
    printf("end of chat\n");
    close(con2);
    close(con);
    unlink(addr_name.sun_path);
}
