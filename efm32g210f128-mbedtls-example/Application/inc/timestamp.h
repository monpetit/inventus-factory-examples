#ifndef __EFM32_TIMESTAMP_H_
#define __EFM32_TIMESTAMP_H_

#include "em_device.h"

#ifdef __cplusplus
extern "C" {
#endif

__STATIC_INLINE void timestamp_counter_enable(void)
{
    /* Enable Trace */
    CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
    CoreDebug->DEMCR |=  CoreDebug_DEMCR_TRCENA_Msk;

    /* Enable Cycle counter */
    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
    DWT->CTRL |=  DWT_CTRL_CYCCNTENA_Msk;

    /* Reset Cycle counter */
    DWT->CYCCNT = 0;
}

__STATIC_INLINE void timestamp_counter_disable(void)
{
    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
}


#define timestamp_counter_get()      (DWT->CYCCNT)
#define timestamp_counter_set(x)     (DWT->CYCCNT = (x))
#define timestamp_counter_reset()    timestamp_counter_set(0)

#ifdef __cplusplus
}
#endif

#endif /* __EFM32_TIMESTAMP_H_ */
