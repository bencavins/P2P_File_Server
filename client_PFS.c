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

	if (argc < ARG_MIN) {
		fprintf(stderr, "Usage: %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
	}

	printf("Hello client\n");
	return 0;
}
