// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
#include <kern/pmap.h>
#include <kern/env.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Track the call list of functions", mon_backtrace},
	{ "showmappings", "Display the physical page mappings and corresponding permission bits", mon_showmappings},
	{ "updateperm", "Set, clear, or change the permissions of any mapping", mon_updateperm},
	{ "dumpmem", "Dump the contents of a range of memory given either a virtual or physical address range", mon_dumpmem},
	{ "onestep", "Single-step one instruction at a time", mon_onestep}
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t* ebp = (uint32_t*) read_ebp();
	while (ebp) {
		uint32_t eip = *(ebp+1);
		cprintf("ebp %x eip %x args ", ebp, eip);
		for (int i = 0; i < 5; i++)
			cprintf("%08x ", *(ebp+2+i));
		cprintf("\n");
		struct Eipdebuginfo debug_info;
		debuginfo_eip(eip, &debug_info);
		cprintf("%s:%d: %.*s+%d\n",debug_info.eip_file, debug_info.eip_line, debug_info.eip_fn_namelen, debug_info.eip_fn_name, eip-debug_info.eip_fn_addr);
		ebp = (uint32_t*) (*ebp);
	}
	return 0;
}

int
mon_showmappings(int argc, char **argv, struct Trapframe *tf) {
	if (argc < 3) {
        	cprintf("Usage: showmappings begin_addr end_addr\n");
        	return 0;
    	}
	// strtol: turn a string into int
	uint32_t begin = strtol(argv[1], NULL, 16);
	uint32_t end = strtol(argv[2], NULL, 16);
	if (begin >= end) {
		cprintf("end_addr must be larger than begin_addr!\n");
		return 0;
	}
	if (end >= 0xffffffff) {
		cprintf("end_addr overflow!\n");
		return 0;
	}
	if (begin != ROUNDUP(begin, PGSIZE) || end != ROUNDUP(end, PGSIZE)) {
		cprintf("addr not aligned!\n");
		return 0;
	}
	for (; begin <= end; begin+=PGSIZE) {
		pte_t* pte = pgdir_walk(kern_pgdir, (const void*) begin, 0);
		if (!pte || !(*pte & PTE_P))
			cprintf("%08x: not mapped\n",begin);
		else {
			cprintf("%08x: %08x ", begin, PTE_ADDR(*pte));
			cprintf("PTE_P: %x, PTE_W: %x, PTE_U: %x\n", *pte&PTE_P, *pte&PTE_W, *pte&PTE_U);
		}
	}
	return 0;
}

int
mon_updateperm(int argc, char **argv, struct Trapframe *tf) {
	if (argc < 4) {
        	cprintf("Usage: updateperm addr [0|1] [P|W|U]\n");
        	return 0;
    	}
	uint32_t addr = strtol(argv[1], NULL, 16);
	if (addr >= 0xffffffff) {
		cprintf("addr overflow!\n");
		return 0;
	}
	char action = argv[2][0];
	char perm = argv[3][0];
	if (addr != ROUNDUP(addr, PGSIZE)) {
		cprintf("addr not aligned!\n");
		return 0;
	}
	uint32_t perm_bit = 0;
	switch(perm) {
		case 'P':
			perm_bit = PTE_P;
			break;
		case 'W':
			perm = PTE_W;
			break;
		case 'U':
			perm = PTE_U;
			break;
		default:
			cprintf("unknown param: %c\n", perm);
			return 0;
	}
	pte_t* pte = pgdir_walk(kern_pgdir, (const void*)addr, 0);
	if (!pte || !(*pte & PTE_P)) {
		cprintf("not mapped\n");
		return 0;
	}
	if (action == '0') {
		*pte = *pte & ~perm;
		cprintf("%08x clear permission\n", addr);
	}
	else {
		*pte = *pte | perm;
		cprintf("%08x set permission %c\n", addr, perm);
	}
	return 0;	
}	

int
mon_dumpmem(int argc, char **argv, struct Trapframe *tf) {
	if (argc < 4) {
        	cprintf("Usage: dumpmem [p|v] addr n\n");
        	return 0;
    	}
	uint32_t bias = KERNBASE/4;
	char type = argv[1][0];
	void** addr  = (void**)strtol(argv[2], 0, 16);
	uint32_t n = strtol(argv[3], NULL, 0);
	if (!addr) {
		cprintf("addr error!\n");
		return 0;
	}
	if (type == 'p') {
		cprintf("physical memory\n");
		for (uint32_t i = bias; i < n+bias; i++)
			cprintf("%08x: %08x\n", addr+i, addr[i]);
	}
	else if (type == 'v') {
		cprintf("virtual memory\n");
		for (uint32_t i = 0; i < n; i++)
			cprintf("%08x: %08x\n", addr+i, addr[i]);
	}
	else
		cprintf("unknown param %c\n", type);
	return 0;
}

int
mon_onestep(int argc, char **argv, struct Trapframe *tf) {
	if (argc != 1) {
		cprintf("Usage: onestep\n");
		return 0;
	}
	if (tf == NULL) {
		cprintf("step error\n");
		return 0;
	}
	tf->tf_eflags |= FL_TF;
	cprintf("now: %08x\n", tf->tf_eip);
	env_run(curenv);
	return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
