
#ifndef __TNTSC_H__
#define __TNTSC_H__

#include <Arduino.h>

// Screen mode definitions for 72MHz clock operation
  #define SC_128x96   0 // 128x96
	#define SC_256x96   1 // 256x96
	#define SC_256x192  2 // 256x192
	#define SC_512x96   3 // 512x96
	#define SC_512x192  4 // 512x192
	#define SC_128x108  5 // 128x108
	#define SC_256x108  6 // 256x108
	#define SC_256x216  7 // 256x216
	#define SC_512x108  8 // 512x108
	#define SC_512x216  9 // 512x216
  #define SC_224x108  //10 // 448x108
  #define SC_DEFAULT SC_256x192


// NTSC video display class definition
class TNTSC_class {    
  private:
	uint8_t flgExtVram; // Use externally allocated memory (0: Not used, 1: Used)
  
  public:
    void begin(uint8_t mode=SC_DEFAULT,uint8_t spino = 1, uint8_t* extram=NULL);  // NTSC video display start
    void end();                            // NTSC video display end
    uint8_t*  VRAM();                      // Get VRAM address
    void cls();                            // Clear the screen
    void delay_frame(uint16_t x);          // Frame delay
	void setBktmStartHook(void (*func)()); // Ranking period start hook setting
    void setBktmEndHook(void (*func)());   // Ranking period end hook setting

    uint16_t width() ;
    uint16_t height() ;
    uint16_t vram_size();
    uint16_t screen();
	  void adjust(int16_t cnt, int16_t hcnt=0, int16_t vcnt=0);
 
  private:
    static void handle_vout();
    static void SPI_dmaSend(uint8_t *transmitBuf, uint16_t length) ;
    static void DMA1_CH3_handle();
};

extern TNTSC_class TNTSC; // Global object usage declaration

#endif
