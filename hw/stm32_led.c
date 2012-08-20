
#include "sysbus.h"

typedef struct {
    SysBusDevice busdev;
    uint8_t      ledState;
    CharDriverState *chr;
} stm32_led_state;



static const VMStateDescription vmstate_stm32_led = {
    .name       = "stm32_led",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[])
    {
        VMSTATE_UINT8(ledState, stm32_led_state),
        VMSTATE_END_OF_LIST()
    }
};



static void stm32_led_reset (stm32_led_state *s)
{
    s->ledState = 0;
}



static void stm32_led_recvirq (void *opaque, int num_pin, int level)
{
    /* the LED change state */
    stm32_led_state *s = (stm32_led_state *) opaque;
    
    if (s->chr) {
        uint8_t buffer = (uint8_t)level;
        qemu_chr_fe_write(s->chr, &buffer, 1);
    }
}



static int stm32_led_init (SysBusDevice *dev, const char *id)
{
    stm32_led_state *s = FROM_SYSBUS(stm32_led_state, dev);
    
    /* initialize the output pins */
    qdev_init_gpio_in(&dev->qdev, stm32_led_recvirq, 1);

    /* initialize the char device */
    char id_string[20];
    sprintf(id_string, "led_%s", id);
    s->chr = qemu_chr_find(id_string);
    
    stm32_led_reset(s);
    vmstate_register(&dev->qdev, -1, &vmstate_stm32_led, s);

    return 0;
}

 

static int stm32_led_init_Blue (SysBusDevice *dev)
{
    return stm32_led_init(dev, "blue");
}



static int stm32_led_init_Green(SysBusDevice *dev)
{
    return stm32_led_init(dev, "green");
}



static Property stm32_led_blue_sysbus_properties[] = {
    DEFINE_PROP_END_OF_LIST(),
};



static Property stm32_led_green_sysbus_properties[] = {
    DEFINE_PROP_END_OF_LIST(),
};



static void stm32_led_blue_class_init (ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init   = stm32_led_init_Blue;
    dc->desc  = "STM32 LED Blue";
    dc->vmsd  = &vmstate_stm32_led,
    dc->props = stm32_led_blue_sysbus_properties;
}



static void stm32_led_green_class_init (ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init   = stm32_led_init_Green;
    dc->desc  = "STM32 LED Green";
    dc->vmsd  = &vmstate_stm32_led,
    dc->props = stm32_led_green_sysbus_properties;
}



static TypeInfo stm32_led_info_blue = {
    .name       = "stm32_led_blue",
    .parent     = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof (stm32_led_state),
    .class_init = stm32_led_blue_class_init,
};



static TypeInfo stm32_led_info_green = {
    .name       = "stm32_led_green",
    .parent     = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof (stm32_led_state),
    .class_init = stm32_led_green_class_init,
};



static void stm32_led_register_devices(void) {
    type_register_static(&stm32_led_info_blue);
    type_register_static(&stm32_led_info_green);
}



type_init(stm32_led_register_devices)
