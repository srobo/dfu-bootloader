#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/f1/crc.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/iwdg.h>

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


void do_bootloader()
{
	usbd_device *usbd_dev;

	rcc_periph_clock_enable(RCC_GPIOA);

	rcc_clock_setup_in_hsi_out_48mhz();

	gpio_clear(GPIOA, GPIO8);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO8);

	usbd_dev = usbd_init(&stm32f103_usb_driver, &usbdfu_dev, &usbdfu_config, usbdfu_strings, 4, usbdfu_control_buffer, sizeof(usbdfu_control_buffer));
	usbd_register_set_config_callback(usbd_dev, set_config_cb);
	gpio_set(GPIOA, GPIO8);

	usbdfu_sanitise();

	while (1) {
		iwdg_reset();
		usbd_poll(usbd_dev);
	}
}

extern void* vector_table;

void re_enter_bootloader()
{
	// This entire thing is every so slightly dodgy. We can trust:
	//  * The bootloader code in this project hasn't been rewritten
	//  * All the linking inside the bootloader
	//  * All the RO data here
	// But not any data in sram, i.e. .bss and .data. This is /mostly/ fine,
	// but we reset our own variables with usbdfu_sanitise.
	//
	// It's funkier for libopencm3. There are only two pieces of data that
	// might prove troublesome: some rcc frequency counters, and the USB
	// device record which is normally in .bss. We can't trust either as
	// their addresses and data will have changed in the application.
	//
	// However: the USB driver code is always (apparently) overwritten and
	// initialized by libopencm3. And in the worst case, the rcc vars are
	// not static.
	SCB_VTOR = (uint32_t)vector_table;
	usbdfu_sanitise();
	do_bootloader();
}

bool
verify_firmware()
{

	// To work out how far we should read memory, we need the build
	// environment to tell us how much memory there is on this IC. While
	// all our boards are using F1s, they /might/ not all be 103's.
	unsigned int image_size = SR_BOOTLOADER_FLASHSIZE;
	uint32_t *image_addr = (uint32_t*)APP_ADDRESS;
	image_size -= 0x2000; // Size of bootloader itself

	// Un-gate CRC unit
	rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_CRCEN);

	// Reset CRC calculation
	crc_reset();
	crc_calculate(*image_addr);
	crc_reset();
	// Accumulate VTOR and entry addr
	crc_calculate_block(image_addr, 2);
	image_size -= 8;
	image_addr += 2; // Two uint32_t's

	// Accumulate zero for the CRC32 position
	crc_calculate(0);
	image_size -= 4;
	// Load the baked-in crc value
	uint32_t flashcrc = *image_addr;
	image_addr += 1; // One uint32_t

	// Accumulate the rest of the flash image. 2nd argument is the number
	// of 32 bit words to crc over
	uint32_t thecrc = crc_calculate_block(image_addr, image_size / 4);

	// Result is whether the CRC matches.
	return flashcrc == thecrc;
}

uint32_t reenter_bootloader_addr __attribute__((section(".reentry")))
	= (uint32_t)&re_enter_bootloader;

/* Expect outside environment to have built and linked in a force_bootloader
 * function. This means this bootloader is not device independent. This is a
 * feature. */
extern bool force_bootloader();

int main(void)
{

	if (!force_bootloader() && verify_firmware()) {
		/* XXX jmorse: skip configuring the stack location. It doesn't
		 * harm us to have the remains of the bootloader at thet top.
		 * Keep the configuration of the interrupt vectors though */
		/* Set vector table base address. */
		SCB_VTOR = *(volatile uint32_t*)APP_ADDRESS;
		/* Jump to application. */
		(*(void (**)())(APP_ADDRESS + 4))();
	}

	do_bootloader();
}
