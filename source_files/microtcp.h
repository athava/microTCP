/*
 * microtcp.h
 *
 *  Created on: Oct 25, 2015
 *      Author: surligas
 */

#ifndef LIB_MICROTCP_H_
#define LIB_MICROTCP_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>

/*
 * Several useful constants
 */
#define MICROTCP_ACK_TIMEOUT_US 200000
#define MICROTCP_MSS 1400
#define MICROTCP_RECVBUF_LEN 8192
#define MICROTCP_WIN_SIZE MICROTCP_RECVBUF_LEN
#define MICROTCP_INIT_CWND (3 * MICROTCP_MSS)
#define MICROTCP_INIT_SSTHRESH MICROTCP_WIN_SIZE


typedef enum {
	LISTEN,
	ESTABLISHED,
	CLOSING_BY_PEER,
	CLOSING_BY_HOST,
	CLOSED,
	INVALID,
	UNKNOWN
} mircotcp_state_t;


typedef struct {
	

	mircotcp_state_t 	state;		
	int				sd;	

	size_t			init_win_size;	
	size_t			curr_win_size;	
	
	uint8_t 		*recvbuf; 	
	size_t			buf_fill_level; 

	size_t			cwnd;
	size_t			ssthresh;

	size_t			seq_number; 
	size_t			ack_number; 

	uint8_t			FIN_is_called;
	int 			is_server;

	const struct sockaddr *address_clnt ; 
	socklen_t address_clnt_len;
} microtcp_sock_t;


/**
 * microTCP header structure
 */
typedef struct {
	uint32_t	seq_number;  /**< Sequence number */
	uint32_t	ack_number;  /**< ACK number */
	uint16_t	control;     /**< Control bits (e.g. SYN, ACK, FIN) */
	uint16_t	window;      /**< Window size in bytes */
	uint32_t	data_len;    /**< Data legth in bytes (EXCLUDING header) */
	uint32_t	future_use0; /**< 32-bits for future use */
	uint32_t	future_use1; /**< 32-bits for future use */
	uint32_t	future_use2; /**< 32-bits for future use */
	uint32_t	checksum;    /**< CRC-32 checksum, see crc32() in utils folder */
} microtcp_header_t;


microtcp_sock_t  microtcp_socket(int domain, int type, int protocol);

int microtcp_bind(microtcp_sock_t socket, const struct sockaddr *address, socklen_t address_len);

microtcp_sock_t microtcp_connect(microtcp_sock_t socket, const struct sockaddr *address, socklen_t address_len);

microtcp_sock_t microtcp_accept(microtcp_sock_t socket, struct sockaddr *address, socklen_t address_len);

microtcp_sock_t microtcp_shutdown(microtcp_sock_t socket, int how);

ssize_t microtcp_send(microtcp_sock_t *socket, const void *buffer, size_t length, int flags);

ssize_t microtcp_recv(microtcp_sock_t *socket, void *buffer, size_t length, int flags);

#endif /* LIB_MICROTCP_H_ */
