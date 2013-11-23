/*
 * server_PFS.c
 *
 *  Created on: Nov 23, 2013
 *      Author: Ben Cavins
 */


#include <stdio.h>
#include <stdlib.h>

#define USAGE "<port number>"
#define ARG_MIN 1

int main(int argc, char *argv[]) {

	int port;

	if (argc < ARG_MIN + 1) {
		fprintf(stderr, "Usage: %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
	}

	port = atoi(argv[1]);

	printf("port = %d\n", port);

	return EXIT_SUCCESS;
}
