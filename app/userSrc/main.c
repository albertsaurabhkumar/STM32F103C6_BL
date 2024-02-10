/*
**  Processor: S32K144
**  Author: Saurabh Kumar
**  GCC Version: 12.3
**  
**  Main program file for the GCC Compiler
*/

/************************ Include Header Files **************************/
#include "main.h"

/************************ Start the Global Variables **************************/
#define APP_ADD 0x08006004
#define VectorAdd 0x08006000
#define NEW_MSP 0x20002800

typedef void (*voidFunc)(void);
volatile uint32_t dlyCnt;
volatile uint32_t counter;
uint32_t timeoutVar;
uint16_t count=0;
uint16_t pendingData;

bool validAppAvailble;
uint32_t i=0;
uint8_t tempdata[1024];
state_machin_t currstate;

/************************ End the Global Variables **************************/

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

void canInit() {
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_AFIO);
  rcc_periph_clock_enable(RCC_CAN);
  
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_CAN_TX);   /* Set the mode as output of above GPIO */
  gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO_CAN_RX);   /* Set the mode as output of above GPIO */
  
  while(can_init(CAN1,false,false,false,false,false,false,1,CAN_BTR_TS1_7TQ,CAN_BTR_TS2_8TQ,1,false,false)==1);
  GPIO_ODR(GPIOA) = GPIO_ODR(GPIOA)|GPIO_CAN_RX;

  can_filter_init(0,true,false,0,0,0,1);
}

void uartSetup(){
  rcc_periph_clock_enable(RCC_USART1);
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,GPIO_USART1_TX);
  gpio_set_mode(GPIOA, GPIO_MODE_INPUT,GPIO_CNF_INPUT_PULL_UPDOWN, GPIO_USART1_RX);
  GPIO_ODR(GPIOA) = GPIO_ODR(GPIOA)|GPIO_USART1_RX;

  usart_set_baudrate(USART1_BASE , 9600);
  usart_set_databits(USART1_BASE , 8);
  usart_set_mode(USART1_BASE , USART_MODE_TX_RX);
  //usart_enable_rx_interrupt(USART1_BASE);
  usart_enable(USART1_BASE);
  //nvic_enable_irq(37);

}

void erase_application(uint32_t appAddress) {
  for(uint32_t i=appAddress;i<0x8008000;i+=0x400){
    flash_unlock();
    flash_erase_page(i);
    flash_lock();
  }
}

int main(void) {
  __asm__ volatile ("CPSIE I\n"); /* Enable the GLOBAL interrupts */
  DwReqPkt_t DrequestPkt;
  ReqRespPkt_t tempPkt;
  DataPkt_t tempDataPkt;
  //canTxStruct_t canTxPckt;
  //canRxStruct_t canRxPckt;
  currstate = DownloadReq;
  validAppAvailble = (*(uint32_t*)APP_ADD) != 0xFFFFFFFF;
  //uint8_t data[8]={1,2,3,4,5,6,7,8};

  rcc_periph_clock_enable(RCC_GPIOC);                                       /* intialize the clock for the gpio PC13 */ 
	rcc_periph_clock_enable(RCC_GPIOA);
  uartSetup();
  gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);   /* Set the mode as output of above GPIO */

  /* Erasing Application because expecting a application flashing */
  /* Erase from 24 -> 08006000 sector to 31 */

  while(1)
  {
    // if(USART_SR(USART1_BASE) & USART_SR_RXNE) {
    //  tempdata[i++] = usart_recv(USART1_BASE);
    //  if(i%31==0){
    //   usart_send(USART1,0x55);
    //  }
    // }

  switch(currstate) {

    case DownloadReq:
    {
    /* Received a download Request */
    if(USART_SR(USART1_BASE) & USART_SR_RXNE) {
      DrequestPkt = ReadDwReqPkt();
      if(DrequestPkt.pktype == dwReqPkt) {
       currstate = Ack;
      }
      }
    } break;
    
    case Ack:
    {
      /* Send the Ack to the User for Download Request */
      tempPkt.pktype = ackPkt;
      currstate = GetData;
      erase_application(VectorAdd);
      writeReqRespPkt(&tempPkt);
    } break;

    case GetData:
    {
      /* Receive the data to be flashed */
      flash_unlock();
      pendingData=DrequestPkt.totalSize;
      while (1)
      {

      if(USART_SR(USART1_BASE) & USART_SR_RXNE) {
      tempDataPkt = ReadDataPkt();
      if(tempDataPkt.pktype == dataPkt) {
        if(pendingData>0){
          for(uint8_t i=0;i<tempDataPkt.dlc;i+=2){
            uint16_t temp=tempDataPkt.data[i]|tempDataPkt.data[i+1]<<8;
            flash_program_half_word(VectorAdd+count,temp);
            count+=2;
          } 
          pendingData-=tempDataPkt.dlc;
          writeReqRespPkt(&tempPkt);
        } else {
          flash_lock();
          break;
        }
        }
      } else if(pendingData == 0) {
        break;
      }
      }
      currstate = TransferDone;
    } break;
    
    case TransferDone:
    {
      /* Tranfer Completed and Send Ack */
      if(timeoutVar==4747) {
        validAppAvailble=0;
        currstate = DownloadReq;
      } else {
      //tempPkt.pktype = ackPkt;
      validAppAvailble = (*(uint32_t*)APP_ADD) != 0xFFFFFFFF;
      }
    } break;
  }
    /* Toggle the LED 10 times */

    // gpio_toggle(GPIOC, GPIO13);
    // delay(1000);
  /* if no uart packet available then jump to the application */
    if(validAppAvailble) {
      JumpToApp();
    }
  }
    return 0;
}


//if((CAN_RF0R(CAN1)&CAN_RF0R_FMP0_MASK)||(CAN_RF1R(CAN1)&CAN_RF0R_FMP0_MASK)) 
//{        
//canReceive(&canRxPckt,1);        
//}
//initCanPck(&canTxPckt, 0x470, 0, &data, 8);
//canTransmit(&canTxPckt);
