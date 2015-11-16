#ifndef DEPARTMENT_H
#define DEPARTMENT_H


#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "4160"
#define MAXDATASIZE 100 // max number of bytes we can get at once 

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
 

#endif
