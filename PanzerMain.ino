  #include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ESP32Servo.h>

const int kEngine_pins[2][3] = {
  {5, 4, 0}, // right:  {speed, IN1, IN2}
  {17, 15, 2}   // left:   {speed, IN3, IN4}
};

float joyVals[2]; // x-axis, y-axis

float aimVals[2]; // x-axis, y-axis

bool btnState = false;

// Percentage to Byte (0-255)
int percToByte(float percentage) {
  return (255.0 / 100.0) * percentage;
}

// Network-name
const char* ssid = "Tank-Controller";

// Network-password
const char *pw = ".geheim.";

// Settings for Network
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// WebServer instance is created with the ESPAsyncWebServer.h Library
AsyncWebServer server(80);

// Servo instances are created with the ESP32_Servo.h Library
Servo turn_servo;
Servo tilt_servo;
Servo reload_servo;

// Turret-Move variables
const int turn_servo_pin = 21;
const int tilt_servo_pin = 18;

const int turn_default_servo = 90;
const int turn_min_servo = 0;
const int turn_max_servo = 180;

const int tilt_default_servo = 133;
const int tilt_min_servo = 100;
const int tilt_max_servo = 160;

// Turret-Reload variables
const int servoReload_pin = 19;

const int reload_pushed_servo = 75;
const int reload_default_servo = 122;

const int reload_time = 300;

const int shoot_motor_pin = 16;

// shortcut delay
void d(int seconds) {
  delay(seconds * 1000);
}

// shortcut digitalWrite
void dW(int pin, bool state) {
  digitalWrite(pin, state);
}

// shortcut analogWrite
void aW(int pin, byte value) {
  analogWrite(pin, value);
}

// Function to write a direction to the motors
void kEngineWrite(String side, bool direction1, bool direction2) {
  if (side == "r" || side == "R") {
    dW(kEngine_pins[0][1], direction1);
    dW(kEngine_pins[0][2], direction2);
  } else if (side == "l" || side == "L") {
    dW(kEngine_pins[1][1], direction1);
    dW(kEngine_pins[1][2], direction2);
  }
}

// Function to write a percentage to the turret-servos
void servoAngle(String servoType, int anglePerc) {
  int angle;
  if (btnState == true) {
    return;
  } else {
    if (servoType == "turn" || servoType == "Turn") {
      angle = map(anglePerc, 0, 100, turn_min_servo, turn_max_servo);
      turn_servo.write(angle);
    } else if (servoType == "tilt" || servoType == "Tilt") {
      angle = map(anglePerc, 0, 100, tilt_min_servo, tilt_max_servo);
      tilt_servo.write(angle);
    } else if (servoType == "reload" || servoType == "Reload") {
      angle = map(anglePerc, 0, 100, reload_pushed_servo, reload_default_servo);
      reload_servo.write(angle);
    } else {
      return;
    }
  }
}

// Function to reload the turret
void reloadTurret() {
  reload_servo.write(reload_pushed_servo);
  d(1);
  reload_servo.write(reload_default_servo);
}

// Function to start/stop the turret-motors
void turretMotor(bool turretState) {
  if (turretState == true) {
    dW(shoot_motor_pin, HIGH);
  } else {
    dW(shoot_motor_pin, LOW);
  }
}

//Function to shoot
void shoot() {
  turretMotor(true);
  d(1.25);
  reloadTurret();
  d(0.5);
  btnState = false;
  turretMotor(false);
}

// Function to convert and write values of the joystick to the motorengines
// y-axis: -100 (100% Backward) to 100 (100% Forward)
// x-axis: -100 (100% Right) to 100 (100% left)
void kEngine(float y_axis, float x_axis) {
  int speed_r;
  int speed_l;

  int direction_r;
  int direction_l;

  int speed_x = percToByte(x_axis);
  int speed_y = percToByte(y_axis);

  Serial.print("speed_x: "); Serial.println(speed_x);
  Serial.print("speed_y: "); Serial.println(speed_y);

  if (speed_y >= 25.5) { // Forward
  Serial.println("Forward");
    if (speed_x >= 38.5) { // Right
    Serial.println("Right");
      speed_l = speed_y;
      speed_r = speed_y - speed_x;

      direction_l = 1;

      if (speed_r < 0) {
        speed_r *= -1;
        direction_r = -1;
      } else if (speed_r > 0) {
        direction_r = 1;
      } else if (speed_r == 0) {
        direction_r = 0;
      }
    } else if (speed_x <= -38.5) { // Left
    Serial.println("Left");
      speed_l = speed_y - (speed_x * -1);
      speed_r = speed_y;

      direction_r = 1;

        if (speed_l < 0) {
          speed_l *= -1;
          direction_l = -1;
        } else if (speed_l > 0) {
          direction_l = 1;
        } else if (speed_l == 0) {
          direction_l = 0;
        }
    } else if (speed_x < 38.5 && speed_x > -38.5) { // No turn
    Serial.println("No Turn");
      speed_r = speed_y;
      speed_l = speed_y;

      direction_r = 1;
      direction_l = 1;
    }
  } else if (speed_y <= -25.5) { // Backward
  Serial.println("Backwards");
    speed_y *= -1;
    if (speed_x >= 38.5) { // Right
    Serial.println("Right");
      speed_l = speed_y;
      speed_r = speed_y - speed_x;

      direction_l = 1;

      if (speed_r < 0) {
        speed_r *= -1;
        direction_r = -1;
      } else if (speed_r > 0) {
        direction_r = -1;
      } else if (speed_r == 0) {
        direction_r = 0;
      }
    } else if (speed_x <= -38.5) { // Left
    Serial.println("Left");
      speed_l = speed_y - (speed_x * -1);
      speed_r = speed_y;

      direction_r = 1;

      if (speed_l < 0) {
        speed_l *= -1;
        direction_l = -1;
      } else if (speed_l > 0) {
        direction_l = -1;
      } else if (speed_l == 0) {
        direction_l = 0;
      }
    } else if (speed_x < 38.5 && speed_x > -38.5) { // No turn
    Serial.println("No Turn");
      speed_r = speed_y;
      speed_l = speed_y;

      direction_r = -1;
      direction_l = -1;
    }
  } else if (speed_y < 25.5 && speed_y > -25.5) { // Stand
  Serial.println("Stand");
    if (speed_x >= 15) { // Right
    Serial.println("Right");
      speed_r = speed_x;
      speed_l = speed_x;

      direction_l = 1;
      direction_r = -1;
    } else if (speed_x <= -38.5) { // Left
    Serial.println("Left");
      speed_x *= -1;

      speed_r = speed_x;
      speed_l = speed_x;

      direction_l = -1;
      direction_r = 1;
    } else if (speed_x <= 38.5 && speed_x >= -38.5) { // No turn
    Serial.println("No Turn");
      speed_r = 0;
      speed_l = 0;

      direction_r = 0;
      direction_l = 0;
    }
  }

  Serial.print("direction_r: "); Serial.println(direction_r);
  Serial.print("direction_l: "); Serial.println(direction_l);
  Serial.print("speed_r: "); Serial.println(speed_r);
  Serial.print("speed_l: "); Serial.println(speed_l);

  switch (direction_r) {
    case 1:
      kEngineWrite("r", HIGH, LOW);
    break;
        
    case 0:
      kEngineWrite("r", LOW, LOW);
    break;

    case -1:
      kEngineWrite("r", LOW, HIGH);
    break;
  }

  switch (direction_l) {
    case 1:
      kEngineWrite("l", HIGH, LOW);
    break;
        
    case 0:
      kEngineWrite("l", LOW, LOW);
    break;

    case -1:
      kEngineWrite("l", LOW, HIGH);
    break;
  }

  aW(kEngine_pins[0][0], speed_r);
  aW(kEngine_pins[1][0], speed_l);
}

void setup() {
  Serial.begin(9600);

  // Network opened and modified
  // name, password, hide-name, max-connections
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pw, 1, 0, 1);
  WiFi.softAPConfig(local_ip, gateway, subnet);

  while (!Serial) {
    delay(0.001);
  }

  pinMode(shoot_motor_pin, OUTPUT);
  digitalWrite(shoot_motor_pin, LOW);

  kEngine(0, 0);

  turn_servo.attach(turn_servo_pin);
  tilt_servo.attach(tilt_servo_pin);
  reload_servo.attach(servoReload_pin);

  turn_servo.write(turn_default_servo);
  tilt_servo.write(tilt_default_servo);
  reload_servo.write(reload_default_servo);

  for (int j = 0; j < 2; j++) {
    for (int i = 0; i < 3; i++) {
      pinMode(kEngine_pins[j][i], OUTPUT);
    }
  }

  // Check if Filemanager started
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS!");
    return;
  }

  // return;

  // request = "IP" requestAnswer = "index.html"
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // request = "/style.css" requestAnswer = "style.css"
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // request = "/function.js" requestAnswer = "function.js"
  server.on("/function.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/function.js", "text/javascript");
  });

  // request = "/joy.js" requestAnswer = "joy.js"
  server.on("/joy.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/joy.js", "text/javascript");
  });

  //request = "/favicon.ico" requestAnswer = "favicon.ico"
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/favicon.ico", "image");
  });

  // request = "/joy" requestAnswer = "joystick-values", write values to chain engines
  server.on("/joy", HTTP_GET, [](AsyncWebServerRequest *request) {
    int arguments = request->args();
    
    for (int i = 0; i < arguments; i++) {
      if (request->argName(i) == "joyX") {
        joyVals[0] = (request->arg(i)).toFloat(); // x-axis
      } else if (request->argName(i) == "joyY") {
        joyVals[1] = (request->arg(i)).toFloat(); // y-axis
      }

      Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(), request->arg(i).c_str());
    }
    kEngine(joyVals[1], joyVals[0]);

    request->redirect("/");
  });

  // request = "/aimField" requestAnswer = "aimField-values", wirte values to turret servos
  server.on("/aimField", HTTP_GET, [](AsyncWebServerRequest *request) {
    int arguments = request->args();
    for (int i = 0; i < arguments; i++) {
      if (request->argName(i) == "aimX") {
        aimVals[0] = request->arg(i).toFloat();
      } else if (request->argName(i) == "aimY") {
        aimVals[1] = request->arg(i).toFloat();
      }

      Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(), request->arg(i).c_str());
    }
    servoAngle("turn", aimVals[0]);
    servoAngle("tilt", aimVals[1]);

    request->redirect("/");
  });

  // request = "/shootBtn" requestAnswer = "button-pressed: true/false",
  // start/stop turret motors, write values to reload servo
  server.on("/shootBtn", HTTP_GET, [](AsyncWebServerRequest *request) {
    int arguments = request->args();
    for (int i = 0; i < arguments; i++) {
      if (request->argName(i) == "btn") {
        if (request->arg(i) == "true") {
          btnState = true;
        }
        
        Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(), request->arg(i).c_str());
      }
    }

    if (btnState == true) {
      shoot();
    }

    request->redirect("/");
  });

  // Start Server
  server.begin();
}

void loop() {
  d(0.001);
}
