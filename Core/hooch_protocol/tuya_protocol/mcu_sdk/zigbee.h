/***********************************************************
 * @file     zigbee.h
 * @brief    define:            common macro.
 * @version  3.3.5
 * @date     2026.04.08
 * @copyright Copyright (c) tuya.inc 2024
 **********************************************************/

#ifndef __ZIGBEE_H_
#define __ZIGBEE_H_

#include "system.h"
#include "protocol.h"
#include "mcu_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

/***********************************************************
 * Macro Definitions
 **********************************************************/

#ifndef TRUE
#define TRUE                    1
#endif

#ifndef FALSE
#define FALSE                   0
#endif

#ifndef NULL
#define NULL                    ((void *)0)
#endif

#ifndef SUCCESS
#define SUCCESS                 1
#endif

#ifndef ERROR
#define ERROR                   0
#endif

#ifndef INVALID
#define INVALID                 0xFF
#endif

#ifndef ENABLE
#define ENABLE                  1
#endif

#ifndef DISABLE
#define DISABLE                 0
#endif

/***********************************************************
 * Typedef Definitions
 **********************************************************/


/***********************************************************
 * Variable Declarations
 **********************************************************/


/***********************************************************
 * Function Declarations
 **********************************************************/


#ifdef __cplusplus
}
#endif

#endif
/* -END OF FILE-  */
