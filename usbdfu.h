#include <libopencm3/usb/usbd.h>

extern uint8_t usbdfu_control_buffer[1024];
extern const struct usb_device_descriptor usbdfu_dev;
extern const struct usb_config_descriptor usbdfu_config;
extern const char *usbdfu_strings[];

int usbdfu_control_request(usbd_device *usbd_dev, struct usb_setup_data *req,
	uint8_t **buf, uint16_t *len,
	void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req));
void usbdfu_sanitise();
void disable_dfu_iface(void);
void enable_dfu_iface(void);
