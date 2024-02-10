#include "uart.h"
#include <libopencm3/stm32/usart.h>

DwReqPkt_t ReadDwReqPkt() {
  uint8_t i=0;
  DwReqPkt_t pkt;
  pkt.totalSize = 0;
  uint16_t temp;
  while (i<3)
  {
  if((MMIO32((USART1_BASE) + 0x0) & (1 << 5))) {
    temp=MMIO32((USART1_BASE) + 0x04)& 0x1FF;
    if(i==0){
      pkt.pktype = temp;
    } else {
      pkt.totalSize = temp << (i-1)*8 | pkt.totalSize;
    }
    i++;
    }
    }
    return pkt;
}

DataPkt_t ReadDataPkt() {
  uint8_t i=0;
  DataPkt_t pkt;
  uint16_t temp;
  while (i<11)
  {
  if((MMIO32((USART1_BASE) + 0x0) & (1 << 5))) {
    temp=MMIO32((USART1_BASE) + 0x04)& 0x1FF;
    if(i==0){
      pkt.pktype = temp;
      } else if (i==10) {
        pkt.crc = temp;
      } else if (i==1) {
        pkt.dlc = temp;
      } else {
        pkt.data[i-2] = temp;
      }
      i++;
      }
    }
    return pkt;
}

ReqRespPkt_t ReadReqRespPkt() {
  uint8_t i=0;
  ReqRespPkt_t pkt;
  uint16_t temp;
  while (i<3)
  {
  if((MMIO32((USART1_BASE) + 0x0) & (1 << 5))) {
    temp=MMIO32((USART1_BASE) + 0x04)& 0x1FF;
    if(i==0){
      //pkt.pktype = temp;
    } else if (i==2) {
      //pkt.crc = temp;
      if(temp==0x71)
      pkt.pktype=dwReqPkt;
    } else if (i==1) {
      //pkt.dlc = temp;
    }
    i++;
    }
    }
    return pkt;
}

bool writeDataPkt(DataPkt_t *pkt) {
    usart_send_blocking(USART1_BASE,pkt->pktype);
    usart_send_blocking(USART1_BASE,pkt->dlc);
    for(uint8_t i=0;i<8;i++) {
      usart_send_blocking(USART1_BASE,pkt->data[i]);
    }
    usart_send_blocking(USART1_BASE,pkt->crc);
    return 1;
}

bool writeReqRespPkt(ReqRespPkt_t *pkt) {
    MMIO32((USART1_BASE) + 0x4) = (pkt->pktype & 0x1FF);
    while (!(MMIO32((USART1_BASE) + 0x0) & (1 << 6)));
    return 1;
}