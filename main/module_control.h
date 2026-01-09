/**
 * CONTROL Module - Main Unit Implementation
 * 
 * Handles main unit initialization and event callbacks
 * 
 * @author ninharp
 * @date 2026-01-09
 */

#ifndef MODULE_CONTROL_H
#define MODULE_CONTROL_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_MODULE_ROLE_CONTROL

/**
 * Initialize the main control unit
 * Sets up display, buttons, buzzer, web server, game logic
 */
void module_control_init(void);

/**
 * Run the main control unit loop
 * Handles display updates and status monitoring
 */
void module_control_run(void);

#endif // CONFIG_MODULE_ROLE_CONTROL

#ifdef __cplusplus
}
#endif

#endif // MODULE_CONTROL_H
