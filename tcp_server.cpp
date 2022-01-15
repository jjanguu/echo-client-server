#include <iostream>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <list>
#define BUFSIZE 65535

std::list<int> broadcast;

struct Param {
	bool echo{false};
    bool broadcast{false};
	uint16_t port{0};

	bool parse(int argc, char* argv[]) {
		for (int i = 1; i < argc; i++) {
			if (strcmp(argv[i], "-e") == 0) {
				echo = true;
				continue;
			}
            else if (strcmp(argv[i], "-b") == 0) {
				broadcast = true;
				continue;
			}
			port = atoi(argv[i]);
		}
		return port != 0;
	}
} param;


void *threadServer(void * client_fd){
    char buf[BUFSIZE];
    int Client_fd =  *(int *)client_fd;
    while (true) {
		int readBuf = recv(Client_fd, buf, BUFSIZE - 1, 0);
        if (readBuf == 0 || readBuf == -1) {
			perror("recv");
			break;
		}
		buf[readBuf] = '\0';
		printf("%s", buf);
		fflush(stdout);

		if (param.echo) {
			readBuf = ::send(Client_fd, buf, readBuf, 0);
			if (readBuf == 0 || readBuf == -1) {
				perror("send");
				break;
			}
		}

        if (param.broadcast) {
            for(auto fd : broadcast){
                if (fd != Client_fd)
                send(fd, buf, readBuf, 0);
            }
		}

	}
    printf("disconnected\n");
    close(Client_fd);
}

void usage() {
	printf("echo-server <port> [-e[-b]]\n");
	printf("sample: ts -e -b 1234\n");
}


int main(int argc, char * argv[]){
    	if (!param.parse(argc, argv)) {
		usage();
		return -1;
	}

    int sockFD = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

    if (sockFD == -1) {
        perror("socket goes wrong");
        return -1;
    }

    struct sockaddr_in addr;
    socklen_t addlen = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(param.port);

    if(bind(sockFD, (sockaddr *)&addr, addlen) == -1){
        perror("socket bind error");
        return -1;
    }

    if(listen(sockFD, 1) == -1){
        perror("socket listen error");
        return -1;
    }

    while(true){
        struct sockaddr_in client_addr;
        socklen_t client_addlen = sizeof(client_addr);

        int client_fd = accept(sockFD, (struct sockaddr *)&client_addr, &client_addlen);

        if (client_fd == -1) {
            perror("socket accept error");
            return -1;
        }

        if(param.broadcast == true){
        broadcast.push_back(client_fd);
        }

        pthread_t t;

        pthread_create(&t,NULL,threadServer,(void *)&client_fd);
        pthread_detach(t);
    }

    close(sockFD);
}
