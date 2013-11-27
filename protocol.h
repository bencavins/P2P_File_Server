/*
 * protocol.h
 *
 *  Created on: Nov 27, 2013
 *      Author: Ben Cavins
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#define CMD_REGISTER 1

struct packet_header {
	int flags;
	int length;
	int command;
};

typedef struct packet_header* packet_header_p;

packet_header_p create_packet_header();

void destroy_packet_header(packet_header_p pkt_hdr);

#endif /* PROTOCOL_H_ */
