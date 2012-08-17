
#include "sysbus.h"

typedef struct {
    SysBusDevice busdev;
    uint8_t buttonState;
    qemu_irq gpio_out;
    CharDriverState* chr;
} stm32_button_state;



static const VMStateDescription vmstate_stm32_button = {
    .name       = "stm32_button",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[])
    {
        VMSTATE_UINT8(buttonState, stm32_button_state),
        VMSTATE_END_OF_LIST()
    }
};



static void stm32_button_reset (stm32_button_state *s)
{
    s->buttonState = 0;
}



static int stm32_can_receive (void *opaque) { return 1; } /* always return 1 */



static void stm32_receive(void *opaque, const uint8_t* buf, int size)
{

    stm32_button_state *s = (stm32_button_state *) opaque;
    
    int i = 0;
    for(i = 0; i < size; i++) {
        //Leve une IRQ
        uint8_t etat   = buf[i];
        s->buttonState = etat;
        printf("the BUTTON change state->%d\n", (int)etat);
        qemu_set_irq(s->gpio_out, (int)etat);
    }
}



static void stm32_event(void *opaque, int event) { } // NOT USED



static int stm32_button_init(SysBusDevice *dev, const char* id)
{
    stm32_button_state *s = FROM_SYSBUS(stm32_button_state, dev);
    
    // initial the output pins
    qdev_init_gpio_out(&dev->qdev, &s->gpio_out, 1);
    
    // inital the char dev
    s->chr = qemu_char_get_next_serial();
    //s->id  = id;

    if (s->chr) {
        qemu_chr_add_handlers(s->chr, stm32_can_receive, stm32_receive, stm32_event, s);
    }
    
    stm32_button_reset(s);
    vmstate_register(&dev->qdev, -1, &vmstate_stm32_button, s);
 
    return 0;
}



static int stm32_button_init_(SysBusDevice *dev)
{
    return stm32_button_init(dev, "user0");
}



static Property stm32_button_sysbus_properties[] = {
    //XXX: DEFINE_PROP_CHR("chardev", NULL, chr),
    DEFINE_PROP_END_OF_LIST(),
};



static void stm32_button_class_init (ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init   = stm32_button_init_;
    dc->desc  = "STM32 Button";
    dc->vmsd  = &vmstate_stm32_button;
    dc->props = stm32_button_sysbus_properties;

}



static TypeInfo stm32_button_info = {
    .name       = "stm32_button",
    .parent     = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof (stm32_button_state),
    .class_init = stm32_button_class_init,
};



static void stm32_button_register_devices(void) {
    type_register_static(&stm32_button_info);
}



type_init(stm32_button_register_devices)
