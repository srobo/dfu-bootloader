#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/scb.h>

#include "usbdfu.h"

#define APP_ADDRESS     0x08002000

static void set_config_cb(usbd_device *usbd_dev, uint16_t wValue)
{

        usbd_register_control_callback(
                                usbd_dev,
                                USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
                                USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
                                usbdfu_control_request);
}

/* Expect outside environment to have built and linked in a force_bootloader
 * function. This means this bootloader is not device independent. This is a
 * feature. */
extern bool force_bootloader();

int main(void)
{
	usbd_device *usbd_dev;

	rcc_periph_clock_enable(RCC_GPIOA);

	if (!force_bootloader()) {
		/* XXX jmorse: skip configuring the stack location. It doesn't
		 * harm us to have the remains of the bootloader at thet top.
		 * Keep the configuration of the interrupt vectors though */
		/* Set vector table base address. */
		SCB_VTOR = *(volatile uint32_t*)APP_ADDRESS;
		/* Jump to application. */
		(*(void (**)())(APP_ADDRESS + 4))();
	}

	rcc_clock_setup_in_hsi_out_48mhz();

	gpio_clear(GPIOA, GPIO8);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO8);

	usbd_dev = usbd_init(&stm32f103_usb_driver, &usbdfu_dev, &usbdfu_config, usbdfu_strings, 4, usbdfu_control_buffer, sizeof(usbdfu_control_buffer));
	usbd_register_set_config_callback(usbd_dev, set_config_cb);
	gpio_set(GPIOA, GPIO8);

	while (1)
		usbd_poll(usbd_dev);
}
