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
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "protocol.h"
#include "list.h"

#define USAGE "<port number>"
#define ARG_MIN 1
#define BACKLOG 10

typedef struct master_list_entry {
	char filename[FILENAME_MAX];
	char size[64];
	char owner[MAX_CLIENT_NAME_LEN];
	char ip[24];
	char port[24];
} ml_entry_t;

typedef struct client {
	char name[MAX_CLIENT_NAME_LEN];
	char listen_port[24];
	int sock;
} client_t;

list_p thread_pool;
list_p clients;
list_p master_list;

int client_exists(char *name) {
	list_iter_p iter = list_iterator(clients, FRONT);
	while (list_next(iter) != NULL) {
		client_t *client = list_current(iter);
		if (strcmp(name, client->name) == 0) {
			return 1;
		}
	}
	return 0;
}

client_t *get_client_by_sock(int sock) {
	list_iter_p iter = list_iterator(clients, FRONT);
	while (list_next(iter) != NULL) {
		client_t *client = list_current(iter);
		if (sock == client->sock) {
			return client;
		}
	}
	return NULL;
}

void print_client_list() {
	list_iter_p iter = list_iterator(clients, FRONT);
	printf("--- Clients ---\n");
	while (list_next(iter) != NULL) {
		client_t *client = list_current(iter);
		printf("%s, %s, %d\n", client->name, client->listen_port, client->sock);
	}
}

int reg_client(char *str, int sock) {
	char name[MAX_CLIENT_NAME_LEN];
	char port[24];
	client_t client;

	sscanf(str, "%s %s", name, port);

	strncpy(client.name, name, sizeof(client.name));
	strncpy(client.listen_port, port, sizeof(client.listen_port));
	client.sock = sock;

	list_add(clients, &client, sizeof(client));

	return 0;
}

void remove_client(char *name) {
	list_iter_p iter = list_iterator(clients, FRONT);
	while (list_next(iter) != NULL) {
		client_t *client = list_current(iter);
		if (strcmp(name, client->name) == 0) {
			free(list_current(iter));
			list_pluck(clients, iter->current);
			break;
		}
	}
}

int add_files(int sock, char *data) {
	client_t *client;
	ml_entry_t entry;
	char *filename;
	char ip[24];
	char *bytes;
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	int first = 1;
	client = get_client_by_sock(sock);
	if (getpeername(sock, (struct sockaddr *) &addr, &len) < 0) {
		perror("getpeername");
		return -1;
	}
	strncpy(ip, inet_ntoa(addr.sin_addr), sizeof(ip));
	while (1) {
		if (first) {
			filename = strtok(data, " ");
			bytes = strtok(NULL, " ");
			first = 0;
		} else {
			filename = strtok(NULL, " ");
			bytes = strtok(NULL, " ");
		}
		if (filename == NULL || bytes == NULL) {
			break;
		}
		strncpy(entry.filename, filename, sizeof(entry.filename));
		strncpy(entry.ip, ip, sizeof(entry.ip));
		strncpy(entry.owner, client->name, sizeof(entry.owner));
		strncpy(entry.port, client->listen_port, sizeof(entry.port));
		strncpy(entry.size, bytes, sizeof(entry.size));
		list_add(master_list, &entry, sizeof(entry));
	}
	return 0;
}

void create_file_list(char *buf) {
	list_iter_p iter = list_iterator(master_list, FRONT);
	char line[sizeof(ml_entry_t) + 18];
	sprintf(buf, "File Name || File Size || File Owner || Owner IP || Owner Port\n");
	while (list_next(iter) != NULL) {
		ml_entry_t *entry = list_current(iter);
		sprintf(line, "%s || %s || %s || %s || %s\n",
				entry->filename, entry->size, entry->owner, entry->ip, entry->port);
		strcat(buf, line);
	}
}

void print_file_list() {
//	list_iter_p iter = list_iterator(master_list, FRONT);
//	printf("File Name || File Size || File Owner || Owner IP || Owner Port\n");
//	while (list_next(iter) != NULL) {
//		ml_entry_t *entry = list_current(iter);
//		printf("%s || %s || %s || %s || %s\n",
//				entry->filename, entry->size, entry->owner, entry->ip, entry->port);
//	}
	char *buf;
	buf = malloc(1024 * 25);
	create_file_list(buf);
	printf("%s\n", buf);
	free(buf);
}

//int request_file_list(int sock) {
//	int result;
//	packet_header_p pkt_hdr = create_packet_header();
//	pkt_hdr->command = CMD_LS;
//	pkt_hdr->length = 0;
//	if (send_packet(sock, "", 0, pkt_hdr) < 0) {
//		return -1;
//	}
//	recv_header(sock, 0, pkt_hdr);
//	result = pkt_hdr->error;
//	destroy_packet_header(pkt_hdr);
//	return result;
//}
int perform_ls(int sock) {
	//int result;
	packet_header_p ph = create_packet_header();
	void *buf = NULL;

	ph->command = CMD_LS;
	ph->error = 0;
	ph->flags = 0;
	ph->length = 0;

	if (send_packet(sock, "", 0, ph) < 0) {
		perror("send_packet");
		return -1;
	}

	toreceive(sock, 0, &ph, &buf, 1, 0);

	if (ph->error != E_SUCCESS) {
		return -1;
	}

	if (buf != NULL) {
		printf("Buffer = %s\n", (char *) buf);
		// TODO Add files to master list
		add_files(sock, (char *) buf);
	}

	destroy_packet_header(ph);
	free(buf);
	return 0;
}

static void handler(int signum) {
	printf("Hello, signal %d!\n", signum);
	printf("list size = %d\n", thread_pool->length);

	// Join all threads
	while (thread_pool->length > 0) {
		pthread_t *thread_ptr = list_pop(thread_pool);
		pthread_join(*thread_ptr, NULL);
	}

	printf("list size = %d\n", thread_pool->length);

	// Destroy data structures
	destroy_list(thread_pool);
	destroy_list(clients);
	destroy_list(master_list);

	exit(EXIT_SUCCESS);
}

void *thread_process(void *params) {

	int sock = *((int *) params);
	int result;
	char *buf = NULL;

	printf("Hello thread\n");

	for (;;) {

		packet_header_p pkt_hdr = create_packet_header();
		int res = recv_header(sock, MSG_WAITALL, pkt_hdr);
		if (res < 0) {
			perror("recv_header");
		} else if (res == 0) {
			printf("client has closed the connection\n");
			destroy_packet_header(pkt_hdr);
			return 0;
		}

		printf("command = %d\n", pkt_hdr->command);
		printf("flags = %d\n", pkt_hdr->flags);
		printf("length = %d\n", pkt_hdr->length);
		printf("error = %d\n", pkt_hdr->error);

		buf = malloc(pkt_hdr->length);

		res = recv_data(sock, MSG_WAITALL, buf, pkt_hdr->length);
		if (res < 0) {
			perror("recv_data");
		} else if (res == 0) {
			printf("client has closed the connection\n");
			destroy_packet_header(pkt_hdr);
			free(buf);
			return 0;
		}

		printf("data = %s\n", buf);

		/*** Register Client ***/
		if (pkt_hdr->command == CMD_REGISTER_CLIENT) {

			if (client_exists(buf)) {
				printf("client already exists\n");
				send_error(sock, 0, E_DUPLICATE_NAME);
			} else {
				printf("registering client %s\n", buf);
				//list_add(clients, buf, pkt_hdr->length);
				reg_client(buf, sock);
				print_client_list();
				send_error(sock, 0, E_SUCCESS);

				// Request file list from client
				result = perform_ls(sock);
				if (result < 0) {
					fprintf(stderr, "ls failed\n");
				} else {
					printf("Printing file list...\n");
					//print_file_list();
					// TODO Send file list to clients
					char *buf;
					buf = malloc(1024 * 25);
					create_file_list(buf);
					printf("%s\n", buf);
					packet_header_p ph = create_packet_header();
					ph->command = CMD_PUSH_LIST;
					ph->error = E_SUCCESS;
					ph->length = strlen(buf) + 1;
					send_packet(sock, buf, 0, ph);
					destroy_packet_header(ph);
					free(buf);
				}
			}

		/*** Remove Client ***/
		} else if (pkt_hdr->command == CMD_UNREGISTER_CLIENT) {

			remove_client(buf);
			print_client_list();
			send_error(sock, 0, E_SUCCESS);

		/*** Unknown Command ***/
		} else {
			printf("Unknown command\n");
		}

		free(buf);
		destroy_packet_header(pkt_hdr);
	}


	close(sock);

	return NULL;
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

	// Initialize data structures
	thread_pool = create_list();
	clients = create_list();
	master_list = create_list();

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
