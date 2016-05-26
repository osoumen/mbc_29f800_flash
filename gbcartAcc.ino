#define DATAMODETEXT 1
//#define NOWAIT 1

#ifdef NOWAIT
#define _delay_us(x)   ;
#else
#include <util/delay.h>
#endif

#include <SPI.h>
#define ADDRLATCHPIN   (1 << 2)
#define ADDRDATAPIN    (1 << 3)
#define ADDRCLKPIN     (1 << 5)

#define FWEPIN      (1 << 2)
#define WEPIN       (1 << 3)
#define MREQPIN     (1 << 4)
#define RDPIN       (1 << 5)

#define ADDRESS_CLOSE  (0x8000)

unsigned int  val;
unsigned char bus_close;

unsigned int flBank = 1;

// for 29f800
unsigned int fCmdFirstAddr = 0x0aaa;
unsigned int fCmdSecondAddr = 0x0555;

// for 29f040
//unsigned int fCmdFirstAddr = 0x5555;
//unsigned int fCmdSecondAddr = 0x2aaa;

#ifdef DATAMODETEXT
int datamode=1;
#endif

void setup()
{
  DDRC |= FWEPIN | WEPIN | MREQPIN | RDPIN;

  bus_close = 0xff;
  PORTC = bus_close;

  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  addressSet(ADDRESS_CLOSE);
  setDirectionIn();
  PORTB |= B00000011;
  PORTD |= B11111100;

  Serial.begin(230400);
  Serial.println("OK");
}

void setDirectionIn()
{
  //PORT2-9を入力に設定する
  DDRB &= B11111100;
  DDRD &= B00000011;

  //プルアップ状態にする
  PORTB |= B00000011;
  PORTD |= B11111100;   
}

void setDirectionOut()
{
  //PORT2-9を出力に設定する
  DDRB |= B00000011;
  DDRD |= B11111100;
}

void dataSet(unsigned char data)
{
  //8bit値を出力する
  PORTB &= ~0x03;
  PORTD &= ~0xfc;
  PORTB |= data >> 6;
  PORTD |= (data << 2) & 0xfc;
}

unsigned char dataRead()
{
  //8bit値を入力する
  unsigned char data = ((PINB & 0x03) << 6) | ((PIND & 0x0fc) >> 2);
  return data;
}

void addressSet(unsigned int addr)
{
  unsigned char high = (addr & 0xff00) >> 8;
  unsigned char low = addr & 0xff;
  //74HC595に16bitのアドレスを出力
  PORTB &= ~ADDRLATCHPIN;
  SPI.transfer(high);  //MSB
  SPI.transfer(low);   //LSB
  PORTB |= ADDRLATCHPIN;
}

unsigned char readserial()
{
  //シリアルポートから1バイト読み込む
  while (Serial.available() == 0);
  return Serial.read();
}

unsigned char readhex()
{
  //シリアルポートから16進数(2バイト)読み込む
  unsigned char data[2];
  data[0]=readserial();
  data[1]=readserial();
  if((data[0]>='0')&&(data[0]<='9'))
    data[0]-='0';
  else if ((data[0]>='A')&&(data[0]<='F'))
  {
    data[0]-='A';
    data[0]+=10;
  }
  else if ((data[0]>='a')&&(data[0]<='f'))
  {
    data[0]-='a';
    data[0]+=10;
  }
  else
    data[0]=0;

  if((data[1]>='0')&&(data[1]<='9'))
    data[1]-='0';
  else if ((data[1]>='A')&&(data[1]<='F'))
  {
    data[1]-='A';
    data[1]+=10;
  }
  else if ((data[1]>='a')&&(data[1]<='f'))
  {
    data[1]-='a';
    data[1]+=10;
  }
  else
    data[1]=0;

  return (data[0]<<4)|data[1];
}

unsigned char readROM(unsigned int addr)
{
  unsigned char data;
  setDirectionIn();
  addressSet(addr);
  PORTC &= ~RDPIN;
  //データが出力されるまで最大200ns程度掛かる
  __asm__ __volatile__ ("nop");
  __asm__ __volatile__ ("nop");
  __asm__ __volatile__ ("nop");
  data = dataRead();
  PORTC |= RDPIN;
  return data;
}

void writeROM(unsigned int addr, unsigned char data)
{
  addressSet(addr);
  setDirectionOut();
  dataSet(data);
  PORTC &= ~WEPIN;
  __asm__ __volatile__ ("nop");
  PORTC |= WEPIN;
  setDirectionIn();
}

void writeFlashViaChip(unsigned int addr, unsigned char data)
{
  // 制御チップ経由でFlashROMにコマンドを書き込むとき用
  addressSet(addr);
  setDirectionOut();
  dataSet(data);
  PORTC &= ~WEPIN;
  DDRB &= B11111100;
  DDRD &= B00000011;
  // WEを上げたときにハイインピーダンス状態にする
  PORTC |= WEPIN;
  dataSet(0xff);
}

void enableChipAccess()
{
  writeROM(0x0120, 0x09);
  writeROM(0x0121, 0xaa);
  writeROM(0x0122, 0x55);
  writeROM(0x013f, 0xa5);
}

void disableChipAccess()
{
  writeROM(0x0120, 0x08);
  writeROM(0x013f, 0xa5);
}

void disableChipProtection()
{
  writeROM(0x0120, 0x0a);
  writeROM(0x0125, 0x62);
  writeROM(0x0126, 0x04);
  writeROM(0x013f, 0xa5);
  writeROM(0x0120, 0x01);
  writeROM(0x013f, 0xa5);
  writeROM(0x0120, 0x02);
  writeROM(0x013f, 0xa5);
}

void changeROMBank(unsigned int bank)
{
  writeROM(0x2100, bank&0xff);
}

void writeFlashChip(unsigned int addr, unsigned char data)
{
  writeROM(0x0120, 0x0f);
  writeROM(0x0125, (addr >> 8) & 0xff);
  writeROM(0x0126, addr & 0xff);
  writeROM(0x0127, data);
  writeFlashViaChip(0x013f, 0xa5);
}

void writeFlash(unsigned int addr, unsigned char data)
{
  addressSet(addr);
  setDirectionOut();
  dataSet(data);
  PORTC &= ~FWEPIN;
  __asm__ __volatile__ ("nop");
  __asm__ __volatile__ ("nop");
  __asm__ __volatile__ ("nop");
  PORTC |= FWEPIN;
  setDirectionIn();
}

void programFlash(unsigned int addr, unsigned char data)
{
  unsigned char verify = 0;

  writeFlash(fCmdFirstAddr, 0xaa);
  writeFlash(fCmdSecondAddr, 0x55);
  writeFlash(fCmdFirstAddr, 0xa0);
  writeFlash(addr, data);
  do {
    verify = readROM(addr);
  } 
  while (verify != data);
}

void programFlashChip(unsigned int addr, unsigned char data)
{
  unsigned char verify = 0;

  // バンクを先頭に
  changeROMBank(1);

  writeFlashChip(0x5555, 0xaa);
  writeFlashChip(0x2aaa, 0x55);
  writeFlashChip(0x5555, 0xa0);

  // バンクの切り替え
  changeROMBank(flBank);

  writeFlashChip(addr, data);
}

void terminateProgramFlashChip(unsigned int addr)
{
  writeFlashChip(addr, 0xff);
  _delay_us(10000);
  writeFlashChip(0, 0xf0);
}

unsigned char readRAM(unsigned int addr)
{
  unsigned char data;

  addressSet(addr);
  setDirectionIn();
  PORTC &= ~(MREQPIN | RDPIN);
  //データが出力されるまで最大200ns程度掛かる
  __asm__ __volatile__ ("nop");
  __asm__ __volatile__ ("nop");
  __asm__ __volatile__ ("nop");
  data = dataRead();
  PORTC |= MREQPIN | RDPIN;
  return data;
}

void writeRAM(unsigned int addr, unsigned char data)
{
  addressSet(addr);
  setDirectionOut();
  dataSet(data);
  PORTC &= ~(MREQPIN | WEPIN);
  __asm__ __volatile__ ("nop");
  __asm__ __volatile__ ("nop");
  __asm__ __volatile__ ("nop");
  PORTC |= MREQPIN | WEPIN;
  setDirectionIn();
}

unsigned int inputAddress()
{
  unsigned int addr;
#ifdef DATAMODETEXT
  if (!datamode) {
#endif
    val = readserial();
    addr += val << 8;
    addr += readserial();
#ifdef DATAMODETEXT
  }
  else {
    val = readhex();
    addr += val << 8;
    addr += readhex();
  }
#endif
  return addr;
}

unsigned char inputData()
{
  unsigned char  data;
#ifdef DATAMODETEXT
  if (!datamode) {
#endif
    data = readserial();
#ifdef DATAMODETEXT
  }
  else {
    data = readhex();
  }
#endif
  return data;
}

void loop()
{
  unsigned long addr;
  unsigned char data;
  val = readserial();
  switch (val) {
    /*
  case 'A':
     addr = inputAddress();
     addressSet(addr);
     break;
     case 'B':
     data = inputData();
     dataSet(data);
     break;
     */
  case 'D':
#ifdef DATAMODETEXT
    datamode=readserial()-'0';
    if(datamode)
      Serial.println("Data mode set to Text");
#else
    readserial();  //次のバイトを空読み
#endif
    break;
    /*
  case 'Q':
     digitalWrite(WEPIN, LOW);
     digitalWrite(MREQPIN, LOW);
     setDirectionIn();
     break;
     case 'q':
     digitalWrite(MREQPIN, HIGH);
     digitalWrite(WEPIN, HIGH);
     setDirectionOut();
     break;
     */
  case 'k':  //ACK
    Serial.println("OK");
    break;

  case 'W':  //ROMに1バイト書き込む
    {
      addr = inputAddress();
      data = inputData();
      writeROM(addr, data);
      addressSet(ADDRESS_CLOSE);
      break;
    }

  case 'w':  //RAMに1バイト書き込む
    {
      addr = inputAddress();
      data = inputData();
      writeRAM(addr, data);
      addressSet(ADDRESS_CLOSE);
      break;
    }

  case 'b':
    {
      flBank = inputData();
      changeROMBank(flBank);
      break;
    }

  case 'f':  //FlashROMに1バイト書き込む
    {
      addr = inputAddress();
      data = inputData();
      writeFlash(addr, data);
      addressSet(ADDRESS_CLOSE);
      break;
    }

  case 'R':  //ROMから1バイト読み込む
    {
      addr = inputAddress();
      data = readROM(addr);
#ifdef DATAMODETEXT
      if (!datamode) {
#endif
        Serial.write(data);
#ifdef DATAMODETEXT
      }
      else {
        if(data<16)
          Serial.print(0,HEX);
        Serial.println(data,HEX);
      }
#endif
      //setDirectionOut();
      break;
    }

  case 'r':
    {
      addr = inputAddress();
      data = readRAM(addr);
#ifdef DATAMODETEXT
      if (!datamode) {
#endif
        Serial.write(data);
#ifdef DATAMODETEXT
      }
      else {
        if(data<16)
          Serial.print(0,HEX);
        Serial.println(data,HEX);
      }
#endif
      //setDirectionOut();
      break;
    }
/*
  case 'E':
    {
      // フラッシュカートリッジのチップ消去
      writeFlash(0x0555, 0xaa);
      writeFlash(0x02aa, 0x55);
      writeFlash(0x0555, 0x80);
      writeFlash(0x0555, 0xaa);
      writeFlash(0x02aa, 0x55);
      writeFlash(0x0555, 0x10);
      addressSet(ADDRESS_CLOSE);
      break;
    }
    
  case 'e':
    {
      // NP8Mのチップ消去
      enableChipAccess();
      writeFlashChip(0x5555, 0xaa);
      writeFlashChip(0x2aaa, 0x55);
      writeFlashChip(0x5555, 0x80);
      writeFlashChip(0x5555, 0xaa);
      writeFlashChip(0x2aaa, 0x55);
      writeFlashChip(0x5555, 0x10);
      writeFlashChip(0x5555, 0xaa);
      writeFlashChip(0x2aaa, 0x55);
      writeFlashChip(0x5555, 0x10);
      //disableChipAccess();
      addressSet(ADDRESS_CLOSE);
      break;
    }
*/
  case 'P':
    {
      // フラッシュカートリッジに1バイト書き込む
      addr = inputAddress();
      data = inputData();
      programFlash(addr, data);
      addressSet(ADDRESS_CLOSE);
      break;
    }

  case 'm':
    {
      char cmd_ind = readserial();
      if (cmd_ind == '0') {
        fCmdFirstAddr = inputAddress();
      }
      if (cmd_ind == '1') {
        fCmdSecondAddr = inputAddress();
      }
      break;
    }
    
  case 'M':
    {
      char cmd_ind = readserial();
      if (cmd_ind == '0') {
        Serial.println(fCmdFirstAddr,HEX);
      }
      if (cmd_ind == '1') {
        Serial.println(fCmdSecondAddr,HEX);
      }
      break;
    }
  
  case 'i':
    {
      // NP8Mのコマンド有効
      enableChipAccess();
      addressSet(ADDRESS_CLOSE);
      break;
    }
    
  case 'd':
    {
      // NP8Mのメモリ保護を無効にする
      disableChipProtection();
      addressSet(ADDRESS_CLOSE);
      break;
    }
    
  case 'c':
    {
      // NP8MのFlashROMにWrite
      addr = inputAddress();
      data = inputData();
      writeFlashChip(addr, data);
      addressSet(ADDRESS_CLOSE);
      break;
    }

  case 'p':
    {
      // NP8Mに1バイト書き込む
      addr = inputAddress();
      data = inputData();
      enableChipAccess();
      programFlashChip(addr, data);
      terminateProgramFlashChip(addr);
      disableChipAccess();
      addressSet(ADDRESS_CLOSE);
      break;
    }

  case 'C':
    {
      // NP8Mに連続書き込み
      val = inputData();
      addr = val << 8;
      val = inputData();
      unsigned long  bytes = val << 8;
      unsigned long  endaddr = addr+bytes;
      if ( endaddr > 0x10000 ) {
        endaddr = 0x10000;
      }
      unsigned long  blocks = bytes / 128;
      enableChipAccess();
      for (int i=0; i<blocks; i++) {
        data = readserial();
        programFlashChip(addr, data);
        addr++;
        for (int j=1; j<128 && addr<endaddr; j++) {
          data = readserial();
          writeFlashChip(addr, data);
          addr++;
        }
        terminateProgramFlashChip(addr-1);
      }
      disableChipAccess();
      addressSet(ADDRESS_CLOSE);
      break;
    }

  case 'F':
    {
      // フラッシュカートリッジに連続書き込み
      val = inputData();
      addr = val << 8;
      val = inputData();
      unsigned long  bytes = val << 8;
      unsigned long  endaddr = addr+bytes;
      if ( endaddr > 0x10000 ) {
        endaddr = 0x10000;
      }
      while (addr<endaddr) {
        data = readserial();
        programFlash(addr, data);
        addr++;
      }
      //setDirectionOut();
      addressSet(ADDRESS_CLOSE);
      break;
    }
  
  case 'T':    //256バイト単位でROMを読み出す
    {
      val = inputData();
      addr = val << 8;
      val = inputData();
      unsigned long  bytes = val << 8;
      unsigned long  endaddr = addr+bytes;
      if ( endaddr > 0x10000 ) {
        endaddr = 0x10000;
      }
      while (addr<endaddr) {
        data = readROM(addr);
#ifdef DATAMODETEXT
        if (!datamode) {
#endif
          Serial.write(data);
#ifdef DATAMODETEXT
        }
        else {
          if(data<16)
            Serial.print(0,HEX);
          if ((addr%16) == 15) {
            Serial.println(data,HEX);
          }
          else {
            Serial.print(data,HEX);
          }
        }
#endif
        addr++;
      }
      //setDirectionOut();
      break;
    }

  case 't':    //256バイト単位でRAMを読み出す
    {
      val = inputData();
      addr = val << 8;
      val = inputData();
      unsigned long  bytes = val << 8;
      unsigned long  endaddr = addr+bytes;
      if ( endaddr > 0x10000 ) {
        endaddr = 0x10000;
      }
      while (addr<endaddr) {
        data = readRAM(addr);
        Serial.write(data);
        addr++;
      }
      setDirectionIn();
      break;
    }
  }
}














































