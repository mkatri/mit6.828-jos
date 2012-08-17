// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at vpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if((err & FEC_WR) != FEC_WR)
		panic("pgfault\n");

	uintptr_t fault_va = ROUNDDOWN((uintptr_t) addr, PGSIZE);
	pte_t pte = vpt[PGNUM(fault_va)];
	if(!(pte & PTE_P) || !(pte & PTE_COW))
		panic("pgfault\n");
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.
	
	// LAB 4: Your code here.
	struct Page *p;
	if((r = sys_page_alloc(0, (void *) PFTEMP, PTE_P|PTE_U|PTE_W)) < 0)
		panic("pgfault handler error: %e\n", r);
	memmove((void *) PFTEMP, (void *) fault_va, PGSIZE);
	if((r = sys_page_map(0, (void *) PFTEMP, 
		0,  (void *) fault_va, PTE_P|PTE_U|PTE_W)) < 0)
		panic("pgfault handler error: %e\n", r);
	sys_page_unmap(0, (void *) PFTEMP);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	pte_t pte = vpt[pn];
	uint32_t perm = pte & PTE_SYSCALL;
	if((perm & PTE_W) || (perm & PTE_COW)){
		perm &= ~PTE_W;		
		perm |= PTE_COW;
	}

	if((r = sys_page_map(0, (void *) (pn*PGSIZE), 
		envid, (void *) (pn*PGSIZE), perm)) < 0)
		return r;
	
	if(perm & PTE_COW)
		if((r = sys_page_map(0, (void *) (pn*PGSIZE), 
			0, (void *) (pn*PGSIZE), perm)) < 0)
			return r;
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use vpd, vpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	envid_t child = sys_exofork();
	if(child < 0)
		return child;

	set_pgfault_handler(&pgfault); //before copying, set pgfault handler for both
	
	if(child == 0){
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	uintptr_t addr = 0;
	int r;
	for(; addr < UXSTACKTOP-PGSIZE; addr += PGSIZE){
		if((vpd[PDX(addr)] & PTE_P) && (vpt[PGNUM(addr)] & PTE_P))
			if((r = duppage(child, PGNUM(addr))) < 0)
				return 0;		
	}

	if((r = sys_page_alloc(child, 
		(void *) UXSTACKTOP-PGSIZE, PTE_P|PTE_U|PTE_W)) < 0)
		return r;

	if((r = sys_env_set_status(child, ENV_RUNNABLE)) < 0)
		return r;

	return child;	
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
