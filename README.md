# ESP8266 AutoWater
Automatic watering based on ESP8266

Utilizes [WiFiManager](https://github.com/tzapu/WiFiManager) to setup WiFi connection and MQTT server address. [Arduino Client for MQTT](https://github.com/knolleary/pubsubclient) is used to handle publish/subscribe operations with MQTT server.

## MQTT enabled

If AutoWater is connected to MQTT server, it will publish sensor readings and subscribe to pump commands.

Below is a list of topics to which AutoWater publish/subscribe. In each case **{id}** is device id set during configuration.

If MQTT server IP address or port is not set up during configuration, AutoWater will not try to connect to MQTT server. Otherwise, it will try to connect to it every 5s. During connection attempt inputs are not read and outputs are not set, so it is suggested to configure AutoWater correctly, depending on whether MQTT server will be used or not.

### Publish

AutoWater publishes sensor states and pump state. Values are published only when they change.

|         Topic         | Values |                                              Description                                              |
|-----------------------|--------|-------------------------------------------------------------------------------------------------------|
| {id}/pump/state       | 0 or 1 | Whether the pump is working (_1_) or not (_0_).                                                       |
| {id}/mode             | 0 or 1 | Current operating mode (see below). Manual mode is _0_, automatic mode is _1_.                        |
| {id}/humidity/digital | 0 or 1 | Digital humidity sensor state. If sensor reads _dry_, _1_ is published, otherwise _0_ is published.   |
| {id}/water_level      | 0 or 1 | Water level sensor state. If sensor reads _has water_, _0_ is published, otherwise _1_ is published.  |

### Subscribe

|       Topic       | Values |                                                                           Description                                                                           |
|-------------------|--------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------|
| {id}/pump/control | 0 or 1 | Controls pump state. For _1_ pump is started, for _0_ pump i stopped. Used only in manual mode (see below). If mode changes, last published value will be used. |

## Automatic or manual mode
Depending on mode switch state, AutoWater works in manual or automatic mode.

In automatic mode, the pump will be started when digital humidity sensor reads _dry_ and water level sensor reads _has water_. Pump will be autoamtically stopped if digital humidity sensor or water level sensor reads any other value.

In manual mode, the pump is started and stopped by MQTT commands.
