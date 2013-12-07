/*
 * client_PFS.c
 *
 *  Created on: Nov 23, 2013
 *      Author: Ben Cavins
 */


#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

#include "protocol.h"
#include "list.h"

#define USAGE "<client name> <server IP> <server port>"
#define ARG_MIN 3
#define MAX_INPUT_LEN 1024
#define BACKLOG 10

int end = 0;
sem_t mutex;

void *thread_process(void *params) {
	int sock = *((int *) params);
	packet_header_p pkt_hdr;
	fd_set fdset;
	struct timeval tv;
	int retval;

	for (;;) {

		pkt_hdr = create_packet_header();

		FD_ZERO(&fdset);
		FD_SET(sock, &fdset);

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		retval = select(sock+1, &fdset, NULL, NULL, &tv);

		if (retval < 0) {
			perror("select");
		} else if (retval == 0) {
			printf("timeout!\n");
		} else {
			if (FD_ISSET(sock, &fdset)) {
				printf("yay data!\n");
				recv_header(sock, 0, pkt_hdr);
				printf("command = %d\n", pkt_hdr->command);
				if (pkt_hdr->command == CMD_LS) {
					send_error(sock, 0, E_SUCCESS);
				}
			}
		}

		destroy_packet_header(pkt_hdr);

		sem_wait(&mutex);
		if (end) {
			sem_post(&mutex);
			break;
		}
		sem_post(&mutex);
	}

	return NULL;
}

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

int register_client(int sock, char *name) {
	int result;
	packet_header_p pkt_hdr = create_packet_header();
	pkt_hdr->command = CMD_REGISTER_CLIENT;
	pkt_hdr->length = strlen(name) + 1;
	if (send_packet(sock, name, pkt_hdr->length, pkt_hdr) < 0) {
		return -1;
	}
	recv_header(sock, 0, pkt_hdr);
	result = pkt_hdr->error;
	printf("error = %d\n", pkt_hdr->error);
	destroy_packet_header(pkt_hdr);
	return result;
}

void unregister_client(int sock, char *name) {
	packet_header_p pkt_hdr = create_packet_header();
	pkt_hdr->command = CMD_UNREGISTER_CLIENT;
	pkt_hdr->length = strlen(name) + 1; 
	if (send_packet(sock, name, 0, pkt_hdr) < 0) {
		perror("send_packet");
		return;
	}
	recv_header(sock, 0, pkt_hdr);
	printf("error = %d\n", pkt_hdr->error);
	destroy_packet_header(pkt_hdr);    
}

void *listen_process(void *params) {
	int sock = *((int *) params);
	struct sockaddr_in remote_addr;
	socklen_t len = sizeof(remote_addr);
	int new_sock;
	fd_set fdset;
	struct timeval tv;
	int retval;

	// TODO code select function to check for accept
	for (;;) {
		FD_ZERO(&fdset);
		FD_SET(sock, &fdset);

		tv.tv_sec = 0;
		tv.tv_usec = 500;

		retval = select(sock+1, &fdset, NULL, NULL, &tv);

		if (retval < 0) {
			perror("select");
		} else if (retval > 0) {
			if (FD_ISSET(sock, &fdset)) {
				new_sock = accept(sock, (struct sockaddr *) &remote_addr, &len);
				printf("accepted\n");
				printf("port = %d\n", ntohs(remote_addr.sin_port));
				// TODO spawn new thread to handle possible get request
			}
		}

		sem_wait(&mutex);
		if (end) {
			sem_post(&mutex);
			break;
		}
		sem_post(&mutex);
	}

	return NULL;
}

int main(int argc, char *argv[]) {

	char *client_name;
	char *server_ip;
	int server_port;
	int server_sock;
	int listen_sock;
	int listen_port;
	struct sockaddr_in server_addr;
	char input[MAX_INPUT_LEN];
	int result;
	list_p thread_pool;

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

	// Initialize data structures
	thread_pool = create_list();
	if (sem_init(&mutex, 0, 1) < 0) {
		perror("sem_init");
		return EXIT_FAILURE;
	}

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

	// Listen on newly bound port
	if (listen(listen_sock, BACKLOG) < 0) {
		perror("listen");
		return EXIT_FAILURE;
	}

	// TODO spawn thread that waits for get requests
	pthread_t listen_thread;
	pthread_create(&listen_thread, NULL, listen_process, &listen_sock);
	list_add(thread_pool, &listen_thread, sizeof(listen_thread));

	result = register_client(server_sock, client_name);
	printf("result = %d\n", result);
	if (result == E_DUPLICATE_NAME) {
		fprintf(stderr, "register_client: Client name %s already exists\n", client_name);
		return EXIT_FAILURE;
	} else if (result < 0) {
		perror("register_client");
		return EXIT_FAILURE;
	}

	// TODO spawn thread that waits for server requests
	pthread_t thread;
	pthread_create(&thread, NULL, thread_process, &server_sock);
	//pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	//pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	list_add(thread_pool, &thread, sizeof(thread));

	while (1) {
		printf(">> ");
		fgets(input, sizeof(input), stdin);
		if (strcmp("exit\n", input) == 0) {
			printf("exiting...\n");
			unregister_client(server_sock, client_name);
			sem_wait(&mutex);
			end = 1;
			sem_post(&mutex);
			break;
		} else {
			printf("unknown command\n");
		}
	}

	// Join threads
	while (thread_pool->length > 0) {
		pthread_t *thread_ptr = list_pop(thread_pool);
		//pthread_cancel(*thread_ptr);
		//pthread_kill(*thread_ptr, SIGINT);
		pthread_join(*thread_ptr, NULL);
	}

	// Destroy data structures
	destroy_list(thread_pool);
	if (sem_destroy(&mutex) < 0) {
		perror("sem_destroy");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
