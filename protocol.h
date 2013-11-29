/*
 * protocol.h
 *
 *  Created on: Nov 27, 2013
 *      Author: Ben Cavins
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#define CMD_REGISTER_CLIENT 1

struct packet_header {
	int flags;
	int length;
	int command;
};

typedef struct packet_header* packet_header_p;

packet_header_p create_packet_header();

void destroy_packet_header(packet_header_p pkt_hdr);

int send_packet(int fd, void *data, int flags, packet_header_p pkt_hdr);

int recv_header(int fd, int flags, packet_header_p pkt_hdr);

int recv_data(int fd, int flags, void *data, size_t size);

#endif /* PROTOCOL_H_ */
