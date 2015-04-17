	/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <elf/elf.h>
#include <elf/elf32.h>
#include "vmm/platform/elf_helper.h"

/*
	Reads the elf header and elf program headers from a file
		when given a sufficiently large memory buffer
*/
int vmm_read_elf_headers(void *buf, vmm_t *vmm, int fd, size_t buf_size) {
	int res;
	if(buf_size < sizeof(struct Elf32_Header))
		return -1;

	res = vmm->plat_callbacks.read(buf, fd, 0, buf_size);
	if(res < 0)
		return -1;
	if(elf_checkFile(buf) < 0)
		return -1;

	struct Elf32_Header *chck_elf = (struct Elf32_Header *)buf;
	if(chck_elf->e_ehsize + (chck_elf->e_phnum * chck_elf->e_phentsize) > buf_size)
		return -1;

	return 0;
}
