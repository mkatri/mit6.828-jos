#include <kern/e1000.h>

// LAB 6: Your driver code here
//mac address is currently hard-coded with qemu's default value
//should be read from EEPROM
//this driver uses polling rather than interrupts, so is not very efficient
//uint32_t mac[2] = {0x12005452, 0x5634};

uint32_t * volatile e1000;
struct tx_desc tx_d[TXRING_LEN] __attribute__ ((aligned (PGSIZE))) 
	= {{0, 0, 0, 0, 0, 0, 0}};
struct packet pbuf[TXRING_LEN] __attribute__ ((aligned (PGSIZE)))
	= {{{0}}};
/*
struct rx_desc rx_d[RXRING_LEN] __attribute__ ((aligned (PGSIZE)))
	= {{0, 0, 0, 0, 0, 0}};
struct packet prbuf[RXRING_LEN] __attribute__ ((aligned (PGSIZE)))
	= {{{0}}};
*/
static void
init_desc(){
	int i;
	for(i = 0; i < TXRING_LEN; i++){
		memset(&tx_d[i], 0, sizeof(tx_d[i]));
		tx_d[i].addr = PADDR(&pbuf[i]);
		tx_d[i].status = TXD_STAT_DD;
		tx_d[i].cmd = TXD_CMD_RS|TXD_CMD_EOP;
	}
/*
	for(i = 0; i < RXRING_LEN; i++){
		memset(&rx_d[i], 0, sizeof(rx_d[i]));
		rx_d[i].addr = PADDR(&prbuf[i]);
		tx_d[i].status = 0;
	}
*/
}

int
e1000_pci_attach(struct pci_func *pcif){
	pci_func_enable(pcif);
	init_desc();

	e1000 = (uint32_t *)mmio_map_region(kern_pgdir, ROUNDDOWN(pcif->reg_base[0], PGSIZE), 
		ROUNDUP(pcif->reg_size[0], PGSIZE), PTE_W|PTE_PCD|PTE_PWT);

	e1000[TDBAL/4] = PADDR(tx_d);
	e1000[TDBAH/4] = 0;
	e1000[TDLEN/4] = TXRING_LEN*sizeof(struct tx_desc);
	e1000[TDH/4] = 0;
	e1000[TDT/4] = 0;
	e1000[TCTL/4] = TCTL_EN|TCTL_PSP|(TCTL_CT & (0x10 << 4))|(TCTL_COLD & (0x40 <<12));
	e1000[TIPG/4] = 10|(8<<10)|(6<<20);
	
/*
	e1000[RA/4+1] = RAS_DEST;
	e1000[RA/4] = mac[0];
	e1000[RA/4+1] = mac[1];
	e1000[RA/4+1] |= RAV;

	cprintf("e1000: mac address %x:%x\n", e1000[RA/4+1], e1000[RA/4]);

	memset(&e1000[MTA/4], 0, 127*4);
	e1000[RDBAL/4] = PADDR(rx_d);
	e1000[RDBAH/4] = 0;
	e1000[RDLEN/4] = RXRING_LEN*sizeof(struct rx_desc);
	e1000[RDH/4] = 0;
	e1000[RDT/4] = 0;
	e1000[RCTL/4] = 0|RCTL_EN|RCTL_BSIZE|RCTL_SECRC;
*/
	cprintf("e1000: status %x\n", e1000[STATUS/4]);
	return 1;
};

int
e1000_transmit(void *addr, size_t length){
	uint32_t tail = e1000[TDT/4];
	struct tx_desc *nxt = &tx_d[tail];
	if((nxt->status & TXD_STAT_DD) != TXD_STAT_DD)
		return -1;
	
	if(length > TBUFFSIZE)
		length = TBUFFSIZE;

	memmove(&pbuf[tail], addr, length);
	nxt->length = (uint16_t)length;
	nxt->status &= !TXD_STAT_DD;
	e1000[TDT/4] = (tail+1)%TXRING_LEN;
	cprintf("e1000: end of transmit\n");
	return 0;
}

int
e1000_receive(void *addr, size_t bufflength){
/*
	uint32_t tail = e1000[RDT/4];
	struct rx_desc *nxt = &rx_d[tail];
	if((nxt->status & RXD_STAT_DD) != RXD_STAT_DD)
		return -1;

	if(nxt->length < bufflength)
		bufflength = nxt->length;
	
	memmove(&prbuf[tail], addr, bufflength);
	nxt->status &= !TXD_STAT_DD;
	e1000[RDT/4] = (tail+1)%RXRING_LEN;
	
	return bufflength;
*/
	return -1;
}
