
#include <USB.h>
#include "USBHIDKeyboard.h"

USBHIDKeyboard Keyboard;

void enviaComando(String text, bool useWinR, bool multiCmd) {
  Keyboard.begin();
  USB.begin();
  delay(2000);

  if (useWinR) {
    Keyboard.press(KEY_LEFT_GUI);
    Keyboard.press('r');
    Keyboard.releaseAll();
    delay(1000);
  }

  if (multiCmd) {
    int startPos = 0;
    int endPos = text.indexOf('\n', startPos);
    while (endPos != -1) {
      String cmd = text.substring(startPos, endPos);
      Keyboard.print(cmd);
      delay(500);
      Keyboard.press(KEY_RETURN);
      Keyboard.releaseAll();
      delay(500);

      startPos = endPos + 1;
      endPos = text.indexOf('\n', startPos);
    }

    // Print the last command or the entire command if no newline is present
    String lastCmd = text.substring(startPos);
    if (lastCmd.length() > 0) {
      Keyboard.print(lastCmd);
      delay(500);
      Keyboard.press(KEY_RETURN);
      Keyboard.releaseAll();
    }
  } else {
    Keyboard.print(text);
  }

  delay(1000);
  Keyboard.releaseAll();
}
