#include <libopencm3/usb/usbd.h>

#define REENTER_BOOTLOADER_RENDEZVOUS	0x08001FFC
#define SERIALNUM_BOOTLOADER_LOC	0x08001FE0

extern uint8_t usbdfu_control_buffer[1024];
extern const struct usb_device_descriptor usbdfu_dev;
extern const struct usb_config_descriptor usbdfu_config;
extern const char *usbdfu_strings[];

enum usbd_request_return_codes usbdfu_control_request(usbd_device *usbd_dev, struct usb_setup_data *req,
	uint8_t **buf, uint16_t *len,
	void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req));
void usbdfu_sanitise();
