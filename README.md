# ESPHome BTHome Button

Kleine External Component für **BTHome v2 Tasten**. Kein Sensor-Baukasten, kein Schlüssel.

Zwei Richtungen, dieselbe Air-Schnittstelle:

| Richtung | ESPHome | Auf der Luft |
|---|---|---|
| Empfangen | ein `binary_sensor` pro Geste, kurz ON dann OFF | Objekt `0x3A` |
| Senden | Action `bthome.transmit` | `0x44`, Packet-ID, `0x3A` |

Das ist dasselbe Muster wie `remote_receiver` und `remote_transmitter`: ein Code, ein Binary Sensor; senden ist eine Action, die ein Template-Button oder ein GPIO aufruft.

Benötigt ESPHome **2026.8** oder neuer und einen ESP32. Der Empfang hängt an `ble_device_base`, das Senden an `esp32_ble`.

## Pakete

Der Baum ist absichtlich schmal. Jede Schicht ist eine eigene Datei und kann allein reviewt werden.

1. **Codec** — `components/bthome/codec.*`, Host-Test `test/test_codec.cpp`. Device-Info, Packet-ID, Button-Objekt, Überspringen anderer fester Objekte (zum Beispiel Batterie davor). Verschlüsselte Pakete werden erkannt und verworfen.
2. **Senden** — `bthome:` plus Action `bthome.transmit`. Burst, dann Funkstille. Packet-ID liegt im Flash und steigt pro Geste.
3. **Empfangen** — `binary_sensor` Plattform. Pro `(mac, index, event)` ein Puls.
4. **Manufacturer Data** — optionales Rohfeld, siehe unten.

Noch nicht drin, und bewusst nicht in diesem Stand: Bindkey, Dimmer `0x3C`, Command `0x3B`, Messwerte. Die kommen als eigene Änderungen auf denselben Codec, nicht als zweites Protokoll.

## Empfangen

```yaml
esp32_ble_tracker:

bthome:

binary_sensor:
  - platform: bthome
    mac_address: "3C:2E:F5:71:D5:2A"
    type: button
    index: 1
    event: press
    name: "Taste kurz"
    on_press:
      - switch.toggle: relay
```

`index` ist die Position des `0x3A`-Objekts, bei einer Taste immer 1. Mehrere Tasten in einem Paket bleiben in dieser Reihenfolge.

`event` ist eine der Gesten:

| `event` | Byte |
|---|---|
| `press` | `0x01` |
| `double_press` | `0x02` |
| `triple_press` | `0x03` |
| `long_press` | `0x04` |
| `long_double_press` | `0x05` |
| `long_triple_press` | `0x06` |
| `hold` | `0x80` |

`0x00` löst nichts aus. `0xFE` wird nur beim Empfang von `hold` wie `0x80` gewertet. Dieselbe Packet-ID im Burst zählt einmal. Ohne Packet-ID gilt eine Pause von 1,2 s. `pulse_length` (Default 200 ms) ist die Zeit, die der Binary Sensor ON bleibt. `on_press` läuft auf dem ESP.

Verschlüsselte Advertisements werden ignoriert. Dafür gibt es noch keinen Schlüssel.

## Senden

```yaml
esp32_ble:

bthome:
  burst_duration: 1500ms

button:
  - platform: template
    name: "Kurz"
    on_press:
      - bthome.transmit:
          event: press
          index: 1
```

Die Action baut:

```text
D2 FC          BTHome UUID
44             Version 2, nur bei Ereignis
00 <id>        Packet-ID, steigt bei jedem Aufruf, bleibt über Neustart erhalten
3A <event>     Taste
```

`index: 2` sendet davor `3A 00`, damit die zweite Taste die zweite Stelle belegt. Der ESP wiederholt das Paket für `burst_duration` (Default 1,5 s) und hört dann auf. Das Intervall ist das des ESP32-BLE-Advertisers. Der Gerätename wird für den Burst aus dem Advertisement genommen, sonst passt die BTHome-Payload nicht in die 31 Byte.

Die Bluetooth-Adresse muss die feste öffentliche Adresse des ESP sein. Eine wechselnde Privacy-Adresse ist für den Empfänger jedes Mal ein anderes Gerät. Diese Komponente schaltet die Adresse nicht um.

## Manufacturer Data

Optional. Beides zusammen, oder keins:

```yaml
bthome:
  manufacturer_id: 0xFFFF
  manufacturer_data: "01"
```

`manufacturer_id` ist die 16-bit Company ID aus der BLE-Spec, little-endian im Advertisement. `manufacturer_data` ist die Hex-Payload **dahinter**, höchstens 15 Byte. Es gibt keinen Default und keine hinterlegten Inhalte. Was dort stehen muss, steht in der Spec des Empfängers, nicht hier.

## Test

```bash
g++ -std=c++17 -Wall -Wextra -o test/test_codec test/test_codec.cpp components/bthome/codec.cpp
./test/test_codec
```

Der Firmware-Teil lässt sich hier nicht übersetzen. Den prüft ein normales ESPHome-Compile gegen 2026.8 oder neuer.
