#include "TCPRequestChannel.h"
#include "common.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <thread>

using namespace std;

TCPRequestChannel::TCPRequestChannel(const string host_name, const string port_no) {
	if(host_name == "") {
		int new_fd;
		struct addrinfo hints, *serv;
		struct sockaddr_storage client_addr;
		socklen_t sin_size;
		char s[INET6_ADDRSTRLEN];
		int rv;
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;
		if((rv = getaddrinfo(NULL, port_no.c_str(), &hints, &serv)) != 0) {
			cerr << "getaddrinfo: " << gai_strerror(rv) << endl;
			exit(1);
		}
		if((sockfd = socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol)) == -1) {
			perror("server: socket");
			exit(1);
		}
		if(bind(sockfd, serv->ai_addr, serv->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			exit(1);
		}
		freeaddrinfo(serv);
		if(listen(sockfd, 20) == -1) {
			perror("listen");
			exit(1);
		}
		cout << "server: waiting for connections..." << endl;
	}
	else {
		struct addrinfo hints, *res;
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		int status;
		if ((status = getaddrinfo(host_name.c_str(), port_no.c_str(), &hints, &res)) != 0) {
			cerr << "getaddrinfo: " << gai_strerror(status) << endl;
			exit(1);
		}
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if(sockfd < 0) {
			perror("Cannot create socket");
			exit(1);
		}
		if(connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
			perror("Cannot Connect");
			exit(1);
		}
		cout << "Connected" << endl;
		freeaddrinfo(res);
	}
}

TCPRequestChannel::TCPRequestChannel(int fd) {
	sockfd = fd;
}

TCPRequestChannel::~TCPRequestChannel() {
	close(sockfd);
}

int TCPRequestChannel::cread(void* msgbuf, int bufcapacity) {
	return recv(sockfd, msgbuf, bufcapacity, 0);
}

int TCPRequestChannel::cwrite(void* msgbuf, int len) {
	return send(sockfd, msgbuf, len, 0);
}

int TCPRequestChannel::getfd() {
	return sockfd;
}