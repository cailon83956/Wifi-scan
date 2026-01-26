#include <ESP8266WiFi.h>
extern "C" {
  #include "user_interface.h"
}

/* ===== 802.11 MAC header (24 bytes, non-QoS) ===== */
typedef struct {
  uint16_t frame_ctrl;
  uint16_t duration;
  uint8_t addr1[6];
  uint8_t addr2[6];
  uint8_t addr3[6];
  uint16_t seq_ctrl;
} __attribute__((packed)) wifi_ieee80211_mac_hdr_t;

/* ===== LLC + SNAP ===== */
typedef struct {
  uint8_t dsap;
  uint8_t ssap;
  uint8_t control;
  uint8_t oui[3];
  uint16_t ethertype;
} __attribute__((packed)) llc_snap_hdr_t;

/* ===== EAPOL header ===== */
typedef struct {
  uint8_t version;
  uint8_t type;
  uint16_t length;
} __attribute__((packed)) eapol_hdr_t;

/* ===== Promiscuous callback ===== */
static void promisc_cb(uint8_t *buf, uint16_t len) {

  if (len < 60) return;

  /* --- RX control (ESP8266: 12 bytes) --- */
  struct RxControl *rx = (struct RxControl *)buf;
  int8_t rssi = rx->rssi;

  uint8_t *payload = buf + sizeof(struct RxControl);
  uint16_t payload_len = len - sizeof(struct RxControl);

  if (payload_len < sizeof(wifi_ieee80211_mac_hdr_t) + sizeof(llc_snap_hdr_t))
    return;

  /* --- 802.11 MAC --- */
  wifi_ieee80211_mac_hdr_t *mac =
      (wifi_ieee80211_mac_hdr_t *)payload;

  uint16_t fc = mac->frame_ctrl;

  /* Chỉ xử lý DATA frame */
  if ((fc & 0x000C) != 0x0008) return;

  bool toDS   = fc & (1 << 8);
  bool fromDS = fc & (1 << 9);

  int mac_len = 24;
  if (toDS && fromDS) mac_len = 30;   // Addr4
  if (fc & 0x0080) mac_len += 2;       // QoS

  if (payload_len < mac_len + sizeof(llc_snap_hdr_t))
    return;

  /* --- LLC / SNAP --- */
  llc_snap_hdr_t *llc =
      (llc_snap_hdr_t *)(payload + mac_len);

  if (llc->dsap != 0xAA || llc->ssap != 0xAA) return;
  if (llc->ethertype != htons(0x888E)) return;  // EAPOL

  /* --- EAPOL --- */
  eapol_hdr_t *eapol =
      (eapol_hdr_t *)(payload + mac_len + sizeof(llc_snap_hdr_t));

  uint16_t eapol_len = ntohs(eapol->length);
  uint16_t eapol_total = sizeof(eapol_hdr_t) + eapol_len;

  if (payload_len < mac_len + sizeof(llc_snap_hdr_t) + eapol_total)
    return;

  /* --- OUTPUT --- */
  Serial.println("\n===== EAPOL DETECTED =====");
  Serial.printf("RSSI: %d dBm\n", rssi);
  Serial.printf("EAPOL type: %u\n", eapol->type);
  Serial.printf("EAPOL length: %u\n", eapol_len);

  uint8_t *raw = (uint8_t *)eapol;
  Serial.print("EAPOL hex: ");
  for (int i = 0; i < eapol_total; i++) {
    if (raw[i] < 16) Serial.print("0");
    Serial.print(raw[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

/* ===== SETUP ===== */
void setup() {
  Serial.begin(115200);
  Serial.println();

  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);

  /* ===== FIX CHANNEL 11 ===== */
  wifi_set_channel(11);

  wifi_set_promiscuous_rx_cb(promisc_cb);
  wifi_promiscuous_enable(1);

  Serial.println("ESP8266 sniffing EAPOL on CHANNEL 11");
}

/* ===== LOOP ===== */
void loop() {
  // Không cần làm gì
}
