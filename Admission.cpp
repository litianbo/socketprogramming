#include "Admission.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <string>
#include <map>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <iostream>
#include <vector>

void *get_in_addr(struct sockaddr *sa);

//counts how many departments have sent packets, 
//shared memory between child and parent process
int *counter;
//counter to count how many students have sent applications
int *counter2;
int main(int argc, char *argv[])
{
	//data structure to store program requirement
	map<string, float>prog;
	//allocate memory
	counter = (int*)mmap(NULL, sizeof *counter, PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	counter2 = (int*)mmap(NULL, sizeof *counter, PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	//intialize counter to zero
	*counter = 0;
	*counter2 = 0;

	//*results = "";
	//vector<studentInfo> *students = new vector<studentInfo>();
	//structure to store address info.
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	//struct sockaddr_in dest;
	struct addrinfo hints, *res, *p, *res2;
	int sockfd, new_fd, numbytes, numbytes1, numbytes2;
	char s[INET6_ADDRSTRLEN];
	struct sockaddr_in sin, udpin;
	socklen_t len = sizeof(sin);
	socklen_t udplen = sizeof(udpin);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	//get address info from port 4160 (3300 + 860 which is my SID)
	getaddrinfo("localhost", "4160", &hints, &res);
	char buf[6], buf1[6], buf2[6];
	char bufmax[MAXDATASIZE], bufmax2[MAXDATASIZE];
	for (p = res; p != NULL; p = p->ai_next) {
		void *addr;
		char const *ipver;

		//create a socket
		if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
			perror("socket");
			exit(1);

		}
		//bind it to a port
		if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1){
			perror("bind");
			exit(1);
		}
		//get ip address and port from server
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
			printf("The admission office has TCP port %d, and IP address %s \n", ntohs(sin.sin_port),
				s);
		}
		break;
	}
	//listen to port 4160
	if (listen(sockfd, 5) == -1){
		perror("listen");
		exit(1);
	}

	//printf("Admission: waiting for connections...\n");   
	//while loop to accept connections from two departments
	while (1) {
		addr_size = sizeof their_addr;
		//waiting to accept connection.
		//if a socket wants to connect, create a new socket for it.
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}
		//for debugging purpose, get the ip and port from the client!
		/*
			  char hoststr[NI_MAXHOST];
			  char portstr[NI_MAXSERV];

			  inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),s,sizeof s);
			  int rc = getnameinfo((struct sockaddr *)&their_addr, addr_size, hoststr, sizeof(hoststr), portstr, sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV);
			  if (rc == 0) printf("New connection from %s port number: %s\n", hoststr, portstr);
			  */
		//create a child process to operate child socket.
		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			//thread sleep 10 usec, to let the packets to arrive.
			usleep(10);
			//receive packets from clients
			if ((numbytes = recv(new_fd, buf, MAXDATASIZE, 0)) == -1) {
				perror("recv");
				exit(1);
			}
			//after all packets arrived.
			if (numbytes == 18){
				printf("Received the program list from <Department%c>\n", buf[0]);
				//store packets

				if (buf[0] == 'A'){
					//write result to a file to store it
					//tried to use shared memory to store it, but stucked on it. 
					ofstream outfile("outputA");
					outfile.write(buf, 18);
					outfile.close();
					//increase the global counter
					*counter += 1;
				}
				else{
					//same as above for departmentB
					ofstream outfile("outputB");
					outfile.write(buf, 18);
					outfile.close();
					*counter += 1;
				}
			}
			buf[numbytes] = '\0';
			// printf("Admission Office: received '%s'\n",buf);
			close(new_fd);
			exit(0);
			//all done with communication
		}
		close(new_fd);  // parent doesn't need this
		//if we successfully stored requirements into 2 files, break the loop
		wait(NULL);
		if (*counter == 2){
			printf("%s\n", "Admission Office: End of Phase 1 for the admission office");
			break;
		}

	}
	//store program requirement
	ifstream fileA("outputA");
	if (!fileA.is_open()){
		printf("could not open output A");
	}
	else{
		string line;
		getline(fileA, line);
		string program1 = line.substr(0, 6);
		string program2 = line.substr(6, 6);
		string program3 = line.substr(12, 6);
		prog[program1.substr(0, 2)] = atof(program1.substr(3, 3).c_str());
		prog[program2.substr(0, 2)] = atof(program2.substr(3, 3).c_str());
		prog[program3.substr(0, 2)] = atof(program3.substr(3, 3).c_str());


	}
	fileA.close();
	ifstream fileB("outputB");
	if (!fileB.is_open()){
		printf("could not open output A");
	}
	else{
		string line;
		getline(fileB, line);
		string program1 = line.substr(0, 6);
		string program2 = line.substr(6, 6);
		string program3 = line.substr(12, 6);
		prog[program1.substr(0, 2)] = atof(program1.substr(3, 3).c_str());
		prog[program2.substr(0, 2)] = atof(program2.substr(3, 3).c_str());
		prog[program3.substr(0, 2)] = atof(program3.substr(3, 3).c_str());
	}
	fileB.close();
	close(sockfd);
	*counter = 0;
	//start phase II:
	//vector<studentInfo> students;
	//int sockfd1, new_fd1;
	for (p = res; p != NULL; p = p->ai_next) {
		void *addr;
		char const *ipver;
		//create a socket
		if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
			perror("socket");
			exit(1);

		}
		//bind it to a port
		if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1){
			perror("bind");
			exit(1);
		}

		//get ip address and port from server
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
			printf("The admission office has TCP port %d, and IP address %s \n", ntohs(sin.sin_port),
				s);
		}
		break;
	}
	//listen to port 4160
	if (listen(sockfd, 5) == -1){
		perror("listen");
		exit(1);
	}

	while (1) {
		addr_size = sizeof their_addr;
		//waiting to accept connection.
		//if a socket wants to connect, create a new socket for it.
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

		if (new_fd == -1) {
			perror("accept");
			continue;
		}
		else{
			*counter2 += 1;

			/*
					   char hoststr[NI_MAXHOST];
					   char portstr[NI_MAXSERV];

					   inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),s,sizeof s);
					   int rc = getnameinfo((struct sockaddr *)&their_addr, addr_size, hoststr, sizeof(hoststr), portstr, sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV);
					   if (rc == 0) printf("New connection from %s port number: %s\n", hoststr, portstr); */

			if (!fork()) { // this is the child process
				//printf("test2\n");
				close(sockfd); // child doesn't need the listener
				//receive size first
				if ((numbytes = recv(new_fd, bufmax, MAXDATASIZE, 0)) == -1) {
					perror("recv");
					exit(1);
				}
				//printf("%s\n",bufmax);
				//ACK student
				if (send(new_fd, "ok", 2, 0) == -1)
					perror("send");
				stringstream strValue1;
				int value;
				strValue1 << bufmax;
				strValue1 >> value;
				//printf("%d\n",value);
				//receive content using MSG_WAITALL and size
				if ((numbytes = recv(new_fd, bufmax, value + 8, MSG_WAITALL)) == -1) {
					perror("recv");
					exit(1);
				}

				bufmax[numbytes] = '\0';
				//printf("received: %s\n",bufmax);
				char outname[8];
				struct studentInfo student;
				//printf("%d\n",numbytes);
				char* text;
				string line;
				if (numbytes >= 39){
					text = bufmax;
					string s6(text);
					line = s6;
				}
				//store message into struct studentInfo
				student.name = line.substr(0, 8);
				student.gpa = atof(line.substr(12, 3).c_str());
				//printf("%f\n",student.gpa);
				int index = line.find("#", 25);
				//printf("%d\n",index);
				int interest1end = index - 10;
				student.interest1 = line.substr(25, interest1end - 25 + 1);
				//printf("%s\n",line.substr(25,27).c_str());
				student.interest2 = line.substr(index + 1);
				//printf("%f %f\n",prog["A1"],prog["A2"]);
				printf("Admission office receive the application from %s\n", student.name.c_str());
				//if none of the student's interests match our record, send him a message.
				if (!prog.count(student.interest1)){
					if (!prog.count(student.interest2)){
						if (send(new_fd, "0", 1, 0) == -1)
							perror("send");
						//create a file for him so that later it won't have error
						ofstream outfile(student.name.c_str());

						outfile.close();
						close(new_fd);
						exit(0);
						return 0;
					}
				}

				if (send(new_fd, "1", 1, 0) == -1)
					perror("send");
				//UDP
				int portindex = bufmax[7] - '0';
				int port = portindex * 100 + 21300 + 860;
				//printf("%d\n", portindex);
				hints.ai_family = AF_INET;
				hints.ai_socktype = SOCK_DGRAM;
				stringstream strs;
				strs << port;
				string tempstr = strs.str();
				const char* po = tempstr.c_str();
				getaddrinfo("localhost", po, &hints, &res);
				//printf("%s \n",po);
				int udpfd;
				//getaddrinfo(NULL, port, &hints, &res);
				struct sockaddr_in* dest;
				void *addr;
				char const *ipver;
				for (p = res; p != NULL; p = p->ai_next){

					if ((udpfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
						perror("Admission: UDP socket");
						exit(1);
					}
					if (p->ai_family == AF_INET) { // IPv4
						struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
						addr = &(ipv4->sin_addr);
						dest = ipv4;
						ipver = "IPv4";
					}
					else { // IPv6
						struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
						//dest = (struct sockaddr_in *)p->ai_addr;
						addr = &(ipv6->sin6_addr);
						//dest = ipv6;
						ipver = "IPv6";
					}
					void * poi = &(dest->sin_addr);
					inet_ntop(p->ai_family, poi, s, sizeof s);
					//printf();
					// printf("%s %d\n", s,dest->sin_port);
					struct sockaddr_in dest1 = *dest;
					string result;
					//calculate admission results
					if (prog.count(student.interest1)){
						if (prog[student.interest1] <= student.gpa){
							result = student.interest1;
						}
						else {
							if (prog.count(student.interest2)){
								if (prog[student.interest2] <= student.gpa){
									result = student.interest2;
								}
								else{
									result = "reject";
								}
							}
							else{
								result = "reject";
							}
						}
					}
					else{
						if (prog.count(student.interest2)){
							if (prog[student.interest2] <= student.gpa)
								result = student.interest2;
							else{
								result = "reject";
							}
						}
						else{
							result = "reject";
						}
					}
					student.result = result;
					ofstream outfile2(student.name.c_str());
					string studentmes;
					//write result to a file
					if (result != "reject"){
						// *couter3+=1;
						ostringstream strValue1;
						string message;
						strValue1 << student.gpa;
						string gpa = strValue1.str();
						if (strValue1.str().size() == 1){
							gpa += ".0";
						}
						message = string(student.name) + string("#") +
							gpa + string("#") + string(student.result);
						studentmes += string("Accept") + string("#") +
							student.result;
						outfile2.write(message.c_str(), message.size());
						// printf("%s\n", message.c_str());  
					}

					outfile2.close();
					if (studentmes.size() == 0)
						studentmes = "Reject";
					else{
						char temps = student.result.at(0);
						if (temps == 'A')
							studentmes += string("#") + "departmentA";
						else
							studentmes += string("#") + "departmentB";
					}
					//printf("%s\n", studentmes.c_str());
					//send result to student
					sendto(udpfd, studentmes.c_str(), studentmes.size(),
						0, (struct sockaddr*)&dest1, sizeof dest1);
					//need to print this message after calling sendto, otherwise port would be 0
					if (getsockname(udpfd, (struct sockaddr *)&udpin, &udplen) == -1)
						perror("getsockname");
					else{
						inet_ntop(p->ai_family, addr, s, sizeof s);
						printf("The Admission Office has UDP port %d, and IP address %s for phase 2\n",
							ntohs(udpin.sin_port), s);
					}

					break;
				}

				printf("The Admission office has send the application result to <%s>\n"
					, student.name.c_str());
				close(udpfd);
				close(new_fd);
				exit(0);
			}//fork

			//all done with communication
			close(new_fd);  // parent doesn't need this
		}//else
		// printf("%d\n", students->size());
		//something to terminate loop

		if (*counter2 == 5){
			//printf("%s\n", "Admission Office: sent all 5 back to students");
			break;
		}



		//break;
	}//end of while
	//below is department part
	int portA = 22060;
	int portB = 22160;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	stringstream strs, strs2;
	strs << portA;
	strs2 << portB;
	string tempstr = strs.str();
	string tempstr2 = strs2.str();
	const char* po = tempstr.c_str();
	const char* po2 = tempstr2.c_str();
	getaddrinfo("localhost", po, &hints, &res);
	getaddrinfo("localhost", po2, &hints, &res2);
	//printf("%s \n",po);
	int udpfd, udpfd2;
	//getaddrinfo(NULL, port, &hints, &res);
	struct sockaddr_in* dest;
	struct sockaddr_in* dest2;
	void *addr;
	char const *ipver;
	for (p = res; p != NULL; p = p->ai_next){

		if ((udpfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
			perror("Admission: UDP socket");
			exit(1);
		}
		if (p->ai_family == AF_INET) { // IPv4
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv4->sin_addr);
			dest = ipv4;
			ipver = "IPv4";
		}
		else { // IPv6
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
			//dest = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv6->sin6_addr);
			//dest = ipv6;
			ipver = "IPv6";
		}
		void * poi = &(dest->sin_addr);
		inet_ntop(p->ai_family, poi, s, sizeof s);
		//printf();
		//printf("%s %d\n", s,dest->sin_port);

		break;
	}
	for (p = res2; p != NULL; p = p->ai_next){

		if ((udpfd2 = socket(res2->ai_family, res2->ai_socktype, res2->ai_protocol)) == -1){
			perror("Admission: UDP socket");
			exit(1);
		}
		if (p->ai_family == AF_INET) { // IPv4
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv4->sin_addr);
			dest2 = ipv4;
			ipver = "IPv4";
		}
		else { // IPv6
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
			//dest = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv6->sin6_addr);
			//dest = ipv6;
			ipver = "IPv6";
		}
		void * poi = &(dest->sin_addr);
		inet_ntop(p->ai_family, poi, s, sizeof s);
		//printf();
		//printf("%s %d\n", s,dest->sin_port);

		break;
	}
	struct sockaddr_in dest1 = *dest;
	struct sockaddr_in dest3 = *dest2;
	//printf("%d\n",students->size());
	int admittedA = 0;
	int admittedB = 0;
	for (int i = 1; i < 6; i++){
		stringstream ss;
		ss << i;
		string postfix;
		ss >> postfix;
		string filename = "Student" + postfix;
		ifstream infile2;
		infile2.open(filename.c_str());
		if (!infile2.is_open()){
			printf("couldn't open %s\n", filename.c_str());
		}
		string line4;
		getline(infile2, line4);
                infile2.close();
infile2.open(filename.c_str());
		if (!infile2.is_open()){
			printf("couldn't open %s\n", filename.c_str());
		}
		
		getline(infile2, line4);
printf("header:%s %s\n",filename.c_str(),line4.c_str());
		if (line4.size() > 0){
			if (line4.substr(13, 1) == "A"){
				admittedA += 1;
                            
			}
			else
				admittedB += 1;
		}
                infile2.close();
	}
	stringstream ss;
	ss << admittedA;
	string stringadA;
	ss >> stringadA;
	stringstream ss2;
	ss2 << admittedB;
	string stringadB;
	ss2 >> stringadB;
	//here, we send size of data first, 
	//let the department to calculate how many recvfrom() are needed
	sendto(udpfd, stringadA.c_str(), 1,
		0, (struct sockaddr*)&dest1, sizeof dest1);
	sendto(udpfd, stringadB.c_str(), 1,
		0, (struct sockaddr*)&dest3, sizeof dest3);
	for (int i = 1; i < 6; i++){
		stringstream ss;
		ss << i;
		string postfix;
		ss >> postfix;
		string filename = "Student" + postfix;
		ifstream infile;
		infile.open(filename.c_str());

		if (!infile.is_open()){
			printf("couldn't open %s\n", filename.c_str());
		}
		string line3;
		getline(infile, line3);
//printf("data: %s\n",line3.c_str());
		if (line3.size() > 0){

			if (line3.substr(13, 1) == "A"){
				sendto(udpfd, line3.c_str(), line3.size() + 1,
					0, (struct sockaddr*)&dest1, sizeof dest1);
				printf("The admission office has send one admitted student to <DepartmentA>\n");
			}
			else{
				sendto(udpfd2, line3.c_str(), line3.size() + 1,
					0, (struct sockaddr*)&dest3, sizeof dest3);
				printf("The admission office has send one admitted student to <DepartmentB>\n");
			}

		}
                 infile.close();

	}
	close(udpfd);
	close(udpfd2);
	printf("End of Phase 2 for the admission office\n");
	close(sockfd);


	return 0;
}
