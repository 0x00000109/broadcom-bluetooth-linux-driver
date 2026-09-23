/*
 * Broadcom Bluetooth USB Driver for Modern Linux Kernels (6.x / 7+)
 * Supports Broadcom BCM43xx / BCM4352 / BCM4360 and vendor variants (Apple, Broadcom, etc.)
 * Fully functional implementation with URB management, endpoint discovery, and HCI integration.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/unaligned.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

MODULE_AUTHOR("Reverse Engineering Team");
MODULE_DESCRIPTION("Broadcom Bluetooth USB Modern Linux Driver (Full Implementation)");
MODULE_VERSION("1.2.2");
MODULE_LICENSE("GPL");

#define APPLE_VENDOR_ID    0x05ac
#define BROADCOM_VENDOR_ID 0x0a5c

#define BULK_BUFFER_SIZE   2048
#define INT_BUFFER_SIZE    64

static const struct usb_device_id brcmbt_table[] = {
    /* Apple integrations of Broadcom BCM43xx/4352/4360 BT */
    { USB_DEVICE(APPLE_VENDOR_ID, 0x8213) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x8215) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x8218) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x821A) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x821B) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x821D) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x821F) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x8281) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x8286) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x8287) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x8289) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x828A) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x828B) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x828C) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x828D) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x828E) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x828F) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x8290) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x8291) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x8293) },
    { USB_DEVICE(APPLE_VENDOR_ID, 0x8294) },
    /* Generic Broadcom USB Bluetooth devices */
    { USB_DEVICE(BROADCOM_VENDOR_ID, 0x21e8) },
    { USB_DEVICE(BROADCOM_VENDOR_ID, 0x21ec) },
    { }
};
MODULE_DEVICE_TABLE(usb, brcmbt_table);

struct brcmbt_data {
    struct usb_device *udev;
    struct usb_interface *intf;
    struct hci_dev *hdev;

    __u8 bulk_in_ep;
    __u8 bulk_out_ep;
    __u8 int_in_ep;

    struct urb *bulk_in_urb;
    struct urb *int_in_urb;

    void *bulk_in_buffer;
    void *int_in_buffer;

    struct usb_anchor bulk_anchor;
    struct usb_anchor tx_anchor;

    bool firmware_loaded;
};

static void brcmbt_bulk_in_complete(struct urb *urb);
static void brcmbt_int_in_complete(struct urb *urb);
static void brcmbt_bulk_out_complete(struct urb *urb);

static int brcmbt_submit_bulk_in(struct brcmbt_data *data, gfp_t gfp)
{
    struct urb *urb = data->bulk_in_urb;
    int err;

    usb_fill_bulk_urb(urb, data->udev,
                     usb_rcvbulkpipe(data->udev, data->bulk_in_ep),
                     data->bulk_in_buffer, BULK_BUFFER_SIZE,
                     brcmbt_bulk_in_complete, data);

    usb_anchor_urb(urb, &data->bulk_anchor);
    err = usb_submit_urb(urb, gfp);
    if (err < 0) {
        usb_unanchor_urb(urb);
        bt_dev_err(data->hdev, "Failed to submit bulk IN URB (%d)", err);
    }
    return err;
}

static int brcmbt_submit_int_in(struct brcmbt_data *data, gfp_t gfp)
{
    struct urb *urb = data->int_in_urb;
    int err;

    usb_fill_int_urb(urb, data->udev,
                    usb_rcvintpipe(data->udev, data->int_in_ep),
                    data->int_in_buffer, INT_BUFFER_SIZE,
                    brcmbt_int_in_complete, data, 1);

    usb_anchor_urb(urb, &data->bulk_anchor);
    err = usb_submit_urb(urb, gfp);
    if (err < 0) {
        usb_unanchor_urb(urb);
        bt_dev_err(data->hdev, "Failed to submit interrupt IN URB (%d)", err);
    }
    return err;
}

static void brcmbt_bulk_in_complete(struct urb *urb)
{
    struct brcmbt_data *data = urb->context;
    struct hci_dev *hdev = data->hdev;
    int err;

    if (urb->status) {
        if (urb->status == -ENOENT || urb->status == -ECONNRESET || urb->status == -ESHUTDOWN)
            return;
        bt_dev_err(hdev, "Bulk IN URB status %d", urb->status);
        goto resubmit;
    }

    if (urb->actual_length > 1) {
        struct sk_buff *skb;
        unsigned char *ptr = urb->transfer_buffer;
        int len = urb->actual_length;

        skb = bt_skb_alloc(len - 1, GFP_ATOMIC);
        if (skb) {
            hci_skb_pkt_type(skb) = ptr[0];
            skb_put_data(skb, ptr + 1, len - 1);
            if (hci_recv_frame(hdev, skb) < 0)
                bt_dev_err(hdev, "Failed to forward frame to HCI layer");
        }
    }

resubmit:
    usb_anchor_urb(urb, &data->bulk_anchor);
    err = usb_submit_urb(urb, GFP_ATOMIC);
    if (err < 0 && err != -ENODEV && err != -EPERM)
        bt_dev_err(hdev, "Failed to resubmit bulk IN URB (%d)", err);
}

static void brcmbt_int_in_complete(struct urb *urb)
{
    struct brcmbt_data *data = urb->context;
    struct hci_dev *hdev = data->hdev;
    int err;

    if (urb->status) {
        if (urb->status == -ENOENT || urb->status == -ECONNRESET || urb->status == -ESHUTDOWN)
            return;
        bt_dev_err(hdev, "Interrupt IN URB status %d", urb->status);
        goto resubmit;
    }

    if (urb->actual_length > 1) {
        struct sk_buff *skb;
        unsigned char *ptr = urb->transfer_buffer;
        int len = urb->actual_length;

        skb = bt_skb_alloc(len - 1, GFP_ATOMIC);
        if (skb) {
            hci_skb_pkt_type(skb) = ptr[0];
            skb_put_data(skb, ptr + 1, len - 1);
            if (hci_recv_frame(hdev, skb) < 0)
                bt_dev_err(hdev, "Failed to forward interrupt frame to HCI");
        }
    }

resubmit:
    usb_anchor_urb(urb, &data->bulk_anchor);
    err = usb_submit_urb(urb, GFP_ATOMIC);
    if (err < 0 && err != -ENODEV && err != -EPERM)
        bt_dev_err(hdev, "Failed to resubmit interrupt IN URB (%d)", err);
}

static void brcmbt_bulk_out_complete(struct urb *urb)
{
    struct sk_buff *skb = urb->context;

    if (urb->status)
        pr_debug("brcmbt_usb: Bulk OUT URB status %d\n", urb->status);

    kfree_skb(skb);
    usb_free_urb(urb);
}

static int brcmbt_hci_open(struct hci_dev *hdev)
{
    struct brcmbt_data *data = hci_get_drvdata(hdev);
    int err;

    bt_dev_dbg(hdev, "open");

    err = brcmbt_submit_bulk_in(data, GFP_KERNEL);
    if (err < 0)
        return err;

    if (data->int_in_ep) {
        err = brcmbt_submit_int_in(data, GFP_KERNEL);
        if (err < 0) {
            usb_kill_anchored_urbs(&data->bulk_anchor);
            return err;
        }
    }

    return 0;
}

static int brcmbt_hci_close(struct hci_dev *hdev)
{
    struct brcmbt_data *data = hci_get_drvdata(hdev);

    bt_dev_dbg(hdev, "close");

    usb_kill_anchored_urbs(&data->bulk_anchor);
    usb_kill_anchored_urbs(&data->tx_anchor);

    return 0;
}

static int brcmbt_hci_flush(struct hci_dev *hdev)
{
    struct brcmbt_data *data = hci_get_drvdata(hdev);

    bt_dev_dbg(hdev, "flush");
    usb_kill_anchored_urbs(&data->tx_anchor);

    return 0;
}

static int brcmbt_hci_send_frame(struct hci_dev *hdev, struct sk_buff *skb)
{
    struct brcmbt_data *data = hci_get_drvdata(hdev);
    struct urb *urb;
    int err;

    bt_dev_dbg(hdev, "send_frame len %d", skb->len);

    urb = usb_alloc_urb(0, GFP_ATOMIC);
    if (!urb)
        return -ENOMEM;

    usb_fill_bulk_urb(urb, data->udev,
                     usb_sndbulkpipe(data->udev, data->bulk_out_ep),
                     skb->data, skb->len,
                     brcmbt_bulk_out_complete, skb);

    urb->transfer_flags |= URB_ZERO_PACKET;

    usb_anchor_urb(urb, &data->tx_anchor);
    err = usb_submit_urb(urb, GFP_ATOMIC);
    if (err < 0) {
        bt_dev_err(hdev, "Failed to submit bulk OUT URB (%d)", err);
        usb_unanchor_urb(urb);
        usb_free_urb(urb);
        return err;
    }

    return 0;
}

static int brcmbt_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    struct usb_device *udev = interface_to_usbdev(intf);
    struct usb_host_interface *iface_desc;
    struct brcmbt_data *data;
    struct hci_dev *hdev;
    int i, error;

    dev_info(&intf->dev, "Broadcom Bluetooth USB device probing (VID: 0x%04x, PID: 0x%04x)\n", id->idVendor, id->idProduct);

    data = kzalloc(sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->udev = udev;
    data->intf = intf;
    init_usb_anchor(&data->bulk_anchor);
    init_usb_anchor(&data->tx_anchor);

    // Endpoint discovery
    iface_desc = intf->cur_altsetting;
    for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
        struct usb_endpoint_descriptor *ep = &iface_desc->endpoint[i].desc;

        if (usb_endpoint_is_bulk_in(ep) && !data->bulk_in_ep)
            data->bulk_in_ep = ep->bEndpointAddress;
        else if (usb_endpoint_is_bulk_out(ep) && !data->bulk_out_ep)
            data->bulk_out_ep = ep->bEndpointAddress;
        else if (usb_endpoint_is_int_in(ep) && !data->int_in_ep)
            data->int_in_ep = ep->bEndpointAddress;
    }

    if (!data->bulk_in_ep || !data->bulk_out_ep) {
        dev_err(&intf->dev, "Missing required bulk endpoints\n");
        error = -ENODEV;
        goto err_free_data;
    }

    // Allocate URBs
    data->bulk_in_urb = usb_alloc_urb(0, GFP_KERNEL);
    data->int_in_urb = data->int_in_ep ? usb_alloc_urb(0, GFP_KERNEL) : NULL;
    data->bulk_in_buffer = kmalloc(BULK_BUFFER_SIZE, GFP_KERNEL);
    data->int_in_buffer = data->int_in_ep ? kmalloc(INT_BUFFER_SIZE, GFP_KERNEL) : NULL;

    if (!data->bulk_in_urb || !data->bulk_in_buffer || (data->int_in_ep && (!data->int_in_urb || !data->int_in_buffer))) {
        error = -ENOMEM;
        goto err_free_urbs;
    }

    // Allocate HCI device
    hdev = hci_alloc_dev();
    if (!hdev) {
        error = -ENOMEM;
        goto err_free_urbs;
    }

    data->hdev = hdev;
    hdev->bus = HCI_USB;
    hci_set_drvdata(hdev, data);

    hdev->open  = brcmbt_hci_open;
    hdev->close = brcmbt_hci_close;
    hdev->flush = brcmbt_hci_flush;
    hdev->send  = brcmbt_hci_send_frame;

    SET_HCIDEV_DEV(hdev, &intf->dev);

    error = hci_register_dev(hdev);
    if (error < 0) {
        hci_free_dev(hdev);
        goto err_free_urbs;
    }

    usb_set_intfdata(intf, data);
    dev_info(&intf->dev, "Broadcom Bluetooth HCI registered successfully\n");
    return 0;

err_free_urbs:
    usb_free_urb(data->bulk_in_urb);
    usb_free_urb(data->int_in_urb);
    kfree(data->bulk_in_buffer);
    kfree(data->int_in_buffer);
err_free_data:
    kfree(data);
    return error;
}

static void brcmbt_disconnect(struct usb_interface *intf)
{
    struct brcmbt_data *data = usb_get_intfdata(intf);

    if (!data)
        return;

    dev_info(&intf->dev, "Broadcom Bluetooth USB device disconnected\n");

    usb_kill_anchored_urbs(&data->bulk_anchor);
    usb_kill_anchored_urbs(&data->tx_anchor);

    if (data->hdev) {
        hci_unregister_dev(data->hdev);
        hci_free_dev(data->hdev);
    }

    usb_free_urb(data->bulk_in_urb);
    usb_free_urb(data->int_in_urb);
    kfree(data->bulk_in_buffer);
    kfree(data->int_in_buffer);

    usb_set_intfdata(intf, NULL);
    kfree(data);
}

static int brcmbt_suspend(struct usb_interface *intf, pm_message_t message)
{
    struct brcmbt_data *data = usb_get_intfdata(intf);

    if (data && data->hdev)
        hci_suspend_dev(data->hdev);

    return 0;
}

static int brcmbt_resume(struct usb_interface *intf)
{
    struct brcmbt_data *data = usb_get_intfdata(intf);

    if (data && data->hdev)
        hci_resume_dev(data->hdev);

    return 0;
}

static struct usb_driver brcmbt_driver = {
    .name = "brcmbt_usb",
    .probe = brcmbt_probe,
    .disconnect = brcmbt_disconnect,
    .suspend = brcmbt_suspend,
    .resume = brcmbt_resume,
    .supports_autosuspend = 1,
    .id_table = brcmbt_table,
};

module_usb_driver(brcmbt_driver);
