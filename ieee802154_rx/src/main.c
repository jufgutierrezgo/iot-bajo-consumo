#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/ieee802154.h>
#include <zephyr/net/ieee802154_mgmt.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/sys/printk.h>

#define IEEE802154_CHANNEL 11  // Set the channel to your network's channel
#define IEEE802154_PAN_ID 0xABCD // Example PAN ID
#define IEEE802154_SHORT_ADDR 0x7777 // Example short address

// LOG_MODULE_REGISTER(ieee802154, LOG_LEVEL_DBG);
// LOG_MODULE_REGISTER(ieee802154_cc13xx_cc26xx, LOG_LEVEL_DBG);
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

// Function to process a received packet
void process_packet(struct net_pkt *pkt)
{
    struct net_buf *frag;
    size_t total_len = 0;

    for (frag = pkt->frags; frag; frag = frag->frags) {
        total_len += frag->len;
        printk("Frag. Length: %d\n", frag->len);
        printk("Frag Data: ");
        for (size_t i = 0; i < frag->len; i++) {
            printk("%02x ", frag->data[i]);
        }
        printk("\n");
    }
    printk("Total pkt length: %zu bytes\n", total_len);

    net_pkt_unref(pkt);
}

// Configure IEEE 802.15.4 settings
void configure_ieee802154(struct net_if *iface)
{
    struct ieee802154_req_params params = {0};

    // Set the channel
    params.channel = IEEE802154_CHANNEL;
    if (net_mgmt(NET_REQUEST_IEEE802154_SET_CHANNEL, iface, &params.channel, sizeof(params.channel)) < 0) {
        printk("Failed to set channel\n");
        return;
    }

    // Set PAN ID
    params.pan_id = IEEE802154_PAN_ID;
    if (net_mgmt(NET_REQUEST_IEEE802154_SET_PAN_ID, iface, &params.pan_id, sizeof(params.pan_id)) < 0) {
        printk("Failed to set PAN ID\n");
        return;
    }

    // Set short address
    params.short_addr = IEEE802154_SHORT_ADDR;
    if (net_mgmt(NET_REQUEST_IEEE802154_SET_SHORT_ADDR, iface, &params.short_addr, sizeof(params.short_addr)) < 0) {
        printk("Failed to set short address\n");
        return;
    }

    printk("IEEE 802.15.4 PAN ID, short address, and channel configured.\n");

    int ret = net_promisc_mode_on(iface);
	if (ret < 0) {
		LOG_INF("Cannot set promiscuous mode for interface %p (%d)",
			iface, ret);
		return;
	}

	LOG_INF("Promiscuous mode enabled for interface %p", iface);
}

void print_iface_info(struct net_if *iface) {
    if (!iface) {
        printk("Network interface not available\n");
        return;
    }

    // Print the interface index (unique identifier for the interface)
    printk("Interface index: %d\n", net_if_get_by_iface(iface));

    // Print the link address (MAC address or equivalent)
    struct net_linkaddr *link_addr = net_if_get_link_addr(iface);
    printk("Link address: ");
    for (int i = 0; i < link_addr->len; i++) {
        printk("%02x", link_addr->addr[i]);
        if (i < link_addr->len - 1) {
            printk(":");
        }
    }
    printk("\n");

    // Print if the interface is up or down
    if (net_if_is_up(iface)) {
        printk("Interface is up\n");
    } else {
        printk("Interface is down\n");
    }
}

void main(void)
{
    struct net_if *iface;
    struct net_pkt *pkt;

    // Get the default network interface
    iface = net_if_get_default();
    if (!iface) {
        printk("No network interface found\n");
        return;
    }
    
    // Configure PAN ID and short address
    configure_ieee802154(iface);
    net_if_up(iface);

    print_iface_info(iface);    

    printk("Waiting for packets.\n");

    // Keep running to receive packets
    while (1) {
        pkt = net_promisc_mode_wait_data(K_FOREVER);
		if (pkt) {
			process_packet(pkt);
		}

		net_pkt_unref(pkt);
    }
}