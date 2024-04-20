# Venstar ColorTouch Thermostat ESP32 Controller

## Created by Ayman Taleb 12/25/2023
![thermostat controller circuit schematic](screenshot.png)

A way to control my Venstar ColorTouch thermostat using my ESP32 over a local network connection. The ColorTouch thermostat is a smart thermostat made by Venstar. It has an app and web interface through their [Skyport Cloud service](https://venstar.com/skyport/). However, I like the idea of having a small device in my room that can control my thermostat. Thankfully, this thermostat has a local API mode. Which means I can just send it HTTP requests to get info and control it. The API is really simple to use if you follow the [documentation](https://developer.venstar.com/). I created the schematic above using [Frizing](https://fritzing.org/).


---
### How the API works:
The API uses the [SSDP protocol](https://en.wikipedia.org/wiki/Simple_Service_Discovery_Protocol) to send HTTP requests. In order to communication with the thermostat, you must enable local API in the settings on the [device](https://www.youtube.com/watch?v=kB_HcJ3kqCg). Then you just make HTTP requests using what ever API test software, I use [Postman](https://www.postman.com/). I am only using two endpoints, a GET request, to get updates on status/temperature, and a POST request, to send controls to the thermostat. 

#### GET Example:
To get the current info from the thermostat you make this GET request:

``http://192.168.1.100/query/info
``
