# ESP8266-RFID-Access-Control-System
WiFi-connected RFID access control system built on ESP8266 + MFRC522 — grants/denies access by card and logs every attempt to a live web dashboard.
`esp8266` `rfid` `mfrc522` `arduino` `iot` `access-control` `nodemcu`

## Features
- Reads RFID card/keyfob UIDs over SPI
- Local whitelist of authorized cards
- Green LED + buzzer + relay pulse on success (drive a lock/strike/anything)
- Red LED + buzzer on denial
- NTP-synced timestamps
- Built-in web page at the device's IP showing the last 20 access attempts
- Telegram push notification to your phone on every scan (granted or denied)

## Hardware

| Part | Notes |
|---|---|
| NodeMCU ESP8266 (or Wemos D1 mini) | Main controller |
| MFRC522 RFID reader | 13.56 MHz |
| 5V relay module | Drives the lock/strike |
| Red LED + Green LED + 220Ω resistors | Status indicators |
| Active buzzer | Audio feedback |

## Wiring

| MFRC522 Pin | NodeMCU Pin |
|---|---|
| SDA (SS) | D4 |
| SCK | D5 |
| MOSI | D7 |
| MISO | D6 |
| RST | D3 |
| 3.3V | 3V3 (**not** 5V) |
| GND | GND |

| Other | Pin |
|---|---|
| Green LED | D0 |
| Red LED | D8 |
| Buzzer | D2 |
| Relay IN | D1 |

## Setup

1. Install the required libraries in the Arduino IDE (Library Manager):
   - `MFRC522` by GithubCommunity
   - `NTPClient` by Fabrice Weinberg
   - ESP8266 board package (adds `ESP8266WiFi` / `ESP8266WebServer` /
     `ESP8266HTTPClient` / `WiFiClientSecure`, all bundled — no separate install needed)
2. Clone this repo.
3. Copy the config template and fill in your own values:
   ```bash
   cp config.h.example config.h
   ```
4. Edit `config.h` with your WiFi SSID/password and your card UID(s).
   - To find a card's UID: flash the sketch once, open Serial Monitor at
     115200 baud, and scan the card — its UID prints even before it's
     authorized.
5. Set up Telegram notifications (optional but recommended):
   - Open Telegram, message **@BotFather**, send `/newbot`, and follow the
     prompts. You'll get a bot token like `123456789:AAF...`.
   - Send your new bot any message (e.g. "hi") so it registers a chat with you.
   - Visit `https://api.telegram.org/bot<YOUR_TOKEN>/getUpdates` in a browser
     and find `"chat":{"id": ...}` in the JSON response — that's your chat ID.
   - Put both values into `config.h` as `TELEGRAM_BOT_TOKEN` and
     `TELEGRAM_CHAT_ID`.
6. Upload `RFID_Access_Control.ino` to the board.
7. Visit the IP address printed in Serial Monitor to view the live access log.
   You should also get a Telegram notification on your phone every time a
   card is scanned, granted or denied.

## Notes
- `config.h` is gitignored — your WiFi credentials and card list never leave
  your machine.
- The relay stays energized for 3 seconds per successful scan; adjust the
  `delay(3000)` in `grantAccess()` in the `.ino` file to change that.
- If driving a real electric strike/lock, power it from a separate supply
  sized for the lock, not from the NodeMCU's 5V pin.

## License
This project is licensed under the [GNU General Public License v3.0](LICENSE).
You're free to use, modify, and distribute it, but any derivative work you
distribute must also be open-sourced under GPLv3.
