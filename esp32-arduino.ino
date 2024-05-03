// Include Libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

// Define OLED parameters
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // 0x3D for 128x64

#define NTP_SERVER "pool.ntp.org"
#define UTC_OFFSET 0
#define UTC_OFFSET_DST 0

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""

// Environment Thresholds
#define TEMP_UPPER 32
#define TEMP_LOWER 26
#define HUMID_UPPER 80
#define HUMIN_LOWER 60

//************************************ Pin configuration for Magicbit *****************
// Define pins for magicbit
// #define BUZZER 25
// #define LED_1 27
// #define PB_CANCEL 40 // 35 -> 32
// #define PB_OK 34

// // Define pins for Menu
// #define PB_DOWN 35 // 32 -> 35
// #define PB_UP 48

//************************************ Pin configuration for Wokwi *****************
//  Define pins for Wokwi
#define BUZZER 5
#define LED_1 15
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35
#define DHT_PIN 12

// ********************************************************************************
// Declare objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor;

// Time
int utc_offset = 19800; // Default offset is set to Sri Lankan time;  (in seconds)
struct tm timeinfo;
int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;

// alarms
bool alarm_enabled = true;
int n_alarms = 3;
int alarm_hours[] = {0, 1, 2};
int alarm_minutes[] = {1, 2, 10};
bool alarm_triggered[] = {false, false, false};

// buzzer
bool buzzer_on = true;

int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C, D, E, F, G, A, B, C_H};

// Menu
int current_mode = 0;
int max_modes = 5;
String modes[] = {"1 - Set Time Zone",
                  "2 - Set Alarm 1",
                  "3 - Set Alarm 2",
                  "4 - Set Alarm 3",
                  "5 - Disable All Alarms"};



// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void setup()
{
  // Initialize Serial Communication
  Serial.begin(115200);

  // Initialize input & outputs
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_DOWN, INPUT);
  pinMode(PB_UP, INPUT);

  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);

  // Initialize DHT Sensor
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  delay(2000);

  // Initialize wifi connection
  Serial.print("Connecting to WiFi");
  display.clearDisplay();
  print_line("Connecting to WIFI", 0, 0, 2);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 6);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
    display.clearDisplay();
    print_line("Waiting for connection...", 0, 0, 2);
  }
  Serial.println(" Connected!");
  display.clearDisplay();
  print_line("Connected to WIFI", 0, 0, 2);

  // Configure time zone
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
  delay(1000);

  // Clear the buffer
  display.clearDisplay();
}

void loop()
{
  update_time_with_check_alarm();

  // If Ok button is pressed go to menu, otherwise show time 
  if (digitalRead(PB_OK) == LOW)
  {
    delay(200); // small delay to debounce push button
    go_to_menu();
  }

  // Check the temperature and humiditiy of the environment
  check_environment(TEMP_LOWER, TEMP_UPPER, HUMIN_LOWER, HUMID_UPPER);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%



// function to print any string on the display
void print_line(String text, int column, int row, int text_size)
{

  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row); // column, row
  display.println(text);
  display.display();
}

// function to print the time on display
void print_time_now(void)
{

  // display.clearDisplay();
  display.fillRect(0, 0, 128, 16, BLACK);

  print_line(String(hours) + ":" + String(minutes) + ":" + String(seconds), 0, 0, 2);
  display.fillRect(0, 16, 128, 30, BLACK);
  print_line(String(days), 0, 22, 1);
}

// function to update time from info
void update_time()
{

  getLocalTime(&timeinfo);

  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  hours = atoi(timeHour);

  char timeMinute[3];
  strftime(timeHour, 3, "%M", &timeinfo);
  minutes = atoi(timeHour);

  char timeSecond[3];
  strftime(timeSecond, 3, "%S", &timeinfo);
  seconds = atoi(timeHour);

  char timeDay[3];
  strftime(timeDay, 3, "%d", &timeinfo);
  days = atoi(timeDay);
}

// function to update time while checking the alarm
void update_time_with_check_alarm()
{
  update_time();

  print_time_now();

  if (alarm_enabled == true)
  {
    for (int i = 0; i < n_alarms; i++)
    {
      // alarm trigger logic
      if ((alarm_triggered[i] == false) && (alarm_hours[i] == hours) && (alarm_minutes[i] == minutes))
      {
        ring_alarm();
        alarm_triggered[i] = true;
      }
    }
  }
}

// function to ring the alarms
void ring_alarm()
{
  display.clearDisplay();
  print_line("MEDICINE TIME", 0, 0, 2);

  blink_led(LED_1);
  digitalWrite(LED_1, HIGH);

  bool break_happened = false;

  // Ring the buzzer
  while (break_happened == false && digitalRead(PB_CANCEL) == HIGH)
  {

    // Ring the buzzer
    if (buzzer_on == true)
    {
      for (int i = 0; i < n_notes; i++)
      {
        if (digitalRead(PB_CANCEL) == LOW)
        {
          // for debouncing
          break_happened = true;
          delay(200);
          break;
        }

        tone(BUZZER, notes[i]);
        delay(500);
        noTone(BUZZER);
        delay(2);
      }
    }
  }
  digitalWrite(LED_1, LOW);
  display.clearDisplay();
}

void blink_led(int led)
{
  digitalWrite(led, HIGH);
  delay(300);
  digitalWrite(led, LOW);
  delay(300);
}

// function to get the button press as an input
int wait_for_button_press()
{
  while (true)
  {
    if (digitalRead(PB_UP) == LOW)
    {
      delay(200);
      return PB_UP;
    }
    else if (digitalRead(PB_DOWN) == LOW)
    {
      delay(200);
      return PB_DOWN;
    }

    else if (digitalRead(PB_OK) == LOW)
    {
      delay(200);
      return PB_OK;
    }

    else if (digitalRead(PB_CANCEL) == LOW)
    {
      delay(200);
      return PB_CANCEL;
    }

    update_time();
  }
}

// function to enter the menu
void go_to_menu()
{

  while (digitalRead(PB_CANCEL) == HIGH)
  {
    display.clearDisplay();
    print_line(modes[current_mode], 0, 0, 2);

    int pressed = wait_for_button_press();

    if (pressed == PB_UP)
    {
      delay(200);
      current_mode += 1;
      current_mode = current_mode % max_modes;
    }

    else if (pressed == PB_DOWN)
    {
      delay(200);
      current_mode -= 1;

      if (current_mode < 0)
      {
        current_mode = max_modes - 1;
      }
    }

    else if (pressed == PB_OK)
    {
      delay(200);
      run_mode(current_mode);
    }

    else if (pressed == PB_CANCEL)
    {
      delay(200);
      break;
    }
  }
}

// function to set alarms
void set_alarm(int alarm_number)
{

  // set hours
  int temp_hour = alarm_hours[alarm_number];

  while (true)
  {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP)
    {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    }

    else if (pressed == PB_DOWN)
    {
      delay(200);
      temp_hour -= 1;

      if (temp_hour < 0)
      {
        temp_hour = 23;
      }
    }

    else if (pressed == PB_OK)
    {
      delay(200);
      alarm_hours[alarm_number] = temp_hour;
      break;
    }

    else if (pressed == PB_CANCEL)
    {
      delay(200);
      break;
    }
  }

  // set minutes
  int temp_minute = alarm_minutes[alarm_number];

  while (true)
  {
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minute), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP)
    {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    }

    else if (pressed == PB_DOWN)
    {
      delay(200);
      temp_minute -= 1;

      if (temp_minute < 0)
      {
        temp_minute = 59;
      }
    }

    else if (pressed == PB_OK)
    {
      delay(200);
      alarm_minutes[alarm_number] = temp_minute;
      break;
    }

    else if (pressed == PB_CANCEL)
    {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Alarm is set", 0, 0, 2);
  delay(1000);
}

// function to run the relevant mode
void run_mode(int mode)
{
  if (mode == 0)
  {
    set_time_zone();
  }
  else if (mode == 1 || mode == 2 || mode == 3)
  {
    set_alarm(mode - 1); // alarm number = mode number -1.
  }
  else if (mode == 4)
  {
    alarm_enabled = false;
    display.clearDisplay();
    print_line("All Alarms disabled", 0, 0, 2);
    delay(1000);
  }
}

// function to check the environment of the Medicine
void check_environment(int temp_low, int temp_high, int humid_low, int humid_high)
{
  // Get data from the DHT sensor
  TempAndHumidity data = dhtSensor.getTempAndHumidity();

  // 
  if (data.temperature > temp_high)
  {
    display.clearDisplay();
    print_line("TEMP HIGH", 0, 40, 1);
  }

  if (data.temperature < temp_low)
  {
    display.clearDisplay();
    print_line("TEMP LOW", 0, 40, 1);
  }

  if (data.humidity > humid_high)
  {
    display.clearDisplay();
    print_line("HUMIDITY HIGH", 0, 50, 1);
  }

  if (data.temperature < humid_low)
  {
    display.clearDisplay();
    print_line("HUMIDITY LOW", 0, 50, 1);
  }
}

// function to set the time zone
void set_time_zone()
{
  int temp_offset_hours = utc_offset / 3600;
  int temp_offset_minutes = utc_offset / 60 - temp_offset_hours * 60;

  // Change offset hours
  while (true)
  {
    display.clearDisplay();
    print_line("Enter UTC offset hours: " + String(temp_offset_hours), 0, 0, 2);

    int pressed = wait_for_button_press();

    if (pressed == PB_UP)
    {
      delay(200);
      temp_offset_hours++;
      if (temp_offset_hours > 14)
      {                          // 14 hours multiplies by 60.
        temp_offset_hours = -12; // 12 hours multiplies by 60.
      }
    }
    else if (pressed == PB_DOWN)
    {
      delay(200);
      temp_offset_hours--;
      if (temp_offset_hours < -12)
      {
        temp_offset_hours = 14;
      }
    }
    else if (pressed == PB_OK)
    {
      delay(200); 
      break;
    }
    else if (pressed == PB_CANCEL)
    {
      delay(200);
      break;
    }
  }

  // Change offset minutes
  while (true)
  {
    display.clearDisplay();
    print_line("Enter offset minutes: " + String(temp_offset_minutes), 0, 0, 2);

    int pressed = wait_for_button_press();

    if (pressed == PB_UP)
    {
      delay(200);
      temp_offset_minutes++;
      temp_offset_minutes = temp_offset_minutes % 60;
    }
    else if (pressed == PB_DOWN)
    {
      delay(200);
      temp_offset_minutes--;
      temp_offset_minutes = temp_offset_minutes % 60;
      if (temp_offset_minutes < 0)
      {
        temp_offset_minutes = 59;
      }
    }
    else if (pressed == PB_OK)
    {
      delay(200);
      utc_offset = temp_offset_hours * 3600 + temp_offset_minutes * 60;
      break;
    }
    else if (pressed == PB_CANCEL)
    {
      delay(200);
      break;
    }
  }

  // Configure time according to the Changed offset
  configTime(utc_offset, UTC_OFFSET_DST, NTP_SERVER);
  display.clearDisplay();
  print_line("Time zone is set", 0, 0, 2);
  delay(1000);
}
