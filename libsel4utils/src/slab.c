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

#include <sel4utils/slab.h>
#include <vka/capops.h>
#include <vka/object.h>
#include <utils/attribute.h>

typedef struct {
    /* number of objects in the slab */
    seL4_CPtr n;
    /* next free object in the slab */
    seL4_CPtr next;
    /* list of objects in the slab */
    vka_object_t *objects;
} slab_t;

typedef struct {
    /* allocator to delegate further allocations to */
    vka_t *delegate;
    /* allocation slab of each type */
    slab_t slabs[seL4_ObjectTypeCount];
    /* untyped to allocate from */
    vka_object_t untyped;
} slab_data_t;

typedef struct {
    ssize_t size_bits;
    seL4_Word type;
} object_desc_t;

static int compare_object_descs(const void *a, const void *b)
{
    const object_desc_t *obj_a = a;
    const object_desc_t *obj_b = b;
    /* order by biggest first */
    return obj_b->size_bits - obj_a->size_bits;
}

static int delegate_cspace_alloc(void *data, seL4_CPtr *res)
{
    slab_data_t *sdata = data;
    assert(data != NULL);
    assert(sdata->delegate != NULL);
    return vka_cspace_alloc(sdata->delegate, res);
}

static void delegate_cspace_make_path(void *data, seL4_CPtr slot, cspacepath_t *res)
{
    slab_data_t *sdata = data;
    vka_cspace_make_path(sdata->delegate, slot, res);
}

static void delegate_cspace_free(void *data, seL4_CPtr slot)
{
    slab_data_t *sdata = data;
    vka_cspace_free(sdata->delegate, slot);
}

static int slab_utspace_alloc(void *data, const cspacepath_t *dest, seL4_Word type,
        seL4_Word size_bits, seL4_Word *res)
{
    slab_data_t *sdata = data;

    if (type >= seL4_ObjectTypeCount) {
        return -1;
    }

    slab_t *slab = &sdata->slabs[type];
    if (slab->next == slab->n) {
        ZF_LOGW("Slab of type %lu expired, using delegate allocator", type);
        return vka_utspace_alloc(sdata->delegate, dest, type, size_bits, res);
    }

    cspacepath_t src;
    vka_cspace_make_path(sdata->delegate, slab->objects[slab->next].cptr, &src);
    if (vka_cnode_move(dest, &src) != seL4_NoError) {
        ZF_LOGW("Dest invalid\n");
        return -1;
    }

    slab->next++;
    return 0;
}

static int slab_utspace_alloc_maybe_device(void *data, const cspacepath_t *dest, seL4_Word type,
                                           seL4_Word size_bits, bool can_use_dev, seL4_Word *res)
{
    return slab_utspace_alloc(data, dest, type, size_bits, res);
}

static int delegate_utspace_alloc_at(void *data, const cspacepath_t *dest, seL4_Word type, seL4_Word size_bits,
                                     uintptr_t paddr, seL4_Word *res)
{
    slab_data_t *sdata = data;
    return vka_utspace_alloc_at(sdata->delegate, dest, type, size_bits, paddr, res);
}

static void
slab_utspace_free(void *data, seL4_Word type, seL4_Word size_bits, seL4_Word target)
{
    //TODO this should be possible - swap the object in at next, but do we need it?
    ZF_LOGW("slab_utspace_free not implemented");
}

static size_t calculate_total_size(size_t object_freq[seL4_ObjectTypeCount]) {
    size_t total_size = 0;
    for (int i = 0; i < seL4_ObjectTypeCount; i++) {
        if (object_freq[i] > 0) {
            size_t object_size = vka_get_object_size(i, 0);
            if (object_size == 0 || object_size == -1) {
                ZF_LOGE("Object of undetermined size passed to slab allocator");
                return 0;
            }
            total_size += (BIT(object_size)) * object_freq[i];
        }
    }
    return total_size;
}

static void slab_destroy(vka_t *slab)
{
    ZF_LOGW("Slab destroy not implemented");
}

static seL4_Error alloc_object(vka_t *delegate, vka_object_t *untyped, size_t size_bits, seL4_Word type,
                               vka_object_t *object)
{
    /* allocate slot for object */
    object->type = type;
    object->size_bits = size_bits;
    seL4_Error error = vka_cspace_alloc(delegate, &object->cptr);
    if (error != seL4_NoError) {
        ZF_LOGE("Failed to allocate cslot");
        return error;
    }

    /* convert to cspacepath */
    cspacepath_t path;
    vka_cspace_make_path(delegate, object->cptr, &path);

    /* retype object */
    return seL4_Untyped_Retype(untyped->cptr, type, size_bits, path.root, path.dest,
                                        path.destDepth, path.offset, 1);
}

static int alloc_object_slab(vka_t *delegate, vka_object_t *untyped, slab_t *slab, size_t n,
                             size_t size_bits, seL4_Word type)
{
    ZF_LOGI("Preallocating %zu objects of %zu size bits, %lu type\n", n, size_bits, (long) type);

    slab->n = n;
    slab->next = 0;

    if (n > 0) {
        slab->objects = calloc(n, sizeof(vka_object_t));
        if (slab->objects == NULL) {
            ZF_LOGI("Failed to allocate %zu objects of %zu size bits, %lu type", n, size_bits, (long) type);
            return -1;
        }
    }

    for (int i = 0; i < slab->n; i++) {
        if (alloc_object(delegate, untyped, size_bits, type, &slab->objects[i]) != seL4_NoError) {
            return -1;
        }
    }

    /* success */
    return 0;
}

int slab_init(vka_t *slab_vka, vka_t *delegate, size_t object_freq[seL4_ObjectTypeCount]) {

    slab_data_t *data = calloc(1, sizeof(slab_data_t));

    if (!data) {
        slab_destroy(slab_vka);
        return -1;
    }

    slab_vka->data = data;
    data->delegate = delegate;

    slab_vka->cspace_alloc = delegate_cspace_alloc;
    slab_vka->cspace_make_path = delegate_cspace_make_path;
    slab_vka->cspace_free = delegate_cspace_free;
    slab_vka->utspace_alloc_at = delegate_utspace_alloc_at;
    slab_vka->utspace_alloc = slab_utspace_alloc;
    slab_vka->utspace_alloc_maybe_device = slab_utspace_alloc_maybe_device;
    slab_vka->utspace_free = slab_utspace_free;

    /* allocate untyped */
    size_t total_size = calculate_total_size(object_freq);
    size_t total_size_bits = seL4_WordBits - CLZL(total_size);
    /* sanity */
    assert(BIT(total_size_bits) >= total_size);
    if (total_size == 0) {
        ZF_LOGW("No objects to allocate\n");
        return 0;
    }

    int error = vka_alloc_untyped(delegate, total_size_bits, &data->untyped);
    if (error != 0) {
        ZF_LOGE("Failed to allocate untyped of size bits %zu\n", total_size_bits);
        slab_destroy(slab_vka);
        return -1;
    }

    /* order object sizes by size */
    object_desc_t object_descs[seL4_ObjectTypeCount];
    for (int i = 0; i < seL4_ObjectTypeCount; i++) {
        object_descs[i].type = i;
        object_descs[i].size_bits = vka_get_object_size(i, 0);
    }

    qsort(object_descs, seL4_ObjectTypeCount, sizeof(object_desc_t), compare_object_descs);

    /* allocate slabs */
    for (int i = 0; i < seL4_ObjectTypeCount && object_descs[i].size_bits != 0; i++) {
        int type = object_descs[i].type;
        error = alloc_object_slab(delegate, &data->untyped, &data->slabs[type],
                                  object_freq[type], object_descs[i].size_bits, type);
        if (error != 0) {
            ZF_LOGE("Failed to create slab\n");
            slab_destroy(slab_vka);
            return -1;
        }
    }

    return 0;
}
