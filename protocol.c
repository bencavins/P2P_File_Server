/*
 * protocol.c
 *
 *  Created on: Nov 27, 2013
 *      Author: Ben Cavins
 */


#include <stdlib.h>

#include "protocol.h"

packet_header_p create_packet_header() {
	packet_header_p pkt_hdr = (packet_header_p) malloc(sizeof(struct packet_header));
	pkt_hdr->flags = 0;
	pkt_hdr->length = 0;
	pkt_hdr->command = 0;
	return pkt_hdr;
}

void destroy_packet_header(packet_header_p pkt_hdr) {
	free(pkt_hdr);
}

int send_packet(int fd, void *data, int flags, packet_header_p pkt_hdr) {
	int result;
	size_t n;
	char *buf;
	int offset;

	n = pkt_hdr->length + sizeof(struct packet_header);
	buf = (char *) malloc(n);
	offset = sizeof(struct packet_header);
	memset(buf, '\0', n);
	memcpy(buf, pkt_hdr, sizeof(struct packet_header));
	memcpy(buf + offset, data, pkt_hdr->length);
	result = send(fd, buf, n, flags);

	return result;
}

char *recv_packet(int fd, int flags, packet_header_p pkt_hdr) {
	char *data;
	int res;

	res = recv(fd, pkt_hdr, sizeof(struct packet_header), flags);
	if (res < 0) {
		return NULL;
	} else if (res == 0) {
		return NULL;
	}
	data = malloc(pkt_hdr->length);
	res = recv(fd, data, pkt_hdr->length, flags);
	if (res < 0) {
		return NULL;
	} else if (res == 0) {
		return NULL;
	}

	return data;
}
