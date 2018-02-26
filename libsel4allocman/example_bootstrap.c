/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <autoconf.h>
#include <sel4/types.h>
#include <sel4/sel4.h>
#include <stdio.h>
#include <allocman/allocman.h>
#include <allocman/cspace/simple1level.h>
#include <allocman/mspace/fixed_pool.h>
#include <allocman/utspace/twinkle.h>
#include <allocman/cspace/two_level.h>
#include <allocman/mspace/virtual_pool.h>
#include <allocman/utspace/trickle.h>
#include <allocman/bootstrap.h>
#include <allocman/cspaceops.h>
#include <string.h>
#include <sel4utils/vspace.h>
#include <allocman/vka.h>

#define VIRTUAL_START 0xA0000000

#define MEM_POOL_SIZE (1*1024*1024)

static char initial_mem_pool[MEM_POOL_SIZE];

allocman_t *test_use_current_cspace_bootinfo() {
    int error;
    allocman_t *allocman;
    vspace_alloc_t vspace;
    vka_t *vka;
    allocman = bootstrap_use_bootinfo(seL4_GetBootInfo(), sizeof(initial_mem_pool), initial_mem_pool);
    assert(allocman);
    vka = allocman_mspace_alloc(allocman, sizeof(*vka), &error);
    assert(!error);
    allocman_make_vka(vka, allocman);
    sel4util_get_vspace_alloc_leaky(&vspace, seL4_CapInitThreadPD, vka, seL4_GetBootInfo());

    reservation_t *reservation = vspace_reserve_range_at(&vspace, VIRTUAL_START, MEM_POOL_SIZE, seL4_AllRights, 1);
    assert(reservation);

    bootstrap_configure_virtual_pool(allocman, VIRTUAL_START, MEM_POOL_SIZE, seL4_CapInitThreadPD);
    error = allocman_fill_reserves(allocman);
    assert(!error);
    return allocman;
}

void test_use_current_1level_cspace() {
    int error;
    seL4_CPtr i;
    allocman_t *allocman;
    seL4_BootInfo *bi = seL4_GetBootInfo();
    allocman = bootstrap_use_current_1level(seL4_CapInitThreadCNode, bi->initThreadCNodeSizeBits, bi->empty.start, bi->empty.end, sizeof(initial_mem_pool), initial_mem_pool);
    assert(allocman);
    /* have to add all our resources manually */
    for (i = bi->untyped.start; i < bi->untyped.end; i++) {
        cspacepath_t slot = allocman_cspace_make_path(allocman, i);
        uint32_t size_bits = bi->untypedSizeBitsList[i - bi->untyped.start];
        uint32_t paddr = bi->untypedPaddrList[i - bi->untyped.start];
        error = allocman_utspace_add_uts(allocman, 1, &slot, &size_bits, &paddr);
        assert(!error);
    }
    error = allocman_fill_reserves(allocman);
    assert(!error);
}

void test_use_current_cspace() {
    int error;
    seL4_CPtr i;
    allocman_t *allocman;
    cspace_single_level_t *cspace;
    utspace_trickle_t *utspace;
    seL4_BootInfo *bi = seL4_GetBootInfo();
    allocman = bootstrap_create_allocman(sizeof(initial_mem_pool), initial_mem_pool);
    assert(allocman);
    /* construct a description of our current cspace */
    cspace = allocman_mspace_alloc(allocman, sizeof(*cspace), &error);
    assert(!error);
    error = cspace_single_level_create(allocman, cspace, (struct cspace_single_level_config) {
        .cnode = seL4_CapInitThreadCNode,
        .cnode_size_bits = bi->initThreadCNodeSizeBits,
        .cnode_guard_bits = seL4_WordBits - bi->initThreadCNodeSizeBits,
        .first_slot = bi->empty.start,
        .end_slot = bi->empty.end
    });
    assert(!error);

    error = allocman_attach_cspace(allocman, cspace_single_level_make_interface(cspace));
    assert(!error);

    utspace = allocman_mspace_alloc(allocman, sizeof(*utspace), &error);
    assert(!error);
    error = allocman_attach_utspace(allocman, utspace_trickle_make_interface(utspace));
    assert(!error);

    /* have to add all our resources manually */
    for (i = bi->untyped.start; i < bi->untyped.end; i++) {
        cspacepath_t slot = allocman_cspace_make_path(allocman, i);
        uint32_t size_bits = bi->untypedSizeBitsList[i - bi->untyped.start];
        uint32_t paddr = bi->untypedPaddrList[i - bi->untyped.start];
        error = allocman_utspace_add_uts(allocman, 1, &slot, &size_bits);
        assert(!error);
    }
    error = allocman_fill_reserves(allocman);
    assert(!error);
}

void test_new_1level_cspace_bootinfo() {
    int error;
    allocman_t *allocman;
    cspace_simple1level_t *boot_cspace;
    cspacepath_t new_path;
    allocman = bootstrap_new_1level_bootinfo(seL4_GetBootInfo(), 14, sizeof(initial_mem_pool), initial_mem_pool, &boot_cspace);
    assert(allocman);
    error = allocman_fill_reserves(allocman);
    assert(!error);
    /* test we can easily use our old cspace */
    error = cspace_move_alloc_cptr(allocman, cspace_simple1level_make_interface(boot_cspace), seL4_CapInitThreadTCB, &new_path);
    assert(!error);
    /* drop our priority */
    error = seL4_TCB_SetPriority(new_path.capPtr, new_path.capPtr, 100);
    assert(error == seL4_NoError);
    /* no longer need that boot cspace information */
    allocman_mspace_free(allocman, boot_cspace, sizeof(*boot_cspace));
}

void test_new_1level_cspace() {
    bootstrap_info_t *bootstrap;
    seL4_BootInfo *bi = seL4_GetBootInfo();
    cspace_simple1level_t boot_cspace;
    allocman_t *allocman;
    cspacepath_t oldroot;
    cspacepath_t currentroot;
    cspacepath_t new_path;
    cspacepath_t tcb;
    cspacepath_t pd;
    seL4_CPtr i;
    int error;
    bootstrap = bootstrap_create_info(sizeof(initial_mem_pool), initial_mem_pool);
    assert(bootstrap);
    /* describe the current cspace */
    cspace_simple1level_create(&boot_cspace, (struct cspace_simple1level_config){
        .cnode = seL4_CapInitThreadCNode,
        .cnode_size_bits = bi->initThreadCNodeSizeBits,
        .cnode_guard_bits = seL4_WordBits - bi->initThreadCNodeSizeBits,
        .first_slot = bi->empty.start,
        .end_slot = bi->empty.end - 1
    });
    /* pass to the boot strapper */
    currentroot = _cspace_simple1level_make_path(&boot_cspace, seL4_CapInitThreadCNode);
    bootstrap_set_boot_cspace(bootstrap, cspace_simple1level_make_interface(&boot_cspace), currentroot);
    /* give the untypeds to the bootstrapper */
    for (i = bi->untyped.start; i < bi->untyped.end; i++) {
        cspacepath_t slot = _cspace_simple1level_make_path(&boot_cspace, i);
        uint32_t size_bits = bi->untypedSizeBitsList[i - bi->untyped.start];
        uint32_t paddr = bi->untypedPaddrList[i - bi->untyped.start];
        error = bootstrap_add_untypeds(bootstrap, 1, &slot, &size_bits, &paddr);
        assert(!error);
    }
    /* bootstrapper has enough information to create */
    tcb = _cspace_simple1level_make_path(&boot_cspace, seL4_CapInitThreadTCB);
    pd = _cspace_simple1level_make_path(&boot_cspace, seL4_CapInitThreadPD);
    allocman = bootstrap_new_1level(bootstrap, 14, tcb, pd, &oldroot);
    assert(allocman);
    /* take advantage of knowledge of simple1level cspace and just shift its root :) */
    assert(oldroot.capDepth == 32);
    boot_cspace.config.cnode = oldroot.capPtr;
    /* now test we can use it */
    error = cspace_move_alloc_cptr(allocman, cspace_simple1level_make_interface(&boot_cspace), seL4_CapInitThreadTCB, &new_path);
    assert(!error);
    /* drop our priority */
    error = seL4_TCB_SetPriority(new_path.capPtr, new_path.capPtr, 100);
    assert(error == seL4_NoError);
}

void test_new_2level_cspace_bootinfo() {
    int error;
    allocman_t *allocman;
    cspace_simple1level_t *boot_cspace;
    cspacepath_t new_path;
    allocman = bootstrap_new_2level_bootinfo(seL4_GetBootInfo(), 6,10, sizeof(initial_mem_pool), initial_mem_pool, &boot_cspace);
    assert(allocman);
    error = allocman_fill_reserves(allocman);
    assert(!error);
    /* test we can easily use our old cspace */
    error = cspace_move_alloc_cptr(allocman, cspace_simple1level_make_interface(boot_cspace), seL4_CapInitThreadTCB, &new_path);
    assert(!error);
    /* drop our priority */
    error = seL4_TCB_SetPriority(new_path.capPtr, new_path.capPtr, 100);
    assert(error == seL4_NoError);
    /* no longer need that boot cspace information */
    allocman_mspace_free(allocman, boot_cspace, sizeof(*boot_cspace));
}

void test_new_2level_cspace() {
    bootstrap_info_t *bootstrap;
    seL4_BootInfo *bi = seL4_GetBootInfo();
    cspace_simple1level_t boot_cspace;
    allocman_t *allocman;
    cspacepath_t oldroot;
    cspacepath_t currentroot;
    cspacepath_t new_path;
    cspacepath_t tcb;
    cspacepath_t pd;
    seL4_CPtr i;
    int error;
    bootstrap = bootstrap_create_info(sizeof(initial_mem_pool), initial_mem_pool);
    assert(bootstrap);
    /* describe the current cspace */
    cspace_simple1level_create(&boot_cspace, (struct cspace_simple1level_config){
        .cnode = seL4_CapInitThreadCNode,
        .cnode_size_bits = bi->initThreadCNodeSizeBits,
        .cnode_guard_bits = seL4_WordBits - bi->initThreadCNodeSizeBits,
        .first_slot = bi->empty.start,
        .end_slot = bi->empty.end - 1
    });
    /* pass to the boot strapper */
    currentroot = _cspace_simple1level_make_path(&boot_cspace, seL4_CapInitThreadCNode);
    bootstrap_set_boot_cspace(bootstrap, cspace_simple1level_make_interface(&boot_cspace), currentroot);
    /* give the untypeds to the bootstrapper */
    for (i = bi->untyped.start; i < bi->untyped.end; i++) {
        cspacepath_t slot = _cspace_simple1level_make_path(&boot_cspace, i);
        uint32_t size_bits = bi->untypedSizeBitsList[i - bi->untyped.start];
        uint32_t paddr = bi->untypedPaddrList[i - bi->untyped.start];
        error = bootstrap_add_untypeds(bootstrap, 1, &slot, &size_bits, &paddr);
        assert(!error);
    }
    /* bootstrapper has enough information to create */
    tcb = _cspace_simple1level_make_path(&boot_cspace, seL4_CapInitThreadTCB);
    pd = _cspace_simple1level_make_path(&boot_cspace, seL4_CapInitThreadPD);
    allocman = bootstrap_new_2level(bootstrap, 6, 10, tcb, pd, &oldroot);
    assert(allocman);
    /* take advantage of knowledge of simple1level cspace and just shift its root :) */
    assert(oldroot.capDepth == 32);
    boot_cspace.config.cnode = oldroot.capPtr;
    /* now test we can use it */
    error = cspace_move_alloc_cptr(allocman, cspace_simple1level_make_interface(&boot_cspace), seL4_CapInitThreadTCB, &new_path);
    assert(!error);
    /* drop our priority */
    error = seL4_TCB_SetPriority(new_path.capPtr, new_path.capPtr, 100);
    assert(error == seL4_NoError);
}

int main(void) {
    allocman_t *alloc;
    alloc = test_use_current_cspace_bootinfo();
//    test_use_current_1level_cspace();
//    test_use_current_cspace();
//    test_new_1level_cspace_bootinfo();
//    test_new_1level_cspace();
//    test_new_2level_cspace_bootinfo();
//    test_new_2level_cspace();
    printf("All is well in the universe.\n");
    while(1);
}
