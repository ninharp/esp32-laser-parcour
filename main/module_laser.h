/**
 * LASER Module - Laser Unit Implementation
 * 
 * Handles laser unit initialization and event callbacks
 * 
 * @author ninharp
 * @date 2026-01-09
 */

#ifndef MODULE_LASER_H
#define MODULE_LASER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_MODULE_ROLE_LASER

/**
 * Initialize the laser unit
 * Sets up laser control, sensor, LEDs, pairing
 */
void module_laser_init(void);

/**
 * Run the laser unit loop
 * Handles sensor monitoring and status updates
 */
void module_laser_run(void);

#endif // CONFIG_MODULE_ROLE_LASER

#ifdef __cplusplus
}
#endif

#endif // MODULE_LASER_H
