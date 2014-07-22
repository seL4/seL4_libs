/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/**
 * @file bootstrap.h
 *
 * @brief Helpers for bootstrapping an allocman on freshly running system
 *
 * Bootstrapping a system is hard. These helpers attempt to make it easy to start
 * a system with a 'common' configuration. Systems that are not common should also
 * be possible, but the code here will be able to help you less.
 *
 * In general to bootstrap a system you need to do the following
 * 1. Describe the current cspace
 * 2. Describe where resources (untypeds, slots etc) can be found
 * 3. (maybe) create and switch to a new cspace
 * These bootstrapping functions attempt to hide the requirements of these steps as
 * much as possible, allowing you to substitute 'bootinfo' if you have it, otherweise
 * requiring some minimal description. However enough internals are provided that
 * each of these steps can be broken into if you need to describe some particularly
 * interesting system.
 *
 * The allocman that gets created as a result of bootstrapping is constructed using
 * some default 'sensible' allocators. Unfortunately if you want different allocators
 * then there is currently no way to control that.
 *
 * Bootstrapping here requires some initial 'pool' of memory. This is memory that
 * must NEVER be freed after you have bootstrapped an allocator. Be very careful
 * you do not put this on a stack (and then free it) or have it as a global and use
 * it multiple times. Unfortunately this pool is the 'magic' that allows the allocators
 * to kick themselves going, and cannot be provided for you. The actual size of the pool
 * is something you will need to work out with trial and error.
 *
 * With all of these methods, once they return failure you are basically 100% fucked
 * since they make no effort to clean up after themselves if they detect failure. The
 * ONLY sane thing you can do is log an error and terminate.
 *
 * See example_bootstrap.c for sample code that shows how to bootstrap in various ways
 *
 */

#ifndef _AM_BOOTSTRAP_H_
#define _AM_BOOTSTRAP_H_

#include <sel4/sel4.h>
#include <string.h>
#include <allocman/allocman.h>
#include <allocman/cspace/simple1level.h>
#include <allocman/cspace/two_level.h>
#include <allocman/mspace/fixed_pool.h>
#include <allocman/mspace/virtual_pool.h>
#include <allocman/utspace/twinkle.h>
#include <allocman/utspace/trickle.h>
#include <vspace/vspace.h>
#include <simple/simple.h>
/**
 * Internal data structure for storing bootstrapping information. If you need to break
 * open the boot strapping process, then you will be given a pointer to one of these
 * that you will interract with to finish the boot strapping
 */
typedef struct bootstrap_info bootstrap_info_t;

/**
 * Every allocation manager created by these bootstrapping functions has a dual_pool
 * as its memory manager. For the purposes of bootstrapping only the fixed pool (as
 * passed to the boot strapping functions) is used. If you want to use a virtual pool
 * (you almost certainly do so you don't run out of memory or have a stupidly large
 * static pool) then this function will initial the virtual pool after the fact. The
 * reason for this ordering is that it is expected that the creation of a vspace manager
 * might expect an allocator, which you won't have yet if you are boot strapping.
 *
 * Note that there is no protection against calling this function multiple times or
 * from trying to call it on an allocman that does not have a dual_pool as its underlying
 * memory manager. DO NOT FUCK IT UP
 *
 * @param alloc Allocman whose memory manager to configure
 * @param vstart Start of a virtual address range that will be allocated from.
 * @param vsize Size of the virtual address range
 * @param pd Page directory to invoke when mapping frames/page tables
 */
void bootstrap_configure_virtual_pool(allocman_t *alloc, void *vstart, uint32_t vsize,
        seL4_CPtr pd);

/**
 * Simplest bootstrapping method that uses all the information in seL4_BootInfo
 * assumes you are the rootserver. This keeps using whatever cspace you are currently in.
 *
 * @param bi BootInfo as passed to the rootserver
 * @param pool_size Size of the initial pool. See file comments for details
 * @param pool Initial pool. See file comments for details
 *
 * @return returns NULL on error
 */
allocman_t *bootstrap_use_bootinfo(seL4_BootInfo *bi, uint32_t pool_size, char *pool);

/**
 * Bootstraps using all the information in bootinfo, but switches to a new single
 * level cspace. All untypeds specified in bootinfo will be moved to the new cspace,
 * any other capabilities will be left in the old cspace. If you wish to refer to the
 * boot cspace (most likely since it probably has capabilities you still want), then
 * a cspace description of the old cspace can also be returned.
 *
 * @param bi BootInfo as passed to the rootserver
 * @param cnode_size Number of slot bits (cnode_slots = 2^cnode_size) for the new cnode
 * @param pool_size Size of the initial pool. See file comments for details
 * @param pool Initial pool. See file comments for details
 * @param old_cspace Optional location to store a description of the original cspace. You
 * can free this memory back to the allocman when are done with it
 *
 * @return returns NULL on error
 */
allocman_t *bootstrap_new_1level_bootinfo(seL4_BootInfo *bi, int cnode_size, uint32_t pool_size, char *pool, cspace_simple1level_t **old_cspace);

/**
 * Bootstraps using all the information in bootinfo, but switches to a new two
 * level cspace. All untypeds specified in bootinfo will be moved to the new cspace,
 * any other capabilities will be left in the old cspace. If you wish to refer to the
 * boot cspace (most likely since it probably has capabilities you still want), then
 * a cspace description of the old cspace can also be returned.
 *
 * @param bi BootInfo as passed to the rootserver
 * @param l1size Number of slot bits (l1_slots = 2^l1size) for the level 1 cnode
 * @param l2size Number of slot bits (l2_slots = 2^l2size) for the level 2 cnode
 * @param pool_size Size of the initial pool. See file comments for details
 * @param pool Initial pool. See file comments for details
 * @param old_cspace Optional location to store a description of the original cspace. You
 * can free this memory back to the allocman when are done with it
 *
 * @return returns NULL on error
 */
allocman_t *bootstrap_new_2level_bootinfo(seL4_BootInfo *bi, int l1size, int l2size, uint32_t pool_size, char *pool, cspace_simple1level_t **old_cspace);

/**
 * Give an allocator all the untyped memory that simple knows about.
 *
 * This assumes that all the untyped caps are currently as simple thinks they are.
 * If there have been any cspace reshuffles simple will not give allocman useable information
 */
int allocman_add_simple_untypeds(allocman_t *alloc, simple_t *simple);

/**
 * Bootstraps using all the information provided by simple, but switches to a new two
 * level cspace. All capabilities specified by simple will be moved to the new cspace. All untypeds specified by simple are given to the allocator
 *
 * @param simple simple pointer to the struct
 * @param l1size Number of slot bits (l1_slots = 2^l1size) for the level 1 cnode
 * @param l2size Number of slot bits (l2_slots = 2^l2size) for the level 2 cnode
 * @param pool_size Size of the initial pool. See file comments for details
 * @param pool Initial pool. See file comments for details
 *
 * @return returns NULL on error
 */
allocman_t *bootstrap_new_2level_simple(simple_t *simple, int l1size, int l2size, uint32_t pool_size, char *pool);

/**
 * Bootstraps into the current environment as defined by simple. This will continue
 * to use the cspace described by simple, as well as all the untypeds it knows about
 *
 * @param simple Pointer to simple interface, will not be retained
 * @param pool_size Size of the initial pool. See file comments for details
 * @param pool Initial pool. See file comments for details
 *
 * @return returns NULL on error
 */
allocman_t *bootstrap_use_current_simple(simple_t *simple, uint32_t pool_size, char *pool);

/**
 * Bootstraps an allocator that will reuse the current single level cspace (which
 * you must describe to it). While bootstrapping should succeed, you will need to
 * add untypeds manually to the returned allocman to make it useful.
 *
 * @param root_cnode Location of the cnode that is the current cspace
 * @param cnode_size Size in slot_bits of the current cnode
 * @param start_slot First free slot in the current cspace
 * @param end_slot Last free slot + 1 in the current cspace
 * @param pool_size Size of the initial pool. See file comments for details
 * @param pool Initial pool. See file comments for details
 *
 * @return returns NULL on error
 */
allocman_t *bootstrap_use_current_1level(seL4_CPtr root_cnode, int cnode_size, seL4_CPtr start_slot, seL4_CPtr end_slot, uint32_t pool_size, char *pool);

/**
 * Provides a description of the boot cspace if you are doing a customized
 * bootstrapping. This MUST be set before using boostrap_new_[1|2]level
 *
 * @param bs Internal bootstrapping info as allocated/returned by {@link #bootstrap_create_info}
 * @param cspace CSpace that will be used for bootstrapping purposes. The cspace only needs to exist
 *  for as long as bootstrapping is happening, it will not be used afterwards
 * @param root_cnode Path to the root cnode of cspace. This is needed so that a cap to the old cspace
 *  can be provided in the new cspace
 *
 * @return returns 0 on success
 */
int bootstrap_set_boot_cspace(bootstrap_info_t *bs, cspace_interface_t cspace, cspacepath_t root_cnode);

/**
 * Adds knowledge of untypeds to the bootstrapping information. These untypeds will
 * be moved to the new cspace and be given to the untyped manager  once bootstrapping
 * has completed.
 *
 * @param bs Internal bootstrapping info as allocated/returned by {@link #bootstrap_create_info}
 * @param num Number of untypeds to be added
 * @param uts Path to each of the untypeds
 * @param size_bits Size of each of the untypeds
 * @param paddr Optional physical address of each of the untypeds
 *
 * @return returns 0 on success
 */
int bootstrap_add_untypeds(bootstrap_info_t *bs, int num, cspacepath_t *uts, uint32_t *size_bits, uint32_t *paddr);

/**
 * Adds knowledge of all the untypeds of bootinfo to the bootstrapper. These will
 * be moved to the new cspace and given to the untyped manager once bootstrapping has
 * completed
 *
 * @param bs Internal bootstrapping info as allocated/returned by {@link #bootstrap_create_info}
 * @param bi BootInfo as passed to the rootserver
 *
 * @return returns 0 on success
 */
int bootstrap_add_untypeds_from_bootinfo(bootstrap_info_t *bs, seL4_BootInfo *bi);

/**
 * Completes bootstrapping into a new single level cspace.
 *
 * @param info Internal bootstrapping info as allocated/returned by {@link #bootstrap_create_info}
 * @param cnode_size Size in slot bits of new cspace
 * @param tcb Path to the TCB of the current thread, need to perform an invocation of seL4_TCB_SetSpace
 * @param pd Path to the PD of the current thread. This is needed to work around seL4 restriction that
 *  requires the address space be set at the same time as the cspace
 * @param oldroot Optional location to store a path to a cnode that is root cnode given in {@link #bootstrap_set_boot_cspace}
 *
 * @return returns NULL on error
 */
allocman_t *bootstrap_new_1level(bootstrap_info_t *info, int cnode_size, cspacepath_t tcb, cspacepath_t pd, cspacepath_t *oldroot);

/**
 * Completes bootstrapping into a new two level cspace.
 *
 * @param info Internal bootstrapping info as allocated/returned by {@link #bootstrap_create_info}
 * @param l1size Number of slot bits (l1_slots = 2^l1size) for the level 1 cnode
 * @param l2size Number of slot bits (l2_slots = 2^l2size) for the level 2 cnode
 * @param tcb Path to the TCB of the current thread, need to perform an invocation of seL4_TCB_SetSpace
 * @param pd Path to the PD of the current thread. This is needed to work around seL4 restriction that
 *  requires the address space be set at the same time as the cspace
 * @param oldroot Optional location to store a path to a cnode that is root cnode given in {@link #bootstrap_set_boot_cspace}
 *
 * @return returns NULL on error
 */
allocman_t *bootstrap_new_2level(bootstrap_info_t *info, int l1size, int l2size, cspacepath_t tcb, cspacepath_t pd, cspacepath_t *oldroot);

/**
 * This function starts bootstrapping the system, and then 'breaks out' and
 * allows you to give a description of the boot cspace as well as provide any
 * untypeds. A new 1 or 2 level cspace can then be created.
 *
 * @param pool_size Size of the initial pool. See file comments for details
 * @param pool Initial pool. See file comments for details
 *
 * @return returns NULL on error
 */
bootstrap_info_t *bootstrap_create_info(uint32_t pool_size, char *pool);

/**
 * Creates an empty allocman from a starting pool. The returned allocman will not
 * have an attached cspace or utspace. This function provides the ultimate flexibility
 * in how you can boot strap the system (read: this does basically nothing for you).

 * @param pool_size Size of the initial pool. See file comments for details
 * @param pool Initial pool. See file comments for details
 *
 * @return returns NULL on error
 */
allocman_t *bootstrap_create_allocman(uint32_t pool_size, char *pool);

#endif
