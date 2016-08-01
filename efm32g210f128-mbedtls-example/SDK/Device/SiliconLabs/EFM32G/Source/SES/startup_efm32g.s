/*****************************************************************************
 * Copyright (c) 2014 Rowley Associates Limited.                             *
 *                                                                           *
 * This file may be distributed under the terms of the License Agreement     *
 * provided with this software.                                              *
 *                                                                           *
 * THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND, INCLUDING THE   *
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. *
 *****************************************************************************/

.macro ISR_HANDLER name=
  .section .vectors, "ax"
  .word \name
  .section .init, "ax"
  .thumb_func
  .weak \name
\name:
1: b 1b /* endless loop */
.endm

.macro ISR_RESERVED
  .section .vectors, "ax"
  .word 0
.endm

  .syntax unified
  .global reset_handler

  .section .vectors, "ax"
  .code 16 
  .global _vectors

.macro DEFAULT_ISR_HANDLER name=
  .thumb_func
  .weak \name
\name:
1: b 1b /* endless loop */
.endm

_vectors:
  .word __stack_end__
  .word reset_handler
  ISR_HANDLER   NMI_Handler               /*      NMI Handler               */
  ISR_HANDLER   HardFault_Handler	  /*      Hard Fault Handler        */
  ISR_HANDLER   MemManage_Handler	  /*      MPU Fault Handler         */
  ISR_HANDLER   BusFault_Handler	  /*      Bus Fault Handler         */
  ISR_HANDLER   UsageFault_Handler	  /*      Usage Fault Handler       */
  ISR_RESERVED				  /*      Reserved                  */
  ISR_RESERVED				  /*      Reserved                  */
  ISR_RESERVED				  /*      Reserved                  */
  ISR_RESERVED				  /*      Reserved                  */
  ISR_HANDLER   SVC_Handler		  /*      SVCall Handler            */
  ISR_HANDLER   DebugMon_Handler	  /*      Debug Monitor Handler     */
  ISR_RESERVED				  /*      Reserved                  */
  ISR_HANDLER   PendSV_Handler		  /*      PendSV Handler            */
  ISR_HANDLER   SysTick_Handler		  /*      SysTick Handler           */

  /* External interrupts */

  ISR_HANDLER   DMA_IRQHandler		  /*  0 - DMA       */
  ISR_HANDLER   GPIO_EVEN_IRQHandler	  /*  1 - GPIO_EVEN       */
  ISR_HANDLER   TIMER0_IRQHandler	  /*  2 - TIMER0       */
  ISR_HANDLER   USART0_RX_IRQHandler	  /*  3 - USART0_RX       */
  ISR_HANDLER   USART0_TX_IRQHandler	  /*  4 - USART0_TX       */
  ISR_HANDLER   ACMP0_IRQHandler	  /*  5 - ACMP0       */
  ISR_HANDLER   ADC0_IRQHandler		  /*  6 - ADC0       */
  ISR_HANDLER   DAC0_IRQHandler		  /*  7 - DAC0       */
  ISR_HANDLER   I2C0_IRQHandler		  /*  8 - I2C0       */
  ISR_HANDLER   GPIO_ODD_IRQHandler	  /*  9 - GPIO_ODD       */
  ISR_HANDLER   TIMER1_IRQHandler	  /*  10 - TIMER1       */
  ISR_HANDLER   TIMER2_IRQHandler	  /*  11 - TIMER2       */
  ISR_HANDLER   USART1_RX_IRQHandler	  /*  12 - USART1_RX       */
  ISR_HANDLER   USART1_TX_IRQHandler	  /*  13 - USART1_TX       */
  ISR_HANDLER   USART2_RX_IRQHandler	  /*  14 - USART2_RX       */
  ISR_HANDLER   USART2_TX_IRQHandler	  /*  15 - USART2_TX       */
  ISR_HANDLER   UART0_RX_IRQHandler	  /*  16 - UART0_RX       */
  ISR_HANDLER   UART0_TX_IRQHandler	  /*  17 - UART0_TX       */
  ISR_HANDLER   LEUART0_IRQHandler	  /*  18 - LEUART0       */
  ISR_HANDLER   LEUART1_IRQHandler	  /*  19 - LEUART1       */
  ISR_HANDLER   LETIMER0_IRQHandler	  /*  20 - LETIMER0       */
  ISR_HANDLER   PCNT0_IRQHandler	  /*  21 - PCNT0       */
  ISR_HANDLER   PCNT1_IRQHandler	  /*  22 - PCNT1       */
  ISR_HANDLER   PCNT2_IRQHandler	  /*  23 - PCNT2       */
  ISR_HANDLER   RTC_IRQHandler		  /*  24 - RTC       */
  ISR_HANDLER   CMU_IRQHandler		  /*  25 - CMU       */
  ISR_HANDLER   VCMP_IRQHandler		  /*  26 - VCMP       */
  ISR_HANDLER   LCD_IRQHandler		  /*  27 - LCD       */
  ISR_HANDLER   MSC_IRQHandler		  /*  28 - MSC       */
  ISR_HANDLER   AES_IRQHandler		  /*  29 - AES       */
  ISR_RESERVED				  /*  30 - Reserved      */
  .section .vectors, "ax"
_vectors_end:

  .section .init, "ax"
  .thumb_func

  reset_handler:

#ifndef __NO_SYSTEM_INIT
  ldr r0, =__SRAM_segment_end__
  mov sp, r0
  bl SystemInit
#endif

  b _start

#ifndef __NO_SYSTEM_INIT
  .thumb_func
  .weak SystemInit
SystemInit:
  bx lr
#endif
