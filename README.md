# InfoMatrix 🌈📟

**InfoMatrix** is a Wi-Fi-enabled LED matrix display built using the ESP8266. It allows you to display custom messages, colors, and temperatures through a web interface hosted directly on the microcontroller.

---

## 🚀 Features

- 🌐 Control text, color, scroll speed, and display modes (scroll, loop, static) via a web browser  
- 🌡️ Fetch current temperature from [OpenWeatherMap](https://openweathermap.org/) and show it on the matrix  
- 💾 Store city name and API key using **SPIFFS** for persistent configuration  
- ⚙️ Easy Wi-Fi setup using **WiFiManager**  
- 🧭 Local access using **mDNS** (e.g., `http://infomatrix.local`)

---

## 🛠️ Built With

- [ESP8266 NodeMCU](https://www.espressif.com/en/products/socs/esp8266)  
- [Adafruit NeoMatrix](https://github.com/adafruit/Adafruit_NeoMatrix)  
- HTML + CSS (served from ESP8266)  
- OpenWeatherMap API  
- SPIFFS for config storage  
- WiFiManager for network setup  
- mDNS for easy local discovery

---

## 📸 Preview

> Example:
> ![image](https://github.com/user-attachments/assets/19ba490d-4ad5-44d0-8309-418a13de45c5)

---

## 🔧 How to Use

### 1. 📦 Flash the Firmware
- Clone or download this repo
- Open in Arduino IDE or PlatformIO
- Install required libraries (see below)
- Flash to ESP8266

### 2. 📁 Prepare SPIFFS
- Update `data/config.json` with your city and API key (optional)
- Upload `data/` to SPIFFS using the Arduino SPIFFS uploader

### 3. 🔌 Power & Connect
- On first boot, connect to the WiFiManager hotspot
- Choose your Wi-Fi network and connect

### 4. 🌐 Access the Interface
- Go to `http://infomatrix.local` (or the IP shown in Serial Monitor)
- Use the web UI to update message, color, speed, or fetch temperature

---
