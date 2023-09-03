# ESP32WiFiServerLedController
An ESP32 Project I did to control my LED Stripes via my smartphone and an infrared remote.

When you want to replicate this project, just continue reading :).

1. Set the name and the password for your wifi that the ESP can connect to.
2. Give the ESP a static IP address, I did it through my router interface. Otherwise, you have to constantly change the IP address in the App, which is really annoying.

3. Wire everything up with the ESP:

I connected D5, D18 and D19 to the gates of the MOSFETs with a resistor of 470 Ω in between.

![ESP32 Board Top](https://github.com/danielmehring/ESP32WiFiServerLedController/assets/37716951/ccc14a95-f965-4137-a4d0-6f5fa9d2265d)

![ESP32 Pin Closeup](https://github.com/danielmehring/ESP32WiFiServerLedController/assets/37716951/554e6fd3-51e2-4d42-8edd-2e22e597f062)

I used the IRLZ44N because it is a Logic-Level transistor, which is needed to "fully open" at 3V. Don't use Non-Logic-Level transistors with the ESP32 as they need 5V for that. 

(There is a 10 kΩ resistor between gate and ground/source)

There is one shared ground which also connects to the source of the MOSFETs. Drain is connected to the positive voltage of the voltage supply.

![ESP32 Board Bottom](https://github.com/danielmehring/ESP32WiFiServerLedController/assets/37716951/dfaa38bf-315d-4a67-b39f-87828edea400)

I used N-channel transistors, when you use P-channel transistors, you have to connect source to the positive cable of the voltage supply and drain to ground.

You can use pretty much any infrared sensor, hook its signal up to the D15 pin and connect the ground and power source accordingly.

![ESP32 Infarred](https://github.com/danielmehring/ESP32WiFiServerLedController/assets/37716951/6e776502-e53d-4659-b2f9-72d51ecf5c17)

This is what the board looks like with the ESP attached:
![ESP32 Attached Board Top](https://github.com/danielmehring/ESP32WiFiServerLedController/assets/37716951/618d0da7-4280-4fc3-880b-bc897e271dc6)
