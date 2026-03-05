# NTP Integration Design

## Architecture
ESP32 uses the built-in SNTP client (lwIP stack) for time synchronization. No external NTP library needed.

## Server Configuration

### Priority Order
1. **Primary:** `192.168.1.100` - Local stratum-1 GPS-disciplined server
2. **Secondary:** `192.168.1.101` - Second local stratum-1 server (redundancy)
3. **Tertiary:** `pool.ntp.org` - Internet fallback

### Why Local Stratum-1
- Sub-millisecond accuracy on LAN (vs ~10-50ms for internet NTP)
- No internet dependency for time accuracy
- GPS-disciplined = traceable to UTC
- Already deployed in home network

## ESP32 SNTP Implementation

### Configuration
```cpp
configTime(0, 0, ntpServer1, ntpServer2, ntpServer3);
```
- First two args: GMT offset and daylight offset (both 0 for UTC)
- ESP32 handles server failover automatically
- Internal time maintained between syncs

### Sync Behavior
- **Initial sync:** Blocks until time acquired (startup only)
- **Ongoing sync:** ESP32 SNTP re-syncs automatically (default: 1 hour)
- **Configurable interval:** `sntp_set_sync_interval(ms)` for custom period
- **Sync mode:** SNTP_SYNC_MODE_SMOOTH (gradual adjustment, no time jumps)

### Time Access
```cpp
struct tm timeinfo;
getLocalTime(&timeinfo);  // Returns UTC (since offset=0)
```

## Sync Strategy

### Startup Sequence
1. Connect to WiFi
2. Configure NTP servers
3. Wait for initial time sync (blocking, with timeout)
4. If sync fails after 30s: retry WiFi connection
5. Once synced: begin WWVB transmission

### Ongoing Operation
- NTP re-sync every 1 hour (ESP32 default)
- If WiFi drops: ESP32 auto-reconnect
- During WiFi outage: continue transmitting with last-known time
- ESP32 crystal drift: ~20 ppm = ~1.7 seconds/day (acceptable for short outages)

### Failure Modes
| Failure | Impact | Recovery |
|---------|--------|----------|
| WiFi down | Time drifts ~2s/day | Auto-reconnect, NTP resync |
| NTP servers unreachable | Same as WiFi down | Retry on next sync interval |
| All servers fail | Transmit with drifting time | Continue retrying indefinitely |
| Power loss | Complete restart | WiFi connect → NTP sync → transmit |

### Accuracy Budget
```
NTP sync accuracy (LAN):     ~1 ms
ESP32 crystal between syncs:  ~70 μs/hour (at 20 ppm)
Total worst case:             ~1.1 ms
WWVB requirement:             ±50 ms
Margin:                       45× better than needed
```

## WiFi Configuration

### Connection
- SSID and password stored in `config.h`
- Auto-reconnect enabled (ESP32 default behavior)
- Static IP optional (faster reconnect, but DHCP is fine)

### Power Management
- WiFi modem sleep: enabled (reduces power between NTP syncs)
- WiFi auto-reconnect: enabled
- No WiFi power saving during active NTP sync

## Status Monitoring
- Serial output: NTP sync status, last sync time, current time
- LED: Different blink patterns for synced/unsynced/error states
- Uptime counter: Track continuous operation time
