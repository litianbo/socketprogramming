#include "Student.h"
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
void sendDataInChild(int i);
using namespace std;
// get sockaddr, IPv4 or IPv6:

void sendDataInChild(int i){
	ifstream file,file2;
	string c = "student";
	char numstr[21]; // enough to hold all numbers up to 64-bits
	sprintf(numstr, "%d", i);
	string result = c + numstr + ".txt";
	string name = "Student";
	name = name + numstr;
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	char s[INET6_ADDRSTRLEN];
	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	void *addr;
	char const *ipver;
	char * pch;
	int rv;
	int tcpport, tcpfd;
	char buf[MAXDATASIZE];
	if ((rv = getaddrinfo("localhost", PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return;
	}
	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		//create socket and connect to server
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {

			perror("student: socket");
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("student connect");
			continue;
		}
		if (p->ai_family == AF_INET) { // IPv4
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv4->sin_addr);
			ipver = "IPv4";
		}
		else { // IPv6
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
			addr = &(ipv6->sin6_addr);
			ipver = "IPv6";
		}

		if (getsockname(sockfd, (struct sockaddr *)&sin, &len) == -1){
			perror("getsockname");
		}
		else{
			inet_ntop(p->ai_family, addr, s, sizeof s);
			printf("<%s> has TCP port %d, and IP address %s \n",
				name.c_str(), ntohs(sin.sin_port), s);
			tcpport = ntohs(sin.sin_port);
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "Student: failed to connect\n");
		return;
	}
        file2.open(result.c_str());
        if (!file2.is_open()){
		printf("Student: couldn't open file!\n");
		return;
	}
	
        string line2;
        int size=0;
	while (getline(file2, line2)){
               
		size+= line2.size();
                //printf("%d\n",line2.size());
	}
	file2.close();
        stringstream strs1;
	strs1 << size;
	string tempstr1 = strs1.str();
	const char* size1 = tempstr1.c_str();
        //printf("%d\n", tempstr1.size() );
        int numbytes;
        if (send(sockfd, size1, strlen(size1) , 0) == -1)
		perror("send");
        if ((numbytes = recv(sockfd, buf, 2, MSG_WAITALL)) == -1) {
					perror("recv");
					exit(1);
				}
        
	file.open(result.c_str());
	if (!file.is_open()){
		printf("Student: couldn't open file!\n");
		return;
	}
	if (send(sockfd, name.c_str(), 8, 0) == -1)
		perror("send");
	string line;
	while (getline(file, line)){
		const char * c = line.c_str();
                //printf("%s\n",line.c_str());
		if (send(sockfd, c, line.size(), 0) == -1)
			perror("send");
	}
	file.close();
	printf("Completed sending application for <%s>\n", name.c_str());
	//usleep(10);
	if (recv(sockfd, buf, 1, MSG_WAITALL) == -1) {
		perror("TCP recv");
		exit(1);
	}
	stringstream strValue1;
        int value;
        strValue1 << buf;
        strValue1 >>value;
      //  printf("%s %d\n",name.c_str(),value);
	printf("<%s> has received the reply from the admission office\n", name.c_str());
	if (value == 0)
	{
		close(sockfd);
		printf("End of phase 2 for <%s>\n", name.c_str());
		return;
	}

	close(sockfd);
	//UDP

	int byte_count;
	socklen_t fromlen;
	struct sockaddr_storage  addr1;
	//char buf[512];
	char ipstr[INET6_ADDRSTRLEN];
	int udp;
	// get host info, make socket, bind it to port
	//memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	int port = 21300 + i * 100 + 860;
	//string s1 = to_string(port);
	//const char* po = s1.c_str();
	stringstream strs;
	strs << port;
	string tempstr = strs.str();
	const char* po = tempstr.c_str();
	getaddrinfo("localhost", po, &hints, &servinfo);
	struct sockaddr_in udpin;
	socklen_t udplen = sizeof(udpin);

	for (p = servinfo; p != NULL; p = p->ai_next) {
		//void *addr;
		//char const *ipver;
		if ((udp = socket(servinfo->ai_family,
			servinfo->ai_socktype, servinfo->ai_protocol)) == -1){
			perror("UDP socket");
			exit(1);
		}
		if ((bind(udp, servinfo->ai_addr, servinfo->ai_addrlen)) == -1){
			perror("UDP BIND");
			exit(1);
		}
		if (p->ai_family == AF_INET) { // IPv4
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv4->sin_addr);
			ipver = "IPv4";
		}
		else { // IPv6
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
			addr = &(ipv6->sin6_addr);
			ipver = "IPv6";
		}
		if (getsockname(udp, (struct sockaddr *)&udpin, &udplen) == -1)
			perror("getsockname");
		else{
			inet_ntop(p->ai_family, addr, s, sizeof s);
			printf("%s has UDP port %d, and IP address %s \n", name.c_str(), ntohs(udpin.sin_port),
				s);
		}
		break;
	}
	// no need to accept(), just recvfrom():
	//usleep(10);
	void* addr2;
	fromlen = sizeof addr2;
	while (1){
		//printf("test1\n");
		if ((byte_count = recvfrom(udp, buf, sizeof buf, 0,
			(struct sockaddr *)&addr2, &fromlen)) == -1){
			perror("recvfrom");
			exit(1);
		}
		string s1(buf);
		//printf("%s recv()'d %d bytes of data in buf %s \n",name.c_str() ,byte_count,s1.c_str());

		if (byte_count > 0){
			printf("<%s> has received the application result\n", name.c_str());
			break;
		}
	}
	//printf("%d\n", byte_count);
	printf("End of phase 2 for <%s>\n", name.c_str());
	close(udp);

	exit(0);
	return;
}

int main(int argc, char *argv[])
{
	//fork 5 students
	pid_t pid[5];

	//send applications:
	for (int i = 1; i < 6; i++){
		if ((pid[i - 1] = fork()) < 0){
			perror("fork");
			abort();
		}
		else if (pid[i - 1] == 0){
			sendDataInChild(i);
			exit(0);
		}
	}

	return 0;
}
