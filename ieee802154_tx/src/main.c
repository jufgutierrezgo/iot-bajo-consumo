#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/ieee802154.h>
#include <zephyr/net/ieee802154_mgmt.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/sys/printk.h>

#define IEEE802154_CHANNEL 11  // Set the channel to your network's channel
#define IEEE802154_PAN_ID 0xABCD // Example PAN ID
#define IEEE802154_SHORT_ADDR 0x1234 // Example short address
#define TX_PACKET_SIZE 16

// Network key for encrypted communication (16 bytes for AES-128)
static const uint8_t network_key[16] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};

void configure_ieee802154(struct net_if *iface)
{
    struct ieee802154_req_params params = {0}; // Initialize the params structure

    // Set the channel for the transceiver
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
    

    // // Configure the network key (for security)
    // if (net_mgmt(NET_REQUEST_IEEE802154_SET_SECURITY_KEY, iface, (void *)network_key, sizeof(network_key)) < 0) {
    //     printk("Failed to set network key\n");
    //     return;
    // }

    printk("IEEE 802.15.4 PAN ID, short address, channel, and network key configured.\n");
}


void transmit_packet(struct net_if *iface)
{
    struct net_pkt *pkt;
    int ret;

    uint8_t tx_data[TX_PACKET_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}; // Example payload

    // Allocate a network packet with TX data
    pkt = net_pkt_alloc_with_buffer(iface, TX_PACKET_SIZE, AF_UNSPEC, 0, K_FOREVER);
    if (!pkt) {
        printk("Failed to allocate packet\n");
        return;
    }

    // Add data to the packet
    ret = net_pkt_write(pkt, tx_data, TX_PACKET_SIZE);
    if (ret < 0) {
        printk("Failed to add data to packet: %d\n", ret);
        net_pkt_unref(pkt);
        return;
    }

    // Finalize the packet
    net_pkt_cursor_init(pkt);

    // Send the packet via the network interface
    ret = net_if_send_data(iface, pkt);
    if (ret == 0) {
        printk("Packet sent successfully\n");
    } else {
        printk("Failed to send packet: %d\n", ret);
    }

    // Cleanup
    net_pkt_unref(pkt);
}

void main(void)
{
    struct net_if *iface;

    // Get the default network interface
    iface = net_if_get_default();
    if (!iface) {
        printk("No network interface found\n");
        return;
    }

    // Configure IEEE 802.15.4 parameters
    configure_ieee802154(iface);

    while (1) {
        transmit_packet(iface);
        k_sleep(K_MSEC(1000)); // Send packets every second
    }
}
