#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

/* ========= RX CONTROL (ESP8266 SDK) ========= */
typedef struct {
  signed rssi : 8;
  unsigned rate : 4;
  unsigned is_group : 1;
  unsigned : 1;
  unsigned sig_mode : 2;
  unsigned legacy_length : 12;
  unsigned damatch0 : 1;
  unsigned damatch1 : 1;
  unsigned bssidmatch0 : 1;
  unsigned bssidmatch1 : 1;
  unsigned MCS : 7;
  unsigned CWB : 1;
  unsigned HT_length : 16;
  unsigned Smoothing : 1;
  unsigned Not_Sounding : 1;
  unsigned : 1;
  unsigned Aggregation : 1;
  unsigned STBC : 2;
  unsigned FEC_CODING : 1;
  unsigned SGI : 1;
  unsigned rxend_state : 8;
  unsigned ampdu_cnt : 8;
  unsigned channel : 4;
  unsigned : 12;
} RxControl;

typedef struct {
  RxControl rx_ctrl;
  uint8_t buf[128];   // payload (truncated)
  uint16_t cnt;
  uint16_t len;       // actual payload length
} sniffer_buf;

/* ========= 802.11 MAC ========= */
typedef struct {
  uint16_t frame_ctrl;
  uint16_t duration;
  uint8_t addr1[6];
  uint8_t addr2[6];
  uint8_t addr3[6];
  uint16_t seq_ctrl;
} __attribute__((packed)) wifi_ieee80211_mac_hdr_t;

/* ========= LLC / SNAP ========= */
typedef struct {
  uint8_t dsap;
  uint8_t ssap;
  uint8_t control;
  uint8_t oui[3];
  uint16_t ethertype;
} __attribute__((packed)) llc_snap_hdr_t;

/* ========= EAPOL ========= */
typedef struct {
  uint8_t version;
  uint8_t type;
  uint16_t length;
} __attribute__((packed)) eapol_hdr_t;

/* ========= PROMISCUOUS CALLBACK ========= */
static void promisc_cb(uint8_t *buf, uint16_t len) {

  sniffer_buf *sniffer = (sniffer_buf *)buf;

  uint16_t sig_len = sniffer->len;
  if (sig_len < 24) return;      // MAC header minimum
  if (sig_len > 128) return;     // safety

  int8_t rssi = sniffer->rx_ctrl.rssi;
  uint8_t *payload = sniffer->buf;

  /* ---- 802.11 MAC ---- */
  wifi_ieee80211_mac_hdr_t *mac =
      (wifi_ieee80211_mac_hdr_t *)payload;

  uint16_t fc = mac->frame_ctrl;

  // DATA frame only
  if ((fc & 0x000C) != 0x0008) return;

  bool toDS   = fc & (1 << 8);
  bool fromDS = fc & (1 << 9);

  int mac_len = 24;
  if (toDS && fromDS) mac_len = 30;   // Addr4
  if (fc & 0x0080) mac_len += 2;       // QoS

  if (sig_len < mac_len + sizeof(llc_snap_hdr_t)) return;

  /* ---- LLC / SNAP ---- */
  llc_snap_hdr_t *llc =
      (llc_snap_hdr_t *)(payload + mac_len);

  if (llc->dsap != 0xAA || llc->ssap != 0xAA) return;
  if (ntohs(llc->ethertype) != 0x888E) return; // EAPOL

  /* ---- EAPOL ---- */
  if (sig_len < mac_len + sizeof(llc_snap_hdr_t) + sizeof(eapol_hdr_t)) return;

  eapol_hdr_t *eapol =
      (eapol_hdr_t *)(payload + mac_len + sizeof(llc_snap_hdr_t));

  uint16_t eapol_len = ntohs(eapol->length);

  Serial.println("\n===== EAPOL DETECTED =====");
  Serial.printf("RSSI: %d dBm\n", rssi);
  Serial.printf("EAPOL type: %u\n", eapol->type);
  Serial.printf("EAPOL length: %u\n", eapol_len);
}

/* ========= SETUP ========= */
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();

  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);

  wifi_set_channel(11);   // 🔁 đổi channel nếu cần

  wifi_set_promiscuous_rx_cb(promisc_cb);
  wifi_promiscuous_enable(1);

  Serial.println("ESP8266 EAPOL sniffer started");
}

/* ========= LOOP ========= */
void loop() {
  // nothing
}
