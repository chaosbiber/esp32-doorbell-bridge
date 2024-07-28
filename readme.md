## Doorbell Bridge

ESP 32 doorbell bridge, taking an 3,3V HIGH input signal to wakeup from sleep, connect to WiFi and send an MQTT message. On first boot it configures Home Assistant via [MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery) and creates an [MQTT Device trigger](https://www.home-assistant.io/integrations/device_trigger.mqtt/).

Created with Arduino IDE 2.+, ESP32 and [MQTT library](https://github.com/256dpi/arduino-mqtt).
