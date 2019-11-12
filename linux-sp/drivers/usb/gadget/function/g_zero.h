/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This header declares the utility functions used by "Gadget Zero", plus
 * interfaces to its two single-configuration function drivers.
 */

#ifndef __G_ZERO_H
#define __G_ZERO_H

#if 1	/* sunplus USB driver */
#include <linux/usb/composite.h>

#define USB_BUFSIZ	1024
#endif

#define GZERO_BULK_BUFLEN	4096
#define GZERO_QLEN		32
#define GZERO_ISOC_INTERVAL	4
#define GZERO_ISOC_MAXPACKET	1024
#define GZERO_SS_BULK_QLEN	1
#define GZERO_SS_ISO_QLEN	8

#if 1	/* sunplus USB driver */
extern uint buflen;

#ifdef CONFIG_USB_OTG
static struct usb_otg_descriptor otg_descriptor = {
	.bLength = sizeof otg_descriptor,
	.bDescriptorType = USB_DT_OTG,

	/* REVISIT SRP-only hardware is possible, although
	 * it would not be called "OTG" ...
	 */
	.bmAttributes = USB_OTG_SRP | USB_OTG_HNP,
};

static const struct usb_descriptor_header *otg_desc[] = {
	(struct usb_descriptor_header *)&otg_descriptor,
	NULL,
};
#endif
#endif

struct usb_zero_options {
	unsigned pattern;
	unsigned isoc_interval;
	unsigned isoc_maxpacket;
	unsigned isoc_mult;
	unsigned isoc_maxburst;
	unsigned bulk_buflen;
	unsigned qlen;
	unsigned ss_bulk_qlen;
	unsigned ss_iso_qlen;
};

struct f_ss_opts {
	struct usb_function_instance func_inst;
	unsigned pattern;
	unsigned isoc_interval;
	unsigned isoc_maxpacket;
	unsigned isoc_mult;
	unsigned isoc_maxburst;
	unsigned bulk_buflen;
	unsigned bulk_qlen;
	unsigned iso_qlen;

	/*
	 * Read/write access to configfs attributes is handled by configfs.
	 *
	 * This is to protect the data from concurrent access by read/write
	 * and create symlink/remove symlink.
	 */
	struct mutex			lock;
	int				refcnt;
};

struct f_lb_opts {
	struct usb_function_instance func_inst;
	unsigned bulk_buflen;
	unsigned qlen;

	/*
	 * Read/write access to configfs attributes is handled by configfs.
	 *
	 * This is to protect the data from concurrent access by read/write
	 * and create symlink/remove symlink.
	 */
	struct mutex			lock;
	int				refcnt;
};

void lb_modexit(void);
int lb_modinit(void);

/* common utilities */
	#if 1	/* sunplus USB driver */
void disable_endpoints(struct usb_composite_dev *cdev,
		       struct usb_ep *in, struct usb_ep *out);

struct usb_request *alloc_ep_req(struct usb_ep *ep);

/* Frees a usb_request previously allocated by alloc_ep_req() */
void free_ep_req(struct usb_ep *ep, struct usb_request *req);

extern int usb_add_config(struct usb_composite_dev *cdev,
		struct usb_configuration *config,
		int (*bind)(struct usb_configuration *));
extern int usb_composite_probe(struct usb_composite_driver *driver);
extern int usb_add_function(struct usb_configuration *config, struct usb_function *function);
extern int config_ep_by_speed(struct usb_gadget *g, struct usb_function *f, struct usb_ep *_ep);
extern int usb_interface_id(struct usb_configuration *config, struct usb_function *function);
extern void usb_composite_unregister(struct usb_composite_driver *driver);
extern int usb_string_id(struct usb_composite_dev *c);

	#else
void disable_endpoints(struct usb_composite_dev *cdev,
		struct usb_ep *in, struct usb_ep *out,
		struct usb_ep *iso_in, struct usb_ep *iso_out);
	#endif
#endif /* __G_ZERO_H */
