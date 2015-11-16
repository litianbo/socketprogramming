#include "Department.h"
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sstream>
using namespace std;
// get sockaddr, IPv4 or IPv6:


int main(int argc, char *argv[])
{
	//structure to store address info.
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	void *addr;
	char const *ipver;
	char * pch;
	ifstream file;
	pid_t pid;
	//need two process for two department.
	pid = fork();
	//program info
	vector<char*> s1, p1;
	//get address info
	if ((rv = getaddrinfo("localhost", PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	//read input files:

	if (pid == 0)
		file.open("departmentA.txt");
	else
		file.open("departmentB.txt");
	if (!file.is_open()){
		printf("couldn't open file!\n");
		return 1;
	}
	else{
		string line;
		while (getline(file, line)){
			const char * c = line.c_str();
			char* copy = strdup(c);
			s1.push_back(copy);
			char* copy1 = strdup(c);
			pch = strtok(copy1, "#");
			p1.push_back(pch);
			while (pch != NULL){
				pch = strtok(NULL, "#");
			}
		}
	}
	file.close();



	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		//create socket and connect to server
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("Department: socket");
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("Department: connect");
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
		if (getsockname(sockfd, (struct sockaddr *)&sin, &len) == -1)
			perror("getsockname");
		else{
			inet_ntop(p->ai_family, addr, s, sizeof s);
			if (pid == 0)
				printf("<DepartmentA> has TCP port %d, and IP address %s for phase 1\n",
				ntohs(sin.sin_port), s);
			else
				printf("<DepartmentB> has TCP port %d, and IP address %s for phase 1\n",
				ntohs(sin.sin_port), s);
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "Department: failed to connect\n");
		return 2;
	}
	//if p is not null, then clients connected successfully.
	if (pid == 0)
		printf("<DepartmentA> is now connected to the admission office\n");
	else
		printf("<DepartmentB> is now connected to the admission office\n");

	freeaddrinfo(servinfo); // all done with this structure
	//send 3 packetts
	for (int i = 0; i < 3; i++){
		if (send(sockfd, s1.at(i), 6, 0) == -1)
			perror("send");
		if (pid == 0)
			printf("<DepartmentA> has sent <%s> to the admission office\n", s1.at(i));
		else
			printf("<DepartmentB> has sent <%s> to the admission office\n", s1.at(i));
	}
	if (pid == 0)
		printf("Updating the admission office is done for <DepartmentA>\n");
	else
		printf("Updating the admission office is done for <DepartmentB>\n");
	/*
		if (pid == 0)
		printf("DepartmentA: received '%s'\n",buf);
		else
		printf("DepartmentB: received '%s'\n",buf);*/
	if (pid == 0)
		printf("End of Phase 1 for <DepartmentA>\n");
	else
		printf("End of Phase 1 for <DepartmentB>\n");
	close(sockfd);

	//phase II
	//UDP
	hints.ai_family = AF_INET;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	int udp;
	int port;
	if (pid == 0)
		port = 21200 + 860;
	else
		port = 21300 + 860;
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
			if (pid == 0)
				printf("<DepartmentA> has UDP port %d, and IP address %s \n", ntohs(udpin.sin_port),
				s);
			else
				printf("<DepartmentB> has UDP port %d, and IP address %s \n", ntohs(udpin.sin_port),
				s);
		}
		break;
	}
	int counterint = 0;
	while (1){
		int byte_count;
		void* addr2;
		//receive the students size first
		socklen_t fromlen = sizeof addr2;
		if ((byte_count = recvfrom(udp, buf, sizeof buf, 0,
			(struct sockaddr *)&addr2, &fromlen)) == -1){
			perror("recvfrom");
			exit(1);
		}
               // printf("%c\n",buf[0]);
		stringstream ss1;
		ss1 << buf[0];
		string num;
		ss1 >> num;
		//receive real content now
		int intnum = buf[0] - '0';
		for (int i = 0; i < intnum; i++){
			if ((byte_count = recvfrom(udp, buf, sizeof buf, 0,
				(struct sockaddr *)&addr2, &fromlen)) == -1){
				perror("recvfrom");
				exit(1);
			}
			counterint += 1;
                      //  printf("%s\n",buf);
			if (pid == 0)
			{

				string name = buf;
				printf("<%s> has been admitted to <DepartmentA>\n", name.substr(0, 8).c_str());
			}
			else
			{

				string name = buf;
				printf("<%s> has been admitted to <DepartmentB>\n", name.substr(0, 8).c_str());
			}
		}
		stringstream ss;
		ss << counterint;
		string counter;
		ss >> counter;

		if (counter == num)
			break;
	}
	if (pid == 0)
		printf("End of Phase 2 for <DepartmentA>\n");
	else
		printf("End of Phase 2 for <DepartmentB>\n");
	return 0;
}
