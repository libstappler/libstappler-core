#include "z_asm.h"
#include "z_syscalls.h"
#include "z_utils.h"
#include "z_elf.h"
#include "elf_loader.h"

#define PAGE_SIZE	4096
#define ALIGN		(PAGE_SIZE - 1)
#define ROUND_PG(x)	(((x) + (ALIGN)) & ~(ALIGN))
#define TRUNC_PG(x)	((x) & ~(ALIGN))
#define PFLAGS(x)	((((x) & PF_R) ? PROT_READ : 0) | \
			 (((x) & PF_W) ? PROT_WRITE : 0) | \
			 (((x) & PF_X) ? PROT_EXEC : 0))
#define LOAD_ERR	((unsigned long)-1)

/* Original sp (i.e. pointer to executable params) passed to entry, if any. */
unsigned long *entry_sp;

#define Z_PROG		0
#define Z_INTERP	1

int main(int argc, char *argv[]);

void init_exec_elf(char *argv[]) {
	/* We assume that argv comes from the original executable params. */
	if (entry_sp == NULL) {
		entry_sp = (unsigned long *)argv - 1;
	}
}
