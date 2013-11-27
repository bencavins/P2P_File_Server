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
