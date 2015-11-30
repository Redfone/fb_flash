
#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>
#include <float.h>
#include <sys/ioctl.h>

#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>       /* the L2 protocols */
#include <arpa/inet.h>          /* for htons()      */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define SW_VER "1.0"

/****************************************/
void print_usage()
{
	printf("Redfone TDMoE Utility\n-----------------------\n");
	printf("Usage options:\n");
	printf("d:\tDisable packet reflection\n");
	printf("i <nic_idx>:\tUse NIC Index nic_idx\n");
	printf("h:\tPrint this information\n");
	printf("p <cnt>:\tNumber of packets to run for. 0 = infinite\n");
	printf("Sample Dump Commands:\n");
	printf("w <file>:\tFilename to write samples to\n");
	printf("c <chan>:\tChannel offset in packet to read (dflt=0)\n");
	printf("s <sampl>:\tNumber of samples in packet per channel (dflt=8)\n");
	printf("t <type>:\tDump type (0=straight, 1=u-law, 2=16-bit 2's comp. linear\n");
}
unsigned int seg_offset[] = 
{
	1,31,95,223,479,991,2015,4063
};
unsigned int seg_mult[] =
{
	2,4,8,16,32,64,128,256
};
int ulaw_expand(unsigned char code)
{
	unsigned char sign;
	int ret;
	
	if (code & 0x80) sign = 1;
	else sign = 0;
	code &= 0x7F;
	code ^= 0x7F;
	if (code == 0) return 0;
	/* First get segment base */
	ret = seg_offset[code >> 4];
	if ((code >> 4) == 0) ret += seg_mult[code>>4]*((code&0xf)-1);
	else ret += seg_mult[code >> 4]*(code&0xf);
	if (sign) return ret;
	return -ret;
}
#define PAYLOAD_OFFSET 104
#define DUMP_TYPE_STRAIGHT 0
#define DUMP_TYPE_ULAW 1
#define DUMP_TYPE_LINEAR 2
int main(int argc, char *argv[])
{
	int eth_socket;
	int ret_code;
	int x;
	char c;
	extern char *optarg;
	extern int optind, optopt;
	struct sockaddr_ll sa_ll;
	int nic_index = 0;
	unsigned char dont_reflect = 0;
	FILE *dumpfile = NULL;
	unsigned char dump_channel = 0;
	unsigned char dump_samples = 8;
	unsigned char dump_enabled = 0;
	unsigned char dump_type = 0;
	int packet_count = 1000,cur_packet;
	u_char buffer[1500];
	unsigned char *dump_chan_base;
	int tmp = 0;

	eth_socket = socket (AF_PACKET, SOCK_RAW, htons (0xD00D));

	if (eth_socket < 0)
	{
		perror ("socket");
		exit (EXIT_FAILURE);
	}

	/* Parse arguments */
	while ((c = getopt (argc, argv, ":hdi:c:s:w:p:t:")) != -1)
	{
		switch (c)
		{
			case 'h':
				/* Help */
				print_usage ();
				exit (EXIT_SUCCESS);
				break;


			case 'i':
				nic_index = strtol(optarg, NULL, 0); 
				break;

			case 'd':
				dont_reflect = 1;
				break;
				
			case 'w':
				dumpfile = fopen(optarg, "w+");
				if (!dumpfile)
				{
					printf("Error opening file\n");
					return -1;
				}
				dump_enabled = 1;
				break;

			case 'c':
				dump_channel = strtol(optarg, NULL, 0);
				break;

			case 's':
				dump_samples = strtol(optarg, NULL, 0);
				break;
			case 'p':
				packet_count = strtol(optarg, NULL, 0);
				break;
			case 't':
				dump_type = strtol(optarg, NULL, 0);
				break;

		}
	}
	printf("Using NIC Index %d\n",nic_index);
	printf("Packet reflection: %s, Packet count = %d\n",dont_reflect?"Disabled":"Enabled",packet_count);
	if (dump_enabled)
	{
		printf("Channel dump enabled, Channel Offset %d, %d samples per packet\n",dump_channel,dump_samples);
		printf("Recording %s samples\n",(dump_type==DUMP_TYPE_STRAIGHT)?"raw":
				(dump_type == DUMP_TYPE_ULAW)?"mu-law":
				(dump_type == DUMP_TYPE_LINEAR)?"16-bit linear":"undefined");
	}

	/* Setup Ethernet structure */
	sa_ll.sll_family = AF_PACKET;
	sa_ll.sll_protocol = htons (0xD00F);

	sa_ll.sll_ifindex = nic_index + 2; /* Might need to change for non-standard NIC index */
	sa_ll.sll_halen = ETH_ALEN;

	cur_packet = 0;
	dump_chan_base = buffer + PAYLOAD_OFFSET + (dump_channel*dump_samples);
	while(1)
	/* Receive reply */
	{
		ret_code = recv(eth_socket, (void *) buffer, 1500, 0);

		memcpy (sa_ll.sll_addr, buffer, ETH_ALEN);
		/* Swap MACs */
		memcpy (buffer, buffer+6, 6);
		memcpy (buffer+6, sa_ll.sll_addr, 6);

		if (!dont_reflect)
			ret_code = sendto (eth_socket, buffer, ret_code, 0,
				(struct sockaddr *) &sa_ll, sizeof (sa_ll));
		if (dump_enabled)
			for (x=0;x<dump_samples;x++)
			{
				switch(dump_type)
				{
					case DUMP_TYPE_STRAIGHT:
						tmp = dump_chan_base[x];
						break;
					case DUMP_TYPE_ULAW:
						tmp = ulaw_expand(dump_chan_base[x]);
						break;
					case DUMP_TYPE_LINEAR:
						tmp = (dump_chan_base[x] << 8) | (dump_chan_base[x+dump_samples]&0xff);
						if (tmp & (1<<15)) tmp = -((tmp ^ 0xFFFF) + 1);
						break;
				}
				fprintf(dumpfile,"%d\n",tmp);
			}

		if ((packet_count == 0) || (cur_packet++ == (packet_count-1))) break;

	}
	printf("Finished capturing %d packets\n",cur_packet);
	if (dump_enabled) fclose(dumpfile);
	/* Never executed. */
	close (eth_socket);
	exit (EXIT_SUCCESS);

}
