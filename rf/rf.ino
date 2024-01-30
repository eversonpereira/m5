#include <M5StickCPlus.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#define FREQUENCY 433  // Frequência de 433MHz

enum MenuState { MENU_READ, MENU_REPLAY };
MenuState menuState = MENU_READ; // Estado inicial do menu

bool isCapturing = false; // Estado de captura
byte receivedData[255]; // Buffer para armazenar dados recebidos
byte dataLength = 0;    // Tamanho dos dados recebidos

void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);

  ELECHOUSE_cc1101.Init(); // Inicializa o CC1101
  ELECHOUSE_cc1101.setMHZ(FREQUENCY); // Define a frequência
  ELECHOUSE_cc1101.SetRx(); // Coloca o CC1101 em modo de recepção
  
  drawMenu();
}

void drawMenu() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  
  if (menuState == MENU_READ) {
    M5.Lcd.println("Ler");
  } else {
    M5.Lcd.println("Replay");
  }
}

void loop() {
  M5.update();

  // Navegação no menu
  if (M5.BtnA.wasPressed()) {
    menuState = MenuState((menuState + 1) % 2); // Alternar entre MENU_READ e MENU_REPLAY
    drawMenu();
  }

  // Seleção no menu
  if (M5.BtnB.wasPressed()) {
    if (menuState == MENU_READ) {
      if (!isCapturing) {
        // Iniciar captura
        isCapturing = true;
        dataLength = 0;
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.println("Capturando...");
      } else {
        // Parar captura
        isCapturing = false;
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.println("Captura Parada.");
      }
    } else if (menuState == MENU_REPLAY) {
      // Reproduzir dados capturados
      if (dataLength > 0) {
        ELECHOUSE_cc1101.SetTx();
        ELECHOUSE_cc1101.SendData(receivedData, dataLength);
        ELECHOUSE_cc1101.SetRx(); // Retorna ao modo de recepção após enviar
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.println("Replay Enviado!");
      }
    }
  }

  // Captura de dados
  if (isCapturing && ELECHOUSE_cc1101.CheckRxFifo() > 0) {
    dataLength = ELECHOUSE_cc1101.ReceiveData(receivedData);
    if (dataLength > 0) {
      isCapturing = false; // Parar captura após receber dados
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.println("Dados Recebidos!");
    }
  }
}
