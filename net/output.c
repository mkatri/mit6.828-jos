#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	int	perm;
	envid_t eid;
	while(1){
		if(ipc_recv(&eid, &nsipcbuf, &perm) != NSREQ_OUTPUT)
			continue;
/*
		if(eid != ns_envid)
			continue;
*/
		cprintf("sending pakcet\n");
		while(sys_netpacket_send(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len) < 0)
			sys_yield();
	}
}
