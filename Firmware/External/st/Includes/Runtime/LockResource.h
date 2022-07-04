#ifndef VENDOR_ST_RUNTIME_LOCKRESOURCE_H
#define VENDOR_ST_RUNTIME_LOCKRESOURCE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_hal.h"


/* Exported types ------------------------------------------------------------*/
typedef enum
{
    LOCK_RESOURCE_STATUS_OK       = 0x00U,
    LOCK_RESOURCE_STATUS_ERROR    = 0x01U,
    LOCK_RESOURCE_STATUS_TIMEOUT  = 0x02U
} LockResource_Status_t;

/* Exported constants --------------------------------------------------------*/
#define LOCK_RESOURCE_TIMEOUT   100U /* timeout in ms */

/* Exported macro ------------------------------------------------------------*/
#define PERIPH_LOCK(__Periph__)       Periph_Lock(__Periph__, LOCK_RESOURCE_TIMEOUT)
#define PERIPH_UNLOCK(__Periph__)     Periph_Unlock(__Periph__)

/* Exported functions ------------------------------------------------------- */
LockResource_Status_t Periph_Lock(void* Peripheral, uint32_t Timeout);
void Periph_Unlock(void* Peripheral);

#ifdef __cplusplus
};
#endif

#endif
