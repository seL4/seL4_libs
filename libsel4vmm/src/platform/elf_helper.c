/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#include <elf/elf.h>
#include "vmm/platform/elf_helper.h"

/*
	Reads the elf header and elf program headers from a file
		when given a sufficiently large memory buffer
*/
int vmm_read_elf_headers(void *buf, vmm_t *vmm, FILE *file, size_t buf_size, elf_t *elf) {
	size_t result;
	if(buf_size < sizeof(Elf32_Ehdr))
		return -1;

    fseek(file, 0, SEEK_SET);
    result = fread(buf, buf_size, 1, file);
	if(result != 1)
		return -1;

    return elf_newFile_maybe_unsafe(buf, buf_size, true, false, elf);
}
