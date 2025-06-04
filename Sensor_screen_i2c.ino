#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "index_html.h" // Basic page
static const size_t page_len = strlen_P(index_html);

#include <Wire.h>

#include <Adafruit_SSD1306.h>
#include <BME280I2C.h>


// Screen definitions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1

// i2c definitions
#define SDA 1 // sda connected to pin 1
#define SCL 2 // scl connected to pin 2
#define SCREEN_ADDR 0x3c
#define SENSOR_ADDR 0x76

const char *ssid = "Viroeira";
const char *password = "5D6974AF77";

static AsyncWebServer server(80);
static AsyncEventSource events("/events");
// #define PRINT_SENSOR_SERIAL

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Define screen dimensions. Wire will need to be initializated inside setup
BME280I2C bme;

struct BmeData{
  float temp;
  float pressure;
  float humidity;
};

BmeData bme_data {
  0.0,
  0.0,
  0.0,
};

static uint32_t lastSSE = 0;
static uint32_t lastSensorRead = 0;
static uint32_t server_tick = 200;
static uint32_t sensor_tick = 100;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); // For debugging purposes
  Wire.begin(SDA, SCL); // initializes i2c interface

  // init peripherals
  init_screen();
  init_bmp280();

  // init wifi and server
  init_wifi();
  init_server();
  
}

void loop() {
  
  read_bme_sensor();
  display_data();
  update_server_data();
}

void read_bme_sensor() {
  // check if sensor was read recently
  if (millis() - lastSensorRead > sensor_tick){
    bme.read( bme_data.pressure, bme_data.temp, bme_data.humidity);
    #ifdef PRINT_SENSOR_SERIAL
    Serial.printf("temp: %f, pressure: %f, humid: %f\n", bme_data.temp, bme_data.pressure, bme_data.humidity);
    #endif
    lastSensorRead = millis();
  }
}

void display_data(){
  display.clearDisplay();

  display.setCursor(0, 0);     // Start at top-left corner
  // Prints local IP
  display.print("IP: ");
  display.println(WiFi.localIP());

  display.write("temp: ");
  display.print(bme_data.temp);
  display.println(" C");
  // display.display();

  display.write("pressure: ");
  display.print(bme_data.pressure);
  display.println(" hPa");

  // display.setCursor(display.getCursorX(), display.getCursorY()+2);

  display.display();

  delay(100);

}

// TODO
void update_server_data(){
  // checks if last server update was longer than the minimum tick.
  if (millis() - lastSSE > server_tick){
    // sends temperature and pressure
    events.send(String(bme_data.temp).c_str(), "temperature", millis());
    events.send(String(bme_data.pressure).c_str(), "pressure", millis());
    lastSSE = millis();
  }
}

void init_wifi(){

  WiFi.begin(ssid, password);

  Serial.println();
  Serial.print("Connecting to " + String(ssid) + "..");

  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWifi Connected.");
  Serial.print("Ip Address: ");
  Serial.println(WiFi.localIP());

}

void init_server(){

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200,"text/html",(uint8_t *)index_html, page_len);
  });

  // prints on serial when client connects. Also sends a message.
  events.onConnect([](AsyncEventSourceClient *client){
    Serial.printf("SSE Server connected! Client ID: %" PRIu32 "\n", client->lastId()); // PRIu32 is the equivalent of %d, %i, %lu, %u but for uint32_t.
    client->send("Hello! Connected!", NULL, millis(), 1000);
  });

  // prints on serial when client disconnects
  events.onDisconnect([](AsyncEventSourceClient *client){
    Serial.printf("SSE Client disconnected! ID: %" PRIu32 "\n", client->lastId());
  });

  server.addHandler(&events);
  server.begin();  
}

// void template_processor(const){
  
// }

// Peripherals
void init_screen(){
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDR, false, false)) {
    Serial.printf("Failed to initialize screen at addr %s\n", String(uint8_t(SCREEN_ADDR)));
    for(;;); // Stops code here.
  }
  // initialize display stuff 
  display.setTextSize(1);      // Normal 2:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);
}
void init_bmp280(){
  if(!bme.begin()){
      Serial.println("Could not find BMP280 sensor!");
      for(;;); // Stops Code here.
    }
}
