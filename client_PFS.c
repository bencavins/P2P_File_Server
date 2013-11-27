/*
 * client_PFS.c
 *
 *  Created on: Nov 23, 2013
 *      Author: Ben Cavins
 */


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "protocol.h"

#define USAGE "<client name> <server IP> <server port>"
#define ARG_MIN 3

int bind_random_port(int sock) {
	struct sockaddr_in sin;
	struct sockaddr_in new_sin;
	socklen_t addrlen;
	memset(&sin, '\0', sizeof(sin));
	memset(&new_sin, '\0', sizeof(new_sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");
	sin.sin_port = htons(0);
	if (bind(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		return -1;
	}
	addrlen = sizeof(new_sin);
	getsockname(sock, (struct sockaddr *) &new_sin, &addrlen);
	return ntohs(new_sin.sin_port);
}

int main(int argc, char *argv[]) {

	char *client_name;
	char *server_ip;
	int server_port;
	int server_sock;
	int listen_sock;
	int listen_port;
	struct sockaddr_in server_addr;

	if (argc < ARG_MIN + 1) {
		fprintf(stderr, "Usage: %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
	}

	client_name = argv[1];
	server_ip = argv[2];
	server_port = atoi(argv[3]);

	printf("client name = %s\n", client_name);
	printf("server ip = %s\n", server_ip);
	printf("server port = %d\n", server_port);

	// Create server address structure
	memset(&server_addr, '\0', sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port);
	server_addr.sin_addr.s_addr = inet_addr(server_ip);

	// Create server socket
	if ((server_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return EXIT_FAILURE;
	}

	// Connect to server
	printf("Connecting to %s %d...\n", server_ip, server_port);

	if (connect(server_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		perror("connect");
		return EXIT_FAILURE;
	}

	printf("Connection succeeded\n");

	// Create and bind another socket for listening
	if ((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return EXIT_FAILURE;
	}

	if ((listen_port = bind_random_port(listen_sock)) < 0) {
		perror("bind_random_port");
		return EXIT_FAILURE;
	}

	printf("listen port = %d\n", listen_port);

	// TODO Register client

	return EXIT_SUCCESS;
}
