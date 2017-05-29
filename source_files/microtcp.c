/*
 * Αθανασάκη Ευαγγελια 3076
 */

#include "microtcp.h"
#include "crc32.h"
#define min(X, Y) (((X) < (Y)) ? (X) : (Y))


microtcp_header_t do_ntohl(uint8_t *buffer){
	microtcp_header_t header_host;
	microtcp_header_t *header;
	
	header=(microtcp_header_t*)buffer;
	header_host.seq_number = ntohl(header->seq_number);
	header_host.ack_number=ntohl(header->ack_number);
	header_host.control=ntohs(header->control);
	header_host.window=ntohs(header->window);
	header_host.data_len=ntohl(header->data_len);
	header_host.future_use0=ntohl(header->future_use0);
	header_host.future_use1=ntohl(header->future_use1);
	header_host.future_use2=ntohl(header->future_use2);
	header_host.checksum=ntohl(header->checksum);
	return header_host;
}

microtcp_header_t do_htonl(uint8_t *buffer){
	microtcp_header_t header_host;
	microtcp_header_t *header;
	
	header=(microtcp_header_t*)buffer;
	header_host.seq_number = htonl(header->seq_number);
	header_host.ack_number=htonl(header->ack_number);
	header_host.control=htons(header->control);
	header_host.window=htons(header->window);
	header_host.data_len=htonl(header->data_len);
	header_host.future_use0=htonl(header->future_use0);
	header_host.future_use1=htonl(header->future_use1);
	header_host.future_use2=htonl(header->future_use2);
	header_host.checksum=htonl(header->checksum);
	return header_host;
}

void for_checksum(	uint8_t *buffer, ssize_t size){
	microtcp_header_t *header;
	header=(microtcp_header_t*)buffer;

 	uint32_t crc=header->checksum;
	header->checksum=0;
	memcpy (buffer, header , sizeof(microtcp_header_t));
	if(crc!=(crc32(buffer, size))){
		printf("!PROBLEM IN CHECKSUM!\n");
		exit(EXIT_FAILURE);
	}
}

microtcp_sock_t  microtcp_socket(int domain, int type, int protocol){
	
	microtcp_sock_t my_socket;
	my_socket.sd=socket(domain,type, protocol);
	my_socket.init_win_size=MICROTCP_RECVBUF_LEN;
	my_socket.state=UNKNOWN;
	return my_socket;
}

int microtcp_bind(microtcp_sock_t socket, const struct sockaddr *address,  socklen_t address_len){
	socket.is_server=1;
	if (bind(socket.sd, address, address_len)==-1){
		perror("UDP bind");
		exit(EXIT_FAILURE);
	}
	return 0;
}


microtcp_sock_t microtcp_connect(microtcp_sock_t socket, const struct sockaddr *address, socklen_t address_len){
	
	srand(time(NULL));
	socket.is_server=0;
	int ack, rst, syn, fin;
	uint8_t buffer [1024];
	microtcp_header_t header_host;

	socket.address_clnt= address; //to theme gia th shutdown
	socket.address_clnt_len=address_len;
	
	//first send
	ack=0,rst=0,syn=1,fin=0;
	header_host.seq_number = rand();
	header_host.ack_number=0;
	header_host.control=fin + (syn<<1) + (rst<<2) + (ack<<3);
	header_host.window=MICROTCP_RECVBUF_LEN;
	header_host.data_len=0;
	header_host.future_use0=0;
	header_host.future_use1=0;
	header_host.future_use2=0;
	header_host.checksum=0;
	memcpy (buffer, &header_host , sizeof( header_host));
	header_host.checksum=crc32(buffer, sizeof(header_host));
	memcpy (buffer, &header_host , sizeof( header_host));
	header_host=(do_htonl(buffer));
	socket.curr_win_size=header_host.window;
	memcpy (buffer, &header_host , sizeof( header_host));
	if (sendto(socket.sd, buffer, sizeof( header_host), 0,(const struct sockaddr *) address, address_len)==-1){
		perror("fisrt sending error :");
		exit(EXIT_FAILURE);
	}

	//deyteri recv
	const struct sockaddr *address2=address;
	socklen_t address2_len=sizeof(&address2);
	recvfrom( socket.sd, buffer , sizeof( header_host) , 0, ( struct sockaddr *)&address2, &address2_len) ;
	header_host=(do_ntohl(buffer));
	//checkaroume seq_num
		if(header_host.seq_number== socket.ack_number){
	memcpy (buffer, &header_host , sizeof( header_host));
	for_checksum(buffer, sizeof(microtcp_header_t));
	}
	
	//triti send
	ack=0,rst=0,syn=0,fin=0;
	uint32_t temp_ack_num=header_host.ack_number;
	header_host.ack_number=header_host.seq_number+sizeof(microtcp_header_t);
	header_host.seq_number=temp_ack_num;
	header_host.control=fin + (syn<<1) + (rst<<2) + (ack<<3);
	header_host.window=MICROTCP_RECVBUF_LEN;
	header_host.data_len=0;
	header_host.future_use0=0;
	header_host.future_use1=0;
	header_host.future_use2=0;
	header_host.checksum=0;
	memcpy (buffer, &header_host , sizeof( header_host));
	header_host.checksum=crc32(buffer, sizeof(header_host));
	memcpy (buffer, &header_host , sizeof( header_host));
	//for seq_num kai ack_num pou ta theme sth microtcp_send
	socket.seq_number=header_host.seq_number;
	socket.ack_number=header_host.ack_number;

	header_host=(do_htonl(buffer));
	socket.curr_win_size=header_host.window;
	memcpy (buffer, &header_host , sizeof( header_host));
	if (sendto(socket.sd, buffer, sizeof( header_host), 0,  (const struct sockaddr *) address, address_len)==-1){
		perror("third sending error :");
		exit(EXIT_FAILURE);
	}
	
	
	//dimiourgia internal buffer
	socket.recvbuf=(uint8_t *)malloc(MICROTCP_RECVBUF_LEN);
	socket.buf_fill_level=0;
	
	socket.is_server=0;

	socket.state=ESTABLISHED;
	return socket;
}

microtcp_sock_t  microtcp_accept(microtcp_sock_t socket, struct sockaddr *address, socklen_t address_len){

	srand(time(NULL));
	socket.is_server=1;
	int ack, rst, syn, fin;
	uint8_t buffer [1024];
	microtcp_header_t header_host;

	socket.address_clnt= address;  		//gia th shutdown
	socket.address_clnt_len=address_len;
	
	struct sockaddr *address2=address; // ta theme gia ta send-recv gia na mhn petasei segmentasion sto return pou h recv allazei dieythinsh
	socklen_t address2_len=sizeof(&address2);
	struct sockaddr *address3=address;
	socklen_t address3_len=address_len;
	
	
	//prwth recv 
	recvfrom( socket.sd, buffer , sizeof(header_host) , 0 , (struct sockaddr *)&address3, &address3_len) ;
	header_host=(do_ntohl(buffer));
	memcpy (buffer, &header_host , sizeof( header_host));
	for_checksum(buffer, sizeof(microtcp_header_t));
	
	//deyteri send
	ack=1,rst=0,syn=1,fin=0;
	header_host.ack_number=header_host.seq_number+sizeof(microtcp_header_t);;
	header_host.seq_number =rand();
	header_host.control=fin + (syn<<1) + (rst<<2) + (ack<<3);
	header_host.window=MICROTCP_RECVBUF_LEN;
	header_host.data_len=0;
	header_host.future_use0=0; 
	header_host.future_use1=0;
	header_host.future_use2=0;
	header_host.checksum=0;
	memcpy (buffer, &header_host , sizeof( header_host));
	header_host.checksum=crc32(buffer, sizeof(header_host));
	memcpy (buffer, &header_host , sizeof( header_host));
	//for seq_num kai ack_num pou ta theme sth microtcp_send
	socket.seq_number=header_host.seq_number;
	socket.ack_number=header_host.ack_number;

	header_host=(do_htonl(buffer));
	socket.curr_win_size=header_host.window;
	memcpy (buffer, &header_host , sizeof( header_host));
	sendto(socket.sd, buffer, sizeof(header_host), 0, (struct sockaddr *)&address3, address3_len);

	//triti recv
	recvfrom( socket.sd, buffer , sizeof(header_host), 0, ( struct sockaddr *)&address2, &address2_len) ;
	header_host=(do_ntohl(buffer));
	//checkaroume seq_num
		if(header_host.seq_number== socket.ack_number){
	memcpy (buffer, &header_host , sizeof( header_host));
	for_checksum(buffer, sizeof(microtcp_header_t));
}
	

	//dimiourgia internal buffer
	socket.recvbuf=(uint8_t *)malloc(MICROTCP_RECVBUF_LEN);
	socket.buf_fill_level=0;

	socket.is_server=1;
	socket.FIN_is_called=0;
	socket.state=ESTABLISHED;	
	return socket; 
}


microtcp_sock_t microtcp_shutdown(microtcp_sock_t socket, int is_server){// struct sockaddr *address, socklen_t address_len){
	
	int ack, rst, syn, fin;
	microtcp_header_t header_host;
	uint8_t buff[1000];
	if (is_server){
		ack=1,rst=0,syn=0,fin=0;
		header_host.seq_number = socket.ack_number;
		header_host.ack_number =socket.seq_number+ sizeof(microtcp_header_t);
		header_host.control = fin + (syn<<1) + (rst<<2) + (ack<<3);
		header_host.window = MICROTCP_RECVBUF_LEN;
		header_host.data_len = 0;
		header_host.future_use0 = 0;
		header_host.future_use1 = 0;
		header_host.future_use2 = 0;
		header_host.checksum = 0;
		memcpy (buff, &header_host , sizeof( header_host));
		header_host.checksum=crc32(buff, sizeof(header_host));
		memcpy (buff, &header_host , sizeof( header_host));
		socket.seq_number = header_host.seq_number;
		socket.ack_number = header_host.ack_number;
		header_host=(do_htonl(buff));
		socket.curr_win_size=header_host.window;
		memcpy (buff, &header_host , sizeof( header_host));
		sendto(socket.sd, buff, sizeof(microtcp_header_t), 0, (const struct sockaddr *)socket.address_clnt, socket.address_clnt_len);
		
		
		ack=0,rst=0,syn=0,fin=1;
		header_host.seq_number = socket.ack_number;
		header_host.ack_number =socket.seq_number+ sizeof(microtcp_header_t);
		header_host.control = fin + (syn<<1) + (rst<<2) + (ack<<3);
		header_host.window = MICROTCP_RECVBUF_LEN;
		header_host.data_len = 0;
		header_host.future_use0 = 0;
		header_host.future_use1 = 0;
		header_host.future_use2 = 0;
		header_host.checksum = 0;
		memcpy (buff, &header_host , sizeof( header_host));
		header_host.checksum=crc32(buff, sizeof(header_host));
		memcpy (buff, &header_host , sizeof( header_host));
		socket.seq_number = header_host.seq_number;
		socket.ack_number = header_host.ack_number;
		header_host=(do_htonl(buff));
		socket.curr_win_size=header_host.window;
		memcpy (buff, &header_host , sizeof( header_host));
		sendto(socket.sd, buff, sizeof(microtcp_header_t), 0, (const struct sockaddr *)socket.address_clnt, socket.address_clnt_len);
		
		recvfrom(socket.sd,buff, sizeof(microtcp_header_t),0,( struct sockaddr *)socket.address_clnt,&socket.address_clnt_len);
		header_host=(do_ntohl(buff));
		memcpy (buff, &header_host, sizeof(microtcp_header_t)); //pername sto packeto allagmeno to header gia na kanoume elegx gia ta data
		for_checksum(buff, sizeof(microtcp_header_t));

		socket.state=CLOSING_BY_HOST;
		
	}else if(!is_server) {
		fin=1,syn=0,rst=0,ack=0;
		header_host.ack_number = socket.seq_number+sizeof(microtcp_header_t);
		header_host.control = fin + (syn<<1) + (rst<<2) + (ack<<3);
		header_host.window = MICROTCP_RECVBUF_LEN;
		header_host.data_len = 0;
		header_host.future_use0 = 0; 
		header_host.future_use1= 0;
		header_host.future_use2=0;
		header_host.checksum=0;
		memcpy (buff, &header_host , sizeof( header_host));
		header_host.checksum=crc32(buff, sizeof(header_host));
		memcpy (buff, &header_host , sizeof( header_host));
		socket.seq_number = header_host.seq_number;
		socket.ack_number = header_host.ack_number;
		header_host=(do_htonl(buff));
		memcpy (buff, &header_host , sizeof( header_host));
		sendto(socket.sd, buff, sizeof(microtcp_header_t), 0, (const struct sockaddr *)socket.address_clnt, socket.address_clnt_len);
	

		recvfrom(socket.sd,buff, sizeof(microtcp_header_t),0,( struct sockaddr *)socket.address_clnt,&socket.address_clnt_len);
		header_host=(do_ntohl(buff));
		socket.ack_number = header_host.seq_number+sizeof(microtcp_header_t);
		memcpy (buff, &header_host, sizeof(microtcp_header_t)); //pername sto packeto allagmeno to header gia na kanoume elegx gia ta data
		for_checksum(buff, sizeof(microtcp_header_t));
		
		recvfrom(socket.sd,buff, sizeof(microtcp_header_t),0,( struct sockaddr *)socket.address_clnt,&socket.address_clnt_len);
		header_host=(do_ntohl(buff));
		socket.ack_number = header_host.seq_number+sizeof(microtcp_header_t);
		memcpy (buff, &header_host, sizeof(microtcp_header_t)); //pername sto packeto allagmeno to header gia na kanoume elegx gia ta data
		for_checksum(buff, sizeof(microtcp_header_t));

		ack=1,rst=0,syn=0,fin=0;
		uint32_t temp_seq_num = header_host.seq_number;
		header_host.seq_number = header_host.ack_number;
		header_host.ack_number =temp_seq_num+sizeof(microtcp_header_t);
		header_host.control=fin + (syn<<1) + (rst<<2) + (ack<<3);
		header_host.window=MICROTCP_RECVBUF_LEN;
		header_host.data_len=0;
		header_host.future_use0=0; 
		header_host.future_use1=0;
		header_host.future_use2=0;
		header_host.checksum=0;
		memcpy (buff, &header_host , sizeof( header_host));
		header_host.checksum=crc32(buff, sizeof(header_host));
		memcpy (buff, &header_host , sizeof( header_host));
		socket.seq_number = header_host.seq_number;
		socket.ack_number = header_host.ack_number;
		header_host=(do_htonl(buff));
		socket.curr_win_size=header_host.window;
		memcpy (buff, &header_host , sizeof( header_host));
		sendto(socket.sd, buff, sizeof(microtcp_header_t), 0, (const struct sockaddr *)socket.address_clnt, socket.address_clnt_len);
		socket.state=CLOSING_BY_PEER;

	}
	
	free(socket.recvbuf);
	close(socket.sd);

	return socket;
}


ssize_t microtcp_send(microtcp_sock_t *socket, const void *buffer, 	size_t length, int flags){
	
	int ack, rst, syn, fin, i=0, recved_ack=-1;
	microtcp_header_t header_host;
	uint8_t packet [2000];
	uint8_t is_fisrt_time=1;
	ssize_t data_sent=0;
	ssize_t send_data=0;
	uint8_t prev_ack[100];
	uint8_t dupl_recved=0;
	memset(prev_ack, 0, 100);
	memcpy ((*socket).recvbuf, buffer , length);
	(*socket).cwnd=MICROTCP_INIT_CWND;
	(*socket).ssthresh=MICROTCP_INIT_SSTHRESH;
	ssize_t bytes_to_send=0;
	(*socket).curr_win_size=MICROTCP_RECVBUF_LEN;
	ssize_t remaining_data = length ;
	ssize_t total_send_data=0;
	
	struct timeval timeout ;
	timeout.tv_sec = 0;
	timeout.tv_usec = MICROTCP_ACK_TIMEOUT_US ;
	int tm=setsockopt((*socket).sd, SOL_SOCKET,SO_RCVTIMEO, &timeout,sizeof(struct timeval));
	if ( tm ==-1) {
		perror (" setsockopt ");
	}

	

	while ( data_sent < length ){
			//slow start
		if(is_fisrt_time &&((*socket).cwnd<= (*socket).ssthresh)){
			is_fisrt_time=0;
			bytes_to_send = min( (*socket).curr_win_size , (*socket).cwnd);
			bytes_to_send = min( bytes_to_send, remaining_data );
			
			ssize_t chunks = bytes_to_send / MICROTCP_MSS ;
			
			for(i = 0; i < chunks ; i++){
				memset (packet, 0 , 2000); 
				//gia to header
				fin=0,syn=0,rst=0,ack=0;
				header_host.seq_number = (*socket).seq_number+send_data;
				header_host.ack_number = (*socket).ack_number;
				header_host.control = fin + (syn<<1) + (rst<<2) + (ack<<3);
				header_host.window = MICROTCP_RECVBUF_LEN;
				header_host.data_len = MICROTCP_MSS;
				header_host.future_use0 = 0; 
				header_host.future_use1= 0;
				header_host.future_use2=0;
				header_host.checksum=0;
				memcpy (packet, &header_host , sizeof( header_host));
				(*socket).curr_win_size=header_host.window;
				memcpy((packet+sizeof(microtcp_header_t)),((*socket).recvbuf+total_send_data),MICROTCP_MSS); //vazoume ta data sto packeto
				header_host.checksum=crc32(packet, MICROTCP_MSS+sizeof(microtcp_header_t));
				(*socket).seq_number = header_host.seq_number;
				(*socket).ack_number = header_host.ack_number;
				memcpy (packet, &header_host , sizeof( header_host));
				header_host = (do_htonl(packet));
				memcpy (packet, &header_host , sizeof( header_host)); 
				send_data=sendto((*socket).sd, packet, MICROTCP_MSS+sizeof(microtcp_header_t), 0,(const struct sockaddr *)(*socket).address_clnt, (*socket).address_clnt_len);
				total_send_data+= (send_data -sizeof(microtcp_header_t));

			}
			/* Check if there is a semi - filled chunk */
			if( bytes_to_send % MICROTCP_MSS ){
				chunks ++;
				memset (packet, 0 , 2000); 

				//gia to header
				fin=0,syn=0,rst=0,ack=0;
				header_host.seq_number = (*socket).seq_number+(*socket).send_data;
				header_host.ack_number = (*socket).ack_number;
				header_host.control = fin + (syn<<1) + (rst<<2) + (ack<<3);
				header_host.window = MICROTCP_RECVBUF_LEN;
				header_host.data_len = bytes_to_send % MICROTCP_MSS;
				header_host.future_use0 = 0; 
				header_host.future_use1= 0;
				header_host.future_use2=0;
				header_host.checksum=0;
				memcpy (packet, &header_host , sizeof( header_host));
				(*socket).curr_win_size=header_host.window;
				memcpy((packet+sizeof(microtcp_header_t)),((*socket).recvbuf+total_send_data),MICROTCP_MSS); //vazoume ta data sto packeto
				header_host.checksum=crc32(packet, (bytes_to_send % MICROTCP_MSS)+sizeof(microtcp_header_t));
				(*socket).seq_number = header_host.seq_number;
				(*socket).ack_number = header_host.ack_number;
				memcpy (packet, &header_host , sizeof( header_host));
				header_host = (do_htonl(packet));
				memcpy (packet, &header_host , sizeof( header_host)); 
				send_data=sendto((*socket).sd, packet, (bytes_to_send % MICROTCP_MSS)+sizeof(microtcp_header_t), 0,(const struct sockaddr *)(*socket).address_clnt, (*socket).address_clnt_len);
				total_send_data+= (send_data -sizeof(microtcp_header_t));
			
			}
			/* Get the ACKs */
			for(i = 0; i < chunks ; i++){
				recved_ack=recvfrom( (*socket).sd, packet , sizeof(header_host), 0, ( struct sockaddr *)(*socket).address_clnt,&(*socket).address_clnt_len);
				if(recved_ack==-1){
					printf("timeout\n");
					(*socket).ssthresh = (*socket).cwnd /2;
					(*socket).cwnd = min( MICROTCP_MSS , (*socket).ssthresh );
					break;
				}
				

				header_host=(do_ntohl(packet));
				(*socket).ack_number = header_host.seq_number+sizeof(microtcp_header_t);
				(*socket).curr_win_size=header_host.window;
				memcpy(packet, &header_host, sizeof(header_host) );
				for_checksum(packet, recved_ack);
				//	printf("kanoume recv\n");
				//timeout
				
				//3-duplicate
				if(strncmp (prev_ack, packet, sizeof(microtcp_header_t))==0){
					dupl_recved++;
					if(dupl_recved==3){ 
						(*socket).ssthresh = (*socket).cwnd /2;
						(*socket).cwnd = (*socket).cwnd /2 + 1;
						printf("hey 3 duplicate\n");
						dupl_recved=0;
					 	break;
					}
				}
				memcpy(prev_ack,&packet, sizeof(microtcp_header_t));
		
			}
			
			(*socket).cwnd=(*socket).cwnd*2;
			
			
		}
		else {
				//posa bytes tha steilei xwris na perimenei ack
			bytes_to_send = min( (*socket).curr_win_size , (*socket).cwnd);
			bytes_to_send = min( bytes_to_send, remaining_data );

			ssize_t chunks = bytes_to_send / MICROTCP_MSS ;
		
			for(i = 0; i < chunks ; i++){
				memset (packet, 0 , 2000); 
				
				//gia to header
				fin=0,syn=0,rst=0,ack=0;
				header_host.seq_number = (*socket).seq_number + send_data;
				header_host.ack_number = (*socket).ack_number;
				header_host.control = fin + (syn<<1) + (rst<<2) + (ack<<3);
				header_host.window = MICROTCP_RECVBUF_LEN;
				header_host.data_len = MICROTCP_MSS;
				header_host.future_use0 = 0; 
				header_host.future_use1= 0;
				header_host.future_use2=0;
				header_host.checksum=0;
				memcpy (packet, &header_host , sizeof( header_host));
				(*socket).curr_win_size=header_host.window;
				memcpy((packet+sizeof(microtcp_header_t)),((*socket).recvbuf+total_send_data),MICROTCP_MSS); //vazoume ta data sto packeto
				header_host.checksum=crc32(packet, MICROTCP_MSS+sizeof(microtcp_header_t));
				(*socket).seq_number = header_host.seq_number;
				(*socket).ack_number = header_host.ack_number;
				memcpy (packet, &header_host , sizeof( header_host));
				header_host = (do_htonl(packet));
				memcpy (packet, &header_host , sizeof( header_host)); 
				send_data=sendto((*socket).sd, packet, MICROTCP_MSS+sizeof(microtcp_header_t), 0,(const struct sockaddr *)(*socket).address_clnt, (*socket).address_clnt_len);
				total_send_data+= (send_data -sizeof(microtcp_header_t));
				
			}
			/* Check if there is a semi - filled chunk */

			if( bytes_to_send % MICROTCP_MSS ){
				
				chunks ++;
				memset (packet, 0 , 2000); 

				//gia to header
				fin=0,syn=0,rst=0,ack=0;
				header_host.seq_number = (*socket).seq_number + send_data;
				header_host.ack_number = (*socket).ack_number;
				header_host.control = fin + (syn<<1) + (rst<<2) + (ack<<3);
				header_host.window = MICROTCP_RECVBUF_LEN;
				header_host.data_len = bytes_to_send % MICROTCP_MSS;
				header_host.future_use0 = 0; 
				header_host.future_use1= 0;
				header_host.future_use2=0;
				header_host.checksum=0;
				memcpy (packet, &header_host , sizeof( header_host));
				(*socket).curr_win_size=header_host.window;
				memcpy((packet+sizeof(microtcp_header_t)),((*socket).recvbuf+total_send_data),MICROTCP_MSS); //vazoume ta data sto packeto
				header_host.checksum=crc32(packet, (bytes_to_send % MICROTCP_MSS)+sizeof(microtcp_header_t));
				(*socket).seq_number = header_host.seq_number;
				(*socket).ack_number = header_host.ack_number;
				memcpy (packet, &header_host , sizeof( header_host));
				header_host = (do_htonl(packet));
				memcpy (packet, &header_host , sizeof( header_host)); 
				send_data=sendto((*socket).sd, packet, (bytes_to_send % MICROTCP_MSS)+sizeof(microtcp_header_t), 0,(const struct sockaddr *)(*socket).address_clnt, (*socket).address_clnt_len);
				total_send_data+= (send_data -sizeof(microtcp_header_t));
			}

			/* Get the ACKs */
			//	printf("chunks= %zu\n",chunks );
			for(i = 0; i < chunks ; i++){
				recved_ack=recvfrom( (*socket).sd, packet , sizeof(microtcp_header_t), 0, ( struct sockaddr *)(*socket).address_clnt,&(*socket).address_clnt_len);
				if(recved_ack==-1){
					printf("time out\n");
					(*socket).ssthresh = (*socket).cwnd /2;
					(*socket).cwnd = min( MICROTCP_MSS , (*socket).ssthresh );
					break;
				}

				header_host=(do_ntohl(packet));

				(*socket).ack_number = header_host.seq_number+sizeof(microtcp_header_t);
				(*socket).curr_win_size=header_host.window;
				memcpy(packet, &header_host, sizeof(header_host) );
				for_checksum(packet, recved_ack);
				
				//3-duplicate
				if(strncmp (prev_ack, packet, sizeof(microtcp_header_t))==0){
					dupl_recved++;
					if(dupl_recved==3){ 
						(*socket).ssthresh = (*socket).cwnd /2;
						(*socket).cwnd = (*socket).cwnd /2 + 1;
						printf("hey 3 duplicate\n");
						dupl_recved=0;
					 	break;
					}
				}
				memcpy(prev_ack,&packet, sizeof(microtcp_header_t));
		
			}
		(*socket).cwnd+=MICROTCP_MSS;
			
		}
		remaining_data -= bytes_to_send ;
		data_sent += bytes_to_send ;
	}
	return data_sent;
}

ssize_t microtcp_recv(microtcp_sock_t *socket, void *buffer, size_t length, int flags){
	
	ssize_t accepted=0;
	uint8_t temp_buffer [2000];
	uint8_t packet[1600];
	int ack, rst, syn, fin;
	microtcp_header_t header_host;

		

	if (!(*socket).FIN_is_called){
		accepted=recvfrom((*socket).sd, packet, 1432, flags,( struct sockaddr *)(*socket).address_clnt,&(*socket).address_clnt_len);
		header_host=(do_ntohl(packet));
		memcpy (temp_buffer, packet, sizeof(microtcp_header_t)); 

		memcpy (packet, &header_host, sizeof(microtcp_header_t));
		for_checksum(packet, accepted); 
		//checkaroume seq_num
		if(header_host.seq_number== (*socket).ack_number){
			memcpy ((*socket).recvbuf+(*socket).buf_fill_level, packet+sizeof(microtcp_header_t) , accepted- sizeof(microtcp_header_t)); //grafoume sto internal buffer ta data MONO
			(*socket).buf_fill_level+=(accepted- sizeof(microtcp_header_t)); //ayksanoume to xwro pou exoume piasei ston internal buffer
		
		}	
			

		if (((header_host.control)&1)==1){
			(*socket).FIN_is_called=1;		
			microtcp_shutdown((*socket), (*socket).is_server);

		}
		

		//send header gia to ACK
		ack=1,rst=0,syn=0,fin=0;
		header_host.seq_number = (*socket).seq_number+sizeof(microtcp_header_t);
		header_host.ack_number =(*socket).ack_number +accepted;
		header_host.control = fin + (syn<<1) + (rst<<2) + (ack<<3);
		header_host.window = MICROTCP_RECVBUF_LEN-(*socket).buf_fill_level;
		header_host.data_len = 0;
		header_host.future_use0 = 0;
		header_host.future_use1 = 0;
		header_host.future_use2 = 0;
		header_host.checksum = 0;
		memcpy (temp_buffer, &header_host , sizeof( header_host));
		header_host.checksum = crc32(temp_buffer, sizeof(header_host));
		memcpy (temp_buffer, &header_host , sizeof( header_host)); //mazi me to checksum
		(*socket).seq_number = header_host.seq_number;
		(*socket).ack_number = header_host.ack_number;
		header_host = (do_htonl(temp_buffer));
		(*socket).curr_win_size = header_host.window;
		memcpy (temp_buffer, &header_host , sizeof( header_host)); //meta to htonl
		sendto((*socket).sd, temp_buffer, sizeof(microtcp_header_t), 0,(const struct sockaddr *)(*socket).address_clnt, (*socket).address_clnt_len);
	}
	

		memcpy(buffer,(*socket).recvbuf, length);
		if ((*socket).buf_fill_level){(*socket).buf_fill_level-=length;
		}else{	return 0;	}
		memmove((*socket).recvbuf, (*socket).recvbuf+length,(*socket).buf_fill_level);
		

return  length;
}
