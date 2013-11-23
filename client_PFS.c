/*
 * client_PFS.c
 *
 *  Created on: Nov 23, 2013
 *      Author: Ben Cavins
 */


#include <stdio.h>
#include <stdlib.h>

#define USAGE "<client name> <server IP> <server port>"
#define ARG_MIN 3

int main(int argc, char *argv[]) {

	char *client_name;
	char *server_ip;
	int server_port;

	if (argc < ARG_MIN) {
		fprintf(stderr, "Usage: %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
	}

	client_name = argv[1];
	server_ip = argv[2];
	server_port = atoi(argv[3]);

	printf("client name = %s\n", client_name);
	printf("server ip = %s\n", server_ip);
	printf("server port = %d\n", server_port);

	return 0;
}
