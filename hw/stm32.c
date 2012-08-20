
#include "sysbus.h"
#include "exec-memory.h"
#include "arm-misc.h"
#include "boards.h"
#include "devices.h"

#define NB_NVIC_IRQ 5



enum {
    GPIO_A = 0,             /* 0x4002 0000 - 0x4002 03FF */
    GPIO_B,                 /* 0x4002 0400 - 0x4002 07FF */
    GPIO_C,                 /* 0x4002 0800 - 0x4002 0BFF */
    GPIO_D,                 /* 0x4002 0C00 - 0x4002 0FFF */
    GPIO_E,                 /* 0x4002 1000 - 0x4002 13FF */
    GPIO_H,                 /* 0x4002 1400 - 0x4002 17FF */
    NB_GPIO,
};



typedef const struct { 
    const char *name;
    uint16_t    f_size;
} stm32_board_info;



static stm32_board_info stm32_board = {
    "stm32l152rbt6",
    0x0080,                 /* 128kb flash */
};



typedef struct {
    uint32_t     int_status;
    uint32_t     int_mask;
    MemoryRegion iomem;
    qemu_irq     irq;
    stm32_board_info *board;
} ssys_state;



static void ssys_reset (void *opaque) {
    /* unimplemented */
}



static uint64_t ssys_read (void *opaque, target_phys_addr_t offset, 
                                                    unsigned size)
{
    /* nothing to do */
    return 0;
}



static void ssys_write (void *opaque, target_phys_addr_t offset, uint64_t value,
                                                                 unsigned size)
{
    /* unimplemented */
}



static const MemoryRegionOps ssys_ops = {
    .read       = ssys_read,
    .write      = ssys_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};



static int stm32_sys_post_load (void *opaque, int version_id)
{
    /* Nothing to do */
    return 0;
}



static const VMStateDescription vmstate_stm32_sys = {
    .name                   = "stm32_sys",
    .version_id             = 2,
    .minimum_version_id     = 1,
    .minimum_version_id_old = 1,
    .post_load          = stm32_sys_post_load,
    .fields             = (VMStateField[]) {
        VMSTATE_UINT32(int_mask, ssys_state),
        VMSTATE_UINT32(int_status, ssys_state),
        VMSTATE_END_OF_LIST()
    }
};



static int stm32l_sys_init (uint32_t base, qemu_irq irq,
           stm32_board_info *board)
{
    ssys_state *s;

    s = (ssys_state *)g_malloc0(sizeof(ssys_state));
    s->irq   = irq;
    s->board = board;

    memory_region_init_io(&s->iomem, &ssys_ops, s, "ssys", 0x00010000);
    memory_region_add_subregion(get_system_memory(), base, &s->iomem);

    ssys_reset(s);
    vmstate_register(NULL, -1, &vmstate_stm32_sys, s);

    return 0;
}



static void stm32l152rbt6_init (ram_addr_t ram_size, const char *boot_device,
        const char *kernel_filename, const char *kernel_cmdline, 
        const char *initrd_filename, const char *cpu_model)
{

    /* prepare the memory */
    MemoryRegion *address_space_mem = get_system_memory();
    uint16_t flash_size = stm32_board.f_size;   /* 128KBits */
    uint16_t sram_size  = 0x0010;               /* 16 KBits */
    
    
    /* initialize the processor and memory */
    qemu_irq* pic = armv7m_init(address_space_mem, flash_size, sram_size,
                                            kernel_filename, cpu_model);
    stm32l_sys_init(0x1FF00000, pic[28], &stm32_board);
    
    /* structures GPIO */
    static const uint32_t gpio_addr[NB_GPIO] = {
        [GPIO_A] = 0x40020000,
        [GPIO_B] = 0x40020400,
        [GPIO_C] = 0x40020800,
        [GPIO_D] = 0x40020C00,
        [GPIO_E] = 0x40021000,
        [GPIO_H] = 0x40021400 
    };

    DeviceState* gpio_dev[NB_GPIO];    
    
    /* create the botton */
    DeviceState* button = sysbus_create_simple("stm32_button", -1, NULL);
    
    /* create the LEDs */
    DeviceState* led_dev6 = sysbus_create_simple("stm32_led_blue" , -1, NULL);
    DeviceState* led_dev7 = sysbus_create_simple("stm32_led_green", -1, NULL);
    
    
    /* initialize GPIO_A */
    gpio_dev[GPIO_A] = sysbus_create_varargs("stm32_gpio_A", gpio_addr[GPIO_A],
                                                                        NULL);
    qemu_irq entreeBouton = qdev_get_gpio_in(gpio_dev[GPIO_A], 1);

    qdev_connect_gpio_out(button, 0, entreeBouton);
    
    /* initial GPIO_B */
    gpio_dev[GPIO_B] = sysbus_create_varargs("stm32_gpio_B", gpio_addr[GPIO_B],
                                                                        NULL);

    qemu_irq entreeLED6 = qdev_get_gpio_in(led_dev6, 0);
    qdev_connect_gpio_out(gpio_dev[GPIO_B], 6, entreeLED6);
    qemu_irq entreeLED7 = qdev_get_gpio_in(led_dev7, 0);
    qdev_connect_gpio_out(gpio_dev[GPIO_B], 7, entreeLED7);
    
}

static QEMUMachine stm32l152rbt6_machine = {
    .name = "stm32l152rbt6",
    .desc = "STM32L Discovery",
    .init = stm32l152rbt6_init,
    /* 
     *  user manual page 105
     *  STM32 = ARM-based 32-bit microcontroller
     *  L = Low power
     *  152: Devices with LCD
     *  R = 64 pins
     *  B = 128 Kbytes of Flash memory
     *  T = LQFP
     *  6 = Industrial temperature range, –40 to 85 °C
     */
};

static void stm32l_machine_init (void) {
    qemu_register_machine(&stm32l152rbt6_machine);
}

machine_init(stm32l_machine_init);

