import processing.serial.*;

Serial port;

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
  port.write('b');
  port.write(bank);
}

void chipErase() {
  port.write('c');
  port.write(0x55);
  port.write(0x55);
  port.write(0xaa);
  port.write('c');
  port.write(0x2a);
  port.write(0xaa);
  port.write(0x55);
  port.write('c');
  port.write(0x55);
  port.write(0x55);
  port.write(0x80);
  port.write('c');
  port.write(0x55);
  port.write(0x55);
  port.write(0xaa);
  port.write('c');
  port.write(0x2a);
  port.write(0xaa);
  port.write(0x55);
  port.write('c');
  port.write(0x55);
  port.write(0x55);
  port.write(0x10);
  port.write('c');
  port.write(0x55);
  port.write(0x55);
  port.write(0xaa);
  port.write('c');
  port.write(0x2a);
  port.write(0xaa);
  port.write(0x55);
  port.write('c');
  port.write(0x55);
  port.write(0x55);
  port.write(0x10);
}

void fileSelected(File selection) {
  if (selection == null) {
    println("Window was closed or the user hit cancel.");
  }
  else {
    waitReady();
    println("Erasing.");
    port.clear();

    byte rom[] = loadBytes(selection.getAbsolutePath());
    int ptr = 0;
    int banks = rom.length / 0x4000;

    port.write('i');
    port.write('d');
    port.write('c');
    port.write(0x00);
    port.write(0x00);
    port.write(0xf0);
    chipErase();
    delay(8000);

    println("Start Write.");

    // バンク0から書き込む
    for (int i=0; i<banks; i++) {
      selectBank(i);
      port.write('C');
      port.write(0x40);
      port.write(0x40);
      for (int j=0; j<0x4000; j++) {
        port.write(rom[ptr]);
        if ((ptr % 128) == 0) {
          delay(10);
        }
        ptr++;
      }
    }
    println("Write Finished.");

    println("Checking.");
    // 書き込んだデータの読み込み
    byte verify[] = new byte[rom.length];
    for ( int bank=0; bank<banks; bank++) {
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

