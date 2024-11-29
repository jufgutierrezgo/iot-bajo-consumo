#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/sys/printk.h>
// #include <zephyr/random/random.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(radioapi_rx, LOG_LEVEL_DBG);

#define RX_PACKET_SIZE 128  // Example size for receiving packets
#define IEEE802154_CHANNEL 11  // Set the channel to your network's channel
#define IEEE802154_PAN_ID 0xABCD // Example PAN ID
#define IEEE802154_SHORT_ADDR 0x1234 // Example short address

uint8_t mac_addr[8]; /* in little endian */

/* IEEE802.15.4 frame + 1 byte len + 1 byte LQI */
uint8_t tx_buf[IEEE802154_MAX_PHY_PACKET_SIZE + 1 + 1];

/* ieee802.15.4 device */
static struct ieee802154_radio_api *radio_api;
static const struct device *const ieee802154_dev =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_ieee802154));

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

/**
 * Interface to the network stack, will be called when the packet is
 * received
 */
int net_recv_data(struct net_if *iface, struct net_pkt *pkt)
{
	LOG_DBG("Received pkt %p, len %d", pkt, net_pkt_get_len(pkt));

	process_packet(pkt);

	return 0;
}

enum net_verdict ieee802154_handle_ack(struct net_if *iface, struct net_pkt *pkt)
{
	return NET_CONTINUE;
}

/* Initialize the IEEE 802.15.4 interface */
static bool init_ieee802154(void)
{
    LOG_INF("Initialize ieee802.15.4");

    if (!device_is_ready(ieee802154_dev)) {
        LOG_ERR("IEEE 802.15.4 device not ready");
        return false;
    }

    radio_api = (struct ieee802154_radio_api *)ieee802154_dev->api;

    if (IEEE802154_HW_FILTER & radio_api->get_capabilities(ieee802154_dev)) {
        struct ieee802154_filter filter;
        uint16_t short_addr;

        /* Set short address */
        short_addr = (mac_addr[0] << 8) + mac_addr[1];
        filter.short_addr = short_addr;
       
        /* Set PAN ID */
        filter.pan_id = IEEE802154_PAN_ID;
        radio_api->filter(ieee802154_dev, true, IEEE802154_FILTER_TYPE_PAN_ID, &filter);
    }

    /* Set the channel */
    radio_api->set_channel(ieee802154_dev, IEEE802154_CHANNEL);

    /* Start the radio */
    radio_api->start(ieee802154_dev);

    return true;
}

void main(void)
{   
    /* Initialize the IEEE 802.15.4 device */
    if (!init_ieee802154()) {
        LOG_ERR("Unable to initialize ieee802154");
        return;
    }

    /* Receive and print packets in a loop */
    while (1) {
        // receive_packet();  // Call the receive function
        k_sleep(K_SECONDS(1));  // Sleep between packet receptions
    }
}
