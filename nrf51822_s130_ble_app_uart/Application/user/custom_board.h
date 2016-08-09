#ifndef __CUSTOM_BOARD_H
#define __CUSTOM_BOARD_H

#if defined(BOARD_BLE400)
  #include "ble400.h"
#elif defined(BOARD_TINYBLE)
  #include "tinyble.h"  
#else
#error "Board is not defined"
#endif

#endif /* __CUSTOM_BOARD_H */
