#ifndef _COLOR_ALLOCATOR_H_
#define _COLOR_ALLOCATOR_H_ 

#include <sel4cachecolour/gen_config.h>
#include <sel4utils/vspace.h>
#include <sel4utils/process.h>
#include <vka/object.h>
#include <simple/simple.h>

/*magic number for the init domain*/
#define COLOR_INIT_DOMAIN   0x1234



/*The cache coloring allocator*/

typedef struct color_allocator {

    void *root_allocator;       /*the "real" allocator*/
    size_t num_colors;    
    size_t num_domains;   /*colored domains*/
    size_t div[CONFIG_NUM_CACHE_COLOURS];     /*dividing memory among domains*/
    size_t domain;        /*domain number*/
    void *ut_manager[CONFIG_NUM_CACHE_COLOURS];   /*untype manager for colored domains*/
    struct color_allocator *init_allocator;   /*the init allocator created by root task*/
}  __attribute__ ((aligned (64))) color_allocator_t;  



/*external interfaces*/
size_t color_get_num_colors(void);

/*creating init allocator with simple */
color_allocator_t *color_create_init_allocator_use_simple(
        simple_t *simple, 
        size_t pool_size, char *pool, size_t num_domains, size_t div[]); 


/*configuring the init allocator*/
void color_config_init_allocator(color_allocator_t *allocator, void *vstart, size_t vsize, seL4_CPtr pd);

/*creating an allocator for a colored domain*/
color_allocator_t *color_create_domain_allocator(color_allocator_t *init_allocator, size_t color_num); 

/*manipulating ut space*/
int color_utspace_alloc(color_allocator_t *allocator, cspacepath_t *dest, seL4_Word type, seL4_Word size_bits, seL4_Word *ret); 

int color_utspace_alloc_at(color_allocator_t *allocator, cspacepath_t *dest, seL4_Word type, seL4_Word size_bits, uintptr_t paddr,  seL4_Word *ret); 


void color_utspace_free(color_allocator_t *allocator, seL4_Word cookie, seL4_Word size_bits); 

/*get phycial address of an allocated objects*/ 
uintptr_t color_utspace_paddr(color_allocator_t *allocator,
         seL4_Word cookie, seL4_Word size_bits);
 

/*making vka interfaces*/
void color_make_vka(vka_t *vka, color_allocator_t *allocator); 

/*allocat from the mspace, currently only support allocating from the root 
 allocator, the mspace is not coloured*/
void *color_mspace_alloc(color_allocator_t *alloc, size_t size, int *_error);


#endif /*_COLOR_ALLOCATOR_H_*/ 


