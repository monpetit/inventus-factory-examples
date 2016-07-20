
#include <reent.h>  // required for _write_r
#include "em_usart.h"

/*********************************************************************
 *
 *       Types
 *
 **********************************************************************
 */
//
// If necessary define the _reent struct
// to match the one passed by the used standard library.
//
struct _reent;

/*********************************************************************
 *
 *       Function prototypes
 *
 **********************************************************************
 */
int _write(int file, char *ptr, int len);
_ssize_t _write_r(struct _reent *r, int file, const void *ptr, size_t len);

/*********************************************************************
 *
 *       Global functions
 *
 **********************************************************************
 */

/*********************************************************************
 *
 *       _write()
 *
 * Function description
 *   Low-level write function.
 *   libc subroutines will use this system routine for output to all files,
 *   including stdout.
 *   Write data via USART1.
 */
int _write(int file, char *ptr, int len)
{
    int i;
    (void) file; /* Not used, avoid warning */

    for (i = 0; i < len; i++) {
        if (*ptr == 0)
            break;
        else if (*ptr == '\n')
            USART_Tx(USART1, (uint8_t) '\r');
        USART_Tx(USART1, (uint8_t) *ptr);
        ptr++;
    }

    return len;
}

/*********************************************************************
 *
 *       _write_r()
 *
 * Function description
 *   Low-level reentrant write function.
 *   libc subroutines will use this system routine for output to all files,
 *   including stdout.
 *   Write data via USART1.
 */
_ssize_t _write_r(struct _reent *r, int file, const void *ptr, size_t len)
{
    size_t i;
    uint8_t* buffer = (uint8_t*)ptr;

    (void) file; /* Not used, avoid warning */
    (void) r; /* Not used, avoid warning */

    for (i = 0; i < len; i++) {
        if (*buffer == 0)
            break;
        else if (*buffer == '\n')
            USART_Tx(USART1, (uint8_t) '\r');
        USART_Tx(USART1, *buffer);
        buffer++;
    }

    return len;
}

/****** End Of File *************************************************/

