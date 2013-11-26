/*
 * server_PFS.c
 *
 *  Created on: Nov 23, 2013
 *      Author: Ben Cavins
 */


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>


#define USAGE "<port number>"
#define ARG_MIN 1


static void handler(int signum) {
	printf("Hello, signal %d!\n", signum);
	exit(EXIT_SUCCESS);
}


int main(int argc, char *argv[]) {

	int port;
	struct sockaddr_in local_addr;
	int sock;
	struct sigaction sigact;

	// Check args
	if (argc < ARG_MIN + 1) {
		fprintf(stderr, "Usage: %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
	}

	port = atoi(argv[1]);

	// Set up signal handler
	sigact.sa_handler = handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = SA_RESTART;  // Restart functions if interrupted by handler

	if (sigaction(SIGINT, &sigact, NULL) < 0) {
		perror("sigaction");
		return EXIT_FAILURE;
	}

	// Create local address structure
	memset(&local_addr, '\0', sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(port);
	local_addr.sin_addr.s_addr = INADDR_ANY;

	// TODO create socket

	// TODO Bind socket to port

	// TODO Listen on socket

	// TODO Accept connection on listening socket

	for (;;) {}

	printf("port = %d\n", port);

	return EXIT_SUCCESS;
}
