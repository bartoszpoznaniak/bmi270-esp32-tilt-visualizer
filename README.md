# BMI270-ESP32 Tilt Visualizer

Projekt wizualizacji nachylenia (tilt) z wykorzystaniem czujnika BMI270 na platformie ESP32. Interaktywny wskaźnik poziomu z interfejsem webowym.

## Funkcjonalności

- Odczyt danych z czujnika BMI270
- Wizualizacja nachylenia w formie interaktywnego wskaźnika
- Interfejs webowy dostępny w sieci lokalnej
- Możliwość zerowania pozycji czujnika
- Ograniczenie zakresu nachylenia do ±15 stopni

## Wymagania sprzętowe

- ESP32 (testowane na ESP32 Dev Module)
- Czujnik BMI270
- Połączenie I2C:
  - SDA -> GPIO21
  - SCL -> GPIO22
  - VCC -> 3.3V
  - GND -> GND

## Wymagania programistyczne

- Arduino IDE
- Biblioteki:
  - SparkFun BMI270 Arduino Library
  - WiFi (wbudowana w ESP32)
  - WebServer (wbudowana w ESP32)

## Konfiguracja

1. Zainstaluj wymagane biblioteki w Arduino IDE
2. Podłącz czujnik BMI270 do ESP32 zgodnie z instrukcją powyżej
3. Zmodyfikuj dane dostępowe do WiFi w pliku `bmi270_esp32.ino`:
   ```cpp
   const char* ssid = "NAZWA_SIECI";
   const char* password = "HASLO_SIECI";
   ```
4. Wgraj kod do ESP32
5. Po uruchomieniu, w Serial Monitorze zobaczysz adres IP ESP32
6. Otwórz przeglądarkę i wpisz adres IP ESP32

## Użycie

1. Po uruchomieniu, zobaczysz interaktywny wskaźnik z czerwoną kropką
2. Kropka będzie się poruszać w zależności od nachylenia czujnika
3. Aby zerować pozycję, kliknij przycisk "Resetuj czujnik"
4. Po zerowaniu, aktualna pozycja czujnika zostanie ustawiona jako punkt zerowy

## Licencja

MIT License "# bmi270-esp32-tilt-visualizer" 
