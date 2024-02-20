#include <M5Cardputer.h>
#include "PCA9554.h"
#include <SD.h>
#include <FS.h>

PCA9554 ioCon1(
    0x27);  // Cria objeto. 

uint8_t res;
#define RECEPTOR_PIN 4
#define TRANSMISSOR_PIN 0

struct SignalEvent {
    unsigned long time;
    bool state;
    SignalEvent(unsigned long t = 0, bool s = false) : time(t), state(s) {}
};

const int maxEvents = 10000;
SignalEvent events[maxEvents];
int eventIndex = 0;
bool capturing = false;
bool readyToRepeat = false;
bool sendRF = false;

int menuIndex = 0;
bool inSubMenu = false;
int subMenuIndex = 0;
const int submenuSize = 7; 

String fileList[10]; // Supondo um máximo de 10 arquivos para simplificação
int fileCount = 0;
int currentFileIndex = 0; // Índice do arquivo atualmente selecionado
bool selectFileMode = false; // Modo de seleção de arquivo ativado

void setup() {
    M5Cardputer.begin();
    M5Cardputer.Power.begin();
    Wire.begin();
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setSwapBytes(true);
    M5Cardputer.Display.setRotation(1);
    delay(1000);

    ioCon1.twiWrite(
        21, 22);  // Sets the I2C pin of the connection.
    delay(10);
    res = 1;
    ioCon1.twiRead(res);

    M5Cardputer.Display.fillScreen(TFT_BLACK);
    displaySubMenu();
    ioCon1.pinMode(TRANSMISSOR_PIN, OUTPUT);
    ioCon1.digitalWrite(TRANSMISSOR_PIN, LOW);
    ioCon1.pinMode(RECEPTOR_PIN, INPUT);
    
    if (!SD.begin()) {
        showMessage("Falha ao iniciar SD");
    } else {
        loadFileList();
    }
}

void loop() {
    M5Cardputer.update();
    if (selectFileMode) {
        navigateFileSelection();
    } else {
        navigateSubMenu();
    }
}

void loadFileList() {
    File root = SD.open("/");
    while (File file = root.openNextFile()) {
        if (!file.isDirectory() && fileCount < 10) {
            fileList[fileCount++] = file.name();
        }
        file.close();
    }
    root.close();
}

void navigateSubMenu() {
    if (M5Cardputer.Keyboard.isKeyPressed('.')) {
        subMenuIndex = (subMenuIndex + 1) % submenuSize;
        displaySubMenu();
        delay(150);
    } else if (M5Cardputer.Keyboard.isKeyPressed(';')) {
        subMenuIndex = (subMenuIndex - 1 + submenuSize) % submenuSize;
        displaySubMenu();
        delay(150);
    } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) || M5Cardputer.Keyboard.isKeyPressed('/')) {
        delay(1000);
        executeRFAction(subMenuIndex);
    }
}

void enterSubMenu() {
    inSubMenu = true;
    subMenuIndex = 0;
    displaySubMenu();
}

void displaySubMenu() {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setTextSize(2);
    if ( subMenuIndex < 3 ){
      M5Cardputer.Display.setCursor(10, 20);
      M5Cardputer.Display.setTextSize(subMenuIndex == 0 ? 3 : 2);
      M5Cardputer.Display.println("Capturar");
      M5Cardputer.Display.setTextSize(subMenuIndex == 1 ? 3 : 2);
      M5Cardputer.Display.setCursor(10, 60);
      M5Cardputer.Display.println("Replay");
      M5Cardputer.Display.setTextSize(subMenuIndex == 2 ? 3 : 2);
      M5Cardputer.Display.setCursor(10, 100);
      M5Cardputer.Display.println("CapturarSD");
    } else if ( subMenuIndex < 6 ){
      M5Cardputer.Display.setTextSize(subMenuIndex == 3 ? 3 : 2);
      M5Cardputer.Display.setCursor(10, 20);
      M5Cardputer.Display.println("Jammmer 1");
      M5Cardputer.Display.setTextSize(subMenuIndex == 4 ? 3 : 2);
      M5Cardputer.Display.setCursor(10, 60);
      M5Cardputer.Display.println("Jammer 2");
      M5Cardputer.Display.setTextSize(subMenuIndex == 5 ? 3 : 2);
      M5Cardputer.Display.setCursor(10, 100);
      M5Cardputer.Display.println("ReplaySD");
    } else if ( subMenuIndex < 7 ){
      M5Cardputer.Display.setTextSize(subMenuIndex == 6 ? 3 : 2);
      M5Cardputer.Display.setCursor(10, 20);
      M5Cardputer.Display.println("Voltar");
    }
}


void executeRFAction(int action) {
    switch (action) {
        case 0:
            startCapture();
            break;
        case 1:
            if (readyToRepeat) repeatSignal();
            break;
        case 2:
            captureToFile();
            break;
        case 3:
            jam1();
            break;
        case 4:
            jam2();
            break;
        case 5:
            selectFileMode = true;
            break;
        case 6:
            inSubMenu = false;
            displaySubMenu();
            break;
    }
}

void startCapture() {
    capturing = true;
    eventIndex = 0;
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setCursor(0, 60);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.println("Capturando... Aperte para parar");

    delay(500); // Dá um breve momento para o usuário soltar o botão

    unsigned long lastCheckTime = micros();

    while (capturing) {
        M5Cardputer.update();

        // Verifica se o botão A foi pressionado para parar a captura
        if (M5Cardputer.BtnA.wasPressed()) {
            capturing = false;
            break;
        }

        byte pin = RECEPTOR_PIN;
        // Certifique-se de ler o estado do pino em intervalos regulares
        if (micros() - lastCheckTime > 1000) { 
            byte pinState = ioCon1.digitalRead(pin);
            unsigned long currentTime = micros();
            events[eventIndex++] = SignalEvent(currentTime, pinState);
            lastCheckTime = currentTime;
            if (eventIndex >= maxEvents) break; // Evita overflow do buffer
        }
    }

    showMessage("Captura Concluída. Eventos: " + String(eventIndex));
    readyToRepeat = true;
}



void repeatSignal() {
    if (eventIndex <= 1) {
        // Não há eventos suficientes para repetir
        showMessage("Não há eventos suficientes para repetir.");
        return;
    }

    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setCursor(0, 60);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.println("Repetindo...");
    delay(1000); // Dá um breve momento antes de iniciar a repetição

    for (int i = 1; i < eventIndex; i++) {
        unsigned long duration = events[i].time - events[i - 1].time;
        ioCon1.digitalWrite(TRANSMISSOR_PIN, events[i - 1].state ? HIGH : LOW);
        delayMicroseconds(duration);
    }

    // Garante que o pino volte ao estado inicial após a repetição
    ioCon1.digitalWrite(TRANSMISSOR_PIN, LOW);
    showMessage("Repetição Concluída.");
}

void navigateFileSelection() {
    if (M5Cardputer.Keyboard.isKeyPressed(';')) {
        currentFileIndex = (currentFileIndex + 1) % fileCount;
        displayFileList();
        delay(150);
    } else if (M5Cardputer.Keyboard.isKeyPressed('.')) {
        currentFileIndex = (currentFileIndex - 1 + fileCount) % fileCount;
        displayFileList();
        delay(150);
    } else if (M5Cardputer.BtnA.wasPressed()) {
        selectFileMode = false;
        repeatFromFile(fileList[currentFileIndex]);
    }
}

void displayFileList() {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextSize(2);
    for (int i = 0; i < fileCount; i++) {
        if (i == currentFileIndex) {
            M5Cardputer.Display.setTextColor(TFT_RED);
        }
        M5Cardputer.Display.println(fileList[i]);
        M5Cardputer.Display.setTextColor(TFT_WHITE);
    }
}

void repeatFromFile(String filename) {
    File file = SD.open(filename, FILE_READ);
    if (!file) {
        showMessage("Erro ao abrir arquivo: " + filename);
        return;
    }

    String line;
    bool dataSection = false;
    unsigned long previousTime = 0, currentTime, duration;
    int state;

    // Ler linha por linha do arquivo
    while (file.available()) {
        line = file.readStringUntil('\n');
        
        // Verifica se chegou na seção de dados
        if (line.startsWith("Data:")) {
            dataSection = true;
            continue;
        }

        // Processa apenas linhas de dados após encontrar "Data:"
        if (dataSection) {
            if (line.startsWith("t")) {
                int separatorIndex = line.indexOf(' ');
                if (separatorIndex != -1) {
                    currentTime = line.substring(1, separatorIndex).toInt();
                    state = line.substring(separatorIndex + 1).toInt();

                    // Se não for a primeira leitura, calcula a duração desde o último estado
                    if (previousTime != 0) {
                        duration = currentTime - previousTime;
                        // Pausa pela duração calculada antes de mudar o estado do pino
                        delayMicroseconds(duration);
                    }

                    // Atualiza o estado do pino
                    ioCon1.digitalWrite(TRANSMISSOR_PIN, state ? HIGH : LOW);

                    // Atualiza o tempo anterior para o próximo ciclo
                    previousTime = currentTime;
                }
            }
        }
    }

    file.close();
    showMessage("Reprodução concluída.");
}


void captureToFile() {
    capturing = true;
    eventIndex = 0;
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setCursor(0, 60);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.println("Capturando para arquivo...");

    File file = SD.open("/capture.sub", FILE_WRITE);
    if (!file) {
        showMessage("Erro ao abrir arquivo");
        return;
    }

    file.println("[subGhz]");
    file.print("Frequency: "); file.println("433920000"); // frequência
    file.println("Preset: FuriHalSubGhzPresetOok650Async"); // Preset
    file.println("Protocol: RAW"); // protocolo
    file.println("Data:");

    while (capturing && eventIndex < maxEvents) {
        M5Cardputer.update();
        if (M5Cardputer.BtnA.wasPressed()) {
            capturing = false;
        }

        byte pin = RECEPTOR_PIN;
        byte pinState = ioCon1.digitalRead(pin);
        unsigned long currentTime = micros();
        events[eventIndex] = SignalEvent(currentTime, pinState);
        
        
        file.print("t"); file.print(currentTime);
        file.print(" "); file.println(pinState ? "1" : "0");
        
        eventIndex++;
        if (eventIndex >= maxEvents) break; // Evita overflow do buffer
    }

    file.close();
    showMessage("Captura para arquivo concluída. Eventos: " + String(eventIndex));
    readyToRepeat = true;
}



void jam1() {
  sendRF = true;
  M5Cardputer.Display.fillScreen(TFT_BLACK);
  M5Cardputer.Display.setCursor(0, 60);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.println("Enviando... Aperte para parar");

  delay(1500); // Dá um breve momento para o usuário soltar o botão
  
  ioCon1.digitalWrite(TRANSMISSOR_PIN, HIGH);
  while (sendRF) {
    delay(2000);
    // Verifica se o botão ESC foi pressionado para parar o envio
      if (M5Cardputer.BtnA.wasPressed()) {
          sendRF = false;
          break; // Sai do loop se o botão for pressionado
      }


  }
  ioCon1.digitalWrite(TRANSMISSOR_PIN, LOW);

}

void jam2() {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setCursor(0, 60);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.println("Enviando... Aperte para parar");

    delay(2500); // Dá um breve momento para o usuário soltar o botão
    sendRF = true;

    while (sendRF) {
        // Verifica se o botão A foi pressionado para parar o envio
        if (M5Cardputer.BtnA.wasPressed()) {
            sendRF = false;
            break; // Sai do loop se o botão Enter for pressionado
        }

        // Loop para gerar uma série de pulsos
        for (int sequence = 1; sequence < 50; sequence++) {
            for (int duration = 1; duration <= 3; duration++) {
                ioCon1.digitalWrite(TRANSMISSOR_PIN, HIGH); // Ativa o pino
                // Mantém o pino ativo por um período que aumenta com cada sequência
                for (int widthsize = 1; widthsize <= (1 + sequence); widthsize++) {
                    delayMicroseconds(50);
                }

                ioCon1.digitalWrite(TRANSMISSOR_PIN, LOW); // Desativa o pino
                // Mantém o pino inativo pelo mesmo período
                for (int widthsize = 1; widthsize <= (1 + sequence); widthsize++) {
                    delayMicroseconds(50);
                }
            }
        }
    }
    ioCon1.digitalWrite(TRANSMISSOR_PIN, LOW);

    showMessage("Envio concluído.");
}

void showMessage(const String& message) {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setCursor(0, 60);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.println(message);
    delay(3000);
}

