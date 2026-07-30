#ifndef _FONT5X7_H_
#define _FONT5X7_H_

#include <stdint.h>

/* Printable ASCII (0x20-0x7E), each character is 5 bytes (column-major,
 * bit0=top pixel .. bit6=bottom pixel). Classic "5x7 GLCD" font. */
extern const uint8_t font5x7[95][5];

#endif
