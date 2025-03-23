# esp-read-qrcode - read and decode QRcodes via camera, and connect to Wifi

This repo is a QRcode reading demo for the [M5Stack CoreS3](https://docs.m5stack.com/en/core/CoreS3), which has a builtin camera. Once the code is read successfully, the payload is checked for the [WIFI URI](https://superuser.com/questions/1752083/wifi-qr-code-format-for-wpa2-wpa3) format;
If so, the contained SSID and password are used to connect to the access point.

[Generating a WIFI URI Online](https://qifi.org/)

[Specification of the WIFI URI format](https://www.wi-fi.org/system/files/WPA3%20Specification%20v3.2.pdf#page=25)

# Status
Best results: generating a Wifi URI with [qifi.org](https://qifi.org/) and pointing the camera at the screen or printed out on paper (larger images help, a code of 2cm squared performs worse than a 5x5 or 20x20cm code)

Reading works well from an iPhone if this shortcut is used: https://www.youtube.com/watch?v=yl60Vkyg9CM

While current Android can generate a Wifi QRcode natively, I had less success with this setup as the generated code obviously requires higher camera resolution.