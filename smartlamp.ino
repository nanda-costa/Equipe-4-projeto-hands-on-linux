// Pino do LED (PWM) e pino do LDR (entrada analógica).
// TODO: ajuste esses valores para o pino em que você realmente ligou cada componente.
int ledPin = 23;
int ledValue = 10;  // Valor atual do LED, de 0 a 255 (10 = brilho baixo inicial)

int ldrPin = 4;  // Pino físico rotulado "D4" na placa = GPIO4 (era GPIO34 por engano)
// Valor máximo lido no LDR (calibrar apontando uma lanterna de celular para o sensor
// e observando o maior valor bruto retornado por analogRead()).
int ldrMax = 4000;

int threshold = 50;  // Limite (0-100) usado futuramente pelo daemon para decidir quando acender o LED

#define LED_PWM_FREQ    5000
#define LED_PWM_RES     8  // 8 bits -> ledValue de 0 a 255

String serialBuffer = "";     // Acumula os caracteres recebidos até um '\n'
unsigned long lastLdrBroadcast = 0;
const unsigned long LDR_BROADCAST_INTERVAL_MS = 2000;

void setup() {
    Serial.begin(9600);

    pinMode(ldrPin, INPUT);

    // Configura o pino do LED como saída PWM (ledc é a API de PWM do ESP32, core 3.x)
    ledcAttach(ledPin, LED_PWM_FREQ, LED_PWM_RES);

    ledUpdate();

    Serial.printf("SmartLamp Initialized.\n");
}

// Função loop será executada infinitamente pelo ESP32
void loop() {
    // Obtenha os comandos enviados pela serial e processe-os com a função processCommand
    while (Serial.available() > 0) {
        char c = Serial.read();
        if (c == '\n') {
            serialBuffer.trim();
            if (serialBuffer.length() > 0) {
                processCommand(serialBuffer);
            }
            serialBuffer = "";
        } else {
            serialBuffer += c;
        }
    }

    // Envia o valor do LDR automaticamente a cada 2 segundos (o driver de kernel
    // usa essa leitura periódica para monitorar a luminosidade sem precisar perguntar)
    unsigned long now = millis();
    if (now - lastLdrBroadcast >= LDR_BROADCAST_INTERVAL_MS) {
        lastLdrBroadcast = now;
        Serial.printf("RES GET_LDR %d\n", ldrGetValue());
    }
}

void processCommand(String command) {
    // Separa o nome do comando do parâmetro numérico (se houver)
    int spaceIndex = command.indexOf(' ');
    String cmd = (spaceIndex == -1) ? command : command.substring(0, spaceIndex);
    long param = (spaceIndex == -1) ? -1 : command.substring(spaceIndex + 1).toInt();

    if (cmd == "SET_LED") {
        ledValue = map(constrain(param, 0, 100), 0, 100, 0, 255);
        ledUpdate();
        Serial.printf("RES SET_LED 1\n");

    } else if (cmd == "GET_LED") {
        int percent = map(ledValue, 0, 255, 0, 100);
        Serial.printf("RES GET_LED %d\n", percent);

    } else if (cmd == "GET_LDR") {
        Serial.printf("RES GET_LDR %d\n", ldrGetValue());

    } else if (cmd == "SET_THRESHOLD") {
        threshold = constrain(param, 0, 100);
        Serial.printf("RES SET_THRESHOLD 1\n");

    } else if (cmd == "GET_THRESHOLD") {
        Serial.printf("RES GET_THRESHOLD %d\n", threshold);

    } else {
        Serial.printf("ERR unknown command: %s\n", cmd.c_str());
    }
}

// Função para atualizar o valor do LED
void ledUpdate() {
    // ledValue já está normalizado entre 0 e 255 (ver SET_LED em processCommand)
    ledcWrite(ledPin, ledValue);
}

// Função para ler o valor do LDR
int ldrGetValue() {
    int raw = analogRead(ldrPin);          // ESP32: ADC de 12 bits (0-4095)
    Serial.printf("DEBUG ldr raw=%d\n", raw);  // TODO: remover depois de calibrar
    if (raw > ldrMax) ldrMax = raw;        // ajusta o máximo observado em tempo real
    return map(raw, 0, ldrMax, 0, 100);
}
