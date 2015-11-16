#ifndef ADMISSION_H
#define ADMISSION_H


#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>

using namespace std;

#define MAXDATASIZE 100
struct studentInfo {
float gpa;
string name;
string interest1;
string interest2;
string result;
};
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


 

#endif
