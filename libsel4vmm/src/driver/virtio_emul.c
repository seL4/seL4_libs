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

#include <autoconf.h>
#include <sel4vmm/gen_config.h>

#include <string.h>

#include <vmm/driver/virtio_emul.h>
#include <ethdrivers/virtio/virtio_pci.h>
#include <ethdrivers/virtio/virtio_net.h>
#include <ethdrivers/virtio/virtio_ring.h>
#include <ethdrivers/virtio/virtio_config.h>

#define RX_QUEUE 0
#define TX_QUEUE 1

#define BUF_SIZE 2048

typedef struct ethif_virtio_emul_internal {
    struct eth_driver driver;
    int status;
    uint8_t mac[6];
    uint16_t queue;
    struct vring vring[2];
    uint16_t queue_size[2];
    uint32_t queue_pfn[2];
    uint16_t last_idx[2];
    vspace_t guest_vspace;
    ps_dma_man_t dma_man;
} ethif_virtio_emul_internal_t;

typedef struct emul_tx_cookie {
    uint16_t desc_head;
    void *vaddr;
} emul_tx_cookie_t;

static int read_guest_mem(uintptr_t phys, void *vaddr, size_t size, size_t offset, void *cookie)
{
    memcpy(cookie + offset, vaddr, size);
    return 0;
}

static int write_guest_mem(uintptr_t phys, void *vaddr, size_t size, size_t offset, void *cookie)
{
    memcpy(vaddr, cookie + offset, size);
    return 0;
}

static uint16_t ring_avail_idx(ethif_virtio_emul_t *emul, struct vring *vring)
{
    uint16_t idx;
    vmm_guest_vspace_touch(&emul->internal->guest_vspace, (uintptr_t)&vring->avail->idx, sizeof(vring->avail->idx),
                           read_guest_mem, &idx);
    return idx;
}

static uint16_t ring_avail(ethif_virtio_emul_t *emul, struct vring *vring, uint16_t idx)
{
    uint16_t elem;
    vmm_guest_vspace_touch(&emul->internal->guest_vspace, (uintptr_t) & (vring->avail->ring[idx % vring->num]),
                           sizeof(elem), read_guest_mem, &elem);
    return elem;
}

static struct vring_desc ring_desc(ethif_virtio_emul_t *emul, struct vring *vring, uint16_t idx)
{
    struct vring_desc desc;
    vmm_guest_vspace_touch(&emul->internal->guest_vspace, (uintptr_t) & (vring->desc[idx]), sizeof(desc), read_guest_mem,
                           &desc);
    return desc;
}

static void ring_used_add(ethif_virtio_emul_t *emul, struct vring *vring, struct vring_used_elem elem)
{
    uint16_t guest_idx;
    vmm_guest_vspace_touch(&emul->internal->guest_vspace, (uintptr_t)&vring->used->idx, sizeof(vring->used->idx),
                           read_guest_mem, &guest_idx);
    vmm_guest_vspace_touch(&emul->internal->guest_vspace, (uintptr_t)&vring->used->ring[guest_idx % vring->num],
                           sizeof(elem), write_guest_mem, &elem);
    guest_idx++;
    vmm_guest_vspace_touch(&emul->internal->guest_vspace, (uintptr_t)&vring->used->idx, sizeof(vring->used->idx),
                           write_guest_mem, &guest_idx);
}

static uintptr_t emul_allocate_rx_buf(void *iface, size_t buf_size, void **cookie)
{
    ethif_virtio_emul_t *emul = (ethif_virtio_emul_t *)iface;
    ethif_virtio_emul_internal_t *net = emul->internal;
    if (buf_size > BUF_SIZE) {
        return 0;
    }
    void *vaddr = ps_dma_alloc(&net->dma_man, BUF_SIZE, net->driver.dma_alignment, 1, PS_MEM_NORMAL);
    if (!vaddr) {
        return 0;
    }
    uintptr_t phys = ps_dma_pin(&net->dma_man, vaddr, BUF_SIZE);
    *cookie = vaddr;
    return phys;
}

static void emul_rx_complete(void *iface, unsigned int num_bufs, void **cookies, unsigned int *lens)
{
    ethif_virtio_emul_t *emul = (ethif_virtio_emul_t *)iface;
    ethif_virtio_emul_internal_t *net = emul->internal;
    unsigned int tot_len = 0;
    int i;
    struct vring *vring = &net->vring[RX_QUEUE];
    for (i = 0; i < num_bufs; i++) {
        tot_len += lens[i];
    }
    /* grab the next receive chain */
    struct virtio_net_hdr virtio_hdr;
    memset(&virtio_hdr, 0, sizeof(virtio_hdr));
    uint16_t guest_idx = ring_avail_idx(emul, vring);
    uint16_t idx = net->last_idx[RX_QUEUE];
    if (idx != guest_idx) {
        /* total length of the written packet so far */
        size_t tot_written = 0;
        /* amount of the current descriptor written */
        size_t desc_written = 0;
        /* how much we have written of the current buffer */
        size_t buf_written = 0;
        /* the current buffer. -1 indicates the virtio net buffer */
        int current_buf = -1;
        uint16_t desc_head = ring_avail(emul, vring, idx);
        /* start walking the descriptors */
        struct vring_desc desc;
        uint16_t desc_idx = desc_head;
        do {
            desc = ring_desc(emul, vring, desc_idx);
            /* determine how much we can copy */
            uint32_t copy;
            void *buf_base = NULL;
            if (current_buf == -1) {
                copy = sizeof(struct virtio_net_hdr) - buf_written;
                buf_base = &virtio_hdr;
            } else {
                copy = lens[current_buf] - buf_written;
                buf_base = cookies[current_buf];
            }
            copy = MIN(copy, desc.len - desc_written);
            /* copy it */
            vmm_guest_vspace_touch(&net->guest_vspace, (uintptr_t)desc.addr + desc_written, copy, write_guest_mem,
                                   buf_base + buf_written);
            /* update amounts */
            tot_written += copy;
            desc_written += copy;
            buf_written += copy;
            /* see what's gone over */
            if (desc_written == desc.len) {
                if (!desc.flags & VRING_DESC_F_NEXT) {
                    /* descriptor chain is too short to hold the whole packet.
                     * just truncate */
                    break;
                }
                desc_idx = desc.next;
                desc_written = 0;
            }
            if (current_buf == -1) {
                if (buf_written == sizeof(struct virtio_net_hdr)) {
                    current_buf++;
                    buf_written = 0;
                }
            } else {
                if (buf_written == lens[current_buf]) {
                    current_buf++;
                    buf_written = 0;
                }
            }
        } while (current_buf < num_bufs);
        /* now put it in the used ring */
        struct vring_used_elem used_elem = {desc_head, tot_written};
        ring_used_add(emul, vring, used_elem);

        /* record that we've used this descriptor chain now */
        net->last_idx[RX_QUEUE]++;
        /* notify the guest that there is something in its used ring */
        net->driver.i_fn.raw_handleIRQ(&net->driver, 0);
    }
    for (i = 0; i < num_bufs; i++) {
        ps_dma_unpin(&net->dma_man, cookies[i], BUF_SIZE);
        ps_dma_free(&net->dma_man, cookies[i], BUF_SIZE);
    }
}

static void emul_tx_complete(void *iface, void *cookie)
{
    ethif_virtio_emul_t *emul = (ethif_virtio_emul_t *)iface;
    ethif_virtio_emul_internal_t *net = emul->internal;
    emul_tx_cookie_t *tx_cookie = (emul_tx_cookie_t *)cookie;
    /* free the dma memory */
    ps_dma_unpin(&net->dma_man, tx_cookie->vaddr, BUF_SIZE);
    ps_dma_free(&net->dma_man, tx_cookie->vaddr, BUF_SIZE);
    /* put the descriptor chain into the used list */
    struct vring_used_elem used_elem = {tx_cookie->desc_head, 0};
    ring_used_add(emul, &net->vring[TX_QUEUE], used_elem);
    free(tx_cookie);
    /* notify the guest that we have completed some of its buffers */
    net->driver.i_fn.raw_handleIRQ(&net->driver, 0);
}

static void emul_notify_tx(ethif_virtio_emul_t *emul)
{
    ethif_virtio_emul_internal_t *net = emul->internal;
    struct vring *vring = &net->vring[TX_QUEUE];
    /* read the index */
    uint16_t guest_idx = ring_avail_idx(emul, vring);
    /* process what we can of the ring */
    uint16_t idx = net->last_idx[TX_QUEUE];
    while (idx != guest_idx) {
        uint16_t desc_head;
        /* read the head of the descriptor chain */
        desc_head = ring_avail(emul, vring, idx);
        /* allocate a packet */
        void *vaddr = ps_dma_alloc(&net->dma_man, BUF_SIZE, net->driver.dma_alignment, 1, PS_MEM_NORMAL);
        if (!vaddr) {
            /* try again later */
            break;
        }
        uintptr_t phys = ps_dma_pin(&net->dma_man, vaddr, BUF_SIZE);
        assert(phys);
        /* length of the final packet to deliver */
        uint32_t len = 0;
        /* we want to skip the initial virtio header, as this should
         * not be sent to the actual ethernet driver. This records
         * how much we have skipped so far. */
        uint32_t skipped = 0;
        /* start walking the descriptors */
        struct vring_desc desc;
        uint16_t desc_idx = desc_head;
        do {
            desc = ring_desc(emul, vring, desc_idx);
            uint32_t skip = 0;
            /* if we haven't yet skipped the full virtio net header, work
             * out how much of this descriptor should be skipped */
            if (skipped < sizeof(struct virtio_net_hdr)) {
                skip = MIN(sizeof(struct virtio_net_hdr) - skipped, desc.len);
                skipped += skip;
            }
            /* truncate packets that are too large */
            uint32_t this_len = desc.len - skip;
            this_len = MIN(BUF_SIZE - len, this_len);
            vmm_guest_vspace_touch(&net->guest_vspace, (uintptr_t)desc.addr + skip, this_len, read_guest_mem, vaddr + len);
            len += this_len;
            desc_idx = desc.next;
        } while (desc.flags & VRING_DESC_F_NEXT);
        /* ship it */
        emul_tx_cookie_t *cookie = malloc(sizeof(*cookie));
        assert(cookie);
        cookie->desc_head = desc_head;
        cookie->vaddr = vaddr;
        int result = net->driver.i_fn.raw_tx(&net->driver, 1, &phys, &len, cookie);
        switch (result) {
        case ETHIF_TX_COMPLETE:
            emul_tx_complete(emul, cookie);
            break;
        case ETHIF_TX_FAILED:
            ps_dma_unpin(&net->dma_man, vaddr, BUF_SIZE);
            ps_dma_free(&net->dma_man, vaddr, BUF_SIZE);
            free(cookie);
            break;
        }
        /* next */
        idx++;
    }
    /* update which parts of the ring we have processed */
    net->last_idx[TX_QUEUE] = idx;
}

static void emul_tx_complete_external(void *iface, void *cookie)
{
    emul_tx_complete(iface, cookie);
    /* space may have cleared for additional transmits */
    emul_notify_tx(iface);
}

static struct raw_iface_callbacks emul_callbacks = {
    .tx_complete = emul_tx_complete_external,
    .rx_complete = emul_rx_complete,
    .allocate_rx_buf = emul_allocate_rx_buf
};

static int emul_io_in(struct ethif_virtio_emul *emul, unsigned int offset, unsigned int size, unsigned int *result)
{
    switch (offset) {
    case VIRTIO_PCI_HOST_FEATURES:
        assert(size == 4);
        *result = BIT(VIRTIO_NET_F_MAC);
        break;
    case VIRTIO_PCI_STATUS:
        assert(size == 1);
        *result = emul->internal->status;
        break;
    case VIRTIO_PCI_QUEUE_NUM:
        assert(size == 2);
        *result = emul->internal->queue_size[emul->internal->queue];
        break;
    case 0x14 ... 0x19:
        assert(size == 1);
        *result = emul->internal->mac[offset - 0x14];
        break;
    case VIRTIO_PCI_QUEUE_PFN:
        assert(size == 4);
        *result = emul->internal->queue_pfn[emul->internal->queue];
        break;
    case VIRTIO_PCI_ISR:
        assert(size == 1);
        *result = 1;
        break;
    default:
        printf("Unhandled offset of 0x%x of size %d, reading\n", offset, size);
        assert(!"panic");
    }
    return 0;
}

static int emul_io_out(struct ethif_virtio_emul *emul, unsigned int offset, unsigned int size, unsigned int value)
{
    switch (offset) {
    case VIRTIO_PCI_GUEST_FEATURES:
        assert(size == 4);
        assert(value == BIT(VIRTIO_NET_F_MAC));
        break;
    case VIRTIO_PCI_STATUS:
        assert(size == 1);
        emul->internal->status = value & 0xff;
        break;
    case VIRTIO_PCI_QUEUE_SEL:
        assert(size == 2);
        emul->internal->queue = (value & 0xffff);
        assert(emul->internal->queue == 0 || emul->internal->queue == 1);
        break;
    case VIRTIO_PCI_QUEUE_PFN: {
        assert(size == 4);
        int queue = emul->internal->queue;
        emul->internal->queue_pfn[queue] = value;
        vring_init(&emul->internal->vring[queue], emul->internal->queue_size[queue], (void *)(uintptr_t)(value << 12),
                   VIRTIO_PCI_VRING_ALIGN);
        break;
    }
    case VIRTIO_PCI_QUEUE_NOTIFY:
        if (value == RX_QUEUE) {
            /* Currently RX packets will just get dropped if there was no space
             * so we will never have work to do if the client suddenly adds
             * more buffers */
        } else if (value == TX_QUEUE) {
            emul_notify_tx(emul);
        }
        break;
    default:
        printf("Unhandled offset of 0x%x of size %d, writing 0x%x\n", offset, size, value);
        assert(!"panic");
    }
    return 0;
}

static int emul_notify(ethif_virtio_emul_t *emul)
{
    if (emul->internal->status != VIRTIO_CONFIG_S_DRIVER_OK) {
        return -1;
    }
    emul_notify_tx(emul);
    return 0;
}

ethif_virtio_emul_t *ethif_virtio_emul_init(ps_io_ops_t io_ops, int queue_size, vspace_t *guest_vspace,
                                            ethif_driver_init driver, void *config)
{
    ethif_virtio_emul_t *emul = NULL;
    ethif_virtio_emul_internal_t *internal = NULL;
    int err;
    emul = malloc(sizeof(*emul));
    internal = malloc(sizeof(*internal));
    if (!emul || !internal) {
        goto error;
    }
    memset(emul, 0, sizeof(*emul));
    memset(internal, 0, sizeof(*internal));
    emul->internal = internal;
    emul->io_in = emul_io_in;
    emul->io_out = emul_io_out;
    emul->notify = emul_notify;
    internal->queue_size[RX_QUEUE] = queue_size;
    internal->queue_size[TX_QUEUE] = queue_size;
    /* create dummy rings. we never actually dereference the rings so they can be null */
    vring_init(&internal->vring[RX_QUEUE], emul->internal->queue_size[RX_QUEUE], 0, VIRTIO_PCI_VRING_ALIGN);
    vring_init(&internal->vring[TX_QUEUE], emul->internal->queue_size[RX_QUEUE], 0, VIRTIO_PCI_VRING_ALIGN);
    internal->driver.cb_cookie = emul;
    internal->driver.i_cb = emul_callbacks;
    internal->guest_vspace = *guest_vspace;
    internal->dma_man = io_ops.dma_manager;
    err = driver(&internal->driver, io_ops, config);
    if (err) {
        ZF_LOGE("Fafiled to initialize driver");
        goto error;
    }
    int mtu;
    internal->driver.i_fn.low_level_init(&internal->driver, internal->mac, &mtu);
    return emul;
error:
    if (emul) {
        free(emul);
    }
    if (internal) {
        free(internal);
    }
    return NULL;
}
