#include <avr/pgmspace.h>
#include <stdlib.h>
#include <SPI.h>

#define SSD1306_LCDWIDTH 128
#define SSD1306_LCDHEIGHT 64
#define dc     6
#define cs     7
#define rst  8

#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF

#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA

#define SSD1306_SETVCOMDETECT 0xDB

#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9

#define SSD1306_SETMULTIPLEX 0xA8

#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10

#define SSD1306_SETSTARTLINE 0x40

#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22

#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8

#define SSD1306_SEGREMAP 0xA0

#define SSD1306_CHARGEPUMP 0x8D

#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2
 
// buffer
static uint8_t buffer[SSD1306_LCDHEIGHT * SSD1306_LCDWIDTH / 8] = {}; 

void drawpixel(int x,int y,int colour)
{
	if((x>=128)||(y>=64)) return;
        int i;
        //for(i=0;i<1024;i++)
        i=x+(y/8)*SSD1306_LCDWIDTH;
        buffer[i]=1<<(y&7);
        //Serial.println(i); DEBUG println the buffer index
        
      
}

void ssd1306_data(uint8_t c)
{
	digitalWrite(dc,HIGH);
	fastSPIwrite(c);	
}

void ssd1306_command(uint8_t c)
{
	digitalWrite(dc,LOW);
	fastSPIwrite(c);	
}

void display_buffer()
{
 ssd1306_command(SSD1306_COLUMNADDR);
  ssd1306_command(0);   // Column start address (0 = reset)
  ssd1306_command(SSD1306_LCDWIDTH-1); // Column end address (127 = reset)

  ssd1306_command(SSD1306_PAGEADDR);
  ssd1306_command(0); // Page start address (0 = reset)
  ssd1306_command(7); // Page end address
  digitalWrite(dc,HIGH);

    for (uint16_t i=0; i<(SSD1306_LCDWIDTH*SSD1306_LCDHEIGHT/8); i++) {
      fastSPIwrite(buffer[i]);

}
}

void clearDisplay() {
  memset(buffer, 0, (SSD1306_LCDWIDTH*SSD1306_LCDHEIGHT/8));
}


void fastSPIwrite(uint8_t c)
{
	SPI.transfer(c);
}

void lcdInit()
{
  pinMode(dc, OUTPUT);
  SPI.begin ();
  pinMode(rst, OUTPUT);
  digitalWrite(rst, HIGH);
  delay(1);
    // bring reset low
    digitalWrite(rst, LOW);
    // wait 10ms
    delay(10);
    // bring out of reset
    digitalWrite(rst, HIGH);
    
    ssd1306_command(SSD1306_DISPLAYOFF);                    // 0xAE
    ssd1306_command(SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
    ssd1306_command(0x80);                                  // the suggested ratio 0x80
    ssd1306_command(SSD1306_SETMULTIPLEX);                  // 0xA8
    ssd1306_command(0x3F);
    ssd1306_command(SSD1306_SETDISPLAYOFFSET);              // 0xD3
    ssd1306_command(0x0);                                   // no offset
    ssd1306_command(SSD1306_SETSTARTLINE | 0x0);            // line #0
    ssd1306_command(SSD1306_CHARGEPUMP);                    // 0x8D
    ssd1306_command(0x14); //decided by printing else part
    ssd1306_command(SSD1306_MEMORYMODE);                    // 0x20
    ssd1306_command(0x00);                                  // 0x0 act like ks0108
    ssd1306_command(SSD1306_SEGREMAP | 0x1);
    ssd1306_command(SSD1306_COMSCANDEC);
    ssd1306_command(SSD1306_SETCOMPINS);                    // 0xDA
    ssd1306_command(0x12);
    ssd1306_command(SSD1306_SETCONTRAST);                   // 0x81
    ssd1306_command(0xCF); //decided***
    ssd1306_command(SSD1306_SETPRECHARGE);                  // 0xd9
    ssd1306_command(0xF1); //decided***
    ssd1306_command(SSD1306_SETVCOMDETECT);                 // 0xDB
    ssd1306_command(0x40);
    ssd1306_command(SSD1306_DISPLAYALLON_RESUME);           // 0xA4
    ssd1306_command(SSD1306_NORMALDISPLAY);                 // 0xA6
    
  ssd1306_command(SSD1306_DISPLAYON);//--turn on oled panel
	
}
void verticalLinel(int x,int y,int h,int clour)
{
  if(h>=64)return;
  int i,j;
  for(i=0;i<x;i++)
    {
        drawpixel(i,h,0);
    }
}

void horizontalLine(int x,int y,int w,int colour)
{
  int i,j;
  for(i=x;i<w;i++)
  {
    drawpixel(i,y,0);
  }
}

void setup()
{
	//SPI.begin();
        Serial.begin(9600);
	lcdInit();
	
        clearDisplay();
	//drawpixel(128,5,0);
	//verticalLinel(0,0,63,0);
        horizontalLine(0,0,127,0);
        horizontalLine(0,8,127,0);
        horizontalLine(0,9,127,0);
        horizontalLine(25,25,127,0);
        horizontalLine(0,50,127,0);
        //horizontalLine(0,51,127,0);
        //horizontalLine(0,52,127,0);
        //horizontalLine(0,53,127,0);
        horizontalLine(0,63,127,0);
        display_buffer();
	delay(2000);
}
void loop()
{

}
