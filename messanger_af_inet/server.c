#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

void reset(fd_set *rfds, int socket, const int *clients, int cnt) {
    FD_ZERO(rfds);
    FD_SET(socket, rfds);
    FD_SET(0, rfds);
    for (int i = 0; i < cnt; ++i) {
        FD_SET(clients[i], rfds);
    }
}

struct map {
    int my_fd;
    int friend_fd;
    char my_name[4096];
    char friend_name[4096];
    bool flag;
};

void add_friend(struct map *mp, int cnt2, char *buff, int fr_fd) {
    mp[cnt2].flag = true;
    strcpy(mp[cnt2].friend_name, buff);
    mp[cnt2].friend_fd = fr_fd;
    for (int i = 0; i < cnt2; ++i) {
        if (mp[cnt2].friend_fd == mp[i].my_fd) {
            mp[i].flag = true;
            mp[i].friend_fd = mp[cnt2].my_fd;
            strcpy(mp[i].friend_name, mp[cnt2].my_name);
            break;
        }
    }
}

bool find_friend(struct map *mp, int cnt2, char *buff, int *fr_fd) {
    for (int i = 0; i < cnt2; ++i) {
        if (!strcmp(buff, mp[i].friend_name)) {
            *fr_fd = 0;
            return true;
        }
        if (!strcmp(buff, mp[i].my_name)) {
            *fr_fd = mp[i].my_fd;
        }
    }
    return false;
}

void add_client(int data, struct map *mp, int *cnt2, char *buff) {
    char *message1 = "enter your name";
    char *message2 = "chose your friend name";
    write(data, message1, strlen(message1) + 1);
    if (read(data, buff, 4096) > 0) {
        printf("start saving\n");
        mp[*cnt2].my_fd = data;
        strcpy(mp[*cnt2].my_name, buff);
        mp[*cnt2].flag = false;
        printf("end saving\n");
        if (*cnt2 > 0) {
            write(data, message2, strlen(message2) + 1);
        }
        for (int i = 0; i < *cnt2; ++i) {
            write(data, mp[i].my_name, strlen(mp[i].my_name) + 1);
        }
        int fr_fd;
        if (*cnt2 == 0) {
            printf("cnt2 == 0\n");
            char *nobody = "sorry, there is nobody to talk";
            write(data, nobody, strlen(nobody) + 1);
        } else if (read(data, buff, 4096) > 0) {
            bool flag = find_friend(mp, *cnt2, buff, &fr_fd);
            if (flag) {
                char *sorry = "sorry Mario but your princess is in another castle";
                write(data, sorry, strlen(sorry) + 1);
            } else {
                printf("find friend\n");
                add_friend(mp, *cnt2, buff, fr_fd);
            }
        }
        ++*cnt2;
    }
}

void clean_clients(int *clients, int *cnt, int q, int i) {
    close(clients[q]);
    close(clients[i]);
    int z = i;
    if (z > q) {
        z = q;
        q = i;
    }
    if (z != *cnt && q != *cnt) {
        clients[z] = clients[*cnt];
    }
    if (*cnt >= 2) {
        if (z != *cnt - 1 && q != *cnt - 1) {
            clients[q] = clients[*cnt - 1];
        }
    }
    *cnt -= 2;
}

void clean_mp(struct map *mp, int *cnt2, int j) {
    int k = 0;
    while (mp[k].my_fd != mp[j].friend_fd) {
        ++k;
    }
    if (j > k) {
        int p = k;
        k = j;
        j = p;
    }
    if (j != *cnt2 && k != *cnt2) {
        mp[j].my_fd = mp[*cnt2].my_fd;
        strcpy(mp[j].my_name, mp[*cnt2].my_name);
        mp[j].friend_fd = mp[*cnt2].friend_fd;
        strcpy(mp[j].friend_name, mp[*cnt2].friend_name);
        mp[j].flag = mp[*cnt2].flag;
    }
    if (*cnt2 >= 2) {
        if (j != *cnt2 - 1 && k != *cnt2 - 1) {
            mp[k].my_fd = mp[*cnt2 - 1].my_fd;
            strcpy(mp[k].my_name, mp[*cnt2 - 1].my_name);
            mp[k].friend_fd = mp[*cnt2 - 1].friend_fd;
            strcpy(mp[k].friend_name, mp[*cnt2 - 1].friend_name);
            mp[k].flag = mp[*cnt2 - 1].flag;
        }
    }
    *cnt2 -= 2;
}

int main(int argc, char *argv[]) {
    int con = socket(AF_INET, SOCK_STREAM, 0);
    printf("socket 1\n");
    struct sockaddr_in addr_name;
    addr_name.sin_family = AF_INET;
    addr_name.sin_addr.s_addr =  INADDR_ANY;
    addr_name.sin_port = 11116;
    if (bind(con, (const struct sockaddr *) &addr_name, sizeof(addr_name)) == -1) {
        perror("bind problem");
        exit(1);
    }
    printf("bind\n");
    listen(con, 1);
    pause();
    printf("listen\n");
    char buff[4096];
    int clients[30];
    int cnt = 0;
    fd_set rfds;
    int data;
    int rmax = con;
    struct map mp[30];
    int cnt2 = 0;
    while (true) {
        reset(&rfds, con, clients, cnt);
        int rate = select(rmax + 1, &rfds, NULL, NULL, NULL);
        if (rate == -1) {
            perror("select error\n");
            _exit(1);
        }
        if (rate && FD_ISSET(con, &rfds)) {
            printf("new connection\n");
            data = accept(con, NULL, NULL);
            if (rmax < data) {
                rmax = data;
            }
            clients[cnt] = data;
            ++cnt;
            add_client(data, mp, &cnt2, buff);
        }
        for (int i = 0; i < cnt; ++i) {
            if (rate && FD_ISSET(clients[i], &rfds)) {
                int j = 0;
                int q = 0;
                while (mp[j].my_fd != clients[i]) {
                    ++j;
                }
                while (clients[q] != mp[j].friend_fd) {
                    ++q;
                }
                if (!mp[j].flag) {
                    continue;
                }
                if ((read(mp[j].my_fd, buff, 4096)) > 0) {
                    printf("read %s\n", buff);
                    char buff2[4096];
                    if (!strcmp(buff, "exit")) {
                        write(mp[j].friend_fd, buff, strlen(buff));
                        printf("write %s\n", buff2);
                        clean_clients(clients, &cnt, q, i);
                        clean_mp(mp, &cnt2, j);
                        continue;
                    }
                    char message[4096];
                    strcpy(message, mp[j].my_name);
                    strcat(message, ": ");
                    strcpy(buff2, message);
                    strcat(buff2, buff);
                    write(mp[j].friend_fd, buff2, strlen(buff2));
                    printf("write %s\n", buff2);
                }
            }
        }
        if(rate && FD_ISSET(0,&rfds)){
            char str[4096];
            if(scanf("%[^\r\n]%*1[\r\n]",str) > 0){
                if(!strcmp(str,"exit")){
                    break;
                }
            }
        }

    }
    close(con);
}
