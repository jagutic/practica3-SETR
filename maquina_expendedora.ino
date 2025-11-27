#include <avr/wdt.h>
#include <LiquidCrystal.h>
#include <DHT.h>
#include <Thread.h>
#include <StaticThreadController.h>
#include <ThreadController.h>


// Pin DATA
#define LED1 4
#define LED2 3
#define BUTTON 2
#define US_TRIG 6
#define US_ECHO 7
#define JOYST_Y A2
#define JOYST_X A3
#define JOYST A4

// Global constants
enum states {
  PRE_CHARGING = 0,
  CHARGING,
  PRE_SERVICE_WAITING,
  SERVICE_WAITING,
  PRE_SERVICE_EXEC,
  SERVICE_EXEC,
  PRE_ADMIN,
  ADMIN,
  ADMIN_OPTION,
  PRE_PRICES_MENU,
  PRICES_MENU,
  PRE_CHANGE_PRICES,
  CHANGE_PRICES
};

// Lists to display
String products_menu[] = {
  "Cafe Solo ",
  "Cafe Cort.",
  "Cafe Doble",
  "Cafe Prem.",
  "Chocolate "
};
const int n_products = 5;
float prices[] = {1.00, 1.10, 1.25, 1.50, 2.00};
float aux_prices[n_products];
String prices_str[n_products];

String admin_menu[] = {
  "Ver temperatura",
  "Ver sensor dist.",
  "Ver contador",
  "Mod. precios"
};
const int n_admin_options = 4;

// Global variables
enum states state;
int blink_state = HIGH;
int last_counter, counter;
int option_selected, blinks;
Thread led_thread, sensors_thread, counter_thread;

// Sensors
LiquidCrystal lcd(8, 9, 10, 11, 12, 13); //rs, enable, d4, d5, d6, d7
DHT dht(5, DHT11);

// Sensors values
float temperature, humidity, distance;
float last_temperature, last_humidity, last_distance;

// Threads funcs
void counter_update() {
  counter = millis() / 1000;
}
void sensors_update() {
  distance = get_dist(US_TRIG, US_ECHO);
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
}
void blink() {
  digitalWrite(LED1, blink_state ? HIGH : LOW);
  blink_state = !blink_state;
}


// Interrupt funcs
volatile unsigned long init_button_time;
void buttonISR() {
  if (digitalRead(BUTTON) == LOW) { // FALLING
    init_button_time = millis();

  } else { // RISING
    if (millis() - init_button_time > 5000) {
      // Alternate admin state
      bool admin_mode = state == ADMIN || state == ADMIN_OPTION;
      state = admin_mode ? PRE_SERVICE_EXEC : PRE_ADMIN;

    } else if (millis() - init_button_time > 3000) {
      if (state == SERVICE_EXEC) {
        state = PRE_SERVICE_EXEC;
      }
    }
  }
}

// Convert float list to String list
void to_str(String s_list[], float f_list[], int n) {
  for (int i = 0; i < n; i++) {
    s_list[i] = String(f_list[i], 2); // 2 decimals
  }
}

// Normal funcs
void progresive_led(int led, int time) {
  // Turns on [led] led pin during [time]ms
  int steps = 2000; // More steps more progressive
  int delayPerStep = time / steps;
  float increment = 255.0 / steps; // 255 intensities avialable

  for (float i = 0; i <= 255.0; i += increment) {
    analogWrite(LED2, int(i));
    delay(delayPerStep);
  }
  return;
}

// Display menu passed as parameter
void display_menu(LiquidCrystal &lcd, String menu[],
                  String* menu2, int n_options, int option) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(">");
  lcd.print(menu[option]);
  if (menu2 != nullptr) {
    lcd.print(" ");
    lcd.print(menu2[option]);
  }

  if (option + 1 < n_options) {
    lcd.setCursor(0, 1);
    lcd.print(" ");
    lcd.print(menu[option + 1]);
    if (menu2 != nullptr) {
      lcd.print(" ");
      lcd.print(menu2[option + 1]);
    }
  }
}

// Display humidity and temperature values
void display_dht(LiquidCrystal lcd_sensor, float t, float h) {
  lcd_sensor.clear();
  lcd_sensor.print("Temp: ");
  lcd_sensor.print(t);
  lcd.setCursor(0, 1);
  lcd_sensor.print("Hum: ");
  lcd_sensor.print(h);

  return;
}

// Return distance in cm
float get_dist(int trig, int echo) {
  // Clean
  digitalWrite(trig, LOW);
  delayMicroseconds(2);

  // Send trig
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  // Calculate distance
  float duration = pulseIn(echo, HIGH);
  return duration * 0.034 / 2;
}

void setup() {
  // Init useful things
  Serial.begin(9600);
  dht.begin();
  lcd.begin(16, 2);
  randomSeed(analogRead(A0));
  attachInterrupt(digitalPinToInterrupt(BUTTON), buttonISR, CHANGE);

  // Set pins direction
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(US_TRIG, OUTPUT);
  pinMode(US_ECHO, INPUT);
  pinMode(JOYST_Y, INPUT);
  pinMode(JOYST, INPUT_PULLUP);
  pinMode(BUTTON, INPUT_PULLUP);

  // Set threads
  led_thread = Thread();
  led_thread.enabled = true;
  led_thread.setInterval(1000);
  led_thread.onRun(blink);

  sensors_thread = Thread();
  sensors_thread.enabled = true;
  sensors_thread.setInterval(500);
  sensors_thread.onRun(sensors_update);

  counter_thread = Thread();
  counter_thread.enabled = true;
  counter_thread.setInterval(1000);
  counter_thread.onRun(counter_update);

  // Watchdog set
  wdt_disable();
  wdt_enable(WDTO_8S);

  to_str(prices_str, prices, n_products);
  state = PRE_CHARGING;
}

void loop() {
  if (counter_thread.shouldRun()) {
    counter_thread.run();
  }
  /* PRE cases set everything up to the next state */
  switch (state) {
  case PRE_CHARGING:
  {
    lcd.clear();
    lcd.print("CARGANDO...");
    blinks = 0;

    state = CHARGING;
    break;
  }
  case CHARGING:
  {
    // led1 blinking 3 times
    if (led_thread.shouldRun()) {
      led_thread.run();

      if (++blinks == 6) {
        state = PRE_SERVICE_WAITING;
      }
    }
    break;
  }
  case PRE_SERVICE_WAITING:
  {
    lcd.clear();
    lcd.print("ESPERANDO ");
    lcd.setCursor(0, 1);
    lcd.print("CLIENTE");

    state = SERVICE_WAITING;
    break;
  }
  case SERVICE_WAITING:
  {
    float dist = get_dist(US_TRIG, US_ECHO);
    Serial.print("Dist: ");
    Serial.println(dist);

    if (dist < 100) { // 1 meter
      display_dht(lcd, dht.readTemperature(), dht.readHumidity());
      wdt_reset();
      delay(5000);
      state = PRE_SERVICE_EXEC;
    }

    delay(100);
    break;
  }
  case PRE_SERVICE_EXEC:
  {
    // If coming from admin
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
  
    display_menu(lcd, products_menu, prices_str, n_products, option_selected);
    option_selected = 0;
    state = SERVICE_EXEC;
    break;
  }
  case SERVICE_EXEC:
  {
    // Read JOYSTICK
    int y = analogRead(JOYST_Y);
    int button = digitalRead(JOYST);

    if (button == LOW) {
      lcd.clear();
      lcd.print("Preparando");
      lcd.setCursor(0, 1);
      lcd.print("Cafe...");

      // Prograsive led for random time while lcd message printed
      wdt_reset();
      progresive_led(LED2, random(4000, 8000));

      // Print retire bebida when led at its maximum(coffee ready)
      lcd.clear();
      lcd.print("RETIRE BEBIDA");
      wdt_reset();
      delay(3000);

      // Wait 3 secs until shutting down led and displaying poructs again
      digitalWrite(LED2, LOW);
      display_menu(lcd, products_menu, prices_str, n_products, option_selected);

    // Change option_selected coffee
    } else if (y < 450 && option_selected > 0) { // DOWN
      option_selected--;
      display_menu(lcd, products_menu, prices_str, n_products, option_selected);

    } else if (y > 550 && option_selected < n_products - 1) { // UP
      option_selected++;
      display_menu(lcd, products_menu, prices_str, n_products, option_selected);
    }

    // Check if user is gone
    if (get_dist(US_TRIG, US_ECHO) > 100) {
      state = PRE_SERVICE_WAITING;
    }

    wdt_reset();
    delay(175);
    break;
  }
  case PRE_ADMIN:
  {
    option_selected = 0;
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    display_menu(lcd, admin_menu, nullptr, n_admin_options, option_selected);

    state = ADMIN;
    break;
  }
  case ADMIN:
  {
    // Read JOYSTICK
    int y = analogRead(JOYST_Y);
    int button = digitalRead(JOYST);

    if (button == LOW) {
      state = ADMIN_OPTION;

    // Change option_selected
    } else if (y < 450 && option_selected > 0) { // DOWN
      option_selected--;
      display_menu(lcd, admin_menu, nullptr, n_admin_options, option_selected);

    } else if (y > 550 && option_selected < n_admin_options - 1) { // UP
      option_selected++;
      display_menu(lcd, admin_menu, nullptr, n_admin_options, option_selected);
    }

    wdt_reset();
    delay(175);
    break;
  }
  case ADMIN_OPTION:
  {
    if (sensors_thread.shouldRun()) {
      sensors_thread.run();
    }

    switch (option_selected) {
    case 0: // Show dht
      if (last_temperature != temperature || last_humidity != humidity) {
        display_dht(lcd, temperature, humidity);
        last_temperature = temperature;
        last_humidity = humidity;
      }
      break;
    case 1: // Show distance
      if (last_distance != distance) {
        lcd.clear();
        lcd.print("Dist: ");
        lcd.print(distance);
        lcd.print(" cm");
        last_distance = distance;
      }
      break;
    case 2: // Show time
      if (last_counter != counter) {
        lcd.clear();
        lcd.print(counter);
        lcd.print(" seconds");
        last_counter = counter;
      }
      break;
    case 3: // Change prices
      state = PRE_PRICES_MENU;
      break;
    default:
      lcd.clear();
      lcd.print("No admin opt");
    }

    // Retrun to admin normal menu when joystick to left
    if (analogRead(JOYST_X) > 550) {
      state = PRE_ADMIN;
    }

    wdt_reset();
    break;
  }
  case PRE_PRICES_MENU:
  {
    option_selected = 0;
    to_str(prices_str, prices, n_products);
    display_menu(lcd, products_menu, prices_str, n_products, option_selected);
    state = PRICES_MENU;
    break;
  }
  case PRICES_MENU:
  {
    // At start to avoid bouncing effect
    delay(175);

    // Read JOYSTICK
    int y = analogRead(JOYST_Y);
    int button = digitalRead(JOYST);

    if (button == LOW) {
      state = PRE_CHANGE_PRICES;

    // Change option_selected
    } else if (y < 450 && option_selected > 0) { // DOWN
      option_selected--;
      display_menu(lcd, products_menu, prices_str, n_products, option_selected);

    } else if (y > 550 && option_selected < n_products - 1) { // UP
      option_selected++;
      display_menu(lcd, products_menu, prices_str, n_products, option_selected);
    }

    // Retrun to admin normal menu when joystick to left
    if (analogRead(JOYST_X) > 550) {
      state = PRE_ADMIN;
    }

    wdt_reset();
    break;
  }
  case PRE_CHANGE_PRICES:
  {  
    // Create auxiliar copy of prices
    for (int i = 0; i < n_products; i++) {
      aux_prices[i] = prices[i];
    }

    // Display with inverted order -> price - name
    display_menu(lcd, prices_str, products_menu, option_selected, option_selected);
    state = CHANGE_PRICES;
    break;
  }
  case CHANGE_PRICES:
  {
    // At start to avoid bouncing effect
    delay(175);

    // Read JOYSTICK
    int y = analogRead(JOYST_Y);
    int button = digitalRead(JOYST);

    if (button == LOW) {
      // Return to normal prices menu with new prices
      for (int i = 0; i < n_products; i++) {
        prices[i] = aux_prices[i];
      }
      state = PRE_PRICES_MENU;

    // Change price
    // n_products = option selected so seems to be last one and only one row is printed
    } else if (y < 450) { // DOWN
      aux_prices[option_selected] += 0.05;
      to_str(prices_str, aux_prices, n_products);
      display_menu(lcd, prices_str, products_menu, option_selected, option_selected);

    } else if (y > 550) { // UP
      aux_prices[option_selected] -= 0.05;
      to_str(prices_str, aux_prices, n_products);
      display_menu(lcd, prices_str, products_menu, option_selected, option_selected);
    }

    // Retrun to admin normal menu when joystick to left
    if (analogRead(JOYST_X) > 550) {
      state = PRE_PRICES_MENU;
    }

    wdt_reset();
    break;
  }
  default:
    lcd.clear();
    lcd.print("No state");
    delay(1000);
    wdt_reset();
  }
}
