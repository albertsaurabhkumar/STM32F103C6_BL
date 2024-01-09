/*
**  Processor: S32K144
**  Author: Saurabh Kumar
**  GCC Version: 12.3
**  
**  Main program file for the GCC Compiler
**    // flash_unlock();
  // uint32_t addre=0x8001FF0;
  // uint16_t dat=0xA5A5;
  // for(uint32_t i=0;i<1024;i++) {
  // flash_program_half_word(addre, dat);
  // addre+=2;
  // }
  // flash_erase_page((uint32_t)0x08002010);
  // flash_erase_all_pages();
  // flash_lock();
*/

#include "main.h"

#define APP_ADD 0x08002004
#define VectorAdd 0x08002000
#define NEW_MSP 0x20002800

typedef void (*voidFunc)(void);
volatile uint32_t dlyCnt;
volatile uint32_t counter;
volatile uint32_t* MSR= (uint32_t*)(CAN1+0x004);
volatile uint32_t* ESR= (uint32_t*)(CAN1+0x018);
volatile uint32_t* FMR= (uint32_t*)(CAN1+0x200);
volatile uint32_t* BTR= (uint32_t*)(CAN1+0x01C);
volatile uint32_t* FIFO0= (uint32_t*)(CAN1+0x00C);
volatile uint32_t* FIFO1= (uint32_t*)(CAN1+0x010);
volatile uint32_t* TSR= (uint32_t*)(CAN1+0x08);
volatile uint32_t* RF1R= (uint32_t*)(CAN1+0x1B0);
volatile uint32_t* FILTERReg= (uint32_t*)(CAN1+ 0x21C);

volatile uint32_t* GPIOAODR= (uint32_t*)(GPIO_PORT_A_BASE+0x0c);

volatile uint32_t* RCCregis= (uint32_t*)(RCC_BASE+0x018);
volatile uint32_t* AFIORE= (uint32_t*)(AFIO_BASE+0x004);
state_machin_t currstate;

__attribute__ (( always_inline )) inline void __set_MSP(uint32_t topOfMainStack)
{
  __asm__ volatile ("MSR msp, %0\n" : : "r" (topOfMainStack));
}

void delay(uint32_t a) {
  while(a>0){
    a--;
    dlyCnt=a;
  }
}

void JumpToApp() {
  uint32_t* jumpAddress = (uint32_t*)(APP_ADD);
  uint32_t* jumpAdd = (uint32_t*)(*jumpAddress);
  voidFunc jmpFn = (voidFunc)jumpAdd;
  SCB_VTOR = (uint32_t)(VectorAdd);
  __set_MSP(NEW_MSP);
  jmpFn();
}


int main(void) {
  canTxStruct_t canTxPckt;
  canRxStruct_t canRxPckt;
  currstate = SendAck;
  uint8_t data[8]={1,2,3,4,5,6,7,8};

  rcc_periph_clock_enable(RCC_GPIOC);                                       /* intialize the clock for the gpio PC13 */ 
	rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_AFIO);
  rcc_periph_clock_enable(RCC_CAN);
  

  while(can_init(CAN1,false,false,false,false,false,false,1,CAN_BTR_TS1_7TQ,CAN_BTR_TS2_8TQ,1,false,false)==1);
  gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);   /* Set the mode as output of above GPIO */
  
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_CAN_TX);   /* Set the mode as output of above GPIO */
  gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO_CAN_RX);   /* Set the mode as output of above GPIO */
  
  uint32_t temp = GPIO_ODR(GPIOA);
  GPIO_ODR(GPIOA) = temp | GPIO_CAN_RX;
    GPIO_ODR(GPIOA) = GPIO_CAN_RX;

  //CAN_FMR(CAN1) &= ~CAN_FMR_FINIT;
  can_filter_init(0,true,false,0,0,0,1);

  while(1)
  {

  switch(currstate) {
    case ReceiveMsg:
    {
      if((CAN_RF0R(CAN1)&CAN_RF0R_FMP0_MASK)||(CAN_RF1R(CAN1)&CAN_RF0R_FMP0_MASK)) {        
      canReceive(&canRxPckt,1);        
      }
      currstate = SendAck;
    } break;
    
    case SendAck:
    {
      initCanPck(&canTxPckt, 0x470, 0, &data, 8);
      canTransmit(&canTxPckt);

      currstate = ReceiveMsg;
    } break;

    case SendMsg:
    {

    } break;
    
    case WaitforAck:
    {

    } break;
    
    case BlinkLed:
    {

    } break;
  
    default:
    {
      // Do Nothing
    } break;
  }

    gpio_toggle(GPIOC, GPIO13);    /* Toggle the gpio */
    for(uint8_t temp = 0;temp<10;temp++)
    {
    delay(10000);
    }
    //JumpToApp();           // Never returns from here
  }
  return 0;
}