/**
 * Logging Configuration - Header
 * 
 * Centralized logging level configuration for all components
 * 
 * @author ninharp
 * @date 2026-01-09
 */

#ifndef LOGGING_CONFIG_H
#define LOGGING_CONFIG_H

#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize logging levels for all components
 * Call this at the very start of app_main()
 */
void init_logging(void);

#ifdef __cplusplus
}
#endif

#endif // LOGGING_CONFIG_H
