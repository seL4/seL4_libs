/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#if 0

/*emulation part is ported from QEMU MC146818 rtc*/

typedef struct RTCState {
    ISADevice parent_obj;

    MemoryRegion io;
    uint8_t cmos_data[128];
    uint8_t cmos_index;
    int32_t base_year;
    uint64_t base_rtc;
    uint64_t last_update;
    int64_t offset;
    qemu_irq irq;
    qemu_irq sqw_irq;
    int it_shift;
    /* periodic timer */
    QEMUTimer *periodic_timer;
    int64_t next_periodic_time;
    /* update-ended timer */
    QEMUTimer *update_timer;
    uint64_t next_alarm_time;
    uint16_t irq_reinject_on_ack_count;
    uint32_t irq_coalesced;
    uint32_t period;
    QEMUTimer *coalesced_timer;
    Notifier clock_reset_notifier;
    LostTickPolicy lost_tick_policy;
    Notifier suspend_notifier;
} RTCState;


RTCState vmm_rtc_state; 

static inline int rtc_to_bcd(RTCState *s, int a)
{
    if (s->cmos_data[RTC_REG_B] & REG_B_DM) {
        return a;
    } else {
        return ((a / 10) << 4) | (a % 10);
    }
}

static inline int rtc_from_bcd(RTCState *s, int a)
{
    if ((a & 0xc0) == 0xc0) {
        return -1;
    }
    if (s->cmos_data[RTC_REG_B] & REG_B_DM) {
        return a;
    } else {
        return ((a >> 4) * 10) + (a & 0x0f);
    }
}



static void rtc_set_cmos(RTCState *s, const struct tm *tm)
{
    int year;

    s->cmos_data[RTC_SECONDS] = rtc_to_bcd(s, tm->tm_sec);
    s->cmos_data[RTC_MINUTES] = rtc_to_bcd(s, tm->tm_min);
    if (s->cmos_data[RTC_REG_B] & REG_B_24H) {
        /* 24 hour format */
        s->cmos_data[RTC_HOURS] = rtc_to_bcd(s, tm->tm_hour);
    } else {
        /* 12 hour format */
        int h = (tm->tm_hour % 12) ? tm->tm_hour % 12 : 12;
        s->cmos_data[RTC_HOURS] = rtc_to_bcd(s, h);
        if (tm->tm_hour >= 12)
            s->cmos_data[RTC_HOURS] |= 0x80;
    }
    s->cmos_data[RTC_DAY_OF_WEEK] = rtc_to_bcd(s, tm->tm_wday + 1);
    s->cmos_data[RTC_DAY_OF_MONTH] = rtc_to_bcd(s, tm->tm_mday);
    s->cmos_data[RTC_MONTH] = rtc_to_bcd(s, tm->tm_mon + 1);
    year = tm->tm_year + 1900 - s->base_year;
    s->cmos_data[RTC_YEAR] = rtc_to_bcd(s, year % 100);
    s->cmos_data[RTC_CENTURY] = rtc_to_bcd(s, year / 100);
}


/*get the rtc time from host machine*/
static void rtc_set_date_from_host(ISADevice *dev)
{
    RTCState *s = MC146818_RTC(dev);
    //struct tm tm;

    
    //qemu_get_timedate(&tm, 0);

    /*FIXME: QIAN get the real time from rtc and get the updated clock in ns*/
//    s->base_rtc = mktimegm(&tm);
  //  s->last_update = qemu_get_clock_ns(rtc_clock);
    s->offset = 0;

    /*set a shadow copy of the CMOS data*/
    /* set the CMOS date */
    rtc_set_cmos(s, &tm);
}





/*internal rtc init function*/
static void rtc_realizefn(DeviceState *dev, Error **errp)
{
    RTCState *s = &vmm_rtc_state;
    int base = 0x70;

    s->cmos_data[RTC_REG_A] = 0x26;
    s->cmos_data[RTC_REG_B] = 0x02;
    s->cmos_data[RTC_REG_C] = 0x00;
    s->cmos_data[RTC_REG_D] = 0x80;

    /* This is for historical reasons.  The default base year qdev property
     * was set to 2000 for most machine types before the century byte was
     * implemented.
     *
     * This if statement means that the century byte will be always 0
     * (at least until 2079...) for base_year = 1980, but will be set
     * correctly for base_year = 2000.
     */
    if (s->base_year == 2000) {
        s->base_year = 0;
    }

    rtc_set_date_from_host(isadev);

    switch (s->lost_tick_policy) {
    case LOST_TICK_SLEW:
        s->coalesced_timer =
            qemu_new_timer_ns(rtc_clock, rtc_coalesced_timer, s);
        break;
    case LOST_TICK_DISCARD:
        break;
    default:
        error_setg(errp, "Invalid lost tick policy.");
        return;
    }

    s->periodic_timer = qemu_new_timer_ns(rtc_clock, rtc_periodic_timer, s);
    s->update_timer = qemu_new_timer_ns(rtc_clock, rtc_update_timer, s);
    check_update_timer(s);

    s->clock_reset_notifier.notify = rtc_notify_clock_reset;
    qemu_register_clock_reset_notifier(rtc_clock, &s->clock_reset_notifier);

    s->suspend_notifier.notify = rtc_notify_suspend;
    qemu_register_suspend_notifier(&s->suspend_notifier);

    memory_region_init_io(&s->io, OBJECT(s), &cmos_ops, s, "rtc", 2);
    isa_register_ioport(isadev, &s->io, base);

    qdev_set_legacy_instance_id(dev, base, 3);
    qemu_register_reset(rtc_reset, s);

    object_property_add(OBJECT(s), "date", "struct tm",
                        rtc_get_date, NULL, NULL, s, NULL);
}




/*FIXME: we may need a rtc structure for every guest vmm*/
void rtc_init(ISABus *bus, int base_year, void intercept_irq)
{
   /*set the base year*/
    vmm_rtc_state->base_year = base_year;

    /*FIXME: QIAN connect the irq with this rtc*/
}

#endif
