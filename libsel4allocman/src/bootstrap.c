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
#include <sel4/sel4.h>
#include <string.h>
#include <allocman/allocman.h>
#include <allocman/cspace/simple1level.h>
#include <allocman/cspace/two_level.h>
#include <allocman/mspace/dual_pool.h>
#include <allocman/utspace/split.h>
#include <allocman/bootstrap.h>
#include <allocman/arch/reservation.h>
#include <stdio.h>
#include <vka/capops.h>
#include <vka/object.h>
#include <sel4utils/api.h>
#include <sel4utils/mapping.h>
#include <sel4utils/process.h>
#include <simple/simple.h>
#include <simple/simple_helpers.h>
#include <utils/arith.h>
#include <sel4platsupport/pmem.h>

/* configure the choice of boot strapping allocators */
#define UTMAN split

/*do some nasty macro expansion to get the string substitution to happen how we want */
#define CON2(a, b,c) a##b##c
#define CON(a, b,c) CON2(a,b,c)
#define UTMAN_TYPE CON(utspace_, UTMAN,_t)
#define UTMAN_CREATE CON(utspace_, UTMAN,_create)
#define UTMAN_MAKE_INTERFACE CON(utspace_, UTMAN,_make_interface)
#define UTMAN_ADD_UTS CON(_utspace_, UTMAN,_add_uts)

struct bootstrap_info {
    allocman_t *alloc;
    int have_boot_cspace;
    /* a path to the current boot cnode */
    cspacepath_t boot_cnode;
    /* this is prefixed with 'maybe' since we are just using it as some preallocated memory
     * if a temp bootstrapping cspace is needed. it may not actually be used if a more
     * complicated cspace is passed in directly by the user */
    cspace_simple1level_t maybe_boot_cspace;
    cspace_interface_t boot_cspace;
    /* if we have switched cspaces, a patch to our old root cnode (based in the current cnode) */
    cspacepath_t old_cnode;
    /* a path to the page directory cap, we need this to be able to change the cspace root */
    cspacepath_t pd;
    cspacepath_t tcb;
    int uts_in_current_cspace;
    size_t num_uts;
    cspacepath_t *uts;
    size_t *ut_size_bits;
    uintptr_t *ut_paddr;
    bool *ut_isDevice;
    simple_t *simple;
};

typedef struct add_untypeds_state {
    int num_regions;
    bool region_alloc;
    pmem_region_t *regions;
    utspace_split_t split_ut;
    utspace_interface_t ut_interface;
} add_untypeds_state_t;

static void bootstrap_free_info(bootstrap_info_t *bs) {
    if (bs->uts) {
        allocman_mspace_free(bs->alloc, bs->uts, sizeof(cspacepath_t) * bs->num_uts);
        allocman_mspace_free(bs->alloc, bs->ut_size_bits, sizeof(size_t) * bs->num_uts);
    }
    allocman_mspace_free(bs->alloc, bs, sizeof(bootstrap_info_t));
}

allocman_t *bootstrap_create_allocman(size_t pool_size, void *pool) {
    uintptr_t cur_pool = (uintptr_t)pool;
    uintptr_t pool_top = cur_pool + pool_size;
    mspace_dual_pool_t *mspace;
    allocman_t *alloc;
    int error;
    /* first align the pool */
    cur_pool = ALIGN_UP(cur_pool, sizeof(size_t));
    /* carve off some of the pool for the allocman and memory allocator itself */
    alloc = (allocman_t*)cur_pool;
    cur_pool += sizeof(*alloc);
    mspace = (mspace_dual_pool_t*)cur_pool;
    cur_pool += sizeof(*mspace);
    if (cur_pool >= pool_top) {
        LOG_ERROR("Initial memory pool too small");
        return NULL;
    }
    /* create the allocator */
    mspace_dual_pool_create(mspace, (struct mspace_fixed_pool_config){(void*)cur_pool, pool_top - cur_pool});
    error = allocman_create(alloc, mspace_dual_pool_make_interface(mspace));
    if (error) {
        return NULL;
    }
    return alloc;
}

int bootstrap_set_boot_cspace(bootstrap_info_t *bs, cspace_interface_t cspace, cspacepath_t root_cnode) {
    bs->boot_cspace = cspace;
    bs->boot_cnode = root_cnode;
    bs->have_boot_cspace = 1;
    return 0;
}

static int bootstrap_create_temp_bootinfo_cspace_at(bootstrap_info_t *bs, seL4_BootInfo *bi, seL4_CPtr root_cnode) {
    /* create a cspace to represent the bootinfo cspace. */
    cspace_simple1level_create(&bs->maybe_boot_cspace, (struct cspace_simple1level_config){
        .cnode = root_cnode,
        .cnode_size_bits = bi->initThreadCNodeSizeBits,
        .cnode_guard_bits = seL4_WordBits - bi->initThreadCNodeSizeBits,
        .first_slot = bi->empty.start,
        .end_slot = bi->empty.end - 1
    });
    return bootstrap_set_boot_cspace(bs,
                                     cspace_simple1level_make_interface(
                                        &bs->maybe_boot_cspace),
                                        _cspace_simple1level_make_path(&bs->maybe_boot_cspace, root_cnode));
}

static int bootstrap_create_temp_bootinfo_cspace(bootstrap_info_t *bs, seL4_BootInfo *bi) {
    return bootstrap_create_temp_bootinfo_cspace_at(bs, bi, seL4_CapInitThreadCNode);
}

static bootstrap_info_t *_create_info(allocman_t *alloc) {
    int error;
    bootstrap_info_t *bs = allocman_mspace_alloc(alloc, sizeof(bootstrap_info_t), &error);
    if (error) {
        LOG_ERROR("Failed to allocate bootstrap_info");
        return NULL;
    }
    memset(bs, 0, sizeof(bootstrap_info_t));
    bs->alloc = alloc;
    /* currently have no untypeds so this is true */
    bs->uts_in_current_cspace = 1;
    return bs;
}

static int _add_ut(bootstrap_info_t *bs, cspacepath_t slot, size_t size_bits, uintptr_t paddr, bool isDevice) {
    cspacepath_t *new_uts;
    size_t *new_size_bits;
    uintptr_t *new_paddr;
    bool *new_isDevice;
    int error;
    new_uts = allocman_mspace_alloc(bs->alloc, sizeof(cspacepath_t) * (bs->num_uts + 1), &error);
    if (error) {
        LOG_ERROR("Failed to allocate space for untypeds (new_uts)");
        return error;
    }
    new_size_bits = allocman_mspace_alloc(bs->alloc, sizeof(size_t) * (bs->num_uts + 1), &error);
    if (error) {
        LOG_ERROR("Failed to allocate space for untypeds (new_size_bits)");
        return error;
    }
    new_paddr = allocman_mspace_alloc(bs->alloc, sizeof(uintptr_t) * (bs->num_uts + 1), &error);
    if (error) {
        LOG_ERROR("Failed to allocate space for untypeds (new_paddr)");
        return error;
    }
    new_isDevice = allocman_mspace_alloc(bs->alloc, sizeof(bool) * (bs->num_uts + 1), &error);
    if (error) {
        ZF_LOGE("Failed to allocate space for untypeds (new_isDevice)");
        return error;
    }
    if (bs->uts) {
        memcpy(new_uts, bs->uts, sizeof(cspacepath_t) * bs->num_uts);
        memcpy(new_size_bits, bs->ut_size_bits, sizeof(size_t) * bs->num_uts);
        memcpy(new_paddr, bs->ut_paddr, sizeof(uintptr_t) * bs->num_uts);
        memcpy(new_isDevice, bs->ut_isDevice, sizeof(bool) * bs->num_uts);
        allocman_mspace_free(bs->alloc, bs->uts, sizeof(cspacepath_t) * bs->num_uts);
        allocman_mspace_free(bs->alloc, bs->ut_size_bits, sizeof(size_t) * bs->num_uts);
        allocman_mspace_free(bs->alloc, bs->ut_paddr, sizeof(uintptr_t) * bs->num_uts);
        allocman_mspace_free(bs->alloc, bs->ut_isDevice, sizeof(bool) * bs->num_uts);
    }
    new_uts[bs->num_uts] = slot;
    new_size_bits[bs->num_uts] = size_bits;
    new_paddr[bs->num_uts] = paddr;
    new_isDevice[bs->num_uts] = isDevice;
    bs->uts = new_uts;
    bs->ut_size_bits = new_size_bits;
    bs->ut_paddr = new_paddr;
    bs->ut_isDevice = new_isDevice;
    bs->num_uts++;
    return 0;
}

int bootstrap_add_untypeds(bootstrap_info_t *bs, size_t num, const cspacepath_t *uts, size_t *size_bits, uintptr_t *paddr, bool isDevice) {
    size_t i;
    int error;
    for (i = 0; i < num; i++) {
        error = _add_ut(bs, uts[i], size_bits[i], paddr ? paddr[i] : ALLOCMAN_NO_PADDR, isDevice);
        if (error) {
            return error;
        }
    }
    return 0;
}

int bootstrap_add_untypeds_from_bootinfo(bootstrap_info_t *bs, seL4_BootInfo *bi) {
    int error;
    seL4_CPtr i;
    /* if we do not have a boot cspace, or we have added some uts that aren't in the
     * current space then just bail */
    if (!bs->have_boot_cspace || (bs->uts && !bs->uts_in_current_cspace)) {
        return 1;
    }
    for (i = bi->untyped.start; i < bi->untyped.end; i++) {
        size_t index = i - bi->untyped.start;
        cspacepath_t slot = bs->boot_cspace.make_path(bs->boot_cspace.cspace, i);
        size_t size_bits = bi->untypedList[index].sizeBits;
        uintptr_t paddr = bi->untypedList[index].paddr;
        error = _add_ut(bs, slot, size_bits, paddr, bi->untypedList[index].isDevice);
        if (error) {
            return error;
        }
    }
    /* we assume this is true here */
    bs->uts_in_current_cspace = 1;
    return 0;
}

static int bootstrap_add_untypeds_from_simple(bootstrap_info_t *bs, simple_t *simple) {
    int error;
    int i;
    /* if we do not have a boot cspace, or we have added some uts that aren't in the
     * current space then just bail */
    if (!bs->have_boot_cspace || (bs->uts && !bs->uts_in_current_cspace)) {
        return 1;
    }
    for (i = 0; i < simple_get_untyped_count(simple); i++) {
        size_t size_bits;
        uintptr_t paddr;
        bool device;
        cspacepath_t slot = bs->boot_cspace.make_path(bs->boot_cspace.cspace,
                                                      simple_get_nth_untyped(simple, i, &size_bits, &paddr, &device));
        error = _add_ut(bs, slot, size_bits, paddr, device);
        if (error) {
            return error;
        }
    }
    /* we assume this is true here */
    bs->uts_in_current_cspace = 1;
    return 0;
}

static int _remove_ut(bootstrap_info_t *bs, size_t i) {
    cspacepath_t *new_uts = NULL;
    size_t *new_size_bits = NULL;
    uintptr_t *new_paddr = NULL;
    bool *new_isDevice = NULL;
    int error;
    if (bs->num_uts == 0) {
        /* what? */
        return 1;
    }
    /* if there is only 1 then we will just skip straight to freeing the memory and setting
     * the arrays to NULL */
    if (bs->num_uts > 1) {
        new_uts = allocman_mspace_alloc(bs->alloc, sizeof(cspacepath_t) * (bs->num_uts - 1), &error);
        if (error) {
            return error;
        }
        new_size_bits = allocman_mspace_alloc(bs->alloc, sizeof(size_t) * (bs->num_uts - 1), & error);
        if (error) {
            return error;
        }
        new_paddr = allocman_mspace_alloc(bs->alloc, sizeof(uintptr_t) * (bs->num_uts - 1), & error);
        if (error) {
            return error;
        }
        new_isDevice = allocman_mspace_alloc(bs->alloc, sizeof(bool) * (bs->num_uts - 1), &error);
        if (error) {
            return error;
        }
        memcpy(&new_uts[0], &bs->uts[0], i * sizeof(cspacepath_t));
        memcpy(&new_uts[i], &bs->uts[i + 1], (bs->num_uts - i - 1) * sizeof(cspacepath_t));
        memcpy(&new_size_bits[0], &bs->ut_size_bits[0], i * sizeof(size_t));
        memcpy(&new_size_bits[i], &bs->ut_size_bits[i + 1], (bs->num_uts - i - 1) * sizeof(size_t));
        memcpy(&new_paddr[0], &bs->ut_paddr[0], i * sizeof(uintptr_t));
        memcpy(&new_paddr[i], &bs->ut_paddr[i + 1], (bs->num_uts - i - 1) * sizeof(uintptr_t));
        memcpy(&new_isDevice[0], &bs->ut_isDevice[0], i * sizeof(bool));
        memcpy(&new_isDevice[i], &bs->ut_isDevice[i + 1], (bs->num_uts - i - 1) * sizeof(bool));
    }
    allocman_mspace_free(bs->alloc, bs->uts, sizeof(cspacepath_t) * (bs->num_uts));
    allocman_mspace_free(bs->alloc, bs->ut_size_bits, sizeof(size_t) * (bs->num_uts));
    allocman_mspace_free(bs->alloc, bs->ut_paddr, sizeof(uintptr_t) * (bs->num_uts));
    allocman_mspace_free(bs->alloc, bs->ut_isDevice, sizeof(bool) * (bs->num_uts));
    bs->uts = new_uts;
    bs->ut_size_bits = new_size_bits;
    bs->ut_paddr = new_paddr;
    bs->ut_isDevice = new_isDevice;
    bs->num_uts--;
    return 0;
}

static int _split_ut(bootstrap_info_t *bs, cspacepath_t ut, cspacepath_t p1, cspacepath_t p2, size_t size) {
    int error;
    error = seL4_Untyped_Retype(ut.capPtr, seL4_UntypedObject, size, p1.root, p1.dest, p1.destDepth, p1.offset, 1);
    if (error != seL4_NoError) {
        return 1;
    }
    error = seL4_Untyped_Retype(ut.capPtr, seL4_UntypedObject, size, p2.root, p2.dest, p2.destDepth, p2.offset, 1);
    if (error != seL4_NoError) {
        return 1;
    }
    return 0;
}

static int _retype_cnode(bootstrap_info_t *bs, cspacepath_t ut, cspacepath_t cnode, seL4_Word sel4_size) {
    int error;
    error = seL4_Untyped_Retype(ut.capPtr, seL4_CapTableObject, sel4_size,
                                cnode.root, cnode.dest, cnode.destDepth, cnode.offset, 1);
    if (error != seL4_NoError) {
        return 1;
    }
    return 0;
}

static int bootstrap_allocate_cnode(bootstrap_info_t *bs, size_t size, cspacepath_t *slot) {
    size_t ut_size;
    int i;
    int best = -1;
    uintptr_t best_paddr;
    size_t best_size;
    bool best_isDevice;
    int error;
    cspacepath_t best_path;
    if (!bs->have_boot_cspace) {
        return 1;
    }
    ut_size = size + seL4_SlotBits;
    /* find the smallest untyped to allocate from */
    for (i = 0; i < bs->num_uts; i++) {
        if (bs->ut_size_bits[i] >= ut_size && ( best == -1 || (bs->ut_size_bits[best] > bs->ut_size_bits[i]) ) && !bs->ut_isDevice[i]) {
            best = i;
        }
    }
    if (best == -1) {
        return 1;
    }
    best_size = bs->ut_size_bits[best];
    best_path = bs->uts[best];
    best_paddr = bs->ut_paddr[best];
    best_isDevice = bs->ut_isDevice[best];
    /* we searched for a non device one, but make sure here */
    assert(!best_isDevice);
    error = _remove_ut(bs, best);
    if (error) {
        return error;
    }
    while(best_size > ut_size) {
        /* split the untyped in half */
        cspacepath_t slot1, slot2;
        size_t temp_size;
        error = bs->boot_cspace.alloc(bs->alloc, bs->boot_cspace.cspace, &slot1);
        if (error) {
            return error;
        }
        error = bs->boot_cspace.alloc(bs->alloc, bs->boot_cspace.cspace, &slot2);
        if (error) {
            return error;
        }
        error = _split_ut(bs, best_path, slot1, slot2, best_size - 1);
        if (error) {
            return error;
        }
        /* put one half back in the uts list, and then we keep going with the other one. for the purposes
         * of book keeping physical addresses it is important to note that we are putting the
         * FIRST half back in */
        temp_size = best_size - 1;
        error = bootstrap_add_untypeds(bs, 1, &slot1, &temp_size, &best_paddr, best_isDevice);
        if (error) {
            return error;
        }
        best_size--;
        best_paddr = best_paddr == ALLOCMAN_NO_PADDR ? best_paddr : best_paddr + BIT(best_size);
        best_path = slot2;
    }
    error = bs->boot_cspace.alloc(bs->alloc, bs->boot_cspace.cspace, slot);
    if (error) {
        return error;
    }
    error = _retype_cnode(bs, best_path, *slot, size);
    return error;
}

static int bootstrap_use_current_cspace(bootstrap_info_t *bs, cspace_interface_t cspace) {
    int error;
    error = allocman_attach_cspace(bs->alloc, cspace);
    if (error) {
        return error;
    }
    bs->boot_cspace = cspace;
    bs->have_boot_cspace = 1;
    return 0;
}

static int bootstrap_use_current_1level_cspace(bootstrap_info_t *bs, seL4_CPtr cnode, size_t size_bits, size_t start_free_index, size_t end_free_index) {
    cspace_single_level_t *cspace;
    int error;
    cspace = allocman_mspace_alloc(bs->alloc, sizeof(*cspace), &error);
    if (error) {
        LOG_ERROR("Initial memory pool too small to allocate cspace allocator");
        return error;
    }
    error = cspace_single_level_create(bs->alloc, cspace, (struct cspace_single_level_config){
        .cnode = cnode,
        .cnode_size_bits = size_bits,
        .cnode_guard_bits = seL4_WordBits - size_bits,
        .first_slot = start_free_index,
        .end_slot = end_free_index
    });
    if (error) {
        LOG_ERROR("Failed to initialize cspace allocator");
        return error;
    }
    error = bootstrap_use_current_cspace(bs, cspace_single_level_make_interface(cspace));
    if (error) {
        LOG_ERROR("Failed to bootstrap current cspace");
    }
    return error;
}

static int bootstrap_new_1level_cspace(bootstrap_info_t *bs, int size) {
    cspacepath_t node;
    cspace_single_level_t *cspace;
    int error;
    /* create the actual cnodes */
    error = bootstrap_allocate_cnode(bs, size, &node);
    if (error) {
        return error;
    }
    /* put a cap back to ourselves */
    seL4_CPtr new_cnode = SEL4UTILS_CNODE_SLOT;
    error = seL4_CNode_Mint(
        node.capPtr, new_cnode, size,
        node.root, node.capPtr, node.capDepth,
        seL4_AllRights, api_make_guard_skip_word(seL4_WordBits - size));
    if (error != seL4_NoError) {
        return 1;
    }
    /* put our old cnode into the final slot in our cnode */
    seL4_CPtr old_cnode_slot = BIT(size) - 1u;
    error = seL4_CNode_Copy(
        node.capPtr, old_cnode_slot, size,
        bs->boot_cnode.root, bs->boot_cnode.capPtr, bs->boot_cnode.capDepth,
        seL4_AllRights);
    if (error != seL4_NoError) {
        return 1;
    }
    /* now we can call set space */
    error = api_tcb_set_space(bs->tcb.capPtr, 0,
                node.capPtr,
                api_make_guard_skip_word(seL4_WordBits - size),
                bs->pd.capPtr, seL4_NilData);
    if (error != seL4_NoError) {
        return 1;
    }
    /* create the cspace now */
    cspace = allocman_mspace_alloc(bs->alloc, sizeof(*cspace), &error);
    if (error) {
        return error;
    }
    error = cspace_single_level_create(bs->alloc, cspace, (struct cspace_single_level_config){
        .cnode = new_cnode,
        .cnode_size_bits = size,
        .cnode_guard_bits = seL4_WordBits - size,
        .first_slot = simple_last_valid_cap(bs->simple) + 1,
        .end_slot = BIT(size) - 2u}); // subtract 2 as the last slot is occupied
    if (error) {
        return error;
    }
    error = allocman_attach_cspace(bs->alloc, cspace_single_level_make_interface(cspace));
    if (error) {
        return error;
    }
    /* construct path to our old cnode */
    bs->old_cnode = allocman_cspace_make_path(bs->alloc, old_cnode_slot);
    /* update where our current booting cnode is */
    bs->boot_cnode = allocman_cspace_make_path(bs->alloc, new_cnode);
    /* any untypeds are no longer valid */
    bs->uts_in_current_cspace = 0;
    return 0;
}
int bootstrap_transfer_caps_simple(bootstrap_info_t *bs, simple_t *simple, int levels) {

    int error;
    size_t i;
    size_t cap_count = simple_get_cap_count(simple);
    seL4_CPtr cnode = simple_get_cnode(simple);

    for(i = 0ul; i < cap_count; i++) {
        seL4_CPtr pos = simple_get_nth_cap(simple,i);

        /* Because we are going to switch root cnodes don't move the old cnode cap
         * The cap to the real root cnode is already there.
         * Also don't move any untyped caps, the untyped allocator is looking
         * after those now.
         */
        if (pos == cnode || simple_is_untyped_cap(simple, pos)) continue;

        cspacepath_t slot;
        if (levels == 1) {
            slot = _cspace_single_level_make_path(bs->alloc->cspace.cspace, pos);
        } else if (levels == 2) {
            slot = _cspace_two_level_make_path(bs->alloc->cspace.cspace, pos);
        } else {
            ZF_LOGE("Too many cspace levels!\n");
            return 1;
        }

        if(pos == bs->tcb.capPtr || pos == bs->pd.capPtr) {
            error = seL4_CNode_Copy(slot.root, pos, slot.capDepth,
                    bs->old_cnode.capPtr, pos, bs->old_cnode.capDepth,
                    seL4_AllRights);
        } else {
            error = seL4_CNode_Move(
                    slot.root, pos, slot.capDepth,
                    bs->old_cnode.capPtr, pos, bs->old_cnode.capDepth);
        }
        if (error != seL4_NoError) {
            return 1;
        }
    }
    return 0;
}

static int bootstrap_new_2level_cspace(bootstrap_info_t *bs, size_t l1size, size_t l2size, seL4_CPtr cnode, seL4_CPtr old_cnode, size_t total_caps) {
    cspacepath_t l1node;
    cspacepath_t l2node;
    cspace_two_level_t *cspace;
    int error;
    size_t i;
    seL4_CPtr l2nodeforbackpointer = 0;
    seL4_CPtr last_cap = MAX(cnode, old_cnode);
    /* create the actual cnodes */
    error = bootstrap_allocate_cnode(bs, l1size, &l1node);
    if (error) {
        return error;
    }
    error = bootstrap_allocate_cnode(bs, l2size, &l2node);
    if (error) {
        return error;
    }
    /* put a cap back to the level 1 in the level 2. That way we have a cap at depth 32 to our cspace */
    error = seL4_CNode_Mint(
        l2node.capPtr, cnode, l2size,
        l1node.root, l1node.capPtr, l1node.capDepth,
        seL4_AllRights, api_make_guard_skip_word(seL4_WordBits - l1size - l2size));
    if (error != seL4_NoError) {
        return 1;
    }

    for(i = 0; i < (total_caps / BIT(l2size)) + 1; i++) {
        if(i != 0) {
            error = bootstrap_allocate_cnode(bs, l2size, &l2node);
            if(error) {
                return error;
            }
        }
        /* see if this is the l2 node we will need for installing
         * the pointer back to our old cnode */
        if (old_cnode / BIT(l2size) == i) {
            l2nodeforbackpointer = l2node.capPtr;
        }
        /* put the level 2 slot into the level 1 */
        error = seL4_CNode_Copy(
            l1node.capPtr, i, l1size,
            l2node.root, l2node.capPtr, l2node.capDepth,
            seL4_AllRights);
        if (error != seL4_NoError) {
            return 1;
        }

    }
    size_t end_existing_index = i;
    /* put our old cnode into a slot in the level 2 one */
    error = seL4_CNode_Copy(
        l2nodeforbackpointer, old_cnode & MASK(l2size), l2size,
        bs->boot_cnode.root, bs->boot_cnode.capPtr,
        bs->boot_cnode.capDepth, seL4_AllRights);
    if (error != seL4_NoError) {
        return 1;
    }
    /* now we can call set space */
    error = api_tcb_set_space(bs->tcb.capPtr, 0,
                l1node.capPtr,
                api_make_guard_skip_word(seL4_WordBits - l1size - l2size),
                bs->pd.capPtr, seL4_NilData);
    if (error != seL4_NoError) {
        return 1;
    }
    /* create the cspace now */
    cspace = allocman_mspace_alloc(bs->alloc, sizeof(*cspace), &error);
    if (error) {
        return error;
    }
    error = cspace_two_level_create(bs->alloc, cspace, (struct cspace_two_level_config){
        .cnode = cnode,
        .cnode_size_bits = l1size,
        .cnode_guard_bits = seL4_WordBits - l1size - l2size,
        .first_slot = 0,
        .end_slot = BIT(l1size),
        .level_two_bits = l2size,
        .start_existing_index = 0,
        .end_existing_index = end_existing_index,
        .start_existing_slot = 0,
        .end_existing_slot = last_cap + 1});
    if (error) {
        return error;
    }
    error = allocman_attach_cspace(bs->alloc, cspace_two_level_make_interface(cspace));
    if (error) {
        return error;
    }
    /* construct path to our old cnode */
    bs->old_cnode = _cspace_two_level_make_path(cspace, old_cnode);
    /* update where our current booting cnode is */
    bs->boot_cnode = _cspace_two_level_make_path(cspace, cnode);
    /* any untypeds are no longer valid */
    bs->uts_in_current_cspace = 0;
    return 0;
}

static void bootstrap_update_untypeds(bootstrap_info_t *bs) {
    int i;
    for (i = 0; i < bs->num_uts; i++) {
        bs->uts[i].root = bs->old_cnode.capPtr;
    }
}

static void bootstrap_set_pd_tcb_bootinfo(bootstrap_info_t *bs) {
    bs->pd = bs->boot_cspace.make_path(bs->boot_cspace.cspace, seL4_CapInitThreadPD);
    bs->tcb = bs->boot_cspace.make_path(bs->boot_cspace.cspace, seL4_CapInitThreadTCB);
}

static void bootstrap_set_pd_tcb(bootstrap_info_t *bs, cspacepath_t pd, cspacepath_t tcb) {
    bs->pd = pd;
    bs->tcb = tcb;
}

static int bootstrap_move_untypeds(bootstrap_info_t *bs) {
    int i;
    int error;
    for (i = 0; i < bs->num_uts; i++) {
        cspacepath_t slot;
        error = allocman_cspace_alloc(bs->alloc, &slot);
        if (error) {
            return error;
        }
        error = vka_cnode_move(&slot, &bs->uts[i]);
        if (error != seL4_NoError) {
            return 1;
        }
        bs->uts[i] = slot;
    }
    bs->uts_in_current_cspace = 1;
    return 0;
}

static int bootstrap_create_utspace(bootstrap_info_t *bs) {
    int error;
    int i;
    UTMAN_TYPE *utspace;
    utspace = allocman_mspace_alloc(bs->alloc, sizeof(*utspace), &error);
    if (error) {
        LOG_ERROR("Initial memory pool too small to allocate untyped allocator");
        return error;
    }
    UTMAN_CREATE(utspace);
    /* we can shove it in the allocman before we start telling it about its untypeds */
    error = allocman_attach_utspace(bs->alloc, UTMAN_MAKE_INTERFACE(utspace));
    if (error) {
        return error;
    }

    for (i = 0; i < bs->num_uts; i++) {
        error = UTMAN_ADD_UTS(bs->alloc, utspace, 1, &bs->uts[i], &bs->ut_size_bits[i], &bs->ut_paddr[i], bs->ut_isDevice[i] ? ALLOCMAN_UT_DEV : ALLOCMAN_UT_KERNEL);
        if (error) {
            LOG_ERROR("Failed to add untypeds to untyped allocator");
            return error;
        }
    }
    return 0;
}

bootstrap_info_t *bootstrap_create_info(size_t pool_size, void *pool) {
    allocman_t *alloc;
    bootstrap_info_t *info;
    /* bootstrap an allocman from the pool */
    alloc = bootstrap_create_allocman(pool_size, pool);
    if (!alloc) {
        return NULL;
    }
    /* create the bootstrapping info */
    info = _create_info(alloc);
    if (!info) {
        return NULL;
    }
    return info;
}

static int _slot_memory_reservations(allocman_t *alloc) {
    int error;
    /* these numbers are pulled completely out of my arse (although given I wrote
     * the allocators my gut feeling should have some weight...) */
    error = allocman_configure_max_freed_slots(alloc, 10);
    if (error) {
        return error;
    }
    error = allocman_configure_max_freed_memory_chunks(alloc, 20);
    if (error) {
        return error;
    }
    error = allocman_configure_max_freed_untyped_chunks(alloc, 10);
    if (error) {
        return error;
    }
    error = allocman_configure_cspace_reserve(alloc, 30);
    if (error) {
        return error;
    }
    return 0;
}

static int _cnode_reservation(allocman_t *alloc, size_t cnode_size) {
    /* assume one extra level 2 cnode is all we need to keep around */
    return allocman_configure_utspace_reserve(alloc, (struct allocman_utspace_chunk) {cnode_size + seL4_SlotBits, seL4_CapTableObject, 1});
}

allocman_t *bootstrap_use_current_1level(seL4_CPtr root_cnode, size_t cnode_size, seL4_CPtr start_slot, seL4_CPtr end_slot, size_t pool_size, void *pool) {
    allocman_t *alloc;
    int error;
    bootstrap_info_t *info;
    /* create the bootstrapping info */
    info = bootstrap_create_info(pool_size, pool);
    if (!info) {
        LOG_ERROR("Failed to create bootstrap info");
        return NULL;
    }
    /* we will use the current cspace */
    error = bootstrap_use_current_1level_cspace(info, root_cnode, cnode_size, start_slot, end_slot);
    if (error) {
        LOG_ERROR("Failed to boostrap in the current cspace");
        return NULL;
    }
    /* set the pd and tcb */
    bootstrap_set_pd_tcb_bootinfo(info);
    /* can now create untyped allocator */
    error = bootstrap_create_utspace(info);
    if (error) {
        LOG_ERROR("Failed to create the untyped allocator");
        return NULL;
    }
    /* add normal slot reservations as well as reservations for memory system. No cnode
     * reservations since using current cnode */
    error = _slot_memory_reservations(info->alloc);
    if (error) {
        LOG_ERROR("Failed to add slot and memory reservations");
        return NULL;
    }
    /* all done */
    alloc = info->alloc;
    bootstrap_free_info(info);
    return alloc;
}

static allocman_t *_post_new_cspace_common(bootstrap_info_t *info, cspacepath_t *oldroot) {
    allocman_t *alloc;
    int error;
    /* update any resources */
    bootstrap_update_untypeds(info);
    /* move untypeds into new cspace */
    error = bootstrap_move_untypeds(info);
    if (error) {
        return NULL;
    }
    /* can now create untyped allocator */
    error = bootstrap_create_utspace(info);
    if (error) {
        return NULL;
    }
    /* add normal slot reservations as well as reservations for memory system. We do not
     * know the cspace geometry, so cannot add cnode reservations */
    error = _slot_memory_reservations(info->alloc);
    if (error) {
        return NULL;
    }
    /* give the location of the cap back to the original root cnode */
    if (oldroot) {
        *oldroot = info->old_cnode;
    }
    /* all done */
    alloc = info->alloc;
    return alloc;
}

/* this does not free the underlying 'info' */
static allocman_t *_bootstrap_new_level1(bootstrap_info_t *info, size_t cnode_size, cspacepath_t tcb, cspacepath_t pd, cspacepath_t *oldroot) {
    int error;
    /* set the pd and tcb */
    bootstrap_set_pd_tcb(info, pd, tcb);
    /* create a new one level cspace and switch to it */
    error = bootstrap_new_1level_cspace(info, cnode_size);
    if (error) {
        return NULL;
    }
    /* perform post cspace switch tasks (move untypeds and finish creating the allocman) */
    return _post_new_cspace_common(info, oldroot);
}

/* this does not free the underlying 'info' */
static allocman_t *_bootstrap_new_level2(bootstrap_info_t *info, size_t l1size, size_t l2size, cspacepath_t tcb, cspacepath_t pd, cspacepath_t *oldroot) {
    int error;
    allocman_t *alloc;
    /* set the pd and tcb */
    bootstrap_set_pd_tcb(info, pd, tcb);
    /* create a new one level cspace and switch to it
     * place the cap to the root cnode at slot 2 and the cap to the old cnode at slot 1
     */
    size_t total_caps = info->num_uts;
    if(info->have_boot_cspace) {
        cspacepath_t to_slot;
        info->boot_cspace.alloc(info->alloc, info->boot_cspace.cspace, &to_slot);
        total_caps += MAX(2, to_slot.capPtr);
        error = bootstrap_new_2level_cspace(info, l1size, l2size, 2, to_slot.capPtr, total_caps);
    } else {
        total_caps += 2;
        error = bootstrap_new_2level_cspace(info, l1size, l2size, 2, 1, total_caps);
    }
    if (error) {
        return NULL;
    }
    /* perform post cspace switch tasks (move untypeds and finish creating the allocman) */
    alloc = _post_new_cspace_common(info, oldroot);
    if (!alloc) {
        return NULL;
    }
    /* add reservations for a second level cnode */
    error = _cnode_reservation(alloc, l2size);
    if (error) {
        return NULL;
    }
    return alloc;
}

allocman_t *bootstrap_new_1level(bootstrap_info_t *info, size_t cnode_size, cspacepath_t tcb, cspacepath_t pd, cspacepath_t *oldroot) {
    allocman_t *alloc = _bootstrap_new_level1(info, cnode_size, tcb, pd, oldroot);
    if (!alloc) {
        return NULL;
    }
    bootstrap_free_info(info);
    return alloc;
}

allocman_t *bootstrap_new_2level(bootstrap_info_t *info, size_t l1size, size_t l2size, cspacepath_t tcb, cspacepath_t pd, cspacepath_t *oldroot) {
    allocman_t *alloc = _bootstrap_new_level2(info, l1size, l2size, tcb, pd, oldroot);
    if (!alloc) {
        return NULL;
    }
    bootstrap_free_info(info);
    return alloc;
}

static bootstrap_info_t *_start_new_from_bootinfo(seL4_BootInfo *bi, size_t pool_size, void *pool) {
    int error;
    bootstrap_info_t *info;
    /* create the bootstrapping info */
    info = bootstrap_create_info(pool_size, pool);
    if (!info) {
        return NULL;
    }
    /* we will use the current cspace (as descrbied by bootinfo) temporarily for bootstrapping */
    error = bootstrap_create_temp_bootinfo_cspace(info, bi);
    if (error) {
        return NULL;
    }
    /* give all the bootinfo untypeds */
    error = bootstrap_add_untypeds_from_bootinfo(info, bi);
    if (error) {
        return NULL;
    }
    return info;
}

static allocman_t *_new_from_bootinfo_common(bootstrap_info_t *info, cspace_simple1level_t **old_cspace) {
    int error;
    allocman_t *alloc;
    /* allocate old cspace if desired */
    if (old_cspace) {
        *old_cspace = allocman_mspace_alloc(info->alloc, sizeof(**old_cspace), &error);
        if (error) {
            return NULL;
        }
        /* we are relying on internal knowledge of this allocator to know that this copy operation is okay */
        **old_cspace = info->maybe_boot_cspace;
    }
    /* all done */
    alloc = info->alloc;
    bootstrap_free_info(info);
    return alloc;
}

allocman_t *bootstrap_new_1level_bootinfo(seL4_BootInfo *bi, size_t cnode_size, size_t pool_size, void *pool, cspace_simple1level_t **old_cspace) {
    allocman_t *alloc;
    bootstrap_info_t *info;
    /* create the bootstrapping info and fill in as much from bootinfo as we can */
    info = _start_new_from_bootinfo(bi, pool_size, pool);
    if (!info) {
        return NULL;
    }
    /* create cspace, switch to it and finish creating the allocman */
    alloc = _bootstrap_new_level1(info,
                                  cnode_size,
                                  info->boot_cspace.make_path(info->boot_cspace.cspace, seL4_CapInitThreadTCB),
                                  info->boot_cspace.make_path(info->boot_cspace.cspace, seL4_CapInitThreadPD),
                                  NULL);
    if (!alloc) {
        return NULL;
    }
    /* do common cleanup */
    return _new_from_bootinfo_common(info, old_cspace);
}

allocman_t *bootstrap_new_2level_bootinfo(seL4_BootInfo *bi, size_t l1size, size_t l2size, size_t pool_size, void *pool, cspace_simple1level_t **old_cspace) {
    allocman_t *alloc;
    bootstrap_info_t *info;
    /* create the bootstrapping info and fill in as much from bootinfo as we can */
    info = _start_new_from_bootinfo(bi, pool_size, pool);
    if (!info) {
        return NULL;
    }
    /* create cspace, switch to it and finish creating the allocman */
    alloc = _bootstrap_new_level2(info,
                                  l1size,
                                  l2size,
                                  info->boot_cspace.make_path(info->boot_cspace.cspace, seL4_CapInitThreadTCB),
                                  info->boot_cspace.make_path(info->boot_cspace.cspace, seL4_CapInitThreadPD),
                                  NULL);
    if (!alloc) {
        return NULL;
    }
    /* do common cleanup */
    return _new_from_bootinfo_common(info, old_cspace);
}

static size_t get_next_alignment_bits(uintptr_t base, uintptr_t goal, size_t max_alignment) {
    /* Calculate alignment of our base pointer */
    uintptr_t alignment = (base == 0) ? max_alignment : MIN(CTZL((long)base), max_alignment);

    /* Our biggest size is either the distance to our goal, or our largest size
     * based on our current alignment. */
    uintptr_t next_size = MIN(goal - base, BIT(alignment));
    ZF_LOGF_IF(next_size == 0, "next_size should not be 0 here.");

    /* The largest untyped we can make is the highest power of 2 component of next_size */
    size_t next_size_bits = seL4_WordBits - 1 - CLZL((long)next_size);

    ZF_LOGD("goal 0x%zx, base: 0x%zx, next_size: 0x%zx, 0x%zx, 0x%zx, 0x%zx", goal, base, (uintptr_t) BIT(next_size_bits), next_size, CLZL((long)next_size),alignment);
    return next_size_bits;
}

/*
    Helper function that takes a region of device untyped and extracts it
    from one utspace manager and deposits into a different utspace manager with
    `untyped_type` type.  It is used for splitting up device untyped caps into ALLOCMAN_UT_DEV_MEM
    and ALLOCMAN_UT_DEV.
 */
static ssize_t perform_sub_allocation(uintptr_t base, uintptr_t goal, allocman_t *alloc,
                        utspace_interface_t *ut_space, utspace_split_t *token, int untyped_type) {
    size_t next_size_bits = get_next_alignment_bits(base, goal, seL4_MaxUntypedBits);
    cspacepath_t path;
    int error = allocman_cspace_alloc(alloc, &path);
    ZF_LOGF_IF(error, "allocman_cspace_alloc failed");

    ut_space->alloc(alloc, token, next_size_bits, seL4_UntypedObject, &path, base, true, &error);
    ZF_LOGF_IF(error, "ut_space.alloc failed");

    error = allocman_utspace_add_uts(alloc, 1, &path, &next_size_bits, &base, untyped_type);
    ZF_LOGF_IF(error, "allocman_utspace_add_uts failed");

    return BIT(next_size_bits);
}

/**
 * Separates device ram memory into separate untyped caps from the cap provided.
 * Arch specific implementation as on x86 we can use the grup multiboot headers to
 * find the address layout.  On arm there is no current implementation, but eventually
 * we should be able to take a device tree and extract memory layout from that.
 *
 * @param  state     handle to device specific state.
 * @param  paddr     base paddr of the untyped cap being passed in.
 * @param  size_bits size in bits of the untyped cap.
 * @param  path      the untyped cap.
 * @param  alloc     handle to an already initialised allocman.
 * @return           0 for success, otherwise error.
 */
static int handle_device_untyped_cap(add_untypeds_state_t *state, uintptr_t paddr, size_t size_bits, cspacepath_t *path, allocman_t *alloc ) {
    bool cap_tainted = false;
    int error;
    uintptr_t ut_end = paddr + BIT(size_bits);
    int num_regions = 0;
    if (state != NULL) {
        num_regions = state->num_regions;
    }
    for (int i = 0; i < num_regions; i++) {
        pmem_region_t *region = &state->regions[i];
        uint64_t region_end = region->base_addr + region->length;

        if (region->type == PMEM_TYPE_RAM && !(paddr >= region_end || ut_end <= region->base_addr)) {
            if (!cap_tainted) {
                cap_tainted = true;
                state->ut_interface.add_uts(alloc, &state->split_ut, 1, path, &size_bits, &paddr, ALLOCMAN_UT_DEV);
            }
            // We have an untyped that is overlapping with some ram.  Lets break it up into sub parts
            ZF_LOGD("basepaddr 0x%zx framesize: %zd", paddr, size_bits);
            ZF_LOGD("\tPhysical Memory Region from %"PRIx64" size %"PRIx64" type %d", region->base_addr, region->length, region->type);

            uintptr_t top = MIN(region_end, ut_end);
            while (paddr < top) {
                uintptr_t goal;
                int untyped_type;
                if (paddr >= region->base_addr) {
                    goal = top;
                    untyped_type = ALLOCMAN_UT_DEV_MEM;
                } else {
                    goal = region->base_addr;
                    untyped_type = ALLOCMAN_UT_DEV;
                }
                ssize_t result = perform_sub_allocation(paddr, goal, alloc, &state->ut_interface, &state->split_ut, untyped_type);
                ZF_LOGF_IF(result < 0, "perform_sub_allocation failed");
                paddr += result;

            }
        }
    }
    if (!cap_tainted) {
        error = allocman_utspace_add_uts(alloc, 1, path, &size_bits, &paddr, ALLOCMAN_UT_DEV);
        ZF_LOGF_IF(error, "allocman_utspace_add_uts failed");
    } else if (paddr != ut_end){
        // Got to allocate the rest of our untyped as UT_DEV
        while (paddr < ut_end) {
            ssize_t result = perform_sub_allocation(paddr, ut_end, alloc, &state->ut_interface, &state->split_ut, ALLOCMAN_UT_DEV);
            ZF_LOGF_IF(result < 0, "perform_sub_allocation failed");
            paddr += result;
        }
    }
    return 0;
}

static void free_device_untyped_cap_state(allocman_t *alloc, add_untypeds_state_t *state) {
    if (state == NULL) {
        ZF_LOGE("free_device_untyped_cap_state with NULL state");
        return;
    }
    if (state->region_alloc) {
        if (state->regions == NULL) {
            ZF_LOGE("free_device_untyped_cap_state with NULL regions");
            return;
        }
        allocman_mspace_free(alloc, state->regions, sizeof(pmem_region_t) * state->num_regions);
    }
    allocman_mspace_free(alloc, state, sizeof(add_untypeds_state_t));
    return;
}
/**
 * Initialise required state for future calls to bootstrap_arch_handle_device_untyped_cap
 * @param  alloc           an allocman with a cspace, mspace and utspace inside it.
 * @param  simple          a simple interface
 * @param  token           For returning a handle to the initialsed state.
 * @param  feature_enabled For returning whether bootstrap_arch_handle_device_untyped_cap can be called.
 * @param  num_regions     number of regions in region array
 * @param  region_list     an array of regions. If NULL this will call sel4platsupport_get_pmem_region_list.
 * @return                 0 for success, otherwise error.
 */
static int prepare_handle_device_untyped_cap(allocman_t *alloc, simple_t *simple, add_untypeds_state_t **token, int num_regions, pmem_region_t *region_list) {
    int error;
    add_untypeds_state_t *state = allocman_mspace_alloc(alloc, sizeof(add_untypeds_state_t), &error);
    ZF_LOGF_IF(error, "Failed to allocate add_untypeds_state_t");

    if (num_regions >= 0 && region_list != NULL) {
        state->num_regions = num_regions;
        state->regions = region_list;
        state->region_alloc = false;
    } else  {
        num_regions = sel4platsupport_get_num_pmem_regions(simple);
        if (num_regions == 0 || num_regions == -1) {
            free_device_untyped_cap_state(alloc, state);
            return 0;
        }
        state->regions = allocman_mspace_alloc(alloc, sizeof(pmem_region_t) * num_regions, &error);
        state->region_alloc = true;
        state->num_regions = sel4platsupport_get_pmem_region_list(simple, num_regions, state->regions);
        ZF_LOGF_IF(state->num_regions != num_regions, "Couldn't extract region list from simple");
    }
    utspace_split_create(&state->split_ut);
    state->ut_interface = utspace_split_make_interface(&state->split_ut);

    *token = state;
    return 0;
}

int allocman_add_simple_untypeds_with_regions(allocman_t *alloc, simple_t *simple, int num_regions, pmem_region_t *region_list) {
    add_untypeds_state_t *state = NULL;
    int error = prepare_handle_device_untyped_cap(alloc, simple, &state, num_regions, region_list);
    ZF_LOGF_IF(error, "bootstrap_prepare_handle_device_untyped_cap Failed");

    size_t i;
    size_t total_untyped = simple_get_untyped_count(simple);

    for(i = 0; i < total_untyped; i++) {
        size_t size_bits;
        uintptr_t paddr;
        bool device;
        cspacepath_t path = allocman_cspace_make_path(alloc, simple_get_nth_untyped(simple, i, &size_bits, &paddr, &device));
        int dev_type = device ? ALLOCMAN_UT_DEV : ALLOCMAN_UT_KERNEL;
        // If it is regular UT memory, then we add cap and move on.
        if (dev_type == ALLOCMAN_UT_KERNEL) {
            error = allocman_utspace_add_uts(alloc, 1, &path, &size_bits, &paddr, dev_type);
            ZF_LOGF_IF(error, "Could not add kernel untyped.");
        } else {
            // Otherwise we are Device untyped.
            error = handle_device_untyped_cap(state, paddr, size_bits, &path, alloc);
            ZF_LOGF_IF(error, "bootstrap_arch_handle_device_untyped_cap failed.");
        }
    }
    if (state) {
        free_device_untyped_cap_state(alloc, state);
    }
    return 0;
}

int allocman_add_simple_untypeds(allocman_t *alloc, simple_t *simple) {
    return allocman_add_simple_untypeds_with_regions(alloc, simple, 0, NULL);
}

allocman_t *bootstrap_use_current_simple(simple_t *simple, size_t pool_size, void *pool) {
    allocman_t *allocman;
    int error;
    /* Initialize inside the current 1 level cspace as defined by simple */
    allocman = bootstrap_use_current_1level(simple_get_cnode(simple), simple_get_cnode_size_bits(simple), simple_last_valid_cap(simple) + 1, BIT(simple_get_cnode_size_bits(simple)), pool_size, pool);
    if (!allocman) {
        LOG_ERROR("Failed to initialize an allocman");
        return allocman;
    }
    error = allocman_add_simple_untypeds(allocman, simple);
    if (error) {
        /* We have no way to cleanup the allocman we started bootstrapping */
        LOG_ERROR("Failed in call to allocman_add_simple_untypeds, cannot cleanup, leaking memory");
        return NULL;
    }
    return allocman;
}

static allocman_t *bootstrap_new_simple(simple_t *simple, int levels, size_t l1size, size_t l2size, size_t pool_size, void *pool) {
    allocman_t *alloc;
    int error;

    assert(levels == 1 || levels == 2);
    assert(l1size > 0);
    assert(levels == 1 || l2size > 0);

    bootstrap_info_t *bootstrap = bootstrap_create_info(pool_size, pool);
    if (bootstrap == NULL) {
        return NULL;
    }
    bootstrap->simple = simple;

    cspace_simple1level_create(&bootstrap->maybe_boot_cspace, (struct cspace_simple1level_config){
            .cnode = simple_get_cnode(simple),
            .cnode_size_bits = simple_get_cnode_size_bits(simple),
            .cnode_guard_bits = seL4_WordBits - simple_get_cnode_size_bits(simple),
            .first_slot = simple_last_valid_cap(simple) + 1,
            .end_slot = BIT(simple_get_cnode_size_bits(simple))
    });

    cspacepath_t init_cnode_path = _cspace_simple1level_make_path(&bootstrap->maybe_boot_cspace, simple_get_cnode(simple));
    bootstrap_set_boot_cspace(bootstrap, cspace_simple1level_make_interface(&bootstrap->maybe_boot_cspace), init_cnode_path);

    /* Tell the boostrapping allocator about the untypeds in the system */
    bootstrap_add_untypeds_from_simple(bootstrap, simple);

    cspacepath_t tcb = _cspace_simple1level_make_path(&bootstrap->maybe_boot_cspace, simple_get_tcb(simple));
    cspacepath_t pd = _cspace_simple1level_make_path(&bootstrap->maybe_boot_cspace, simple_get_pd(simple));

    if (levels == 1) {
        alloc = _bootstrap_new_level1(bootstrap, l1size, tcb, pd, NULL);
    } else {
        alloc = _bootstrap_new_level2(bootstrap, l1size, l2size, tcb, pd, NULL);
    }
    if (!alloc) {
        return NULL;
    }

    /* Take all the caps in the boot cnode and put in them in the same location in the new cspace */
    error = bootstrap_transfer_caps_simple(bootstrap, simple, levels);
    if(error) {
        return NULL;
    }

    /* Cleanup */
    bootstrap_free_info(bootstrap);

    return alloc;
}

allocman_t *bootstrap_new_1level_simple(simple_t *simple, size_t l1size, size_t pool_size, void *pool) {
    return bootstrap_new_simple(simple, 1, l1size, 0, pool_size, pool);
}

allocman_t *bootstrap_new_2level_simple(simple_t *simple, size_t l1size, size_t l2size, size_t pool_size, void *pool) {
    return bootstrap_new_simple(simple, 2, l1size, l2size, pool_size, pool);
}

static int allocman_add_bootinfo_untypeds(allocman_t *alloc, seL4_BootInfo *bi) {
    seL4_CPtr i;
    int error;
    for (i = bi->untyped.start; i < bi->untyped.end; i++) {
        size_t index = i - bi->untyped.start;
        cspacepath_t slot;
        size_t size_bits;
        uintptr_t paddr;
        slot = allocman_cspace_make_path(alloc, i);
        size_bits = bi->untypedList[index].sizeBits;
        paddr = bi->untypedList[index].paddr;
        error = allocman_utspace_add_uts(alloc, 1, &slot, &size_bits, &paddr, bi->untypedList[index].isDevice ? ALLOCMAN_UT_DEV : ALLOCMAN_UT_KERNEL);
        if (error) {
            return error;
        }
    }
    return 0;
}

allocman_t *bootstrap_use_bootinfo(seL4_BootInfo *bi, size_t pool_size, void *pool) {
    allocman_t *alloc;
    int error;
    /* use the current single level cspace as described by boot info */
    alloc = bootstrap_use_current_1level(seL4_CapInitThreadCNode,
                                         bi->initThreadCNodeSizeBits,
                                         bi->empty.start,
                                         bi->empty.end,
                                         pool_size,
                                         pool);
    if (!alloc) {
        return NULL;
    }
    /* now add all the untypeds described in bootinfo */
    error = allocman_add_bootinfo_untypeds(alloc, bi);
    if (error) {
        return NULL;
    }
    return alloc;
}

void bootstrap_configure_virtual_pool(allocman_t *alloc, void *vstart, size_t vsize, seL4_CPtr pd) {
    /* configure reservation for the virtual pool */
    /* assume we are using 4k pages. maybe this should be a Kconfig option at some point?
     * we ignore any errors */
    allocman_configure_utspace_reserve(alloc, (struct allocman_utspace_chunk) {vka_get_object_size(seL4_ARCH_4KPage, 0), seL4_ARCH_4KPage, 3});
    allocman_configure_utspace_reserve(alloc, (struct allocman_utspace_chunk) {vka_get_object_size(seL4_ARCH_PageTableObject, 0), seL4_ARCH_PageTableObject, 1});
    allocman_sel4_arch_configure_reservations(alloc);
    mspace_dual_pool_attach_virtual(
            (mspace_dual_pool_t*)alloc->mspace.mspace,
            (struct mspace_virtual_pool_config){
                .vstart = vstart,
                .size = vsize,
                .pd = pd
            }
    );
}
