/**
 * ============================================================================
 * DIY Кабельный тестер NKS (v2.4 Firmware)
 * Язык: C++ / Arduino Framework (IDE / VS Code PlatformIO)
 * Микроконтроллер: Arduino Nano / Uno (ATmega328P)
 * Дисплей: OLED 0.96" I2C (SSD1306, 128x64 px)
 * Разработчик: Смирнов Никита Олегович (NKS Lab)
 * ============================================================================
 * Описание:
 * Исходный код прошивки главного модуля (Master) для автоматической диагностики 
 * до 10 жил кабеля. Измеряет целостность, короткие замыкания, пересортицу жил, 
 * а также контроллирует напряжение АКБ 18650 и автоматический сон экрана.
 * ============================================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Конфигурация OLED дисплея
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Определение пинов передачи/приема для 10 жил кабеля
const int TEST_PINS[10] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

// Аналоговый пин для контроля напряжения аккумулятора (делитель напряжения)
const int BATTERY_PIN = A0;

// Пин кнопки переворота экрана на 180 градусов
const int ROTATE_BTN_PIN = 12;

// Переменные состояния
bool isScreenRotated = false;
unsigned long lastActivityTime = 0;
const unsigned long SLEEP_TIMEOUT = 180000; // Автоотключение через 3 минуты (180000 мс)

// Структура для хранения результатов теста линии
struct LineResult {
  int sourcePin;
  int targetPin;
  bool isShortCircuit;
  bool isOpenCircuit;
};

LineResult results[10];

/**
 * Инициализация микроконтроллера и периферии
 */
void setup() {
  Serial.begin(115200);

  // Инициализация пина кнопки переворота экрана с внутренней подтяжкой
  pinMode(ROTATE_BTN_PIN, INPUT_PULLUP);

  // Конфигурация тестовых пинов в режим высокоимпедансного входа
  for (int i = 0; i < 10; i++) {
    pinMode(TEST_PINS[i], INPUT);
  }

  // Инициализация OLED дисплея по I2C (адрес 0x3C)
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Ошибка инициализации OLED SSD1306!"));
    for (;;); // Зависание при ошибке аппаратной части
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(10, 25);
  display.println(F("NKS TESTER v2.4"));
  display.display();
  delay(1500);

  lastActivityTime = millis();
}

/**
 * Главный цикл программы
 */
void loop() {
  // 1. Проверка кнопки поворота экрана
  checkRotateButton();

  // 2. Сканирование кабельной трассы (10 жил)
  scanCable();

  // 3. Чтение напряжения аккумулятора 18650
  float voltage = readBatteryVoltage();
  int percent = calculateBatteryPercent(voltage);

  // 4. Отрисовка данных на OLED экране
  updateDisplay(percent, voltage);

  // 5. Проверка таймера автоотключения экрана
  checkSleepTimer();

  delay(300); // Пауза перед следующим циклом проверки (~0.3 сек)
}

/**
 * Метод сканирования линий кабеля методом последовательного импульсного опроса
 */
void scanCable() {
  for (int i = 0; i < 10; i++) {
    results[i].sourcePin = i + 1;
    results[i].targetPin = 0;
    results[i].isShortCircuit = false;
    results[i].isOpenCircuit = true;

    // Подаем HIGH на исследуемую жилу
    pinMode(TEST_PINS[i], OUTPUT);
    digitalWrite(TEST_PINS[i], HIGH);
    delayMicroseconds(50);

    // Считываем ответ с остальных жил для поиска КЗ и пересортицы
    int detectedConnections = 0;
    for (int j = 0; j < 10; j++) {
      if (digitalWriteCheck(TEST_PINS[j])) {
        if (i == j) {
          results[i].targetPin = j + 1;
          results[i].isOpenCircuit = false;
        } else {
          results[i].isShortCircuit = true;
          detectedConnections++;
        }
      }
    }

    // Сброс пина обратно в высокоимпедансный режим
    digitalWrite(TEST_PINS[i], LOW);
    pinMode(TEST_PINS[i], INPUT);
  }
}

/**
 * Проверка нажатия кнопки поворота экрана (защита от дребезга)
 */
void checkRotateButton() {
  if (digitalRead(ROTATE_BTN_PIN) == LOW) {
    delay(50); // Дебаунс
    if (digitalRead(ROTATE_BTN_PIN) == LOW) {
      isScreenRotated = !isScreenRotated;
      display.setRotation(isScreenRotated ? 2 : 0); // 180 градусов
      lastActivityTime = millis();
      while (digitalRead(ROTATE_BTN_PIN) == LOW); // Ожидание отпускания
    }
  }
}

/**
 * Измерение напряжения АКБ через делитель
 */
float readBatteryVoltage() {
  int rawAnalog = analogRead(BATTERY_PIN);
  float voltage = (rawAnalog / 1023.0) * 5.0 * 2.0; // Коэффициент делителя 1:2
  return voltage;
}

/**
 * Расчет процента заряда для Li-Ion 18650 (3.3V ... 4.2V)
 */
int calculateBatteryPercent(float v) {
  if (v >= 4.2) return 100;
  if (v <= 3.3) return 0;
  return (int)((v - 3.3) / (4.2 - 3.3) * 100.0);
}

/**
 * Обновление кадра на OLED дисплее
 */
void updateDisplay(int batPercent, float batVoltage) {
  display.clearDisplay();

  // Шапка статуса
  display.setCursor(0, 0);
  display.print(F("MASTER "));
  display.print(batPercent);
  display.print(F("% "));
  display.print(batVoltage, 1);
  display.println(F("V"));
  display.drawLine(0, 9, 128, 9, SSD1306_WHITE);

  // Вывод сетки из 10 каналов
  for (int i = 0; i < 10; i++) {
    int col = (i < 5) ? 0 : 64;
    int row = 12 + (i % 5) * 10;

    display.setCursor(col, row);
    display.print(i + 1);
    display.print(F(">"));

    if (results[i].isShortCircuit) {
      display.print(F("[!] КЗ"));
    } else if (results[i].isOpenCircuit) {
      display.print(F("-- X"));
    } else {
      display.print(results[i].targetPin);
      display.print(F(" OK"));
    }
  }

  display.display();
}

/**
 * Контроль энергосбережения
 */
void checkSleepTimer() {
  if (millis() - lastActivityTime > SLEEP_TIMEOUT) {
    display.ssd1306_command(SSD1306_DISPLAYOFF); // Выключить OLED
  } else {
    display.ssd1306_command(SSD1306_DISPLAYON);  // Включить OLED
  }
}

/**
 * Вспомогательное чтение состояния цифрового пина
 */
bool digitalWriteCheck(int pin) {
  return digitalRead(pin) == HIGH;
}
