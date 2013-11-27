/*
 * server_PFS.c
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
#include <signal.h>
#include <pthread.h>

#include "protocol.h"
#include "list.h"


#define USAGE "<port number>"
#define ARG_MIN 1
#define BACKLOG 10


list_p thread_pool;


static void handler(int signum) {
	printf("Hello, signal %d!\n", signum);
	printf("list size = %d\n", thread_pool->length);

	// Join all threads
	while (thread_pool->length > 0) {
		pthread_t *thread_ptr = list_pop(thread_pool);
		pthread_join(*thread_ptr, NULL);
	}

	printf("list size = %d\n", thread_pool->length);

	// Destroy thread pool
	destroy_list(thread_pool);

	exit(EXIT_SUCCESS);
}

// TODO Write thread function
void *thread_process(void *params) {
	int sock = *((int *) params);
	char buf[4];
	char buf2[8];

	printf("Hello thread\n");

	recv(sock, &buf, sizeof(buf), 0);
	printf("buf1 = %s\n", buf);
	recv(sock, &buf2, sizeof(buf2), 0);
	printf("buf2 = %s\n", buf2);

	return 0;
}


int main(int argc, char *argv[]) {

	int port;
	struct sockaddr_in local_addr;
	struct sockaddr_in remote_addr;
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

	// Initialize thread pool
	thread_pool = create_list();

	// Create local address structure
	memset(&local_addr, '\0', sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(port);
	local_addr.sin_addr.s_addr = INADDR_ANY;

	// Create socket
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return EXIT_FAILURE;
	}

	// Bind socket to port
	if (bind(sock, (struct sockaddr *) &local_addr, sizeof(local_addr)) < 0) {
		perror("bind");
		return EXIT_FAILURE;
	}

	// Listen on socket
	if (listen(sock, BACKLOG) < 0) {
		perror("listen");
		return EXIT_FAILURE;
	}


	for (;;) {

		// Accept connection on listening socket
		socklen_t len = sizeof(remote_addr);
		int new_sock = accept(sock, (struct sockaddr *) &remote_addr, &len);

		if (new_sock < 0) {
			perror("accept");
			return EXIT_FAILURE;
		}

		printf("Connection accepted from %s %d\n",
				inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));

		// Handle connection in new thread
		pthread_t thread;
		pthread_create(&thread, NULL, thread_process, &new_sock);
		list_add(thread_pool, &thread, sizeof(thread));
	}

	return EXIT_SUCCESS;
}
