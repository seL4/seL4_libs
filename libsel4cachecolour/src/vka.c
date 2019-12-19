/*@LICENSE(NICTA_CORE)*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sel4/sel4.h>
#include <vka/object.h>
#include <allocman/allocman.h>
#include <allocman/util.h>
#include <cachecoloring/color_allocator.h>


/**
 * Allocate a slot in a cspace.
 *
 * @param data cookie for the underlying allocator
 * @param res pointer to a cptr to store the allocated slot
 * @return 0 on success
 */
static int color_vka_cspace_alloc(void *data, seL4_CPtr *res) {

    int error; 
    cspacepath_t path;

    assert(data); 
    assert(res); 

    color_allocator_t *allocator = (color_allocator_t *)data; 
    if (allocator->domain != COLOR_INIT_DOMAIN) 
        allocator = allocator->init_allocator; 

    error = allocman_cspace_alloc((allocman_t *)allocator->root_allocator, &path);
    if (!error) {
        *res = path.capPtr;
    }

    return error;
}

/**
 * Convert an allocated cptr to a cspacepath, for use in
 * operations such as Untyped_Retype
 *
 * @param data cookie for the underlying allocator
 * @param slot a cslot allocated by the cspace alloc function
 * @param res pointer to a cspacepath struct to fill out
 */
static void color_vka_cspace_make_path (void *data, seL4_CPtr slot, cspacepath_t *res)
{

    assert(data); 
    assert(res);

    color_allocator_t *allocator = (color_allocator_t *)data; 
    if (allocator->domain != COLOR_INIT_DOMAIN) 
        allocator = allocator->init_allocator; 

    *res = allocman_cspace_make_path((allocman_t *)allocator->root_allocator, slot);
    
}

/**
 * Free an allocated cslot
 *
 * @param data cookie for the underlying allocator
 * @param slot a cslot allocated by the cspace alloc function
 */
static void color_vka_cspace_free (void *data, seL4_CPtr slot) {

    cspacepath_t path; 
    assert(data);
    
    color_allocator_t *allocator = (color_allocator_t *)data; 
    if (allocator->domain != COLOR_INIT_DOMAIN) 
        allocator = allocator->init_allocator; 

    path = allocman_cspace_make_path((allocman_t *)allocator->root_allocator, slot); 

    allocman_cspace_free((allocman_t *)allocator->root_allocator, &path); 
}

/**
 * Allocate a portion of an untyped into an object
 *
 * @param data cookie for the underlying allocator
 * @param dest path to an empty cslot to place the cap to the allocated object
 * @param type the seL4 object type to allocate (as passed to Untyped_Retype)
 * @param size_bits the size of the object to allocate (as passed to Untyped_Retype)
 * @param res pointer to a location to store the cookie representing this allocation
 * @return 0 on success
 */
static int color_vka_utspace_alloc (void *data, const cspacepath_t *dest, seL4_Word type, seL4_Word size_bits, seL4_Word *res) {

    int error;

    assert(data);
    assert(res);
    assert(dest);

    /* allocman uses the size in memory internally, where as vka expects size_bits
     * as passed to Untyped_Retype, so do a conversion here */
    size_bits = vka_get_object_size(type, size_bits);

    color_allocator_t *allocator = (color_allocator_t *)data;

    /*call the root allocator if it is the init allocator*/
    if (allocator->domain == COLOR_INIT_DOMAIN) {
        allocman_t *root_allocator = allocator->root_allocator; 
        *res = allocman_utspace_alloc(root_allocator, size_bits, type, dest, 0, &error);

        return error;
    }


    return  color_utspace_alloc(allocator,(cspacepath_t*)dest, type, size_bits, res);

}

/**
 * Free a portion of an allocated untyped. Is the responsibility of the caller to
 * have already deleted the object (by deleting all capabilities) first
 *
 * @param data cookie for the underlying allocator
 * @param type the seL4 object type that was allocated (as passed to Untyped_Retype)
 * @param size_bits the size of the object that was allocated (as passed to Untyped_Retype)
 * @param target cookie to the allocation as given by the utspace alloc function
 */
static void color_vka_utspace_free (void *data, seL4_Word type, seL4_Word size_bits, seL4_Word target)
{
    assert(data);

    /* allocman uses the size in memory internally, where as vka expects size_bits
     * as passed to Untyped_Retype, so do a conversion here */
    size_bits = vka_get_object_size(type, size_bits);

    color_allocator_t *allocator = (color_allocator_t *)data;

    /*call the root allocator if it is the init allocator*/
    if (allocator->domain == COLOR_INIT_DOMAIN) {
    
        allocman_t *root_allocator = allocator->root_allocator; 
        allocman_utspace_free(root_allocator, target, size_bits);
   
    } else {
       
        color_utspace_free(allocator, target, size_bits);

    }
}

/*get the physical address of the allocated untypes*/
static uintptr_t color_vka_utspace_paddr(void *data, seL4_Word target, seL4_Word type, seL4_Word size_bits)
{
    assert(data);

    /* allocman uses the size in memory internally, where as vka expects size_bits
     * as passed to Untyped_Retype, so do a conversion here */
    size_bits = vka_get_object_size(type, size_bits);
    
    color_allocator_t *allocator = (color_allocator_t *)data;

    /*call the root allocator if it is the init allocator*/
    if (allocator->domain == COLOR_INIT_DOMAIN) {
        allocman_t *root_allocator = allocator->root_allocator; 
        return allocman_utspace_paddr(root_allocator, target, size_bits);
    }

    return color_utspace_paddr(allocator, target, size_bits); 
}

/**
 * Allocate a portion of an untyped into an object
 *
 * @param data cookie for the underlying allocator
 * @param dest path to an empty cslot to place the cap to the allocated object
 * @param type the seL4 object type to allocate (as passed to Untyped_Retype)
 * @param size_bits the size of the object to allocate (as passed to Untyped_Retype)
 * @param can_use_dev whether the allocator can use device untyped instead of regular untyped
 * @param res pointer to a location to store the cookie representing this allocation
 * @return 0 on success
 */
static int color_vka_utspace_alloc_maybe_device (void *data, const cspacepath_t *dest,
                seL4_Word type, seL4_Word size_bits, bool can_use_dev, seL4_Word *res)
{
    int error;

    assert(data);
    assert(res);
    assert(dest);

    /* allocman uses the size in memory internally, where as vka expects size_bits
     * as passed to Untyped_Retype, so do a conversion here */
    size_bits = vka_get_object_size(type, size_bits);

    color_allocator_t *allocator = (color_allocator_t *)data;

    /*call the root allocator if it is the init allocator*/
    if (allocator->domain == COLOR_INIT_DOMAIN) {
        allocman_t *root_allocator = allocator->root_allocator; 
        *res = allocman_utspace_alloc(root_allocator, size_bits, type, dest, can_use_dev, &error);

        return error;
    }

    /*the coloured allocator does not contain dev frame*/
    return  color_utspace_alloc(allocator,(cspacepath_t*)dest, type, size_bits, res);

}


/**
 * Allocate a portion of an untyped into an object
 *
 * @param data cookie for the underlying allocator
 * @param dest path to an empty cslot to place the cap to the allocated object
 * @param type the seL4 object type to allocate (as passed to Untyped_Retype)
 * @param size_bits the size of the object to allocate (as passed to Untyped_Retype)
 * @param paddr the desired physical address of the start of the allocated object
 * @param res pointer to a location to store the cookie representing this allocation
 * @return 0 on success
 */
static int color_vka_utspace_alloc_at (void *data, const cspacepath_t *dest, seL4_Word type, seL4_Word size_bits, uintptr_t paddr, seL4_Word *res)
{
    int error;

    assert(data);
    assert(res);
    assert(dest);

    /* allocman uses the size in memory internally, where as vka expects size_bits
     * as passed to Untyped_Retype, so do a conversion here */
    size_bits = vka_get_object_size(type, size_bits);
    color_allocator_t *allocator = (color_allocator_t *)data;

    /*call the root allocator if it is the init allocator*/
    if (allocator->domain == COLOR_INIT_DOMAIN) {
        allocman_t *root_allocator = allocator->root_allocator; 
        *res = allocman_utspace_alloc_at(root_allocator, size_bits, type, dest, paddr, true, &error);

        return error;
    }


    return  color_utspace_alloc_at(allocator,(cspacepath_t*)dest, type, size_bits, paddr, res);

}



/*
   attach the interfaces to vka structure
 */
void color_make_vka(vka_t *vka, color_allocator_t *allocator) {

    assert(vka);
    assert(allocator);

    vka->data = allocator;
    vka->cspace_alloc = &color_vka_cspace_alloc;
    vka->cspace_make_path = &color_vka_cspace_make_path;
    vka->utspace_alloc = &color_vka_utspace_alloc;
    vka->utspace_alloc_maybe_device = &color_vka_utspace_alloc_maybe_device;
    vka->utspace_alloc_at = &color_vka_utspace_alloc_at;
    vka->cspace_free = &color_vka_cspace_free;
    vka->utspace_free = &color_vka_utspace_free;
    vka->utspace_paddr = &color_vka_utspace_paddr;
}


