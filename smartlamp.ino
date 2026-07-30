#define LED_PIN 23
#define LDR_PIN 4

int intensidadeLED = 10;

void setup() {
  Serial.begin(115200);

  pinMode(LDR_PIN, INPUT);

  // PWM (API nova)
  ledcAttach(LED_PIN, 5000, 8);

  atualizarLED();
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    processarComando(cmd);
  }
}

void atualizarLED() {
  int pwm = map(intensidadeLED, 0, 100, 0, 255);
  ledcWrite(LED_PIN, pwm);
}

void processarComando(String cmd) {

  if (cmd.startsWith("SET_LED")) {

    String valor = cmd.substring(7);
    valor.trim();

    int x = valor.toInt();

    if (x >= 0 && x <= 100) {
      intensidadeLED = x;
      atualizarLED();
      Serial.println("RES SET_LED 1");
    } else {
      Serial.println("RES SET_LED -1");
    }

  } else if (cmd == "GET_LED") {

    Serial.print("RES GET_LED ");
    Serial.println(intensidadeLED);

  } else if (cmd == "GET_LDR") {

    Serial.print("RES GET_LDR ");
    Serial.println(analogRead(LDR_PIN));

  } else {

    Serial.println("ERR Unknown command.");

  }
}