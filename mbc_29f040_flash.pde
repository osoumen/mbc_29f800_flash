import processing.serial.*;

Serial port;

// for 29f040
static int fCmdFirstAddr = 0x5555;
static int fCmdSecondAddr = 0x2aaa;

void setup() {
  size(200, 200);

  String portName = Serial.list()[findArduinoPort()];
  port = new Serial(this, portName, 230400);
  println(portName);

  noLoop();
}

int findArduinoPort()
{
  int portNum = 0;
  for (int i=0; i<Serial.list().length; i++) {
    if (Serial.list()[i].startsWith("/dev/tty.usbmodem")) {
      portNum = i;
    }
  }
  return portNum;
}

void draw()
{
  selectInput("Select a gb file:", "fileSelected");
}

void waitReady() {
  String val = "";
  port.clear();
  while ( val.startsWith ("OK") == false ) {
    port.write('k');
    delay(100);
    if ( port.available() > 3) {
      val = port.readStringUntil('\n');
    }
  }
  port.write("D0");
}

void selectBank(int bank) {
  port.write('W');
  port.write(0x21);
  port.write(0x00);
  port.write(bank);
}

void chipErase() {
  port.write('f');
  port.write(0x55);
  port.write(0x55);
  port.write(0xaa);
  port.write('f');
  port.write(0x2a);
  port.write(0xaa);
  port.write(0x55);
  port.write('f');
  port.write(0x55);
  port.write(0x55);
  port.write(0x80);
  port.write('f');
  port.write(0x55);
  port.write(0x55);
  port.write(0xaa);
  port.write('f');
  port.write(0x2a);
  port.write(0xaa);
  port.write(0x55);
  port.write('f');
  port.write(0x55);
  port.write(0x55);
  port.write(0x10);
}

void setCommandAddr(int firstAddr, int secondAddr)
{
  port.write('m');
  port.write('0');
  port.write((firstAddr >> 8) & 0xff);
  port.write(firstAddr & 0xff);
  port.write('m');
  port.write('1');
  port.write((secondAddr >> 8) & 0xff);
  port.write(secondAddr & 0xff);
}

void fileSelected(File selection) {
  if (selection == null) {
    println("Window was closed or the user hit cancel.");
  }
  else {
    waitReady();
    println("Erasing.");
    port.clear();
    
    setCommandAddr(fCmdFirstAddr, fCmdSecondAddr);

    byte rom[] = loadBytes(selection.getAbsolutePath());
    int ptr = 0;
    int banks = rom.length / 0x4000;

    chipErase();
    delay(8000);

    println("Start Write.");

    // 固定バンクを書き込む
    selectBank(1);
    port.write('F');
    port.write(0x00);
    port.write(0x40);
    for (int j=0; j<0x4000; j++) {
      port.write(rom[ptr]);
      if ((ptr % 16) == 0) {
        delay(1);
      }
      ptr++;
    }
    // 可変バンクを書き込む
    for (int i=1; i<banks; i++) {
      selectBank(i);
      port.write('F');
      port.write(0x40);
      port.write(0x40);
      for (int j=0; j<0x4000; j++) {
        port.write(rom[ptr]);
        if ((ptr % 16) == 0) {
          delay(1);
        }
        ptr++;
      }
    }
    println("Write Finished.");

    port.write('f');
    port.write(0x00);
    port.write(0x00);
    port.write(0xf0);

    println("Checking.");
    // 書き込んだデータの読み込み
    byte verify[] = new byte[rom.length];
    // 16KBずつ読みだす
    port.write('T');
    port.write(0x00);
    port.write(0x40);
    for ( int i=0; i<0x4000; i++ ) {
      while (port.available () == 0);
      verify[i] = byte(port.read());
    }
    for ( int bank=1; bank<banks; bank++) {
      //バンク切り替え
      selectBank(bank);

      // 16KBずつ読みだす
      port.write('T');
      port.write(0x40);
      port.write(0x40);
      for ( int i=0; i<0x4000; i++ ) {
        while (port.available () == 0);
        verify[0x4000*bank+i] = byte(port.read());
      }
    }
    // 読み込んだデータをチェック
    int errors = 0;
    for (int i=0; i<rom.length; i++) {
      if (rom[i] != verify[i]) {
        println(hex(i, 6) + ": " + hex(verify[i], 2) + " " + hex(rom[i], 2));
        errors++;
      }
    }
    println(str(errors) + " / " + str(rom.length) + " errors.");
    println("Finished.");
  }
  exit();
}

