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
#define CMD_UNREGISTER_CLIENT 2
#define CMD_LS 3
#define CMD_GET 4

#define E_SUCCESS 1
#define E_DUPLICATE_NAME 2

#define FLAG_ERROR 1

struct packet_header {
	int flags;
	int length;
	int command;
	int error;
};

typedef struct packet_header* packet_header_p;

/*
 * Creates and returns a pointer to a packet_header object. This is created
 * on the heap and must be cleared with a call to destroy_packet_header to
 * avoid memory leaks.
 */
packet_header_p create_packet_header();

/*
 * Frees data associated with a packet_header.
 */
void destroy_packet_header(packet_header_p pkt_hdr);

/*
 * A wrapper for send call. Sends a packet to file descriptor fd. Packet
 * contains data prepended by pkt_hdr. The length of the data field is
 * specified in pkt_hdr. flags and return value are the same as send. See the
 * man page for send for more information.
 */
int send_packet(int fd, void *data, int flags, packet_header_p pkt_hdr);

/*
 * Sends a packet with an error message and no data.
 */
int send_error(int fd, int flags, int error);

/*
 * Wrapper for recv call. Receives a packet_header.
 */
int recv_header(int fd, int flags, packet_header_p pkt_hdr);

/*
 * Wrapper for recv call.
 */
int recv_data(int fd, int flags, void *data, size_t size);

int toreceive(int fd, int flags, packet_header_p *pkt_hdr, void **data, long sec, long usec);


#endif /* PROTOCOL_H_ */
