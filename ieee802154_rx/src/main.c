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
#define IEEE802154_SHORT_ADDR 0x1230 // Example short address

// LOG_MODULE_REGISTER(ieee802154, LOG_LEVEL_DBG);
// LOG_MODULE_REGISTER(ieee802154_cc13xx_cc26xx, LOG_LEVEL_DBG);
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

struct ieee802154_rx_data {
    uint8_t len;
    uint8_t raw_data[128];
};

extern struct ieee802154_rx_data rx_data_store;

void print_received_data_from_ieee802154_driver(void)
{
    printk("Received packet length: %d\n", rx_data_store.len);
    printk("Received raw data: ");
    for (int i = 0; i < rx_data_store.len; i++) {
        printk("%02x ", rx_data_store.raw_data[i]);
    }
    printk("\n");
}

// Function to process a received packet
void process_packet(struct net_pkt *pkt)
{
    struct net_buf *frag;
    size_t total_len = 0;

    for (frag = pkt->frags; frag; frag = frag->frags) {
        total_len += frag->len;
        printk("Fragment Length: %d\n", frag->len);
        printk("Fragment Data: ");
        for (size_t i = 0; i < frag->len; i++) {
            printk("%02x ", frag->data[i]);
        }
        printk("\n");
    }
    printk("Total packet length: %zu bytes\n", total_len);

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

    // Print the type of the interface
    // printk("Interface type: %d\n", iface->if_dev->l2->type);

    // Print if the interface is up or down
    if (net_if_is_up(iface)) {
        printk("Interface is up\n");
    } else {
        printk("Interface is down\n");
    }

    // Print whether the interface is a point-to-point (P2P) interface
    if (net_if_flag_is_set(iface, NET_IF_POINTOPOINT)) {
        printk("Interface is point-to-point\n");
    } else {
        printk("Interface is not point-to-point\n");
    }

    // Print the MTU (Maximum Transmission Unit)
    printk("MTU: %d bytes\n", net_if_get_mtu(iface));
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

    // printk("%d\n", net_if_up(iface));
    
    // Configure PAN ID and short address
    configure_ieee802154(iface);

    print_iface_info(iface);    

    // net_pkt_cursor_init(pkt);

    // Assign the interface to the network packet
    // net_pkt_set_iface(pkt, iface);

    // Keep running to receive packets
    while (1) {
        // Poll for incoming packets on the interface
        // pkt = net_pkt_alloc(K_NO_WAIT);
       
        // net_pkt_read(pkt, data, 25);
        // pkt = net_pkt_rx_alloc_with_buffer(
		// 		iface, 25, AF_UNSPEC, 0, K_NO_WAIT);
        // if (pkt && net_recv_data(iface, pkt) == 0) {
        //     printk("Packet received on interface: %p\n", iface);
        //     process_packet(pkt);
        // } else {
        //     net_pkt_unref(pkt);
        // }

        // print_received_data_from_ieee802154_driver();

        k_sleep(K_MSEC(1000)); // Keep the thread alive
    }
}