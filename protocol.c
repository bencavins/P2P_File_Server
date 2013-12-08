/*
 * protocol.c
 *
 *  Created on: Nov 27, 2013
 *      Author: Ben Cavins
 */


#include <sys/select.h>
#include <stdlib.h>

#include "protocol.h"

packet_header_p create_packet_header() {
	packet_header_p pkt_hdr = (packet_header_p) malloc(sizeof(struct packet_header));
	pkt_hdr->flags = 0;
	pkt_hdr->length = 0;
	pkt_hdr->command = 0;
	pkt_hdr->error = 0;
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

int send_error(int fd, int flags, int error) {
	int result;
	packet_header_p pkt_hdr;
	pkt_hdr = create_packet_header();
	pkt_hdr->flags = FLAG_ERROR;
	pkt_hdr->error = error;
	result = send(fd, pkt_hdr, sizeof(struct packet_header), flags);
	destroy_packet_header(pkt_hdr);
	return result;
}

int recv_header(int fd, int flags, packet_header_p pkt_hdr) {
	return recv(fd, pkt_hdr, sizeof(struct packet_header), flags);
}

int recv_data(int fd, int flags, void *data, size_t size) {
	return recv(fd, data, size, flags);
}

int toreceive(int fd, int flags, packet_header_p *pkt_hdr, void **data, long sec, long usec) {
	fd_set fdset;
	struct timeval tv;
	int bytes_recvd = 0;
	int retval;
	packet_header_p ph;

	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);

	tv.tv_sec = sec;
	tv.tv_usec = usec;

	retval = select(fd+1, &fdset, NULL, NULL, &tv);

	if (retval < 0) {
		return -1;
	} else if (retval == 0) {
		return 0;
	} else {
		if (FD_ISSET(fd, &fdset)) {
			if (*pkt_hdr == NULL) {
				*pkt_hdr = create_packet_header();
			}
			retval = recv_header(fd, flags, *pkt_hdr);
			if (retval <= 0) {
				return -1;
			}
			ph = *pkt_hdr;
			bytes_recvd += retval;
			if (ph->length > 0) {
				if (*data == NULL) {
					*data = malloc(ph->length);
				}
				retval = recv_data(fd, flags, *data, ph->length);
				if (retval <= 0) {
					return -1;
				}
				bytes_recvd += retval;
			}
		}
		return bytes_recvd;
	}
}
