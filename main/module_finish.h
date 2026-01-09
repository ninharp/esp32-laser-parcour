/**
 * FINISH Module - Finish Button Implementation
 * 
 * Handles finish button unit initialization and event callbacks
 * 
 * @author ninharp
 * @date 2026-01-09
 */

#ifndef MODULE_FINISH_H
#define MODULE_FINISH_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_MODULE_ROLE_FINISH

/**
 * Initialize the finish button unit
 * Sets up button, LEDs, pairing
 */
void module_finish_init(void);

/**
 * Run the finish button unit loop
 * Handles button monitoring and status updates
 */
void module_finish_run(void);

#endif // CONFIG_MODULE_ROLE_FINISH

#ifdef __cplusplus
}
#endif

#endif // MODULE_FINISH_H
