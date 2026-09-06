

#include <TNTSC.h>
#include <SPI.h>
#include <stm32f1xx_hal.h>
#include <stm32f1xx_hal_dma.h>
#include <stm32f1xx_hal_spi.h>
#include <stm32f1xx_hal_rcc.h>
#include <stm32f1xx_hal_gpio.h>
#include <stm32f1xx_hal_tim.h>
#include "stm32f103xb.h"
#include <HardwareTimer.h>

// Instantiate Timer2 using hardware peripheral TIM2
HardwareTimer *Timer2 = new HardwareTimer(TIM2);


#define gpio_write(pin,val) gpio_write_bit(PIN_MAP[pin].gpio_device, PIN_MAP[pin].gpio_bit, val)

#define PWM_CLK PA1         // Synchronization signal output pin (PWM)
#define DAT PA7             // Image signal output pin
#define NTSC_S_TOP 3        // Vertical synchronization start line
#define NTSC_S_END 5        // Vertical synchronization end line
#define NTSC_VTOP 30        // Image display start line
#define IRQ_PRIORITY  2     // Timer interrupt priority
// Direct channel register definitions for STM32F1
#define MYSPI1_DMA_CH DMA1_Channel3 // SPI1_TX default DMA channel on STM32F1
#define MYSPI2_DMA_CH DMA1_Channel5 // SPI2_TX default DMA channel on STM32F1// SPI2用DMAチャンネル
#define MYSPI_DMA DMA1        // SPI / DMA

// Parameter settings by screen resolution
typedef struct  {
  uint16_t width;   // Screen width in pixels
  uint16_t height;  // Screen height in pixels
  uint16_t ntscH;   // NTSC screen height in pixels
  uint16_t hsize;   // Horizontal byte count
  uint8_t  flgHalf; // Vertical scan line count (0: normal 1: half)
  uint32_t spiDiv;  // SPI clock division
} SCREEN_SETUP;




#define NTSC_TIMER_DIV 3 // System clock division 1/3
const SCREEN_SETUP screen_type[]  {
  { 112, 108, 216, 14, 1, SPI_CLOCK_DIV32 }, // 112x108
  { 224, 108, 216, 28, 1, SPI_CLOCK_DIV16 }, // 224x108 
  { 224, 216, 216, 28, 0, SPI_CLOCK_DIV16 }, // 224x216
  { 448, 108, 216, 56, 1, SPI_CLOCK_DIV8  }, // 448x108 
  { 448, 216, 216, 56, 0, SPI_CLOCK_DIV8  }, // 448x216 
};


#define NTSC_LINE (262+0)                     // Screen line count (for some monitors, 2 lines added)
#define SYNC(V)  gpio_write(PWM_CLK,V)        // Synchronization signal output (PWM)
static uint8_t* vram;                         // Video display frame buffer
static volatile uint8_t* ptr;                 // Pointer for referencing video display frame buffer
static volatile int count=1;                  // Variable for counting scan lines

static void (*_bktmStartHook)() = NULL;       // Block ranking period start hook
static void (*_bktmEndHook)()  = NULL;        // Block ranking period end hook

static uint8_t  _screen;
static uint16_t _width;
static uint16_t _height;
static uint16_t _ntscHeight;
static uint16_t _vram_size;
static uint16_t _ntsc_line = NTSC_LINE;
static uint16_t _ntsc_adjust = 0;
static uint16_t _hAdjust = 0; // Horizontal display position correction
static uint16_t _vAdjust = 0; // Vertical display position correction
static uint8_t  _spino = 1;
//static dma_channel  _spi_dma_ch = MYSPI1_DMA_CH;
//static dma_dev* _spi_dma    = MYSPI_DMA;

// Direct peripheral pointer replacements
//static DMA_Channel_TypeDef* _spi_dma_ch = DMA1_Channel3; // Replace MYSPI1_DMA_CH with actual channel handle, e.g., DMA1_Channel3
//static DMA_TypeDef*         _spi_dma    = DMA1;          // Replace MYSPI_DMA with DMA controller pointer, e.g., DMA1
//DMA_HandleTypeDef hdma_spi1_tx;
//SPI_HandleTypeDef hspi1;
static SPIClass* pSPI;

uint16_t TNTSC_class::width()  {return _width;;} ;
uint16_t TNTSC_class::height() {return _height;} ;
uint16_t TNTSC_class::vram_size() { return _vram_size;};
uint16_t TNTSC_class::screen() { return _screen;};


 // Block ranking period start hook setting
void TNTSC_class::setBktmStartHook(void (*func)()) {
  _bktmStartHook = func;
}

// Block ranking period end hook setting
void TNTSC_class::setBktmEndHook(void (*func)()) {
  _bktmEndHook = func;
}

// DMA Interrupt Handler (Clears data output after SPI transmission finishes)
void TNTSC_class::DMA1_CH3_handle() {
  // Wait until SPI bus is no longer busy
 // while (SPI1->SR & SPI_SR_BSY);
  
  // Clear SPI Data Register
 // SPI1->DR = 0;
 while (SPI1->SR & SPI_SR_BSY)
    {
    }

    SPI1->CR2 &= ~SPI_CR2_TXDMAEN;
}



// Low-level high-performance DMA Data Send (Matching legacy libmaple speed)
void TNTSC_class::SPI_dmaSend(uint8_t *transmitBuf, uint16_t length) {
  // Disable DMA Channel 3
  DMA1_Channel3->CCR &= ~DMA_CCR_EN;

  // Clear all flags for DMA1 Channel 3
  DMA1->IFCR = DMA_IFCR_CGIF3 | DMA_IFCR_CTCIF3 | DMA_IFCR_CHTIF3 | DMA_IFCR_CTEIF3;

  // Set source and destination registers
  DMA1_Channel3->CPAR = (uint32_t)&(SPI1->DR);
  DMA1_Channel3->CMAR = (uint32_t)transmitBuf;
  DMA1_Channel3->CNDTR = length;

  // Configure: Memory to peripheral, 8-bit size, memory increment, TC interrupt, High Priority
  DMA1_Channel3->CCR = DMA_CCR_DIR | 
                       DMA_CCR_MINC | 
                       DMA_CCR_TCIE | 
                       DMA_CCR_PL_1;

  // Enable DMA channel
  DMA1_Channel3->CCR |= DMA_CCR_EN;

  // Trigger SPI TX DMA request
  SPI1->CR2 |= SPI_CR2_TXDMAEN;
}

// Video data display (Raster output)
void TNTSC_class::handle_vout() {
  if (count >=NTSC_VTOP+_vAdjust && count <=_ntscHeight+NTSC_VTOP+_vAdjust-1) {  	

    SPI_dmaSend((uint8_t *)ptr, screen_type[_screen].hsize);
  	//pSPI->dmaSend((uint8_t *)ptr, screen_type[_screen].hsize,1);
  	if (screen_type[_screen].flgHalf) {
      if ((count-NTSC_VTOP) & 1) 
      ptr+= screen_type[_screen].hsize;
    } else {
      ptr+=screen_type[_screen].hsize;
    }
  }
	
 // Next scanline sync pulse width configuration (Direct CMSIS Registers)
if (count >= NTSC_S_TOP - 1 && count <= NTSC_S_END - 1) {
  // Vertical Sync Pulse (Change PWM Pulse Width)
  TIM2->CCR2 = 1412;
} else {
  // Horizontal Sync Pulse (Change PWM Pulse Width)
  TIM2->CCR2 = 112;
}

   count++; 
  if( count > _ntsc_line ){
    count=1;
    ptr = vram;    
  } 

}

void TNTSC_class::adjust(int16_t cnt, int16_t hcnt, int16_t vcnt) {
  _ntsc_adjust = cnt;
  _ntsc_line = NTSC_LINE+cnt;
  _hAdjust = hcnt;
  _vAdjust = vcnt;
}
	
void TNTSC_class::begin(uint8_t mode, uint8_t spino, uint8_t* extram)
{
    // ============================================================
    // Video / VRAM configuration
    // ============================================================

    _screen = (mode <= 4) ? mode : SC_DEFAULT;

    _width      = screen_type[_screen].width;
    _height     = screen_type[_screen].height;
    _vram_size  = screen_type[_screen].hsize * _height;
    _ntscHeight = screen_type[_screen].ntscH;

    _spino = spino;

    flgExtVram = false;

    if (extram) {
        vram = extram;
        flgExtVram = true;
    }
    else {
        vram = (uint8_t*)malloc(_vram_size);
    }

    if (!vram) {
        return;
    }

    cls();

    ptr = vram;
    count = 1;


    // ============================================================
    // SPI1
    //
    // PA7 = SPI1 MOSI
    // SPI1 is used as the video data generator.
    // ============================================================

    pSPI = &SPI;

    // Enable SPI1 peripheral clock
    __HAL_RCC_SPI1_CLK_ENABLE();

    // Enable GPIOA clock
    __HAL_RCC_GPIOA_CLK_ENABLE();


    // ------------------------------------------------------------
    // PA7 = SPI1 MOSI
    //
    // Alternate-function push-pull
    // ------------------------------------------------------------

    GPIO_InitTypeDef GPIO_InitStruct = {};

    GPIO_InitStruct.Pin   = GPIO_PIN_7;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


    // ============================================================
    // Configure SPI1
    // ============================================================

   // Reset CR1
SPI1->CR1 = 0;
SPI1->CR2 = 0;

// Set Master mode, Software Slave Management
SPI1->CR1 |= SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI;

// Set SPI Mode 0 (CPOL=0, CPHA=0) so MOSI rests LOW when idle
SPI1->CR1 &= ~(SPI_CR1_CPOL | SPI_CR1_CPHA);

// Configure prescaler according to screen mode
SPI1->CR1 &= ~SPI_CR1_BR;
switch (_screen) {
    case 0: SPI1->CR1 |= SPI_CR1_BR_2; break;                 // DIV32
    case 1:
    case 2: SPI1->CR1 |= SPI_CR1_BR_1 | SPI_CR1_BR_0; break;   // DIV16
    case 3:
    case 4: SPI1->CR1 |= SPI_CR1_BR_1; break;                 // DIV8
    default: SPI1->CR1 |= SPI_CR1_BR_1 | SPI_CR1_BR_0; break;
}

// Enable SPI peripheral
SPI1->CR1 |= SPI_CR1_SPE;

    // ============================================================
    // DMA1
    //
    // SPI1_TX = DMA1 Channel 3 on STM32F103
    // ============================================================

    __HAL_RCC_DMA1_CLK_ENABLE();

    // Disable channel before configuration
    DMA1_Channel3->CCR &= ~DMA_CCR_EN;

    // Clear all Channel 3 flags
    DMA1->IFCR =
        DMA_IFCR_CGIF3  |
        DMA_IFCR_CTCIF3 |
        DMA_IFCR_CHTIF3 |
        DMA_IFCR_CTEIF3;


    // ------------------------------------------------------------
    // DMA configuration
    // ------------------------------------------------------------

    DMA1_Channel3->CCR = 0;

    DMA1_Channel3->CCR |= DMA_CCR_DIR;     // Memory -> peripheral
    DMA1_Channel3->CCR |= DMA_CCR_MINC;    // Increment memory
    DMA1_Channel3->CCR |= DMA_CCR_TCIE;    // Transfer complete IRQ

    // High priority
    DMA1_Channel3->CCR |= DMA_CCR_PL_1;


    // ------------------------------------------------------------
    // DMA interrupt
    // ------------------------------------------------------------

    HAL_NVIC_SetPriority(
        DMA1_Channel3_IRQn,
        IRQ_PRIORITY,
        1
    );

    HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);


    // ------------------------------------------------------------
    // SPI1 requests DMA when TX buffer is ready
    // ------------------------------------------------------------

    SPI1->CR2 |= SPI_CR2_TXDMAEN;


    // ============================================================
    // ENABLE SPI1
    // ============================================================

    SPI1->CR1 |= SPI_CR1_SPE;


    // ============================================================
    // TIM2
    //
    // NTSC horizontal period
    //
    // Timer clock intended = 24 MHz
    //
    // 1 tick = 41.667 ns
    //
    // ARR = 1523
    //
    // 1524 ticks × 41.667 ns
    //       = 63.5 us
    // ============================================================

    Timer2->pause();

    Timer2->setPrescaleFactor(NTSC_TIMER_DIV);

    Timer2->setOverflow(
        1524,
        TICK_FORMAT
    );


    // ============================================================
    // TIM2 CH2
    //
    // PA1 = horizontal sync
    //
    // 112 ticks × 41.667 ns
    //       = 4.667 us
    // ============================================================

    Timer2->setMode(
        2,
        TIMER_OUTPUT_COMPARE_PWM1,
        PWM_CLK
    );

    // Active LOW
    TIM2->CCER |= TIM_CCER_CC2P;

    Timer2->setCaptureCompare(
        2,
        112,
        TICK_COMPARE_FORMAT
    );


    // ============================================================
    // TIM2 CH1
    //
    // Start video output approximately 9.4 us into line.
    // ============================================================

    Timer2->setMode(
        1,
        TIMER_OUTPUT_COMPARE
    );

    Timer2->setCaptureCompare(
        1,
        225 - 60 + _hAdjust,
        TICK_COMPARE_FORMAT
    );

    Timer2->attachInterrupt(
        1,
        handle_vout
    );


    // ============================================================
    // Start TIM2
    // ============================================================

    Timer2->setCount(0);

    Timer2->refresh();

    Timer2->resume();
}


// Terminate NTSC Video Output
void TNTSC_class::end() {
  // 1. Pause timer and detach interrupt
  Timer2->pause();
  Timer2->detachInterrupt(1);

  // 2. Disable SPI Tx DMA Requests
  SPI1->CR2 &= ~SPI_CR2_TXDMAEN;  // Equivalent to spi_tx_dma_disable()

  // 3. Disable DMA channel and disable NVIC interrupt
  DMA1_Channel3->CCR &= ~DMA_CCR_EN;
  HAL_NVIC_DisableIRQ(DMA1_Channel3_IRQn); // Equivalent to dma_detach_interrupt()

  // 4. End SPI peripheral
  if (pSPI != nullptr) {
    pSPI->end();
  }

  // 5. Free dynamically allocated VRAM if internal
  if (!flgExtVram && vram != nullptr) {
    free(vram);
    vram = nullptr;
  }

  // 6. Delete dynamically created SPI instance
  if (_spino == 2 && pSPI != nullptr) {
    delete pSPI;
    pSPI = nullptr;
  } 
}

// Get VRAM address
uint8_t* TNTSC_class::VRAM() {
  return vram;  
}

// Clear screen
void TNTSC_class::cls() {
  memset(vram, 0, _vram_size);
}

// Frame delay
void TNTSC_class::delay_frame(uint16_t x) {
  while (x) {
    while (count != _ntscHeight + NTSC_VTOP);
    while (count == _ntscHeight + NTSC_VTOP);
    x--;
  }
}

	// Override System Clock setup to force 72 MHz from 8 MHz HSE
extern "C" void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9; // 8MHz * 9 = 72MHz
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}


extern "C" void DMA1_Channel3_IRQHandler(void) {
  // Clear DMA Transfer Complete flag
  if (DMA1->ISR & DMA_ISR_TCIF3) {
    DMA1->IFCR = DMA_IFCR_CTCIF3;
  }
  
  // Wait for SPI to finish last bit transfer
  while (SPI1->SR & SPI_SR_BSY);

  // Disable DMA request to release line
  SPI1->CR2 &= ~SPI_CR2_TXDMAEN;
}

TNTSC_class TNTSC;

