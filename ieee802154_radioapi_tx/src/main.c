#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/sys/printk.h>
// #include <zephyr/random/random.h>

LOG_MODULE_REGISTER(radioapi_tx, LOG_LEVEL_DBG);

#define IEEE802154_CHANNEL 11  // Set the channel to your network's channel
#define IEEE802154_PAN_ID 0xABCD // Example PAN ID
#define IEEE802154_SHORT_ADDR 0x1234 // Example short address
#define TX_PACKET_SIZE 31

// Define macros without endianness
#define FRAME_CONTROL  0xD841  // Frame control (MSB first)
#define SEQUENCE_NUMBER 0x2B   // Sequence number
#define PAN_ID        0xABCD   // PAN ID (MSB first)
#define BROADCAST     0xFFFF   // Broadcast address (MSB first)
#define MAC_ADDR      0x00124B00219FB2EB // MAC address (MSB first)
#define PAYLOAD       { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, \
                        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F }
#define FCS           0x56   // Frame Check Sequence (MSB first)

#define TX_PACKET_SIZE 31  // Total packet size

/* ieee802.15.4 device */
static struct ieee802154_radio_api *radio_api;
static const struct device *const ieee802154_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_ieee802154));
uint8_t mac_addr[8]; /* in little endian */

// Frame construction
uint8_t frame[TX_PACKET_SIZE];

void construct_frame(uint8_t sequence_num) {
    uint8_t payload[] = PAYLOAD;

    // Pointer to the frame buffer
    uint8_t *ptr = frame;

    // Frame Control
    sys_put_le16(FRAME_CONTROL, ptr);
    ptr += 2;

    // Sequence Number
    *ptr++ = sequence_num;

    // PAN ID
    sys_put_le16(PAN_ID, ptr);
    ptr += 2;

    // Broadcast Address
    sys_put_le16(BROADCAST, ptr);
    ptr += 2;

    // MAC Address
    sys_put_le64(MAC_ADDR, ptr);
    ptr += 8;

    // Payload
    for (int i = 0; i < sizeof(payload); i++) {
        *ptr++ = payload[i];
    }

    // Frame Check Sequence
    sys_put_le16(FCS, ptr);
}


/* Initialize the IEEE 802.15.4 interface */
static bool init_ieee802154(void) {
    LOG_INF("Initializing IEEE 802.15.4");

    if (!device_is_ready(ieee802154_dev)) {
        LOG_ERR("IEEE 802.15.4 device not ready");
        return false;
    }

    radio_api = (struct ieee802154_radio_api *)ieee802154_dev->api;

    /* Set the channel */
    radio_api->set_channel(ieee802154_dev, IEEE802154_CHANNEL);

    /* Start the radio */
    radio_api->start(ieee802154_dev);

    return true;
}

/* Transmit a packet over the IEEE 802.15.4 interface */
// void transmit_packet(struct net_if *iface)
void transmit_packet(struct net_if *iface)
{
    struct net_pkt *pkt;
       
    LOG_INF("Transmitting packet");

    /* Allocate a packet buffer with the specified size */
    pkt = net_pkt_alloc_with_buffer(iface, TX_PACKET_SIZE, AF_UNSPEC, 0, K_NO_WAIT);
    // net_pkt_alloc_buffer_raw(pkt, TX_PACKET_SIZE, K_NO_WAIT);
    	
    if (!pkt) {
        LOG_ERR("Failed to allocate packet buffer");
        return;
    }

    /* Write data to the packet */
    if (net_pkt_write(pkt, frame, sizeof(frame))) {
        LOG_ERR("Failed to write data to packet");
        net_pkt_unref(pkt);
        return;
    }

    /* Transmit the packet using the radio API with CSMA-CA mode */
    if (radio_api->tx(ieee802154_dev, IEEE802154_TX_MODE_CSMA_CA, pkt, pkt->buffer) < 0) {
        LOG_ERR("Failed to transmit packet");
    } else {
        LOG_INF("Packet transmitted successfully");
    }

    /* Free the packet */
    net_pkt_unref(pkt);
}

void main(void)
{
    uint8_t seq_num;
    /* Initialize the IEEE 802.15.4 device */
    if (!init_ieee802154()) {
        LOG_ERR("Unable to initialize ieee802154");
        return;
    }
    
    struct net_if *iface = net_if_get_default();
    
    while (1) {
        /* Transmit packets every 1 second */
        construct_frame(++seq_num);
        transmit_packet(iface);
        k_sleep(K_MSEC(1000));
    }

    return 0;
}
