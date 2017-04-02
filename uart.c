/* Uart.c Uart functions */

#include <stdint.h>
#include <ctype.h>
#include <stdio.h>

#if 0

#include "lpc2103.h"
#include "common.h"

/* #define UART_POLLED  1 */

#define FIFODEPTH  14

#define UARTBUFFERSIZE 128
#define UARTINCR(x) ((x+1) % UARTBUFFERSIZE)

/* Uart access defines */
#define PORT(x)  (x * (U1RBR - U0RBR))
#define ARBR(x)   (U0RBR + PORT(x))
#define ATHR(x)   (U0THR + PORT(x))
#define ADLL(x)   (U0DLL + PORT(x))
#define ADLM(x)   (U0DLM + PORT(x))
#define AIER(x)   (U0IER + PORT(x))
#define AIIR(x)   (U0IIR + PORT(x))
#define AFCR(x)   (U0FCR + PORT(x))
#define ALCR(x)   (U0LCR + PORT(x))
#define ALSR(x)   (U0LSR + PORT(x))
#define ASCR(x)   (U0SCR + PORT(x))
#define AACR(x)   (U0ACR + PORT(x))
#define AFDR(x)   (U0FDR + PORT(x))
#define ATER(x)   (U0TER + PORT(x))


enum UARTState_e {
  IDLE,
  BUSY,
  PRIMED
};

typedef struct _UARTState {
  char      inBuffer[UARTBUFFERSIZE];
  volatile uint16_t  inHead, inTail, inCount;  /* Head and tail indexes */

  char      outBuffer[UARTBUFFERSIZE];
  volatile uint16_t  outHead, outTail, outCount;
  uint8_t   lsr;
  uint8_t   iir;
  enum UARTState_e outState;

} UARTState_t;

/* allocate a couple of UART State structures */
static UARTState_t UARTState[2];

/* Default UART to use for puts and friends. */
static uint32_t Stdio=0;
#ifdef USE_2PORTS
static uint32_t Stdio2=1;
#endif

/* Read the line status register, setting any flags as appropriate */
static uint8_t ReadLSR(uint32_t port)
{
  uint8_t lsr;

  lsr = *ALSR(port);
  
  /* Make any bits read sticky in the shadow of this register */
  UARTState[port].lsr |= lsr;

  return lsr;
}

void UARTInt(uint32_t port)
{
#if !defined(UART_POLLED)
  uint32_t i;
  uint8_t iir,c;
  uint16_t inHead,outTail;

 again:
  /* Rx or Tx or other? */
  iir = *AIIR(port);
  
  if(IPEND_R(iir)) return; /* Bail out if there are no pending interrupts */

  switch(IID_R(iir)) {
  case 3:  /* Receive line status */
    /* Read the status register, to clear the interrupt */
    ReadLSR(port);
    goto again; 
    break;
  case 2:  /* Receive data available */
    for(i=0; i<14; i++) {
      if(UARTState[port].inCount < UARTBUFFERSIZE) {
	/* Read the characters out one at a time, and stuff them */
	inHead = UARTState[port].inHead;
	UARTState[port].inBuffer[inHead]=*ARBR(port);
	UARTState[port].inHead = UARTINCR(inHead);
	UARTState[port].inCount++;
      } else {
	/* Overflow! */
	c = *ARBR(port);
      }
    }
  case 6:  /* Character Timeout indicator */
    
    /* While there are characters to read.. */
    while(RDR_R(ReadLSR(port))) {
      /* Is there space in the buffer ? */
      if(UARTState[port].inCount < UARTBUFFERSIZE) {
	/* Read the characters out one at a time, and stuff them */
	inHead = UARTState[port].inHead;
	UARTState[port].inBuffer[inHead]=*ARBR(port);
	UARTState[port].inHead = UARTINCR(inHead);
	UARTState[port].inCount++;
      } else {
	/* Overflow! */
      }
    }
    goto again;
    break;
  case 1:  /* Transmit holding register empty */
    /* This means that we can stuff at least 16 bytes in there if we
       have them to send. */
    if((UARTState[port].outCount)==0) {
      /* We got here and there are no characters left to send... */
      UARTState[port].outState = IDLE;
    } else {
      for(i=0;
	  ((i<16) && (UARTState[port].outCount != 0));
	  i++) {
	outTail = UARTState[port].outTail;
	*ATHR(port) = UARTState[port].outBuffer[outTail];  /* Send it */
	UARTState[port].outTail = UARTINCR(outTail);
	UARTState[port].outCount--;
      }
      UARTState[port].outState = BUSY;
    }
    goto again;
    break;
  default: /* Reserved code */
    UARTState[port].iir = iir;
  }
  
#endif
}

/* Libc emulation */
int puts(const char *buf) 
{
  UARTWriteString(buf,Stdio);
  UARTWriteCooked('\n',Stdio);
#ifdef USE_2PORTS
  UARTWriteString(buf,Stdio2);
  UARTWriteCooked('\n',Stdio2);
#endif
  return 0;
}

#undef putchar
int putchar(int c)
{
  UARTWriteCooked(c,Stdio);

#ifdef USE_2PORTS
  UARTWriteCooked(c,Stdio2);
#endif

  return c;
}

/* Read a single character from the UART */
char UARTRead(uint32_t port)
{
  char c;
  uint32_t i;

#if !defined(UART_POLLED)
  uint16_t tail;
  /* Wait for a character to be ready */
  while(UARTState[port].inCount == 0) {
    for(i=0;i<1000;i++) { asm("nop\n");};
  }

  /* There's at least one character, fetch it */
  /* Crticial section begin */
  IntCoreDisable();
  tail = UARTState[port].inTail;
  c    = UARTState[port].inBuffer[tail];
  UARTState[port].inTail = UARTINCR(tail);
  UARTState[port].inCount--;

  /* Critical section end */
  IntCoreEnable();

  return c;
#else
  /* Spin waiting for a character */
  while(!(RDR_R(ReadLSR(port)))) {
    for(i=0;i<1000;i++) { asm("nop\n");};
  }
  UARTReadNonBlock(port, &c);
  return c;
#endif
}

/* Read a single character from the UART, don't block, return 0 if
   there's a character. */
int UARTReadNonBlock(uint32_t port, char *c)
{
#if !defined(UART_POLLED)
  uint16_t tail;

  /* Check for a character to be ready */
  if(UARTState[port].inCount == 0) {
    /* Nope, just return */
    return 1;
  }

  /* There's at least one character, fetch it */
  /* Crticial section begin */
  IntCoreDisable();
  tail = UARTState[port].inTail;
  *c   = UARTState[port].inBuffer[tail];
  UARTState[port].inTail = UARTINCR(tail);
  UARTState[port].inCount--;

  /* Critical section end */
  IntCoreEnable();

  return 0;
#else
  if((RDR_R(ReadLSR(port)))) {
    /* Something to read */
    *c = *ARBR(port);
    return 0;
  } else {
    /* nothing to read */
    return 1;
  }
#endif
}

/* Output a single character to the UART */
void UARTWrite(char c,uint32_t port)
{
#if !defined(UART_POLLED)
  uint16_t head;

  /* Crticial section begin */
  IntCoreDisable();
  
  /* Check to see if port is busy... */
  if(UARTState[port].outState == IDLE) {
    /* Port is idle, dump the character and indicate priming */
    UARTState[port].outState = PRIMED;
    *ATHR(port) = c;
  } else {
    /* Port is busy, or already primed */
    /* Check for overflow */
    if(UARTState[port].outCount >= (UARTBUFFERSIZE-1)) {
      /* buffer completely full! */
      IntCoreEnable();
      /* Spin until there is at least one place..*/
      while(UARTState[port].outCount >= (UARTBUFFERSIZE-1)) {
	/*for(i=0;i<1000;i++) { asm("nop\n");}; */
      }
      IntCoreDisable();
    }
    /* put the character on the output buffer */
    head = UARTState[port].outHead;
    UARTState[port].outBuffer[head] = c;
    UARTState[port].outHead = UARTINCR(head);
    UARTState[port].outCount++;
  }
  /* Critical section end */
  IntCoreEnable();
#else
  uint32_t i;
  /* Wait for space in the FIFO */
  while(!(THRE_R(ReadLSR(port)))) {
    for(i=0;i<1000;i++) { asm("nop\n");};
  }
  /* Send the character */
  *ATHR(port) = c;
#endif
}

/* Output a single character to the UART, well done */
void UARTWriteCooked(char c, uint32_t port)
{
  if(c == '\n') {
    UARTWrite('\n',port);
    UARTWrite('\r',port);
  } else {
    UARTWrite(c,port);
  }
}

int UARTReadString(char *buf, uint32_t len, uint32_t port)
{
  char c;
  unsigned int count;
  
  count = 0;
  c = '\0';
  while((count < len) && (c != '\r')) {
    c = UARTRead(port);
    /*printf("Got:'%c'\n",c);*/
    /* Check for simple line control characters */
    if((c == 010) && count) {
      /* User pressed backspace */
      UARTWriteString("\010 \010",port); /* Obliterate character */
      buf--;     /* Then remove it from the buffer */
      count--;   /* Then keep track of how many are left */
      continue;  /* Stop thinking about things */
    }

    /* We are only going to save printable characters */
    if(isprint((uint32_t)c)) {
      *buf++ = c;
      count++;
      /* Echo it back to the user */
      UARTWrite(c,port);
    }
  }
 
  /* NULL Terminate anything we have received */
  *buf = 0;
  return count;
}

/* Dump a string to the UART */
void UARTWriteString(const char *buf, uint32_t port)
{
  while(*buf) {
    UARTWriteCooked(*buf,port);
    buf++;
  }      
}

#define DIVISOR(x) ((PCLK_FREQ)/(x)/16)
static void UARTInitInt(uint32_t port, uint32_t baud)
{
  uint32_t divisor;
  /* Clean out the descriptors */
  UARTState[port].inHead   = 0;
  UARTState[port].inTail   = 0;
  UARTState[port].inCount  = 0;

  UARTState[port].outHead  = 0;
  UARTState[port].outTail  = 0;
  UARTState[port].outCount = 0;
  UARTState[port].lsr      = 0;
  UARTState[port].outState = IDLE;

  /* Program divisor register to get desired baud rate */
  *ALCR(port) |= DLAB(1);
  divisor = DIVISOR(baud);
  *ADLM(port) = (uint8_t)(divisor>>8);
  *ADLL(port) = (uint8_t)(divisor);

  *ALCR(port) = ( DLAB(0)
		 | BREAK_CONTROL(0)
		 | PARITY_SELECT(0)
		 | PARITY_ENABLE(0)
		 | STOP_BIT_SELECT(0)
		 | WORD_LENGTH_SELECT(3));

  /* Reset FIFOs */
  *AFCR(port) = ( FIFO_ENABLE(1)
		 | RX_FIFO_RESET(1)
		 | TX_FIFO_RESET(1)
		 | RX_TRIGGER_LEVEL(2));

#if !defined(UART_POLLED)
  /* Turn on appropriate interrupts */
  *AIER(port) = (RBRIE(1)      /* Receive Interrupt */
		 |THRIE(1)     /* Transmit Interrupt */
		 |RXLIE(0));   /* Line interrupt */
#endif
  
}

void UARTInit(void)
{
  UARTInitInt(0,UARTBAUDRATE);
  UARTInitInt(1,UARTBAUDRATE);

#if !defined(UART_POLLED)
  /* Hook interrupt vectors */
  IntHook(INTUART0LEVEL, 6, UART0IRQThunk);
  IntHook(INTUART1LEVEL, 7, UART1IRQThunk);

  /* Turn them on in the controller */
  IntEnable(6);
  IntEnable(7);
#endif

}

void UARTStatus(uint32_t port)
{
  printf("Uart Status for port #%u:\n"
	 "  lsr:0x%02x\n"
	 "  iir:0x%02x\n",
	 (unsigned int)port,
	 (unsigned int)UARTState[port].lsr,
	 (unsigned int)UARTState[port].iir);
  printf("Tx Status: outHead:%d, outTail:%d, outCount:%d\n",
	 (unsigned int)UARTState[port].outHead,
	 (unsigned int)UARTState[port].outTail,
	 (unsigned int)UARTState[port].outCount);
  printf("Rx Status: inHead:%d, inTail:%d, inCount:%d\n",
	 (unsigned int)UARTState[port].inHead,
	 (unsigned int)UARTState[port].inTail,
	 (unsigned int)UARTState[port].inCount);

}
 #endif
