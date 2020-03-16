#include <usb_names.h>

#define MANUFACTURER_NAME	{'j','c','s'}
#define MANUFACTURER_NAME_LEN	3
#define PRODUCT_NAME		{'M','a','i','l','s','t','a','t','i','o','n', \
				    ' ','L','o','a','d','e','r'}
#define PRODUCT_NAME_LEN	18

struct usb_string_descriptor_struct usb_string_manufacturer_name = {
	2 + MANUFACTURER_NAME_LEN * 2,
	3,
	MANUFACTURER_NAME
};
struct usb_string_descriptor_struct usb_string_product_name = {
	2 + PRODUCT_NAME_LEN * 2,
	3,
	PRODUCT_NAME
};
struct usb_string_descriptor_struct usb_string_serial_number = {
	3,
	3,
	{ 1 },
};
