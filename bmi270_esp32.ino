#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <EEPROM.h>
#include "SparkFun_BMI270_Arduino_Library.h"
#include "config.h"

// Inicjalizacja serwera WWW na porcie 80
WebServer server(80);

// Inicjalizacja czujnika BMI270
BMI270 imu;
uint8_t i2cAddress = BMI2_I2C_PRIM_ADDR; // 0x68

// Zmienne do przechowywania danych z czujnika
float accX = 0;
float accY = 0;
float accZ = 0;
float gyroX = 0;
float gyroY = 0;
float gyroZ = 0;

// Offset kalibracji
float offsetX = 0;
float offsetY = 0;

// Maksymalny kąt nachylenia (w stopniach)
const float MAX_ANGLE = 15.0;

// Adresy w EEPROM
const int EEPROM_SIZE = 512;
const int SSID_ADDR = 0;
const int PASS_ADDR = 32;

// HTML strony konfiguracyjnej
const char config_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <title>Konfiguracja WiFi</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; text-align: center; margin: 0px auto; padding-top: 30px; }
        .form-group { margin: 20px 0; }
        input { padding: 10px; width: 200px; }
        button { padding: 10px 20px; font-size: 16px; cursor: pointer; }
    </style>
</head>
<body>
    <h2>Konfiguracja WiFi ErgoHealth</h2>
    <form action="/save" method="post">
        <div class="form-group">
            <label for="ssid">Nazwa sieci (SSID):</label><br>
            <input type="text" id="ssid" name="ssid" required>
        </div>
        <div class="form-group">
            <label for="password">Hasło:</label><br>
            <input type="password" id="password" name="password" required>
        </div>
        <button type="submit">Zapisz</button>
    </form>
</body>
</html>
)rawliteral";

// HTML strony głównej
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <title>:Poduszka ErgoHealth</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; text-align: center; margin: 0px auto; padding-top: 30px; }
        .gauge { width: 360px; height: 360px; margin: 0 auto; position: relative; }
        .gauge-circle { 
            width: 100%; 
            height: 100%; 
            border: 3px solid #FF0000; /* czerwony */
            border-radius: 50%; 
            position: relative; 
        }
        .gauge-circle::before,
        .gauge-circle::after {
            content: '';
            position: absolute;
            border-radius: 50%;
            border: 3px solid;
        }
        .gauge-circle::before {
            top: 10%;
            left: 10%;
            right: 10%;
            bottom: 10%;
            border-color: #FFFF00; /* żółty */
        }
        .gauge-circle::after {
            top: 20%;
            left: 20%;
            right: 20%;
            bottom: 20%;
            border-color: #00FF00; /* zielony */
        }
        .gauge-cross { position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); width: 100%; height: 100%; }
        .gauge-cross::before, .gauge-cross::after { content: ''; position: absolute; background: #eee; }
        .gauge-cross::before { width: 2px; height: 100%; left: 50%; transform: translateX(-50%); }
        .gauge-cross::after { width: 100%; height: 2px; top: 50%; left: 0; transform: translateY(-50%); }
        .ball { width: 20px; height: 20px; background: red; border-radius: 50%; position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); }
        .reset-btn { margin-top: 20px; padding: 10px 20px; font-size: 16px; cursor: pointer; }
        .sensitivity-controls { margin-top: 20px; }
        .sensitivity-controls label { display: block; margin-bottom: 10px; }
        .sensitivity-controls input[type="range"] { width: 200px; margin-right: 10px; }
        .sensitivity-controls input[type="number"] { width: 60px; }
    </style>
</head>
<body>
    <h2>Poduszka ErgoHealth</h2>
    <div class="gauge">
        <div class="gauge-circle">
            <div class="gauge-cross"></div>
            <div class="ball" id="ball"></div>
        </div>
    </div>
    <button class="reset-btn" onclick="resetSensor()">Resetuj czujnik</button>
    <button class="reset-btn" onclick="resetWiFi()" style="margin-left: 10px;">Konfiguracja WiFi</button>
    
    <div class="sensitivity-controls">
        <label>Czulosci: <span id="sensitivityValue">15</span> stopni</label>
        <input type="range" id="sensitivitySlider" min="1" max="15" value="15" oninput="updateSensitivity(this.value)">
        <input type="number" id="sensitivityInput" min="1" max="15" value="15" onchange="updateSensitivity(this.value)">
    </div>
    <div id="connectionStatus" style="color: red; margin-top: 10px;"></div>

    <script>
        var ball = document.getElementById('ball');
        var gauge = document.querySelector('.gauge');
        var gaugeWidth = gauge.offsetWidth;
        var maxOffset = gaugeWidth * 0.4; // 40% promienia
        let sensitivity = 15; // domyslna czulosci

        function updateSensitivity(value) {
            value = Math.min(15, Math.max(1, parseInt(value)));
            sensitivity = value;
            document.getElementById('sensitivityValue').textContent = value;
            document.getElementById('sensitivitySlider').value = value;
            document.getElementById('sensitivityInput').value = value;
        }

        function updateBallPosition() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    if (data.error) {
                        document.getElementById('connectionStatus').textContent = data.error;
                        return;
                    }
                    document.getElementById('connectionStatus').textContent = '';
                    // Obliczanie pozycji kulki na podstawie kątów
                    var xOffset = (data.angleX / sensitivity) * maxOffset;
                    var yOffset = (data.angleY / sensitivity) * maxOffset;
                    
                    ball.style.transform = `translate(calc(-50% + ${xOffset}px), calc(-50% + ${yOffset}px))`;
                });
        }

        function resetSensor() {
            fetch('/reset')
                .then(response => response.text())
                .then(data => {
                    console.log('Czujnik zresetowany');
                });
        }

        function resetWiFi() {
            if(confirm('Czy na pewno chcesz zresetować ustawienia WiFi? Urządzenie uruchomi się w trybie konfiguracyjnym.')) {
                fetch('/resetWiFi')
                    .then(response => response.text())
                    .then(data => {
                        console.log('Resetowanie WiFi...');
                    });
            }
        }

        // Aktualizacja co 100ms
        setInterval(updateBallPosition, 100);
    </script>
</body>
</html>
)rawliteral";

// Funkcja do zapisywania danych w EEPROM
void saveWiFiConfig(const char *ssid, const char *password)
{
  EEPROM.begin(EEPROM_SIZE);

  // Zapisz SSID
  for (int i = 0; i < 32; i++)
  {
    EEPROM.write(SSID_ADDR + i, ssid[i]);
    if (ssid[i] == 0)
      break;
  }

  // Zapisz hasło
  for (int i = 0; i < 64; i++)
  {
    EEPROM.write(PASS_ADDR + i, password[i]);
    if (password[i] == 0)
      break;
  }

  EEPROM.commit();
}

// Funkcja do odczytywania danych z EEPROM
bool loadWiFiConfig(char *ssid, char *password)
{
  EEPROM.begin(EEPROM_SIZE);

  // Sprawdź czy są zapisane dane
  if (EEPROM.read(SSID_ADDR) == 0)
    return false;

  // Odczytaj SSID
  for (int i = 0; i < 32; i++)
  {
    ssid[i] = EEPROM.read(SSID_ADDR + i);
    if (ssid[i] == 0)
      break;
  }

  // Odczytaj hasło
  for (int i = 0; i < 64; i++)
  {
    password[i] = EEPROM.read(PASS_ADDR + i);
    if (password[i] == 0)
      break;
  }

  return true;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("BMI270 Example - Web Server v1.1130.33");

  // Inicjalizacja I2C
  Wire.begin();

  // Inicjalizacja BMI270
  while (imu.beginI2C(i2cAddress) != BMI2_OK)
  {
    Serial.println("Error: BMI270 not connected, check wiring and I2C address!");
    delay(1000);
  }
  Serial.println("BMI270 connected!");
  Serial.println("BMI270 v01.1124.1");

  // Próba załadowania konfiguracji WiFi
  char ssid[32] = {0};
  char password[64] = {0};

  if (loadWiFiConfig(ssid, password))
  {
    // Próba połączenia z zapisaną siecią
    WiFi.begin(ssid, password);
    int attempts = 0;

    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("\nWiFi connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());

      // Konfiguracja endpointów serwera
      server.on("/", HTTP_GET, handleRoot);
      server.on("/data", HTTP_GET, handleData);
      server.on("/reset", HTTP_GET, handleReset);
      server.on("/resetWiFi", HTTP_GET, handleResetWiFi);
      server.begin();
      Serial.println("HTTP server started");
      return;
    }
  }

  // Jeśli nie udało się połączyć, uruchom portal konfiguracyjny
  WiFi.softAP("BMI270_Config", "12345678");
  Serial.println("Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Konfiguracja endpointów serwera dla portalu konfiguracyjnego
  server.on("/", HTTP_GET, handleConfig);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
}

void loop()
{
  server.handleClient();

  // Odczytaj dane z czujnika
  if (imu.getSensorData() == BMI2_OK)
  {
    // Pobierz dane z czujnika
    float rawAccX = imu.data.accelX;
    float rawAccY = imu.data.accelY;
    float rawAccZ = imu.data.accelZ;

    // Oblicz kąty nachylenia
    // X - obrót wokół osi Y (przechylenie w lewo/prawo)
    float angleX = atan2(rawAccX, sqrt(rawAccY * rawAccY + rawAccZ * rawAccZ)) * 180.0 / PI - offsetX;
    // Y - obrót wokół osi X (przechylenie przód/tył) - odwrócony kierunek
    float angleY = -atan2(rawAccY, sqrt(rawAccX * rawAccX + rawAccZ * rawAccZ)) * 180.0 / PI - offsetY;

    // Ogranicz kąty do zakresu ±15 stopni
    angleX = constrain(angleX, -15, 15);
    angleY = constrain(angleY, -15, 15);

    // Aktualizuj zmienne globalne
    accX = angleX;
    accY = angleY;
  }
}

void handleConfig()
{
  server.send(200, "text/html", config_html);
}

void handleSave()
{
  String ssid = server.arg("ssid");
  String password = server.arg("password");

  saveWiFiConfig(ssid.c_str(), password.c_str());

  server.send(200, "text/html", "Konfiguracja zapisana. Restart urządzenia...");
  delay(2000);
  ESP.restart();
}

void handleRoot()
{
  server.send(200, "text/html", index_html);
}

void handleData()
{
  if (imu.getSensorData() == BMI2_OK)
  {
    String json = "{\"angleX\":" + String(accX) + ",\"angleY\":" + String(accY) + "}";
    server.send(200, "application/json", json);
  }
  else
  {
    String json = "{\"error\": \"Brak polaczenia z czujnikiem BMI270\"}";
    server.send(200, "application/json", json);
  }
}

void handleReset()
{
  // Zerowanie offsetów do aktualnej pozycji używając surowych danych z czujnika
  offsetX = atan2(imu.data.accelX, sqrt(imu.data.accelY * imu.data.accelY + imu.data.accelZ * imu.data.accelZ)) * 180.0 / PI;
  offsetY = -atan2(imu.data.accelY, sqrt(imu.data.accelX * imu.data.accelX + imu.data.accelZ * imu.data.accelZ)) * 180.0 / PI;

  // Aktualizuj zmienne globalne na 0
  accX = 0;
  accY = 0;

  server.send(200, "text/plain", "Sensor zerowany");
}

void handleResetWiFi()
{
  // Wyczyść dane WiFi z EEPROM
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < 32; i++)
  {
    EEPROM.write(SSID_ADDR + i, 0);
  }
  for (int i = 0; i < 64; i++)
  {
    EEPROM.write(PASS_ADDR + i, 0);
  }
  EEPROM.commit();

  server.send(200, "text/plain", "Resetowanie ustawień WiFi...");
  delay(2000);
  ESP.restart();
}
