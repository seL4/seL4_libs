#ifndef _VMM_APIC_H
#define _VMM_APIC_H

typedef struct vmm_vcpu vmm_vcpu_t;

typedef struct vmm_apic {
    vmm_vcpu_t *vcpu;
    uint32_t apicbase;
    uint8_t id;
    uint8_t version;
    uint8_t arb_id;
    uint8_t tpr;
    uint32_t spurious_vec;
    uint8_t log_dest;
    uint8_t dest_mode;
    uint32_t isr[8]; /* in service register */
    uint32_t tmr[8]; /* trigger mode register */
    uint32_t irr[8]; /* interrupt request register */
    uint32_t lvt[2];
    uint32_t esr; /* error register */
    uint32_t icr[2];
    int wait_for_sipi;

#if 0    
    uint32_t divide_conf;
    int count_shift;
    uint32_t initial_count;
    int64_t initial_count_load_time;
    int64_t next_time;
    int idx;

    QEMUTimer *timer;
    int64_t timer_expiry;
#endif
} vmm_apic_t;

int vmm_apic_has_interrupt(vmm_vcpu_t *vcpu);
int vmm_apic_get_interrupt(vmm_vcpu_t *vcpu);

int vmm_apic_deliver_pic_intr(vmm_vcpu_t *vcpu, int level);
int vmm_apic_accept_pic_intr(vmm_vcpu_t *vcpu);

int vmm_vcpu_create_apic(vmm_vcpu_t *vcpu);

void vmm_apic_mmio_read(vmm_vcpu_t *vcpu, void *cookie,
        uint32_t offset, int size, uint32_t *result);
void vmm_apic_mmio_write(vmm_vcpu_t *vcpu, void *cookie,
        uint32_t offset, int size, uint32_t value);

#endif

