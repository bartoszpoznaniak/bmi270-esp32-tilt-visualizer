#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
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

// HTML strony głównej
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <title>BMI270 Wizualizacja</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; text-align: center; margin: 0px auto; padding-top: 30px; }
        .gauge { width: 300px; height: 300px; margin: 0 auto; position: relative; }
        .gauge-circle { width: 100%; height: 100%; border: 10px solid #ccc; border-radius: 50%; position: relative; }
        .gauge-cross { position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); width: 100%; height: 100%; }
        .gauge-cross::before, .gauge-cross::after { content: ''; position: absolute; background: #eee; }
        .gauge-cross::before { width: 2px; height: 100%; left: 50%; transform: translateX(-50%); }
        .gauge-cross::after { width: 100%; height: 2px; top: 50%; left: 0; transform: translateY(-50%); }
        .ball { width: 20px; height: 20px; background: red; border-radius: 50%; position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); }
        .reset-btn { margin-top: 20px; padding: 10px 20px; font-size: 16px; cursor: pointer; }
    </style>
</head>
<body>
    <h2>BMI270 Wizualizacja</h2>
    <div class="gauge">
        <div class="gauge-circle">
            <div class="gauge-cross"></div>
            <div class="ball" id="ball"></div>
        </div>
    </div>
    <button class="reset-btn" onclick="resetSensor()">Resetuj czujnik</button>

    <script>
        var ball = document.getElementById('ball');
        var gauge = document.querySelector('.gauge');
        var gaugeWidth = gauge.offsetWidth;
        var maxOffset = gaugeWidth * 0.4; // 40% promienia

        function updateBallPosition() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    // Obliczanie pozycji kulki na podstawie kątów
                    var xOffset = (data.angleX / 15) * maxOffset;
                    var yOffset = (data.angleY / 15) * maxOffset;
                    
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

        // Aktualizacja co 100ms
        setInterval(updateBallPosition, 100);
    </script>
</body>
</html>
)rawliteral";

void setup()
{
  Serial.begin(115200);
  Serial.println("BMI270 Example - Web Server");

  // Inicjalizacja WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Inicjalizacja I2C
  Wire.begin();

  // Inicjalizacja BMI270
  while (imu.beginI2C(i2cAddress) != BMI2_OK)
  {
    Serial.println("Error: BMI270 not connected, check wiring and I2C address!");
    delay(1000);
  }
  Serial.println("BMI270 connected!");
   Serial.println("BMI270 v1.1");

  // Konfiguracja endpointów serwera
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.on("/reset", HTTP_GET, handleReset);

  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  server.handleClient();

  // Odczyt danych z czujnika
  imu.getSensorData();

  // Pobieranie danych z akcelerometru
  accX = imu.data.accelX;
  accY = imu.data.accelY;
  accZ = imu.data.accelZ;

  // Obliczanie kątów
  float angleX = atan2(accX, accZ) * 180.0 / PI - offsetX;
  float angleY = atan2(accY, accZ) * 180.0 / PI - offsetY;

  // Ograniczenie kątów do zakresu ±15 stopni
  angleX = constrain(angleX, -MAX_ANGLE, MAX_ANGLE);
  angleY = constrain(angleY, -MAX_ANGLE, MAX_ANGLE);

  // Aktualizacja zmiennych globalnych
  accX = angleX;
  accY = angleY;
}

void handleRoot()
{
  server.send(200, "text/html", index_html);
}

void handleData()
{
  String json = "{\"angleX\":" + String(accX) + ",\"angleY\":" + String(accY) + "}";
  server.send(200, "application/json", json);
}

void handleReset()
{
  // Zerowanie offsetów do aktualnej pozycji używając surowych danych z czujnika
  offsetX = atan2(imu.data.accelX, imu.data.accelZ) * 180.0 / PI;
  offsetY = atan2(imu.data.accelY, imu.data.accelZ) * 180.0 / PI;
  server.send(200, "text/plain", "Sensor zerowany");
}
