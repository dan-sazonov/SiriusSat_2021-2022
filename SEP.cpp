#include <Wire.h>
#include <EEPROM.h>

#define I2C_SEP 0x1

#define SEP_RX 0
#define SEP_TX 1

#define BTN_1_pin 2
#define BTN_2_pin 3

#define EN_ESC_pin 4
#define EN_BC_pin 5
#define EN_BUSOS_pin 6
#define EN_BUEMU_pin 7

#define H1_SCL_pin A5
#define H1_SDA_pin A4

#define SOLAR_MES_pin A2
#define BAT_MES_pin A1
#define TOTAL_BAT_MES_pin A0

#define TOTAL_LOW_VOLT 6.4
#define BAT_LOW_VOLT 3.2

#define START_TIMEOUT 300 // задержка между запусками лииний питания

bool sep_state = false; // true, если спутник отделился
bool low_volt = false; // true при пониженном заряде
unsigned long last_time = 0;
byte bc_code = 0;
float total_bat, bat_volt, solar_volt = 0;

void setup() {
  Serial.begin(9600);
  Wire.begin(0x1);

  // инит кнопок
  pinMode(BTN_1_pin, INPUT_PULLUP);
  pinMode(BTN_2_pin, INPUT_PULLUP);

  // инит пинов управления
  pinMode(EN_ESC_pin, OUTPUT);
  pinMode(EN_BC_pin, OUTPUT);
  pinMode(EN_BUSOS_pin, OUTPUT);
  pinMode(EN_BUEMU_pin, OUTPUT);
  pwr_on(false);

  // задаем события
  Wire.onReceive(get_code);
  Wire.onRequest(send_data);
}

void loop() {
  // детектим разделение
  bool btn1, btn2;
  btn1 = digitalRead(BTN_1_pin);
  btn2 = digitalRead(BTN_2_pin);
  sep_state = (btn1 || btn2);

  // измеряем напругу
  total_bat = map(analogRead(TOTAL_BAT_MES_pin), 0, 1023, 0, 10.0);
  bat_volt = map(analogRead(BAT_MES_pin), 0, 1023, 0.0, 5.0);
  solar_volt = map(analogRead(SOLAR_MES_pin), 0, 1023, 0.0, 10.0);
  low_volt = (total_bat <= TOTAL_LOW_VOLT) || (bat_volt <= BAT_LOW_VOLT);

  // если произошло разделение и напряжение в норме, даем питание. если напряжение упало, снимаем
  pwr_on(sep_state && !low_volt);
}

byte pwr_on(bool pwr_state) {
  byte en_pins[4] = {EN_ESC_pin, EN_BC_pin, EN_BUSOS_pin, EN_BUEMU_pin};
  int start_timeout = pwr_state ? START_TIMEOUT : 0;
  byte pin_num = 0;

  while (pin_num < 4) {
    if (millis() - last_time > start_timeout) {
      digitalWrite(en_pins[pin_num++], pwr_state);
      last_time = millis();
    }
  }
  return 0;
}

void get_code(int bytes) {
  bc_code = Wire.read();

  --bytes;
  while (bytes) {
    Wire.read();
    --bytes;
  }
}

void send_data() {
  if (bc_code == 30) {
    uint8_t data[sizeof(float)];

    memcpy(data, &total_bat, sizeof(float));
    for (uint8_t i = 0; i < sizeof(float); ++i) { Wire.write(data[i]); }

    memcpy(data, &bat_volt, sizeof(float));
    for (uint8_t i = 0; i < sizeof(float); ++i) { Wire.write(data[i]); }

    memcpy(data, &solar_volt, sizeof(float));
    for (uint8_t i = 0; i < sizeof(float); ++i) { Wire.write(data[i]); }
  }
}
