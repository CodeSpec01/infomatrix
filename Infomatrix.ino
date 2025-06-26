#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiClient.h>
#include <FS.h>
#include <ArduinoJson.h>

#define LED_PIN 13
#define MATRIX_WIDTH 25
#define MATRIX_HEIGHT 20

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(
  MATRIX_WIDTH, MATRIX_HEIGHT, LED_PIN,
  NEO_MATRIX_BOTTOM + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
  NEO_GRB + NEO_KHZ800);

ESP8266WebServer server(80);
String receivedText = "";
int textY = 0;

String loopMessage = "";
bool isLooping = false;

String staticMessage = "";
bool isStatic = false;

String currentMode = "scroll";
int repeatCount = 1;

uint16_t currentColor = matrix.Color(255, 0, 255);

int currentSpeed = 70;

String currentTemp = "";
String city = "Delhi";
String apiKey = "..";

void loadConfig() {
  if (SPIFFS.begin()) {
    if (SPIFFS.exists("/config.json")) {
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, configFile);
        if (!error) {
          if (doc.containsKey("city")) city = doc["city"].as<String>();
          if (doc.containsKey("apiKey")) apiKey = doc["apiKey"].as<String>();
        }
        configFile.close();
      }
    }
  }
}

void saveConfig(String newCity, String newApiKey) {
  StaticJsonDocument<256> doc;
  doc["city"] = newCity;
  doc["apiKey"] = newApiKey;

  File configFile = SPIFFS.open("/config.json", "w");
  if (configFile) {
    serializeJson(doc, configFile);
    configFile.close();
  }
}

void setupWiFi();
void startServer();
void handleRoot();
void handleTextInput();
void handleReset();
void showMatrixMessage(String message, uint16_t color, int delayTime = 1000, int speed = 75);
void showStaticMessage(String message, uint16_t color);

void setup() {
  Serial.begin(115200);
  SPIFFS.begin();
  loadConfig();

  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(150);
  matrix.setTextColor(matrix.Color(255, 0, 255));

  setupWiFi();
  startServer();

  fetchTemperature();
  drawDisplay("");

  showMatrixMessage("InfoMatrix Ready", matrix.Color(0, 255, 0), 1500, 75);
}

void loop() {
  server.handleClient();
  MDNS.update();

  if (receivedText != "") {
    if (currentMode == "scroll") {
      for (int i = 0; i < repeatCount; i++) {
        showMatrixMessage(receivedText, currentColor, 500, currentSpeed);
      }
    } else if (currentMode == "loop") {
      loopMessage = receivedText;
      isLooping = true;
    } else if (currentMode == "static") {
      staticMessage = receivedText;
      isStatic = true;
      isLooping = false;
      showStaticMessage(staticMessage, currentColor);
    }

    receivedText = "";
  }

  if (isLooping && loopMessage != "") {
    showMatrixMessage(loopMessage, currentColor, 0, currentSpeed);
  }
}
void setupWiFi() {
  WiFiManager wm;
  showMatrixMessage("Connecting...", matrix.Color(255, 140, 0), 2000, 75);

  if (!wm.autoConnect("InfoMatrix Setup")) {
    showMatrixMessage("Hotspot Started", matrix.Color(0, 0, 255), 2000);
    ESP.restart();
  }

  String ip = WiFi.localIP().toString();
  showMatrixMessage("Connected", matrix.Color(0, 255, 0), 2000, 75);

  if (MDNS.begin("infomatrix")) {
    MDNS.addService("http", "tcp", 80);
  }
}

void startServer() {
  server.on("/", handleRoot);
  server.on("/submit", HTTP_POST, handleTextInput);
  server.on("/reset", handleReset);
  server.on("/stop", []() {
    isLooping = false;
    loopMessage = "";
    server.send(200, "text/plain", "Looping stopped");
  });
  server.on("/getTemp", HTTP_POST, []() {
    if (server.hasArg("city")) {
      if (server.arg("apiKey") != "") {
        saveConfig(server.arg("city"), server.arg("apiKey"));
        apiKey = server.arg("apiKey");
      } else {
        saveConfig(server.arg("city"), apiKey);
      }
      city = server.arg("city");
    }
    fetchTemperature();
    server.send(200, "text/plain", "Temperature Updated");
  });

  server.begin();
}

void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
      <html>

      <head>
        <title>ESP8266 InfoMatrix</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          body {
            font-family: Arial, sans-serif;
            background-color: #121212;
            color: #ffffff;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            height: 100vh;
            margin: 0;
          }

          h1 {
            font-size: 2.5em;
            margin-bottom: 20px;
          }

          form {
            background-color: #1e1e1e;
            padding: 30px;
            border-radius: 12px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.5);
            min-width: 40vw;
          }

          input[type='text'],
          input[type='number'] {
            font-size: 1.2em;
            padding: 10px;
            margin: 10px 0;
            width: 100%;
            border: none;
            border-radius: 8px;
          }

          input[type='submit'] {
            font-size: 1.2em;
            padding: 10px 20px;
            margin-top: 20px;
            border: none;
            border-radius: 8px;
            background-color: #00c853;
            color: white;
            cursor: pointer;
          }

          input[type='submit']:hover {
            background-color: #00b246;
          }

          .radio-group {
            margin: 15px 0;
          }

          label {
            font-size: 1.1em;
            margin-right: 20px;
          }

          .status-btn {
            background-color: #ff0000;
            margin-top: 15px;
            color: white;
            border: none;
            border-radius: 10px;
            padding: 10px 20px;
            cursor: pointer;
          }

          .temperature-btn {
            background-color: #0e7be7;
            margin-top: 15px;
            color: white;
            border: none;
            border-radius: 10px;
            padding: 15px 20px;
            cursor: pointer;
          }

          .reset-btn {
            background-color: #ff0000;
            margin-top: 15px;
            color: white;
            border: none;
            border-radius: 10px;
            padding: 10px 20px;
            cursor: pointer;
          }

          .btn-group {
            display: flex;
            justify-content: space-between;
          }
        </style>
      </head>

      <body>
        <h1>InfoMatrix Control</h1>
        <form id="textForm">
          <h3>TEXT CONTROL</h3>
          <input type="text" name="text" id="textInput" placeholder="Enter text to display" required>
          <label for="color">Text Color:</label>
          <input type="color" id="color" name="color" value="#ff00ff">
          <br />
          <br />
          <label for="speed">Text Duration:</label>
          <input type="range" id="speed" name="speed" min="10" max="200" value="75"
            oninput="speedValue.innerText = this.value">
          <span id="speedValue">75</span>

          <div class="radio-group">
            <label><input type="radio" name="mode" value="scroll" checked onchange="toggleNumberField()"> Scroll</label>
            <label><input type="radio" name="mode" value="loop" onchange="toggleNumberField()"> Loop</label>
            <label><input type="radio" name="mode" value="static" onchange="toggleNumberField()"> Static</label>
          </div>

          <label>Repeat Count:</label>
          <input type="number" name="repeat" id="repeatCount" value="1" min="1" required>

          <div class="btn-group">
            <input type="submit" value="Send">
            <button id="stopLoopBtn" class="status-btn" onclick="stopLoop()">Stop Loop</button>
            <button class="reset-btn" onclick="resetESP()">RESET ESP</button>
          </div>
        </form>
        
        <p id="confirmation" style="color: #00e676; display: none; font-weight: bold; margin-top: 15px;">
          Message sent!
        </p>

        <p id="loopStopped" style="color: #ff0000; display: none; font-weight: bold; margin-top: 15px;">
          Loop Stopped!
        </p>

        <p id="ESPResetted" style="color: #ff0000; display: none; font-weight: bold; margin-top: 15px;">
          ESP Resetted!
        </p>

        <br>
        <br>
        <br>
        <form onsubmit="event.preventDefault(); updateCity();">
          <h3>TEMPERATURE CONTROL</h3>
          <label for="city">City:</label>
          <input type="text" name="city" id="city" placeholder="Enter city name" required>
          <label for="apiKey">API Key:</label>
          <input type="text" name="apiKey" id="apiKey" placeholder="Enter API Key">
          <button class="temperature-btn" type="submit">Update Temperature</button>
        </form>

        <p id="tempConfirm" style="color: #00e676; display: none; font-weight: bold; margin-top: 15px;">
          Temperature Updated!
        </p>

        <script>
          function toggleNumberField() {
            const mode = document.querySelector('input[name="mode"]:checked').value;
            const repeatField = document.getElementById("repeatCount");
            const stopLoopBtn = document.getElementById("stopLoopBtn");
            repeatField.disabled = (mode === "loop" || mode === "static");
            stopLoopBtn.style.display = (mode === "loop") ? "block" : "none";
          }

          document.getElementById("textForm").addEventListener("submit", function (e) {
            e.preventDefault();
            const formData = new FormData(this);

            fetch('/submit', {
              method: 'POST',
              body: formData
            });

            document.getElementById("textInput").value = "";
            const msg = document.getElementById("confirmation");
            msg.style.display = "block";
            setTimeout(() => msg.style.display = "none", 3000);
          });

          function stopLoop() {
            fetch('/stop');
            const msg = document.getElementById("loopStopped");
            msg.style.display = "block";
            setTimeout(() => msg.style.display = "none", 3000);
          }

          function resetESP() {
            fetch('/reset');
            const msg = document.getElementById("ESPResetted");
            msg.style.display = "block";
            setTimeout(() => msg.style.display = "none", 3000);
          }

          function updateCity() {
            const city = document.getElementById("city").value;
            const apiKey = document.getElementById("apiKey").value ? document.getElementById("apiKey").value : "";
            const formData = new FormData();
            formData.append("city", city);
            formData.append("apiKey", apiKey);

            fetch("/getTemp", {
              method: "POST",
              body: formData
            })
            const msg = document.getElementById("tempConfirm");
            msg.style.display = "block";
            setTimeout(() => msg.style.display = "none", 3000);
          }

          window.onload = toggleNumberField;
        </script>

      </body>

      </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleTextInput() {
  if (server.hasArg("text")) {
    receivedText = server.arg("text");
    currentMode = server.arg("mode");
    repeatCount = server.hasArg("repeat") ? server.arg("repeat").toInt() : 1;

    if (server.hasArg("color")) {
      String hexColor = server.arg("color");
      long colorValue = strtol(hexColor.substring(1).c_str(), NULL, 16);
      byte r = (colorValue >> 16) & 0xFF;
      byte g = (colorValue >> 8) & 0xFF;
      byte b = colorValue & 0xFF;
      currentColor = matrix.Color(r, g, b);
    }

    if (server.hasArg("speed")) {
      currentSpeed = constrain(server.arg("speed").toInt(), 10, 200);  // limit for sanity
    }

    isLooping = false;
    isStatic = false;
  }
}

void handleReset() {
  showMatrixMessage("Resetting WiFi settings...", matrix.Color(255, 0, 0), 500, 75);
  WiFiManager wm;
  wm.resetSettings();
  server.send(200, "text/html", "<h1>WiFi Credentials Erased!</h1><a href='/'>Go Back</a>");
  delay(500);
  ESP.restart();
}

void showMatrixMessage(String message, uint16_t color, int delayTime, int speed) {
  int posX = MATRIX_WIDTH;
  matrix.setTextColor(color);
  while (posX > -((int)message.length() * 6)) {
    matrix.fillScreen(0);
    matrix.setCursor(posX, textY);
    matrix.setTextColor(color);
    matrix.print(message.c_str());

    matrix.setCursor(0, 11);
    matrix.setTextColor(matrix.Color(0, 255, 255));
    matrix.print(currentTemp);

    matrix.show();
    delay(speed);
    posX--;
  }
  delay(delayTime);

  drawDisplay("");
}

void showStaticMessage(String message, uint16_t color) {
  matrix.fillScreen(0);
  matrix.setTextColor(color);

  int maxChars = MATRIX_WIDTH / 6;
  if (message.length() > maxChars) {
    message = message.substring(0, maxChars);
  }

  int x = (MATRIX_WIDTH - message.length() * 6) / 2;
  int y = 1;

  matrix.setCursor(x, y);
  matrix.print(message);

  matrix.setCursor(0, 11);
  matrix.setTextColor(matrix.Color(0, 255, 255));
  matrix.print(currentTemp);

  matrix.show();
}

void fetchTemperature() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "&units=metric&appid=" + apiKey;

    http.begin(client, url);

    int httpCode = http.GET();
    if (httpCode == 200) {
      String payload = http.getString();
      int tempIndex = payload.indexOf("\"temp\":") + 7;
      int commaIndex = payload.indexOf(",", tempIndex);
      currentTemp = payload.substring(tempIndex, commaIndex) + "°C";
    } else {
      currentTemp = "N/A";
    }
    drawDisplay("");

    http.end();
  }
}

void drawDisplay(String topMessage) {
  matrix.fillScreen(0);
  matrix.setCursor(MATRIX_WIDTH, 1);
  matrix.setTextColor(matrix.Color(255, 255, 0));
  matrix.print(topMessage);

  matrix.setCursor(0, 11);
  matrix.setTextColor(matrix.Color(0, 255, 255));
  matrix.print(currentTemp);

  matrix.show();
}
