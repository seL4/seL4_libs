/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __VKA_OBJECT_H__
#define __VKA_OBJECT_H__

#include <assert.h>
#include <vka/vka.h>
#include <vka/kobject_t.h>
#include <stdio.h>

/*
 * A wrapper to hold all the allocation information for an 'object'
 * An object here is just combination of cptr and untyped allocation
 * The type and size of the allocation is also stored to make free
 * more convenient.
 */

typedef struct vka_object {
    seL4_CPtr cptr;
    uint32_t ut;
    seL4_Word type;
    seL4_Word size_bits;
} vka_object_t;

/*
 * Generic object allocator used by functions below, can also be used directly
 */
static inline int vka_alloc_object(vka_t *vka, seL4_Word type, seL4_Word size_bits, vka_object_t *result)
{
    seL4_CPtr cptr;
    uint32_t ut;
    int error;
    cspacepath_t path;
    assert(vka);
    assert(result);
    if ( (error = vka_cspace_alloc(vka, &cptr)) != 0) {
        return error;
    }
    vka_cspace_make_path(vka, cptr, &path);
    if ( (error = vka_utspace_alloc(vka, &path, type, size_bits, &ut)) != 0) {
        fprintf(stderr, "Failed to allocate object of size %lu, error %d\n", BIT(size_bits), error); 
        vka_cspace_free(vka, cptr);
        return error;
    }
    result->cptr = cptr;
    result->ut = ut;
    result->type = type;
    result->size_bits = size_bits;
    return 0;
}

static inline seL4_CPtr vka_alloc_object_leaky(vka_t *vka, seL4_Word type, seL4_Word size_bits)
{
    vka_object_t result = {.cptr = 0, .ut = 0, .type = 0, size_bits = 0};
    return vka_alloc_object(vka, type, size_bits, &result) == -1 ? 0 : result.cptr;
}

static inline void vka_free_object(vka_t *vka, vka_object_t *object)
{
    cspacepath_t path;
    assert(vka);
    assert(object);
    vka_cspace_make_path(vka, object->cptr, &path);
    /* ignore any errors */
    seL4_CNode_Delete(path.root, path.capPtr, path.capDepth);
    vka_cspace_free(vka, object->cptr);
    vka_utspace_free(vka, object->type, object->size_bits, object->ut);
}

static inline uintptr_t vka_object_paddr(vka_t *vka, vka_object_t *object)
{
    return vka_utspace_paddr(vka, object->ut, object->type, object->size_bits);
}

/* Convenience wrappers for allocating objects */
static inline int vka_alloc_untyped(vka_t *vka, uint32_t size_bits, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_UntypedObject, size_bits, result);
}
static inline int vka_alloc_tcb(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_TCBObject, seL4_TCBBits, result);
}
static inline int vka_alloc_endpoint(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_EndpointObject, seL4_EndpointBits, result);
}
static inline int vka_alloc_async_endpoint(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_AsyncEndpointObject, seL4_EndpointBits, result);
}
static inline int vka_alloc_cnode_object(vka_t *vka, uint32_t slot_bits, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_CapTableObject, slot_bits, result);
}
#ifdef CONFIG_EDF
static inline int vka_alloc_sched_context(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, seL4_SchedContextObject, seL4_SchedContextBits, result);
}
#endif /* CONFIG_EDF */

/*resource allocation interfaces for virtual machine extensions on x86*/
#ifdef CONFIG_VTX
static inline int vka_alloc_vcpu (vka_t *vka, vka_object_t *result) {
    return vka_alloc_object(vka, seL4_IA32_VCPUObject, seL4_IA32_VCPUBits, result);
}

static inline int vka_alloc_ept_page_directory_pointer_table (vka_t *vka, vka_object_t *result) {
    return vka_alloc_object(vka, seL4_IA32_EPTPageDirectoryPointerTableObject, seL4_IA32_EPTPageDirectoryPointerTableBits, result);
}

static inline int vka_alloc_ept_page_directory (vka_t *vka, vka_object_t *result) {
    return vka_alloc_object(vka, seL4_IA32_EPTPageDirectoryObject, seL4_IA32_EPTPageDirectoryBits, result);
}

static inline int vka_alloc_ept_page_table (vka_t *vka, vka_object_t *result) {
    return vka_alloc_object(vka, seL4_IA32_EPTPageTableObject, seL4_IA32_EPTPageTableBits, result);
}
#endif /* CONFIG_VTX */

/*resource allocation interfaces for virtual machine extensions on ARM */
#ifdef ARM_HYP
static inline int vka_alloc_vcpu(vka_t *vka, vka_object_t *result) {
    return vka_alloc_object(vka, seL4_ARM_VCPUObject, seL4_ARM_VCPUBits, result);
}
#endif /* ARM_HYP */


#ifdef CONFIG_IOMMU
static inline int vka_alloc_io_page_table(vka_t *vka, vka_object_t *result) {
    return vka_alloc_object(vka, seL4_IA32_IOPageTableObject, seL4_IOPageTableBits, result);
}
#endif

/* For arch specific allocations we call upon kobject to avoid code duplication */
static inline int vka_alloc_frame(vka_t *vka, uint32_t size_bits, vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(KOBJECT_FRAME, size_bits), size_bits, result);
}

#ifdef CONFIG_X86_64

static inline int vka_alloc_page_map_level4(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(KOBJECT_PAGE_MAP_LEVEL4, 0), seL4_PageMapLevel4Bits, result);
}

static inline int vka_alloc_page_directory_pointer_table(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(KOBJECT_PAGE_DIRECTORY_POINTER_TABLE, 0), seL4_PageDirPointerTableBits, result);
}

#endif

static inline int vka_alloc_page_directory(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(KOBJECT_PAGE_DIRECTORY, 0), seL4_PageDirBits, result);
}

static inline int vka_alloc_page_table(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(KOBJECT_PAGE_TABLE, 0), seL4_PageTableBits, result);
}


#ifdef CONFIG_CACHE_COLORING 

static inline int vka_alloc_kernel_image(vka_t *vka, vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(KOBJECT_KERNEL_IMAGE, 0), seL4_KernelImageBits, result);
}


#endif 


/* Implement a kobject interface */
static inline int vka_alloc_kobject(vka_t *vka, kobject_t type, seL4_Word size_bits,
        vka_object_t *result)
{
    return vka_alloc_object(vka, kobject_get_type(type, size_bits), size_bits, result);
}


/* leaky versions of the object allocation functions - throws away the kobject_t */

#define LEAKY(name) \
    static inline seL4_CPtr vka_alloc_##name##_leaky(vka_t *vka) \
{\
    vka_object_t object;\
    if (vka_alloc_##name(vka, &object) != 0) {\
        return 0;\
    }\
    return object.cptr;\
}

LEAKY(tcb)
LEAKY(endpoint)
LEAKY(async_endpoint)
LEAKY(page_directory)
LEAKY(page_table)
#ifdef CONFIG_X86_64
LEAKY(page_map_level4)
LEAKY(page_directory_pointer_table)
#endif
#ifdef CONFIG_EDF
LEAKY(sched_context)
#endif /* CONFIG_EDF */
#ifdef CONFIG_VTX
LEAKY(vcpu)
LEAKY(ept_page_directory_pointer_table)
LEAKY(ept_page_directory)
LEAKY(ept_page_table)
#endif /* CONFIG_VTX */
#ifdef CONFIG_IOMMU
LEAKY(io_page_table)
#endif
#ifdef ARM_HYP
LEAKY(vcpu)
#endif


#define LEAKY_SIZE_BITS(name) \
    static inline seL4_CPtr vka_alloc_##name##_leaky(vka_t *vka, uint32_t size_bits) \
{\
    vka_object_t object;\
    if (vka_alloc_##name(vka, size_bits, &object) != 0) {\
        return 0;\
    }\
    return object.cptr;\
}

LEAKY_SIZE_BITS(untyped)
LEAKY_SIZE_BITS(frame)
LEAKY_SIZE_BITS(cnode_object)

/*
 * Get the size (in bits) of the untyped memory required to create an object of
 * the given size.
 */
static inline unsigned long
vka_get_object_size(seL4_Word objectType, seL4_Word objectSize)
{
    switch (objectType) {
        /* Generic objects. */
    case seL4_UntypedObject:
        return objectSize;
    case seL4_TCBObject:
        return seL4_TCBBits;
    case seL4_EndpointObject:
        return seL4_EndpointBits;
    case seL4_AsyncEndpointObject:
        return seL4_EndpointBits;
    case seL4_CapTableObject:
        return (seL4_SlotBits + objectSize);
#ifdef CONFIG_CACHE_COLORING 
    case seL4_KernelImageObject:
        return seL4_KernelImageBits;
#endif 
#ifdef CONFIG_EDF
    case seL4_SchedContextObject:
        return seL4_SchedContextBits;
#endif /* CONFIG_EDF */
#if defined(ARCH_ARM)
        /* ARM-specific objects. */
    case seL4_ARM_SmallPageObject:
        return seL4_PageBits;
    case seL4_ARM_LargePageObject:
        return 16;
#ifdef ARM_HYP
    case seL4_ARM_SectionObject:
        return 21;
    case seL4_ARM_SuperSectionObject:
        return 25;
    case seL4_ARM_VCPUObject:
        return seL4_ARM_VCPUBits;
#else /* ARM_HYP */
    case seL4_ARM_SectionObject:
        return 20;
    case seL4_ARM_SuperSectionObject:
        return 24;
#endif /* ARM_HYP */
    case seL4_ARM_PageTableObject:
        return seL4_PageTableBits;
    case seL4_ARM_PageDirectoryObject:
        return seL4_PageDirBits;
#elif defined(ARCH_IA32) /* ARCH_ARM */
#ifdef CONFIG_X86_64
    case seL4_X64_4K:
        return seL4_PageBits;
    case seL4_X64_2M:
        return 21;
    case seL4_X64_PageTableObject:
        return seL4_PageTableBits;
    case seL4_X64_PageDirectoryObject:
        return seL4_PageDirBits;
    case seL4_X64_PageDirectoryPointerTableObject:
        return seL4_PageDirPointerTableBits;
    case seL4_X64_PageMapLevel4Object:
        return seL4_PageMapLevel4Bits;

#else /* X86_64 */
        /* IA32specific objects. */
    case seL4_IA32_4K:
        return seL4_PageBits;
    case seL4_IA32_4M:
        return seL4_4MBits;
    case seL4_IA32_PageTableObject:
        return seL4_PageTableBits;
    case seL4_IA32_PageDirectoryObject:
        return seL4_PageDirBits;

#ifdef CONFIG_VTX
    case seL4_IA32_VCPUObject:
        return seL4_IA32_VCPUBits;
    case seL4_IA32_EPTPageDirectoryPointerTableObject:
        return seL4_IA32_EPTPageDirectoryPointerTableBits;
    case seL4_IA32_EPTPageDirectoryObject:
        return seL4_IA32_EPTPageDirectoryBits;
    case seL4_IA32_EPTPageTableObject:
        return seL4_IA32_EPTPageTableBits;
#endif

#ifdef CONFIG_IOMMU
    case seL4_IA32_IOPageTableObject:
        return seL4_IOPageTableBits;
#endif

#endif /* X86_64 */
#else
#error "Unknown arch."
#endif /* ARCH_IA32 */
    default:
        /* Unknown object type. */
        assert(0);
        return -1;
    }
}

#endif /* __VKA_OBJECT_H__ */
