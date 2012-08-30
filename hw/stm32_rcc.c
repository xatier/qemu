#include "sysbus.h"
#include "qemu-timer.h"

typedef struct {
    SysBusDevice busdev;
    uint32_t CR;            /* Clock control register                                   */
    uint32_t ICSCR;         /* Internal clock sources calibration register              */
    uint32_t CFGR;          /* Clock configuration register                             */
    uint32_t CIR;           /* Clock interrupt register                                 */
    uint32_t AHBRSTR;       /* AHB peripheral reset register                            */
    uint32_t APB2RSTR;      /* APB2 peripheral reset register                           */
    uint32_t APB1RSTR;      /* APB1 peripheral reset register                           */
    uint32_t AHBENR;        /* AHB peripheral clock enable register                     */
    uint32_t APB2ENR;       /* APB2 peripheral clock enable register                    */
    uint32_t APB1ENR;       /* APB1 peripheral clock enable register                    */
    uint32_t AHBLPENR;      /* AHB peripheral clock enable in low power mode register   */
    uint32_t APB2LPENR;     /* APB2 peripheral clock enable in low power mode register  */
    uint32_t APB1LPENR;     /* APB1 peripheral clock enable in low power mode register  */
    uint32_t CSR;           /* Control/status register                                  */
    qemu_irq irq;
    unsigned char *id;
    MemoryRegion iomem;
    uint32_t tick_offset;
    uint32_t tick_offset_vmstate;

} stm32_rcc_state;



static const VMStateDescription vmstate_stm32_rcc = {
    .name = "stm32_rcc",
    .version_id = 2,
    .minimum_version_id = 1,
    .fields = (VMStateField[])
    {
        VMSTATE_UINT32(CR, stm32_rcc_state),
        VMSTATE_UINT32(ICSCR, stm32_rcc_state),
        VMSTATE_UINT32(CFGR, stm32_rcc_state),
        VMSTATE_UINT32(CIR, stm32_rcc_state),
        VMSTATE_UINT32(AHBRSTR, stm32_rcc_state),
        VMSTATE_UINT32(APB2RSTR, stm32_rcc_state),
        VMSTATE_UINT32(APB1RSTR, stm32_rcc_state),
        VMSTATE_UINT32(AHBENR, stm32_rcc_state),
        VMSTATE_UINT32(APB2ENR, stm32_rcc_state),
        VMSTATE_UINT32(APB1ENR, stm32_rcc_state),
        VMSTATE_UINT32(AHBLPENR, stm32_rcc_state),
        VMSTATE_UINT32(APB2LPENR, stm32_rcc_state),
        VMSTATE_UINT32(APB1LPENR, stm32_rcc_state),
        VMSTATE_UINT32(CSR, stm32_rcc_state),
        VMSTATE_UINT32(tick_offset_vmstate, stm32_rcc_state),
        VMSTATE_END_OF_LIST()
    }
};



static void stm32_rcc_update (stm32_rcc_state *s) {
    /* FIXME: Implement interrupts.  */
}



static uint64_t stm32_rcc_read (void *opaque, target_phys_addr_t offset, 
    unsigned size) {
    stm32_rcc_state *s = (stm32_rcc_state *) opaque;
    switch (offset) {
        case 0x00:
            return s->CR;
            break;
        case 0x04:
            return s->ICSCR;
            break;
        case 0x08:
            return s->CFGR;
            break;
        case 0x0C:
            return s->CIR;
            break;
        case 0x10:
            return s->AHBRSTR;
            break;
        case 0x14:
            return s->APB2RSTR;
            break;
        case 0x18:
            return s->APB1RSTR;
            break;
        case 0x1C:
            return s->AHBENR;
            break;
        case 0x20:
            return s->APB2ENR;
            break;
        case 0x24:
            return s->APB1ENR;
            break;
        case 0x28:
            return s->AHBLPENR;
            break;
        case 0x2C:
            return s->APB2LPENR;
            break;
        case 0x30:
            return s->APB1LPENR;
            break;
        case 0x34:
            return s->CSR;
        default:
            hw_error("stm32_rcc_read: Bad offset %x\n", (int) offset);
            return 0;
    }
}

static void stm32_rcc_write (void *opaque, target_phys_addr_t offset,
        uint64_t value, unsigned size) {
    stm32_rcc_state *s = (stm32_rcc_state *) opaque;
    switch (offset) {
        case 0x00:
            s->CR = value;
            break;
        case 0x04:
            s->ICSCR = value;
            break;
        case 0x08:
            s->CFGR = value;
            break;
        case 0x0C:
            s->CIR = value;
            break;
        case 0x10:
            s->AHBRSTR = value;
            break;
        case 0x14:
            s->APB2RSTR = value;
            break;
        case 0x18:
            s->APB1RSTR = value;
            break;
        case 0x1C:
            s->AHBENR = value;
            break;
        case 0x20:
            s->APB2ENR = value;
            break;
        case 0x24:
            s->APB1ENR = value;
            break;
        case 0x28:
            s->AHBLPENR = value;
            break;
        case 0x2C:
            s->APB2LPENR = value;
            break;
        case 0x30:
            s->APB1LPENR = value;
            break;
        case 0x34:
            s->CSR = value;
            break;
        default:
            hw_error("stm32_rcc_write: Bad offset %x\n", (int) offset);
    }
    stm32_rcc_update(s);
}



static void stm32_rcc_reset(stm32_rcc_state *s) {
    s->CR        = 0x00000300;
    s->ICSCR     = 0x0000B000;
    s->CFGR      = 0x00000000;
    s->CIR       = 0x00000000;
    s->AHBRSTR   = 0x00000000;
    s->APB2RSTR  = 0x00000000;
    s->APB1RSTR  = 0x00000000;
    s->AHBENR    = 0x00008000;
    s->APB2ENR   = 0x00000000;
    s->APB1ENR   = 0x00000000;
    s->AHBLPENR  = 0x0101903F;
    s->APB2LPENR = 0x0000521D;
    s->APB1LPENR = 0xB0E64A37;
    s->CSR       = 0x0C000000;
}


//static void stm32_rcc_set_irq(void * opaque, int irq, int level) {
//    //TODO gestion des irq
//    //stm32_rcc_state *s = (stm32_rcc_state *) opaque;
//
//}


static MemoryRegionOps rcc_ops = {
    .read  = stm32_rcc_read,
    .write = stm32_rcc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};



static int stm32_rcc_init (SysBusDevice *dev) {
    struct tm tm;
    stm32_rcc_state *s = FROM_SYSBUS(stm32_rcc_state, dev);
    s->id = (unsigned char *)"stm32_rcc";

    memory_region_init_io(&s->iomem, &rcc_ops, s, "rcc", 0x1000);
    sysbus_init_mmio(dev, &s->iomem);

    sysbus_init_irq(dev, &s->irq);
    qemu_get_timedate(&tm, 0);
    s->tick_offset = mktimegm(&tm) - qemu_get_time_ns(rtc_clock) / get_ticj_per_sec();
    s->timer = qemu_new_timer_ns(rtc_clock, s);
    stm32_rcc_reset(s);
    return 0;
}



static Property stm32_rcc_sysbus_properties[] = {
    // nothing
    DEFINE_PROP_END_OF_LIST(),
};



static void stm32_rcc_class_init (ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init   = stm32_rcc_init;
    dc->desc  = "STM32 RCC";
    dc->vmsd  = &vmstate_stm32_rcc;
    dc->props = stm32_rcc_sysbus_properties;

}



static TypeInfo stm32_rcc_info = {
    .name       = "stm32_rcc",
    .parent     = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof (stm32_rcc_state),
    .class_init = stm32_rcc_class_init,
};



static void stm32_rcc_register_devices (void) {
    type_register_static(&stm32_rcc_info);
}



type_init(stm32_rcc_register_devices)
