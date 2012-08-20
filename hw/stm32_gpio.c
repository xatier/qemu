#include "sysbus.h"
#include "exec-memory.h"

#define NB_PIN 64

typedef struct {
    SysBusDevice busdev;

    /* GPIO registers (Reference Manual p139 */
    uint32_t mode;          /* Mode */
    uint16_t otype;         /* Output type */
    uint32_t ospeed;        /* Output speed */
    uint32_t pupd;          /* Pull-up/Pull-down */
    uint16_t ind;           /* Input data */
    uint16_t outd;          /* Output data register */
    uint16_t outd_old;      /* Output data register */
    uint32_t bsr;           /* Bit set/reset */
    uint32_t lck;           /* Lock */
    uint32_t afrl;          /* Alternate function low */
    uint32_t afrh;          /* Alternate function high */
    
    qemu_irq irq_out[NB_PIN];
    unsigned char id;
    MemoryRegion iomem;
} stm32_gpio_state;


static const VMStateDescription vmstate_stm32_gpio = {
    .name       = "stm32_gpio",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[])
    {
        VMSTATE_UINT32(mode, stm32_gpio_state),
        VMSTATE_UINT16(otype, stm32_gpio_state),
        VMSTATE_UINT32(ospeed, stm32_gpio_state),
        VMSTATE_UINT32(pupd, stm32_gpio_state),
        VMSTATE_UINT16(ind, stm32_gpio_state),
        VMSTATE_UINT16(outd, stm32_gpio_state),
        VMSTATE_UINT16(outd_old, stm32_gpio_state),
        VMSTATE_UINT32(bsr, stm32_gpio_state),
        VMSTATE_UINT32(lck, stm32_gpio_state),
        VMSTATE_UINT32(afrl, stm32_gpio_state),
        VMSTATE_UINT32(afrh, stm32_gpio_state),
        VMSTATE_END_OF_LIST()
    }
};


static void stm32_gpio_update (stm32_gpio_state *s)
{
    uint16_t changed;
    uint16_t mask;
    int i;

    /* XOR: all bits to 1 will be changed */
    changed = s->outd_old ^ s->outd;
    if (!changed)
        return;

    s->outd_old = s->outd;
    for (i = 0; i < 8; i++) {
        mask = 1 << i;
        if (changed & mask) {
            /* conditions change */
            /*--Mode register != 00 */
            uint32_t modeMask = (1 << (i*2)) | (1 << ((i*2) + 1));
            if ((s->mode & modeMask) != 0) {
                /* send an IRQ to the device connected to the GPIO pin */
                /*FIXME: the transmition to the pin is direct when it should occur at the next tick of the RCC */
                qemu_set_irq(s->irq_out[i], (s->outd & mask) != 0);
            }
        }
    }
}


static uint64_t stm32_gpio_read (void *opaque, target_phys_addr_t offset, unsigned size)
{
    stm32_gpio_state *s = (stm32_gpio_state *) opaque;

    switch (offset) {
        case 0x00: /* Mode */
            return s->mode;
        case 0x04: /* oType */
            return s->otype;
        case 0x08: /* oSpeed */
            return s->ospeed;
        case 0x0C: /* Pull-up / Pull-down */
            return s->pupd;
        case 0x10: /* Input data register */
            return s->ind;
        case 0x14: /* Output data */
            return s->outd;
        case 0x18: /* BSR */
            return 0x0; /* Write only */
        case 0x1C: /* lock */
            return 0x0;   /* unimplemented */
        case 0x20: /* AFRL */
            return s->afrl;
        case 0x24: /* AFRH */
            return s->afrh;
        default:
            hw_error("stm32_gpio_read: Bad offset %x\n", (int) offset);
            return 0;
    }
}


static void stm32_gpio_write (void *opaque, target_phys_addr_t offset, 
                                        uint64_t value, unsigned size)
{
    stm32_gpio_state *s = (stm32_gpio_state *) opaque;
    int i;
    uint16_t low = (uint16_t)value;
    uint16_t high = (uint16_t)value >> 16;
    
    switch (offset) {
        case 0x00: /* Mode */
            s->mode = value;
            break;
        case 0x04: /* oType */
            s->otype = value;
            break;
        case 0x08: /* oSpeed */
            s->ospeed = value;
            break;
        case 0x0C: /* Pull-up / Pull-down */
            s->pupd = value;
            break;
        case 0x10: /* Input data register */
            /* Read only */
            break;
        case 0x14: /* Output data */
            s->outd = value;
            stm32_gpio_update(s);
            break;
            
        case 0x18: /* BSR */
            /*
             * set   = low
             * reset = high
             */
            for (i = 0; i < 16; i++) {
                int mask = 1 << i;
                if ((high & mask) != 0) { /* if reset bit[i], set it to 0 */
                    s->outd &= ~(1 << i);
                }
                if((low & mask) != 0) {   /* if reset bit[i], set it to 1 */
                    s->outd |= (1 << i);
                }
            }
            stm32_gpio_update(s);
            break;
            
        case 0x1C: /* lock */
            /* XXX: unimplemented */
            break;
        case 0x20: /* AFRL */
            s->afrl = value;
            break;
        case 0x24: /* AFRH */
            s->afrh = value;
            break;
        default:
            hw_error("stm32_gpio_write: Bad offset %x\n", (int) offset);
    }
}



static void stm32_gpio_reset (stm32_gpio_state *s)
{
    /* for GPIO A and GPIO B */
    switch (s->id) {
        case 'A':
            s->mode = 0xA8000000;
            s->pupd = 0x64000000;
            s->ospeed = 0x00000000;
            break;
        case 'B':
            s->mode = 0x00000280;
            s->pupd = 0x00000100;
            s->ospeed = 0x000000C0;
            break;
        default:
            s->mode = 0x00000000;
            s->pupd = 0x00000000;
            s->ospeed = 0x00000000;
    }
    

    /* common value */
    s->ind =    0x00000000;
    s->otype =  0x00000000;
    s->outd =   0x00000000;
    s->outd_old = s->outd;
    s->bsr =    0x00000000;
    s->lck =    0x00000000;
    s->afrh =   0x00000000;
    s->afrl =   0x00000000;

}



static void stm32_gpio_in_recv (void *opaque, int num_pin, int level)
{
    assert(num_pin >= 0 && num_pin < NB_PIN);
    stm32_gpio_state *s = (stm32_gpio_state *) opaque;
    
    
    uint32_t mask = (1 << (num_pin*2)) | (1 << ((num_pin*2) + 1));
    /* check if the pin is confihured as input */
    /* --Pull-up Pull-Down -> must be different trom 00 */
    if ((s->pupd & mask) == 0) {return;} 
    /* --Mode register -> 00 */
    if ((s->mode & mask) != 0) {return;} 
    
    /* Writing to the register input data register */
    if (level) {
        s->ind |= (1 << num_pin);   /* set to 1 */
    } else {
        s->ind &= ~(1 << num_pin);  /* set to 0 */
    }
}



static const MemoryRegionOps gpio_ops = {
    .read  = stm32_gpio_read,
    .write = stm32_gpio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};



static int stm32_gpio_init (SysBusDevice *dev, const unsigned char id)
{
    stm32_gpio_state *s = FROM_SYSBUS(stm32_gpio_state, dev);
    s->id = id;
    
    /* initialisation of the memory range */
    memory_region_init_io(&s->iomem, &gpio_ops, s, "gpio", 0x17FF);
    sysbus_init_mmio(dev, &s->iomem);
    
    /* initialize the pins */
    qdev_init_gpio_in(&dev->qdev, stm32_gpio_in_recv, NB_PIN);
    qdev_init_gpio_out(&dev->qdev, s->irq_out, NB_PIN);

    stm32_gpio_reset(s);
    vmstate_register(&dev->qdev, -1, &vmstate_stm32_gpio, s);

    return 0;
}



static int stm32_gpio_init_A (SysBusDevice *dev)
{
    return stm32_gpio_init(dev, 'A');
}



static int stm32_gpio_init_B (SysBusDevice *dev)
{
    return stm32_gpio_init(dev, 'B');
}



static Property stm32_gpio_sysbus_properties[] = {
    //XXX: DEFINE_PROP_CHR("chardev", NULL, chr),
    DEFINE_PROP_END_OF_LIST(),
};



static void stm32_gpio_class_init_A (ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init   = stm32_gpio_init_A;
    dc->desc  = "STM32 GPIO A";
    dc->vmsd  = &vmstate_stm32_gpio;
    dc->props = stm32_gpio_sysbus_properties;

}



static void stm32_gpio_class_init_B (ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init   = stm32_gpio_init_B;
    dc->desc  = "STM32 GPIO B";
    dc->vmsd  = &vmstate_stm32_gpio;
    dc->props = stm32_gpio_sysbus_properties;

}



static TypeInfo stm32_gpioA_info = {
    .name       = "stm32_gpio_A",
    .parent     = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof (stm32_gpio_state),
    .class_init = stm32_gpio_class_init_A,
};



static TypeInfo stm32_gpioB_info = {
    .name       = "stm32_gpio_B",
    .parent     = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof (stm32_gpio_state),
    .class_init = stm32_gpio_class_init_B,
};



static void stm32_gpio_register_devices(void) {
    type_register_static(&stm32_gpioA_info);
    type_register_static(&stm32_gpioB_info);
}



type_init(stm32_gpio_register_devices)
