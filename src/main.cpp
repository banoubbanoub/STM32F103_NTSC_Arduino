#include <Arduino.h>
#include <TTVout.h>
#include <fontALL.h>
#include "schematic.h"
#include "TVOlogo.h"
#define SC_448x216  
TTVout TV;

int zOff = 150;
int xOff = 0;
int yOff = 0;
int cSize = 50;
int view_plane = 64;
float angle = PI/60;

float cube3d[8][3] = {
  {xOff - cSize,yOff + cSize,zOff - cSize},
  {xOff + cSize,yOff + cSize,zOff - cSize},
  {xOff - cSize,yOff - cSize,zOff - cSize},
  {xOff + cSize,yOff - cSize,zOff - cSize},
  {xOff - cSize,yOff + cSize,zOff + cSize},
  {xOff + cSize,yOff + cSize,zOff + cSize},
  {xOff - cSize,yOff - cSize,zOff + cSize},
  {xOff + cSize,yOff - cSize,zOff + cSize}
};

uint16_t cube2d[8][2];


void intro() {
unsigned char w,l,wb;
  int index;
  w = pgm_read_byte(TVOlogo);
  l = pgm_read_byte(TVOlogo+1);
  if (w&7)
    wb = w/8 + 1;
  else
    wb = w/8;
  index = wb*(l-1) + 2;
  for ( unsigned char i = 1; i < l; i++ ) {
    TV.bitmap((TV.hres() - w)/2,0,TVOlogo,index,w,i);
    index-= wb;
    TV.delay(50);
  }
  for (unsigned char i = 0; i < (TV.vres() - l)/2; i++) {
    TV.bitmap((TV.hres() - w)/2,i,TVOlogo);
    TV.delay(50);
  }
  TV.delay(3000);
  TV.clear_screen();
}


void draw_cube() {
  TV.draw_line(cube2d[0][0],cube2d[0][1],cube2d[1][0],cube2d[1][1],WHITE);
  TV.draw_line(cube2d[0][0],cube2d[0][1],cube2d[2][0],cube2d[2][1],WHITE);
  TV.draw_line(cube2d[0][0],cube2d[0][1],cube2d[4][0],cube2d[4][1],WHITE);
  TV.draw_line(cube2d[1][0],cube2d[1][1],cube2d[5][0],cube2d[5][1],WHITE);
  TV.draw_line(cube2d[1][0],cube2d[1][1],cube2d[3][0],cube2d[3][1],WHITE);
  TV.draw_line(cube2d[2][0],cube2d[2][1],cube2d[6][0],cube2d[6][1],WHITE);
  TV.draw_line(cube2d[2][0],cube2d[2][1],cube2d[3][0],cube2d[3][1],WHITE);
  TV.draw_line(cube2d[4][0],cube2d[4][1],cube2d[6][0],cube2d[6][1],WHITE);
  TV.draw_line(cube2d[4][0],cube2d[4][1],cube2d[5][0],cube2d[5][1],WHITE);
  TV.draw_line(cube2d[7][0],cube2d[7][1],cube2d[6][0],cube2d[6][1],WHITE);
  TV.draw_line(cube2d[7][0],cube2d[7][1],cube2d[3][0],cube2d[3][1],WHITE);
  TV.draw_line(cube2d[7][0],cube2d[7][1],cube2d[5][0],cube2d[5][1],WHITE);
}
void printcube() {
  //calculate 2d points
  for(byte i = 0; i < 8; i++) {
    cube2d[i][0] = (uint16_t)((cube3d[i][0] * view_plane / cube3d[i][2]) + (TV.hres()/2));
    cube2d[i][1] = (uint16_t)((cube3d[i][1] * view_plane / cube3d[i][2]) + (TV.vres()/2));
  }
  TV.delay_frame(1);
  TV.clear_screen();
  draw_cube();
}

void zrotate(float q) {
  float tx,ty,temp;
  for(byte i = 0; i < 8; i++) {
    tx = cube3d[i][0] - xOff;
    ty = cube3d[i][1] - yOff;
    temp = tx * cos(q) - ty * sin(q);
    ty = tx * sin(q) + ty * cos(q);
    tx = temp;
    cube3d[i][0] = tx + xOff;
    cube3d[i][1] = ty + yOff;
  }
}

void yrotate(float q) {
  float tx,tz,temp;
  for(byte i = 0; i < 8; i++) {
    tx = cube3d[i][0] - xOff;
    tz = cube3d[i][2] - zOff;
    temp = tz * cos(q) - tx * sin(q);
    tx = tz * sin(q) + tx * cos(q);
    tz = temp;
    cube3d[i][0] = tx + xOff;
    cube3d[i][2] = tz + zOff;
  }
}

void xrotate(float q) {
  float ty,tz,temp;
  for(byte i = 0; i < 8; i++) {
    ty = cube3d[i][1] - yOff;
    tz = cube3d[i][2] - zOff;
    temp = ty * cos(q) - tz * sin(q);
    tz = ty * sin(q) + tz * cos(q);
    ty = temp;
    cube3d[i][1] = ty + yOff;
    cube3d[i][2] = tz + zOff;
  }
}


void setup() {
  TV.begin(SC_224x108);
  TV.select_font(font6x8);

  intro();
  
  // Set cursor to the far left (X = 0)
  TV.set_cursor(40, 20);
  TV.println("I am the TVout");
  TV.println("library running on a freeduino\n");
  TV.delay(2500);

  TV.set_cursor(40, 20);
  TV.println("I generate a PAL");
  TV.println("or NTSC composite video using");
  TV.println("interrupts\n");
  TV.delay(2500);
  
  TV.clear_screen();
  TV.set_cursor(40, 10);
  TV.println("My schematic:");
  TV.delay(1500);
  
  TV.bitmap(40, 20, schematic);
  TV.delay(10000);
  
  TV.clear_screen();
  TV.set_cursor(40, 20);
  TV.println("Lets see what\nwhat I can do");
  TV.delay(2000);

  // Fonts demonstration aligned to the left (X = 0)
  TV.clear_screen();
  TV.println(40, 20, "Multiple fonts:");
  
  TV.select_font(font4x6);
  TV.println(40, 30, "4x6 font FONT");
  
  TV.select_font(font6x8);
  TV.println(40, 40, "6x8 font FONT");
  
  TV.select_font(font8x8);
  TV.println(40, 52, "8x8 font FONT");
  
  TV.select_font(font6x8);
  TV.delay(2000);

  TV.clear_screen();
  // Print aligned left (X = 0 instead of X = 16/28)
  TV.print(40, 40, "Random Cube");
  TV.print(40, 48, "Rotation");
  TV.delay(2000);

  randomSeed(analogRead(0));
}

void loop() {
  int rsteps = random(10,60);
  switch(random(6)) {
    case 0:
      for (int i = 0; i < rsteps; i++) {
        zrotate(angle);
        printcube();
      }
      break;
    case 1:
      for (int i = 0; i < rsteps; i++) {
        zrotate(2*PI - angle);
        printcube();
      }
      break;
    case 2:
      for (int i = 0; i < rsteps; i++) {
        xrotate(angle);
        printcube();
      }
      break;
    case 3:
      for (int i = 0; i < rsteps; i++) {
        xrotate(2*PI - angle);
        printcube();
      }
      break;
    case 4:
      for (int i = 0; i < rsteps; i++) {
        yrotate(angle);
        printcube();
      }
      break;
    case 5:
      for (int i = 0; i < rsteps; i++) {
        yrotate(2*PI - angle);
        printcube();
      }
      break;
  }
}





/*
#include <Arduino.h>



void setup() {
  // PA1 is TIM2 Channel 2
  pinMode(PA1, OUTPUT);

  // Initialize TIM2 using STM32Duino HardwareTimer API
  HardwareTimer *MyTim = new HardwareTimer(TIM2);

  // Set PWM mode on Channel 2 (PA1)
  MyTim->setMode(2, TIMER_OUTPUT_COMPARE_PWM1, PA1);
  
  // Set frequency to 15734 Hz
  MyTim->setOverflow(15734, HERTZ_FORMAT);
  
  // Set 50% duty cycle
  MyTim->setCaptureCompare(2, 50, PERCENT_COMPARE_FORMAT);
  
  // Start the timer
  MyTim->resume();
}

void loop() {
}
*/