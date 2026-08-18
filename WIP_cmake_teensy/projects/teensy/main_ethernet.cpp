#include <Arduino.h>
#include <QNEthernet.h>

using namespace qindesign::network;


// -------------------------
// Network configuration
// -------------------------

uint8_t mac[] =
{
    0x04, 0xE9, 0xE5, 0x00, 0x00, 0x01
};

IPAddress teensy_ip(192, 168, 1, 50);
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192, 168, 1, 1);
IPAddress dns(gateway);

IPAddress receiver_ip(192, 168, 1, 100);

constexpr uint16_t LOCAL_PORT = 4000;
constexpr uint16_t DEST_PORT  = 5000;


EthernetUDP udp;


// -------------------------
// Packet format
// -------------------------

constexpr uint32_t MAGIC = 0x54455354; // "TEST"


struct PacketHeader
{
    uint32_t magic;

    uint16_t version;
    uint16_t type;

    uint64_t timestamp_us;
    
    uint32_t sequence;

    uint16_t sample_count;
    uint16_t channels;
};


static_assert(sizeof(PacketHeader) == 24);


// -------------------------
// Audio payload
// -------------------------

constexpr uint16_t FRAMES = 128;
constexpr uint16_t CHANNELS = 2;

constexpr size_t SAMPLE_COUNT = FRAMES * CHANNELS;


struct AudioPacket
{
    PacketHeader header;
    int16_t samples[SAMPLE_COUNT];
};


AudioPacket packet;

uint32_t sequence = 0;


// -------------------------
// Setup
// -------------------------

void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println("Starting Ethernet");

    
    
    // Start Ethernet hardware
    if (!Ethernet.begin(mac, teensy_ip, gateway, dns, subnet)) {
        Serial.println("Ethernet.begin failed");
    }
    
        // Static IP configuration
        Ethernet.setLocalIP(teensy_ip);
        Ethernet.setSubnetMask(subnet);
        Ethernet.setGatewayIP(gateway);

    while (!Ethernet.linkStatus()) {
        Serial.println("Waiting for link...");
        delay(100);
    }

    Serial.println("Link is up!");

    udp.begin(LOCAL_PORT);


    delay(1000);


    Serial.print("IP: ");
    Serial.println(Ethernet.localIP());

    Serial.print("Link: ");
    Serial.println(Ethernet.linkStatus());

    Serial.print("IP: ");
    Serial.println(Ethernet.localIP());

    Serial.print("Gateway: ");
    Serial.println(Ethernet.gatewayIP());

    Serial.print("Subnet: ");
    Serial.println(Ethernet.subnetMask());

    Serial.println("UDP started");
}


// -------------------------
// Main loop
// -------------------------

void loop()
{
    packet.header.magic = MAGIC;

    packet.header.version = 1;
    packet.header.type = 1;

    packet.header.sequence = sequence++;

    packet.header.timestamp_us = micros();

    packet.header.sample_count = FRAMES;
    packet.header.channels = CHANNELS;


    // Replace this with Audio library output
    for (size_t i = 0; i < SAMPLE_COUNT; i++)
    {
        packet.samples[i] = i;
    }


    udp.beginPacket(
        receiver_ip,
        DEST_PORT
    );

    udp.write(
        reinterpret_cast<uint8_t*>(&packet),
        sizeof(packet)
    );

    udp.endPacket();


    delay(10);
}