#include <M5StickCPlus2.h>

#define RECEPTOR_PIN 26
#define TRANSMISSOR_PIN 0

struct SignalEvent {
    unsigned long time;
    bool state;
};

const int maxEvents = 10000;
events = new (std::nothrow) SignalEvent[maxEvents];
volatile int eventIndex = 0;
bool capturing = false;
bool readyToRepeat = false;

void IRAM_ATTR onSignalChange() {
    if (capturing && eventIndex < maxEvents) {
        events[eventIndex++] = {micros(), digitalRead(RECEPTOR_PIN)};
    }
}

void startCapture() {
    capturing = true;
    eventIndex = 0;
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Capturando...");
    attachInterrupt(RECEPTOR_PIN, onSignalChange, CHANGE);
}

void stopCapture() {
    capturing = false;
    detachInterrupt(RECEPTOR_PIN);
    readyToRepeat = true;
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Captura Concluida.");
}

void repeatSignal() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Repetindo...");
    if (eventIndex > 0) { // Garante que há eventos para repetir
        unsigned long lastTime = events[0].time;
        for (int i = 0; i < eventIndex; i++) {
            unsigned long currentTime = events[i].time;
            unsigned long duration = i == 0 ? 0 : currentTime - lastTime; // Calcula a diferença de tempo desde o último evento
            delayMicroseconds(duration); // Espera a duração calculada
            digitalWrite(TRANSMISSOR_PIN, events[i].state ? HIGH : LOW); // Define o estado do pino
            lastTime = currentTime; // Atualiza o último tempo registrado
        }
    }
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Repeticao Concluida.");
}


void setup() {
    M5.begin();
    pinMode(TRANSMISSOR_PIN, OUTPUT);
    pinMode(RECEPTOR_PIN, INPUT);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("Pronto.");
}

void loop() {
    M5.update();
    if (M5.BtnA.wasPressed()) {
        if (!capturing) {
            startCapture();
        } else {
            stopCapture();
        }
    }
    if (M5.BtnB.wasPressed() && readyToRepeat) {
        repeatSignal();
        readyToRepeat = false;
    }
}
