/*@LICENSE(NICTA_CORE)*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sel4/sel4.h>
#include <vka/object.h>
#include <allocman/allocman.h>
#include <allocman/bootstrap.h>
#include <allocman/util.h>
#include <allocman/utspace/split.h>
#include <cachecoloring/color_allocator.h>
#include <simple/simple.h>


/*
   get number of colors of current system
 */
size_t color_get_num_colors(void) {

    return CONFIG_NUM_CACHE_COLOURS;

}

/*
   create the root allocator for cache coloring; called by root thread only
   @param num_domains: number of colored domains
   @param div: dividing colors among domains, sum == number of colors
 */
color_allocator_t *color_create_init_allocator_use_simple(
        simple_t *simple,
        size_t pool_size, char *pool, 
        size_t num_domains, size_t div[]) {

    size_t temp = 0;
    struct allocman_mspace_chunk mspace_reserve; 
    int error; 

    if (!div || !num_domains) 
        return NULL;

    for (size_t i = 0; i < num_domains; i++) {
        
        if (!div[i]) {
            printf("colour allocation cannot not be 0\n"); 
            return NULL;

        }
        temp += div[i];
    }

    if (temp != CONFIG_NUM_CACHE_COLOURS) {
        printf("total number of colours not match\n"); 
        return NULL;
    }
    /*create the real allocator*/
    void *root_allocator = bootstrap_use_current_simple(simple, pool_size, pool);
    if (!root_allocator)
        return NULL;

    /*creating the reserved size*/
    mspace_reserve.size = sizeof (struct utspace_split_node);
    mspace_reserve.count = CONFIG_COLOUR_MSPACE_RESERVES; 
    allocman_configure_mspace_reserve(root_allocator, mspace_reserve);

    /*allocate the colored allocator*/
    color_allocator_t *color_allocator = allocman_mspace_alloc(
           root_allocator, sizeof (color_allocator_t), &error);  
    assert(color_allocator);

    memset(color_allocator, 0, sizeof(color_allocator_t));

    color_allocator->root_allocator = root_allocator; 
    color_allocator->num_domains = num_domains; 

    /*create ut manager for each domain*/
    for (size_t i = 0; i < num_domains; i++) {
        color_allocator->div[i] = div[i];

        color_allocator->ut_manager[i] = allocman_mspace_alloc(
                root_allocator, sizeof(utspace_split_t), &error); 
        assert(color_allocator->ut_manager[i]);

        memset(color_allocator->ut_manager[i], 0, sizeof (utspace_split_t)); 

        utspace_split_create((utspace_split_t *)color_allocator->ut_manager[i]);
    }

    color_allocator->domain = COLOR_INIT_DOMAIN;

    return color_allocator;
}



void *color_mspace_alloc(color_allocator_t *alloc, size_t size, int *_error) {

    /*currently only support allocating from the root allocator, the 
     mspace is not coloured*/
    assert(alloc); 
    assert(alloc->root_allocator); 

    return allocman_mspace_alloc(alloc->root_allocator, size, _error); 

}

/*configure the init allocator*/
void color_config_init_allocator(color_allocator_t *allocator, 
        void *vstart, size_t vsize, seL4_CPtr pd) {


    assert(allocator); 
    assert(allocator->root_allocator);

    bootstrap_configure_virtual_pool((allocman_t*)allocator->root_allocator, vstart, vsize, pd);

}

/*
   create the allocator for a colored domain
   @param init_allocator, the allocator created by root task
   @param color_num, the color number for this domain, start form 0.
 */
color_allocator_t *color_create_domain_allocator(color_allocator_t *init_allocator, size_t color_num) {

    int error; 

    if (!init_allocator || color_num >= init_allocator->num_domains)
        return NULL;
    
    color_allocator_t *color_allocator =  allocman_mspace_alloc(
            init_allocator->root_allocator, sizeof (color_allocator_t), &error); 
    assert(color_allocator);
    memset(color_allocator, 0, sizeof(color_allocator_t));

    color_allocator->domain = color_num; 
    color_allocator->init_allocator = init_allocator; 

    return color_allocator; 
}

/*

   manipulating ut managers

 */

static int color_utspace_add_uts(color_allocator_t *init_allocator, size_t domain, size_t num, cspacepath_t *uts, size_t *size_bits, uintptr_t *paddr) {

    allocman_t *root_allocator = (allocman_t *)init_allocator->root_allocator; 
    void *ut_manager = init_allocator->ut_manager[domain]; 
    return _utspace_split_add_uts(root_allocator, ut_manager, num, uts, size_bits, paddr, ALLOCMAN_UT_KERNEL); 

}

static seL4_Word color_utspace_alloc_object_at(color_allocator_t *init_allocator, size_t domain, size_t size_bits, seL4_Word type, cspacepath_t *dest, uintptr_t paddr, int *error) {

    allocman_t *root_allocator = (allocman_t *)init_allocator->root_allocator; 
    void *ut_manager = init_allocator->ut_manager[domain]; 

    return _utspace_split_alloc(root_allocator, ut_manager, size_bits, type, dest, 
            paddr, 0, error);
}


static seL4_Word color_utspace_alloc_object(color_allocator_t *init_allocator, size_t domain, size_t size_bits, seL4_Word type, cspacepath_t *dest, int *error) {

    allocman_t *root_allocator = (allocman_t *)init_allocator->root_allocator; 
    void *ut_manager = init_allocator->ut_manager[domain]; 

    return _utspace_split_alloc(root_allocator, ut_manager, size_bits, type, dest, 
            ALLOCMAN_NO_PADDR, 0, error);
}


static void color_utspace_free_object(color_allocator_t *init_allocator, size_t domain, seL4_Word cookie, size_t size_bits) {

    allocman_t *root_allocator = (allocman_t *)init_allocator->root_allocator; 
    void *ut_manager = init_allocator->ut_manager[domain]; 

    _utspace_split_free(root_allocator, ut_manager, cookie, size_bits);

}

/*

   refill memory from the root allocator
 
 */
static int color_utspace_refill(color_allocator_t *allocator, uintptr_t target_paddr) {


    color_allocator_t *init_allocator = allocator->init_allocator; 
    allocman_t *root_allocator = (allocman_t *)init_allocator->root_allocator; 
    cspacepath_t slot, color_slot;
    int error = 0;
    seL4_Word ret; 
    uintptr_t paddr; 
    size_t size_bits;
    size_t i; 

    /*allocate a slot*/
    error = allocman_cspace_alloc(root_allocator, &slot); 
    if (error)
        return error; 

    /*align the target paddr to the block that contains all colours*/
    if (target_paddr != ALLOCMAN_NO_PADDR) {
        target_paddr &= ~(BIT(CONFIG_COLOUR_ALLOC_SIZEBITS) - 1); 
    }

    ret = allocman_utspace_alloc_at(root_allocator, CONFIG_COLOUR_ALLOC_SIZEBITS, seL4_UntypedObject, &slot, target_paddr, 0, &error);
    if (error) {
        allocman_cspace_free(root_allocator, &slot);
        return error;
    }

    paddr = allocman_utspace_paddr(root_allocator, ret, CONFIG_COLOUR_ALLOC_SIZEBITS);

    assert(IS_ALIGNED(paddr, CONFIG_COLOUR_ALLOC_SIZEBITS));

    /*split the untypeds into N untypeds*/
    for (i = 0; i < init_allocator->num_domains; i++) {
        
        /*untype the object according to the division*/
        for (int colour_bit = CONFIG_COLOUR_ALLOC_SIZEBITS - seL4_PageBits; 
                colour_bit >= 0; colour_bit--) {
            
            if (init_allocator->div[i] & BIT(colour_bit)) {

                /*allocate a slot*/
                error = allocman_cspace_alloc(root_allocator, &color_slot); 
                if (error)
                    return error; 

                size_bits = colour_bit + seL4_PageBits; 

                error = seL4_Untyped_Retype(slot.capPtr, seL4_UntypedObject, size_bits, color_slot.root, color_slot.dest, color_slot.destDepth, color_slot.offset, 1);

                if (error != seL4_NoError) {
                    allocman_cspace_free(root_allocator, &color_slot);
                    return error;
                }
                /*adding the untyped object into its ut_manager*/
                error =  color_utspace_add_uts(init_allocator, i, 1, &color_slot, &size_bits, &paddr); 
                if (error) {
                    allocman_cspace_free(root_allocator, &color_slot);
                    return error;
                }
                
                paddr += BIT(colour_bit + seL4_PageBits);
            }
        }
    }
    return 0;
}


/**
 * Allocate a portion of an untyped into an object at a particular phy address
 *
 * @param data cookie for the underlying allocator
 * @param dest path to an empty cslot to place the cap to the allocated object
 * @param type the seL4 object type to allocate (as passed to Untyped_Retype)
 * @param res pointer to a location to store the cookie representing this allocation
 * @return 0 on success
 */
int color_utspace_alloc_at(color_allocator_t *allocator, cspacepath_t *dest, seL4_Word type, seL4_Word size_bits, uintptr_t paddr, seL4_Word *ret) {

    int error = 0;
    size_t color_num = allocator->domain; 
    color_allocator_t *init_allocator = allocator->init_allocator; 
    
    assert(color_num <= CONFIG_NUM_CACHE_COLOURS); 
    assert(init_allocator); 
    
    /*try to allocate from the utspace first*/
    *ret = color_utspace_alloc_object_at(init_allocator, color_num, size_bits, type, dest, paddr, &error);
    if (!error)  {
        return error; 
    }

    /*refill the untyped object into the ut mangers*/
    error = color_utspace_refill(allocator, paddr); 
 
    if (error) 
        return error; 

    /*try to allocate from the utspace again*/
    *ret = color_utspace_alloc_object_at(init_allocator, color_num, size_bits, type, dest, paddr, &error);
    return error; 

}

/**
 * Allocate a portion of an untyped into an object
 *
 * @param data cookie for the underlying allocator
 * @param dest path to an empty cslot to place the cap to the allocated object
 * @param type the seL4 object type to allocate (as passed to Untyped_Retype)
 * @param res pointer to a location to store the cookie representing this allocation
 * @return 0 on success
 */
int color_utspace_alloc(color_allocator_t *allocator, cspacepath_t *dest, seL4_Word type, seL4_Word size_bits, seL4_Word *ret) {

    int error = 0;
    size_t color_num = allocator->domain; 
    color_allocator_t *init_allocator = allocator->init_allocator; 
    
    assert(color_num <= CONFIG_NUM_CACHE_COLOURS); 
    assert(init_allocator); 
    
    /*try to allocate from the utspace first*/
    *ret = color_utspace_alloc_object(init_allocator, color_num, size_bits, type, dest, &error);
    if (!error)  {
        return error; 
    }

    /*refill the untyped object into the ut mangers*/
    error = color_utspace_refill(allocator, ALLOCMAN_NO_PADDR); 
 
    if (error) 
        return error; 

    /*try to allocate from the utspace again*/
    *ret = color_utspace_alloc_object(init_allocator, color_num, size_bits, type, dest, &error);
    return error; 

}


/**
 * Free a portion of an allocated untyped. Is the responsibility of the caller to
 * have already deleted the object (by deleting all capabilities) first
 * @param target cookie to the allocation as given by the utspace alloc function
 */
void color_utspace_free(color_allocator_t *allocator, seL4_Word cookie, seL4_Word size_bits) {

    size_t color_num = allocator->domain; 
    color_allocator_t *init_allocator = allocator->init_allocator; 
   
    color_utspace_free_object(init_allocator, color_num, cookie, size_bits);

}


uintptr_t color_utspace_paddr(color_allocator_t *allocator,
         seL4_Word cookie, seL4_Word size_bits) {
     
    size_t color_num = allocator->domain; 
    color_allocator_t *init_allocator = allocator->init_allocator; 

    void *ut_manager = init_allocator->ut_manager[color_num]; 

    return _utspace_split_paddr(ut_manager, cookie, size_bits);
}


