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
#define BUF_MIN 24

int end = 0;
sem_t mutex;
list_p thread_pool;

int perform_ls(char **buf) {
	FILE *fp;
	int current_len = 0;
	size_t current_max = BUF_MIN;
	char *line;
	size_t len = 0;
	ssize_t read;
	//char size_str[24];
	//char filename[FILENAME_MAX];
	char perms[12];
	char links[12];
	char bytes[32];
	char month[12];
	char day[4];
	char time[8];
	char name[FILENAME_MAX];

	/* Get file list */
	if ((fp = popen("ls -go", "r")) == NULL) {
		perror("popen");
		return -1;
	}

	/* Allocate memory for buffer */
	if (*buf == NULL) {
		*buf = malloc(BUF_MIN);
		if (*buf == NULL) {
			perror("malloc");
			return -1;
		}
	} else {
		*buf = realloc(*buf, BUF_MIN);
		if (*buf == NULL) {
			perror("realloc");
			return -1;
		}
	}
	memset(*buf, '\0', BUF_MIN);

	/* The first line is irrelevant. Read it and throw it away */
	read = getline(&line, &len, fp);

	/* Add file names and sizes to buffer */
	while ((read = getline(&line, &len, fp)) != -1) {
		//sscanf(line, "%s %s", size_str, filename);
		sscanf(line, "%s %s %s %s %s %s %s", perms, links, bytes, month, day, time, name);
		/* Reallocate buffer if needed */
		if (current_len + strlen(bytes) + strlen(name) + 2*strlen(" ") + 1 > current_max) {
			current_max *= 2;
			*buf = realloc(*buf, current_max);
			printf("new max = %d\n", (int) current_max);
		}
		strcat(*buf, name);
		strcat(*buf, " ");
		strcat(*buf, bytes);
		strcat(*buf, " ");
		current_len += strlen(name) + strlen(bytes) + 2*strlen(" ") + 1;
	}

	free(line);
	return 0;
}

void *thread_process(void *params) {
	int sock = *((int *) params);
	packet_header_p pkt_hdr;
	void *buf = NULL;
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
			//printf("timeout!\n");
		} else {
			if (FD_ISSET(sock, &fdset)) {
				retval = toreceive(sock, 0, &pkt_hdr, &buf, 1, 0);
				if (retval == 0) {
					printf("timeout, maybe\n");
				}
				printf("command = %d\n", pkt_hdr->command);
				if (pkt_hdr->command == CMD_LS) {
					if (perform_ls((char **) &buf) < 0) {
						fprintf(stderr, "ls failed\n");
					} else {
						packet_header_p ph = create_packet_header();
						ph->command = 0;
						ph->error = E_SUCCESS;
						ph->flags = 0;
						ph->length = strlen(buf) + 1;
						send_packet(sock, buf, 0, ph);
						destroy_packet_header(ph);
					}
					//send_error(sock, 0, E_SUCCESS);
				}
				free(buf);
//				printf("yay data!\n");
//				recv_header(sock, 0, pkt_hdr);
//				printf("command = %d\n", pkt_hdr->command);
//				if (pkt_hdr->command == CMD_LS) {
//					send_error(sock, 0, E_SUCCESS);
//				}
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

int register_client(int sock, char *name, int port) {
	int result;
	packet_header_p pkt_hdr;
	char port_str[24];
	char data[strlen(name) + 1 + sizeof(port_str)];

	snprintf(port_str, sizeof(port_str), "%d", port);

	pkt_hdr = create_packet_header();
	pkt_hdr->command = CMD_REGISTER_CLIENT;
	pkt_hdr->error = 0;
	pkt_hdr->flags = 0;
	pkt_hdr->length = sizeof(data);

	memset(data, '\0', sizeof(data));
	strcat(data, name);
	strcat(data, " ");
	strcat(data, port_str);

	if (send_packet(sock, data, pkt_hdr->length, pkt_hdr) < 0) {
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

int spawn_thread(void *(*start_routine) (void *), void *arg) {
	pthread_t thread;
	int retval;
	sem_wait(&mutex);
	retval = pthread_create(&thread, NULL, start_routine, arg);
	list_add(thread_pool, &thread, sizeof(thread));
	sem_post(&mutex);
	return retval;
}

void *handle_get(void *params) {
	int sock = *((int *) params);
	int retval;
	packet_header_p ph = NULL;
	void *buf = NULL;
	FILE *fp;
	void *file_data;
	int fsize;

	retval = toreceive(sock, 0, &ph, &buf, 1, 0);
	if (retval < 0) {
		perror("torecieve");
		return NULL;
	}
	printf("buffer = %s\n", (char *) buf);

	/* Open file */
	if ((fp = fopen((char *) buf, "rb")) == NULL) {
		perror("fopen");
		return NULL;
	}

	/* Determine size of file */
	if (fseek(fp, 0, SEEK_END) < 0) {
		perror("fseek");
		return NULL;
	}
	if ((fsize = ftell(fp)) < 0) {
		perror("ftell");
		return NULL;
	}
	rewind(fp);
	printf("filesize = %d\n", fsize);

	/* Allocate memory for file data */
	file_data = malloc(fsize);

	/* Read data from file */
	fread(file_data, sizeof(char), fsize, fp);

	/* Send data to peer */
	ph->command = 0;
	ph->error = E_SUCCESS;
	ph->flags = FLAG_ERROR;
	ph->length = fsize;
	send_packet(sock, file_data, 0, ph);

	/* Free data and close file */
	fclose(fp);
	destroy_packet_header(ph);
	free(buf);
	free(file_data);

	return NULL;
}

void *listen_process(void *params) {
	int sock = *((int *) params);
	struct sockaddr_in remote_addr;
	socklen_t len = sizeof(remote_addr);
	int new_sock;
	fd_set fdset;
	struct timeval tv;
	int retval;

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
				spawn_thread(handle_get, &new_sock);
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

int perform_get(char *filename, char *ip, char *port) {
	int sock;
	struct sockaddr_in remote_addr;
	socklen_t len = sizeof(remote_addr);
	packet_header_p ph;
	void *buf;
	FILE *fp;

	// Create address structure
	memset(&remote_addr, '\0', sizeof(remote_addr));
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(atoi(port));
	remote_addr.sin_addr.s_addr = inet_addr(ip);

	// Create socket
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return -1;
	}

	// Connect to socket
	if (connect(sock, (struct sockaddr *) &remote_addr, len) < 0) {
		perror("connect");
		return -1;
	}

	// Create header
	ph = create_packet_header();
	ph->command = CMD_GET;
	ph->length = strlen(filename) + 1;

	/* Send packet */
	if (send_packet(sock, filename, 0, ph) < 0) {
		perror("send_packet");
		return -1;
	}

	/* Receive packet */
	if (toreceive(sock, 0, &ph, &buf, 1, 0) < 0) {
		perror("toreceive");
		return -1;
	}

	/* Write data to file */
	if ((fp = fopen(filename, "wb+")) == NULL) {
		perror("fopen");
		return -1;
	}
	if (buf != NULL) {
		fwrite(buf, sizeof(char), ph->length, fp);
	}

	/* Free data and close file */
	destroy_packet_header(ph);
	free(buf);
	fclose(fp);

	return 0;
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

	// Spawn thread that waits for get requests
	if (spawn_thread(listen_process, &listen_sock)) {
		perror("spawn_thread");
		return EXIT_FAILURE;
	}

	// Register client with server
	result = register_client(server_sock, client_name, listen_port);
	printf("result = %d\n", result);
	if (result == E_DUPLICATE_NAME) {
		fprintf(stderr, "register_client: Client name %s already exists\n", client_name);
		return EXIT_FAILURE;
	} else if (result < 0) {
		perror("register_client");
		return EXIT_FAILURE;
	}

	// Spawn thread that waits for server requests
	if (spawn_thread(thread_process, &server_sock)) {
		perror("spawn_thread");
		return EXIT_FAILURE;
	}

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

		} else if (strncmp("get", input, 3) == 0) {

			char filename[FILENAME_MAX];
			char ip_addr[32];
			char port_str[16];
			//char data[FILENAME_MAX + 16];

			sscanf(input, "get %s %s %s", filename, ip_addr, port_str);
			printf("filename = %s\n", filename);
			printf("ip = %s\n", ip_addr);
			printf("port = %s\n", port_str);

			perform_get(filename, ip_addr, port_str);

//			memset(data, '\0', sizeof(data));
//			strcat(data, filename);
//			strcat(data, " ");
//			strcat(data, port_str);
//
//			packet_header_p ph = create_packet_header();
//			ph->length = strlen(data) + 1;
//			ph->command = CMD_GET;

		} else {

			printf("unknown command\n");

		}
	}

	// Join threads
	while (thread_pool->length > 0) {
		pthread_t *thread_ptr = list_pop(thread_pool);
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
