#include <stdio.h>
#include <assert.h>


#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_arpcache.h"
#include "sr_utils.h"
#include "sr_icmp_handler.h"


size_t eth_frame_size = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t) + ICMP_DATA_SIZE


/*
* Return a newly constructed ICMP packet struct given the type, code 
  and length of data the packet holds and the pointer to that data 
  as well.

*/

/* FIX/CHECK from the assignment
You may want to create additional structs for ICMP messages for
 convenience, but make sure to use the packed attribute so that
  the compiler doesn’t try to align the fields in the struct to 
  word boundaries:
*/


/*
 * Return an ICMP packet header given its type and code. 
 */
uint8_t* gen_icmp_packet (int type, int code) {
	uint8_t *icmp_pkt = malloc(sizeof(sr_icmp_hdr_t) + sizeof(uint8_t) * ICMP_DATA_SIZE));
	// pad icmp cargo with 0's
	bzero(icmp_pkt + sizeof(sr_icmp_t3_hdr_t), ICMP_DATA_SIZE);
	
	switch (type) {
		case 0:
			/* ICMP type: echo reply*/
			sr_icmp_hdr_t *icmp_hdr = malloc(sizeof(sr_icmp_hdr_t));
			icmp_hdr->icmp_type = 0;
		    icmp_hdr->icmp_code = 0;
			icmp_hdr->icmp_sum = cksum(icmp_hdr + sizeof(sr_icmp_hdr_t), ICMP_DATA_SIZE);
			icmp_pkt = icmp_hdr;

		case 11: 
			/* ICMP type: time exceeded*/
			sr_icmp_hdr_t *icmp_hdr = malloc(sizeof(sr_icmp_hdr_t));
			icmp_hdr->icmp_type = 11;
		    icmp_hdr->icmp_code = 0;
			icmp_hdr->icmp_sum = cksum(icmp_hdr + sizeof(sr_icmp_hdr_t), ICMP_DATA_SIZE);
			icmp_pkt = icmp_hdr;

		case 3:
			/* ICMP type 3: X unreachable*/
			sr_icmp_t3_hdr_t icmp_hdr = malloc(sizeof(sr_icmp_t3_hdr_t));
			icmp_hdr->type = 3;
			icmp_hdr->icmp_sum = cksum(icmp_hdr + sizeof(icmp_hdr), ICMP_DATA_SIZE);
			icmp_pkt = icmp_hdr;

			switch (code) {				
				case 0:
					/* destination unreachable*/
					icmp_hdr->icmp_code = 0;

				case 1:
					/* host unreachable*/
					icmp_hdr->icmp_code = 1;

				case 3:
					/* port unreachable*/
					icmp_hdr->icmp_code = 3;

				default:
					fprintf("unsupported ICMP code specified.\n");
					icmp_pkt = NULL;
			
			} /* end of inner switch switch*/

		default:
			/* ICMP type: unsupported*/
			fprintf("unsupported ICMP type\n");
			icmp_pkt = NULL;

	} /* end of outer switch statement*/

	return icmp_pkt;
}


/* Return an ethernet frame that encapsulates the ICMP packet.
 * The ICMP packet is encapsulated in an IP packet then ethernet frame.
 */


     /* REMOVE THIS COMMENT
                Ethernet frame
      ------------------------------------------------
      | Ethernet hdr |       IP Packet               |
      ------------------------------------------------
                     ---------------------------------
                     |  IP hdr |   IP Packet         |
                     ---------------------------------
                      		   -----------------------
                      		   |ICMP hdr | ICMP DATA |
                      		   -----------------------
    */

sr_ethernet_hdr_t* gen_eth_frame (sr_ethernet_hdr_t *old_eth_pkt, old_len, uint8_t *icmp_pkt, int icmp_type) {

	/* Create the ethernet header*/
	sr_ethernet_hdr_t *eth_hdr = sizeof(eth_hdr);
	memcpy(eth_hdr->ether_dhost, ether_shost, ETHER_ADDR_LEN);
	memcpy(eth_hdr->ether_shost, ether_dhost, ETHER_ADDR_LEN);
	eth_hdr->ether_type = htons(ether_type_ip);

	/* Create the IP header*/
	sr_ip_hdr_t *ip_hdr = malloc(sizeof(sr_ip_hdr_t));
	ip_hdr->ip_hl = 5;
	ip_hdr->ip_v = 4;
	ip_hdr->ip_tos = 0;				
	ip_hdr->ip_len = (size_t)(sizeof(sr_ip_hdr_t));
	ip_hdr->ip_id = 0;
	ip_hdr->ip_off = htons(IP_DF);
	ip_hdr->ip_ttl = 0;
	ip_hdr->ip_p = ip_protocol_icmp;

	if (icmp_type != 3) {
		ip_hdr->ip_sum = cksum(ip_hdr + sizeof(sr_ip_hdr_t), sizeof(sr_icmp_hdr_t) + ICMP_DATA_SIZE);
	} else { // type == 0
		ip_hdr->ip_sum = cksum(ip_hdr + sizeof(sr_ip_hdr_t), sizeof(sr_icmp_t3_hdr_t) + ICMP_DATA_SIZE);
	}
	
	memcpy(ip_hdr->ip_src, ether_dhost, ETHER_ADDR_LEN);
	memcpy(ip_hdr->ip_dst, ether_shost, ETHER_ADDR_LEN);

	/* Encapsulate the three protocol packets into one*/
	uint8_t *new_eth_pkt = malloc();
	uint8_t *eth_cargo = new_eth_pkt + sizeof(sr_ethernet_hdr_t);
	uint8_t *ip_cargo = eth_cargo + sizeof(sr_ip_hdr_t);
	
	memcpy(new_eth_pkt, eth_hdr, sizeof(sr_ethernet_hdr_t));
	memcpy(eth_cargo, ip_hdr, sizeof(sr_ip_hdr_t));
	memcpy(ip_cargo, icmp_pkt, sizeof(sr_icmp_hdr_t) + ICMP_DATA_SIZE);

	return (sr_ethernet_hdr_t*) new_eth_pkt;
}

	
void send_icmp_echo_request(struct sr_instance *sr, uint8_t *packet, struct sr_if *interface) {
	sr_send_packet(sr, gen_icmp_packet(packet, 0), eth_frame_size, interface);
}


void send_icmp_net_unreachable(struct sr_instance *sr, uint8_t *packet, struct sr_if *interface) {
	sr_send_packet(sr, gen_icmp_packet(packet, 3, 0), eth_frame_size, interface);
}


void send_icmp_host_unreachable(struct sr_instance *sr, uint8_t *packet, struct sr_if *interface) {
	sr_send_packet(sr, gen_icmp_packet(packet, 3, 1), eth_frame_size, interface);
}


void send_icmp_port_unreachable(struct sr_instance *sr, uint8_t *packet, struct sr_if *interface) {
	sr_send_packet(sr, gen_icmp_packet(packet, 3, 3), eth_frame_size, interface);
}


void send_icmp_time_exceeded(struct sr_instance *sr, uint8_t *packet, struct sr_if *interface) {
	sr_send_packet(sr, gen_icmp_packet(packet, 11), eth_frame_size, interface);
}
