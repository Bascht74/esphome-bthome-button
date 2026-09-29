#include <cstdio>
#include <cstring>

#include "../components/bthome/codec.h"

static int failures = 0;

static void expect(bool cond, const char *name) {
  if (!cond) {
    std::printf("FAIL %s\n", name);
    failures++;
  }
}

int main() {
  uint8_t buf[32];
  size_t n = 0;
  expect(bthome::codec::encode_button(7, 0x01, 1, buf, sizeof(buf), &n), "encode press");
  const uint8_t press[] = {0x44, 0x00, 0x07, 0x3A, 0x01};
  expect(n == sizeof(press) && std::memcmp(buf, press, n) == 0, "press bytes");

  expect(bthome::codec::encode_button(1, 0x02, 2, buf, sizeof(buf), &n), "encode second");
  const uint8_t second[] = {0x44, 0x00, 0x01, 0x3A, 0x00, 0x3A, 0x02};
  expect(n == sizeof(second) && std::memcmp(buf, second, n) == 0, "second bytes");
  expect(!bthome::codec::encode_button(1, 0x01, 0, buf, sizeof(buf), &n), "index 0 rejected");

  bthome::codec::Parsed parsed{};
  expect(bthome::codec::parse(press, sizeof(press), &parsed) && parsed.ok, "parse press");
  expect(parsed.trigger_based && parsed.has_packet_id && parsed.packet_id == 7, "pid 7");
  expect(parsed.button_count == 1 && parsed.buttons[0] == 0x01, "button press");

  const uint8_t with_battery[] = {0x44, 0x00, 0x05, 0x01, 0x64, 0x3A, 0x01};
  expect(bthome::codec::parse(with_battery, sizeof(with_battery), &parsed), "skip battery");
  expect(parsed.packet_id == 5 && parsed.button_count == 1 && parsed.buttons[0] == 0x01, "battery then button");

  const uint8_t hold_alias[] = {0x44, 0x00, 0x09, 0x3A, 0xFE};
  expect(bthome::codec::parse(hold_alias, sizeof(hold_alias), &parsed), "parse fe");
  expect(parsed.buttons[0] == 0xFE, "raw fe kept");

  uint8_t mac_included[1 + 6 + 4] = {0x46, 1, 2, 3, 4, 5, 6, 0x00, 0x03, 0x3A, 0x04};
  expect(bthome::codec::parse(mac_included, sizeof(mac_included), &parsed), "mac included");
  expect(parsed.mac_included && parsed.packet_id == 3 && parsed.buttons[0] == 0x04, "long after mac");

  const uint8_t encrypted[] = {0x45, 0x11, 0x22};
  expect(!bthome::codec::parse(encrypted, sizeof(encrypted), &parsed) && parsed.encrypted, "encrypted rejected");

  const uint8_t unknown_after[] = {0x44, 0x3A, 0x01, 0x99};
  expect(bthome::codec::parse(unknown_after, sizeof(unknown_after), &parsed), "unknown stops");
  expect(parsed.ok && parsed.button_count == 1 && parsed.buttons[0] == 0x01, "button kept");

  const uint8_t empty_text[] = {0x44, 0x53, 0x00, 0x3A, 0x01};
  expect(bthome::codec::parse(empty_text, sizeof(empty_text), &parsed), "empty text");
  expect(parsed.ok && parsed.button_count == 1 && parsed.buttons[0] == 0x01, "button after empty text");

  const uint8_t truncated_text[] = {0x44, 0x3A, 0x02, 0x53, 0x04, 0x41};
  expect(bthome::codec::parse(truncated_text, sizeof(truncated_text), &parsed), "truncated text stops");
  expect(parsed.ok && parsed.button_count == 1 && parsed.buttons[0] == 0x02, "button before truncated text");

  const uint8_t v1[] = {0x02, 0x3A, 0x01};
  expect(!bthome::codec::parse(v1, sizeof(v1), &parsed), "v1 rejected");

  if (failures != 0) {
    std::printf("%d failed\n", failures);
    return 1;
  }
  std::printf("ok\n");
  return 0;
}
