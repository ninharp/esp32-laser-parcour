/**
 * Sound Manager Component - Implementation
 * 
 * I2S audio playback for WAV/MP3 files from SD card using ESP-ADF.
 * 
 * @author ninharp
 * @date 2025
 */

#include "sound_manager.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <stdio.h>

#ifdef CONFIG_ENABLE_SOUND_MANAGER

#include "audio_pipeline.h"
#include "audio_element.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "wav_decoder.h"
#include "fatfs_stream.h"
#include "filter_resample.h"

static const char *TAG = "SOUND_MGR";

// Default sound file mappings
static const char *default_sound_files[SOUND_EVENT_MAX] = {
    [SOUND_EVENT_STARTUP] = "startup.mp3",
    [SOUND_EVENT_BUTTON_PRESS] = "beep.mp3",
    [SOUND_EVENT_GAME_START] = "game_start.mp3",
    [SOUND_EVENT_COUNTDOWN] = "countdown.mp3",
    [SOUND_EVENT_GAME_RUNNING] = "background.mp3",
    [SOUND_EVENT_BEAM_BREAK] = "penalty.mp3",
    [SOUND_EVENT_GAME_FINISH] = "finish.mp3",
    [SOUND_EVENT_GAME_STOP] = "game_stop.mp3",
    [SOUND_EVENT_ERROR] = "error.mp3",
    [SOUND_EVENT_SUCCESS] = "success.mp3"
};

// Audio pipeline components
static audio_pipeline_handle_t pipeline = NULL;
static audio_element_handle_t i2s_stream_writer = NULL;
static audio_element_handle_t mp3_decoder = NULL;
static audio_element_handle_t wav_decoder = NULL;
static audio_element_handle_t fatfs_reader = NULL;
static audio_element_handle_t resample_filter = NULL;
static audio_event_iface_handle_t evt = NULL;

// Configuration
static sound_config_t current_config = {0};
static char *event_sound_files[SOUND_EVENT_MAX] = {0};
static uint8_t current_volume = 70;
static bool is_initialized = false;
static bool is_playing = false;
static sound_mode_t current_mode = SOUND_MODE_ONCE;

// NVS storage
#define NVS_NAMESPACE "sound_cfg"
#define NVS_KEY_VOLUME "volume"
#define NVS_KEY_EVENT_PREFIX "evt_"

/**
 * Audio event handler task
 */
static void audio_event_task(void *pvParameters)
{
    audio_event_iface_msg_t msg;
    
    while (1) {
        if (audio_event_iface_listen(evt, &msg, portMAX_DELAY) == ESP_OK) {
            if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) {
                if (msg.source == (void *)mp3_decoder || msg.source == (void *)wav_decoder) {
                    if (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) {
                        audio_element_state_t el_state = (audio_element_state_t)msg.data;
                        if (el_state == AEL_STATE_FINISHED) {
                            ESP_LOGI(TAG, "Audio finished");
                            is_playing = false;
                            
                            // Loop if needed
                            if (current_mode == SOUND_MODE_LOOP) {
                                audio_pipeline_stop(pipeline);
                                audio_pipeline_wait_for_stop(pipeline);
                                audio_pipeline_reset_ringbuffer(pipeline);
                                audio_pipeline_reset_elements(pipeline);
                                audio_pipeline_run(pipeline);
                            } else {
                                audio_pipeline_stop(pipeline);
                                audio_pipeline_wait_for_stop(pipeline);
                                audio_pipeline_terminate(pipeline);
                            }
                        }
                    }
                }
            }
        }
    }
    vTaskDelete(NULL);
}

/**
 * Initialize I2S stream
 */
static esp_err_t init_i2s_stream(void)
{
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    
    // Configure channel for I2S NUM 0
    i2s_cfg.chan_cfg.id = I2S_NUM_0;
    i2s_cfg.chan_cfg.role = I2S_ROLE_MASTER;
    i2s_cfg.chan_cfg.dma_desc_num = 8;
    i2s_cfg.chan_cfg.dma_frame_num = 300;
    i2s_cfg.chan_cfg.auto_clear = true;
    
    // Configure standard mode with clock settings
    // Let ESP-ADF auto-configure sample rate based on audio file
    // Common rates: 24000, 32000, 44100, 48000 Hz
    i2s_cfg.std_cfg.clk_cfg.sample_rate_hz = 44100;  // Default, will be adjusted by decoder
    i2s_cfg.std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    
    // Configure slot (data format) settings - Philips I2S format
    i2s_cfg.std_cfg.slot_cfg.data_bit_width = I2S_DATA_BIT_WIDTH_16BIT;
    i2s_cfg.std_cfg.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO;
    i2s_cfg.std_cfg.slot_cfg.slot_mode = I2S_SLOT_MODE_STEREO;
    i2s_cfg.std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;
    i2s_cfg.std_cfg.slot_cfg.ws_width = I2S_DATA_BIT_WIDTH_16BIT;
    i2s_cfg.std_cfg.slot_cfg.ws_pol = false;
    i2s_cfg.std_cfg.slot_cfg.bit_shift = true;
    i2s_cfg.std_cfg.slot_cfg.left_align = true;
    i2s_cfg.std_cfg.slot_cfg.big_endian = false;
    i2s_cfg.std_cfg.slot_cfg.bit_order_lsb = false;
    
    // Configure GPIO pins
    i2s_cfg.std_cfg.gpio_cfg.bclk = current_config.bck_io_num;
    i2s_cfg.std_cfg.gpio_cfg.ws = current_config.ws_io_num;
    i2s_cfg.std_cfg.gpio_cfg.dout = current_config.data_out_num;
    i2s_cfg.std_cfg.gpio_cfg.din = I2S_GPIO_UNUSED;
    i2s_cfg.std_cfg.gpio_cfg.mclk = I2S_GPIO_UNUSED;
    i2s_cfg.std_cfg.gpio_cfg.invert_flags.mclk_inv = false;
    i2s_cfg.std_cfg.gpio_cfg.invert_flags.bclk_inv = false;
    i2s_cfg.std_cfg.gpio_cfg.invert_flags.ws_inv = false;
    
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);
    if (i2s_stream_writer == NULL) {
        ESP_LOGE(TAG, "Failed to create I2S stream");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

/**
 * Initialize audio pipeline
 */
static esp_err_t init_pipeline(void)
{
    // Create pipeline
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    if (pipeline == NULL) {
        ESP_LOGE(TAG, "Failed to create pipeline");
        return ESP_FAIL;
    }
    
    // Create FATFS stream reader
    fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_cfg.type = AUDIO_STREAM_READER;
    fatfs_reader = fatfs_stream_init(&fatfs_cfg);
    
    // Create MP3 decoder
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    mp3_decoder = mp3_decoder_init(&mp3_cfg);
    
    // Create WAV decoder
    wav_decoder_cfg_t wav_cfg = DEFAULT_WAV_DECODER_CONFIG();
    wav_decoder = wav_decoder_init(&wav_cfg);
    
    // Create resample filter for sample rate conversion
    // This converts any input sample rate to 44100 Hz for I2S output
    rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    rsp_cfg.src_rate = 44100;  // Will be auto-adjusted by decoder
    rsp_cfg.src_ch = 2;        // Stereo input
    rsp_cfg.dest_rate = 44100; // Output to I2S
    rsp_cfg.dest_ch = 2;       // Stereo output
    rsp_cfg.mode = RESAMPLE_DECODE_MODE;
    resample_filter = rsp_filter_init(&rsp_cfg);
    
    // Create I2S stream
    if (init_i2s_stream() != ESP_OK) {
        return ESP_FAIL;
    }
    
    // Create event interface
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    evt = audio_event_iface_init(&evt_cfg);
    
    // Set up event listener
    audio_pipeline_set_listener(pipeline, evt);
    
    // Start event task
    xTaskCreate(audio_event_task, "audio_event", 3072, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Audio pipeline initialized");
    return ESP_OK;
}

esp_err_t sound_manager_init(const sound_config_t *config)
{
    if (is_initialized) {
        ESP_LOGW(TAG, "Sound manager already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing sound manager...");
    
    // Use provided config or defaults from Kconfig
    if (config) {
        memcpy(&current_config, config, sizeof(sound_config_t));
    } else {
        current_config.bck_io_num = CONFIG_I2S_BCK_PIN;
        current_config.ws_io_num = CONFIG_I2S_WS_PIN;
        current_config.data_out_num = CONFIG_I2S_DATA_OUT_PIN;
        current_config.sound_dir = CONFIG_SOUND_FILES_PATH;
        current_config.default_volume = CONFIG_DEFAULT_VOLUME;
    }
    
    current_volume = current_config.default_volume;
    
    // Initialize default sound mappings
    for (int i = 0; i < SOUND_EVENT_MAX; i++) {
        if (default_sound_files[i]) {
            event_sound_files[i] = strdup(default_sound_files[i]);
        }
    }
    
    // Initialize audio pipeline
    if (init_pipeline() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize audio pipeline");
        return ESP_FAIL;
    }
    
    // Load configuration from NVS
    sound_manager_load_config();
    
    is_initialized = true;
    ESP_LOGI(TAG, "Sound manager initialized (I2S pins: BCK=%d, WS=%d, DOUT=%d)",
             current_config.bck_io_num, current_config.ws_io_num, current_config.data_out_num);
    
    return ESP_OK;
}

esp_err_t sound_manager_deinit(void)
{
    if (!is_initialized) {
        return ESP_OK;
    }
    
    // Stop and cleanup pipeline
    if (pipeline) {
        audio_pipeline_stop(pipeline);
        audio_pipeline_wait_for_stop(pipeline);
        audio_pipeline_terminate(pipeline);
        audio_pipeline_unregister(pipeline, fatfs_reader);
        audio_pipeline_unregister(pipeline, mp3_decoder);
        audio_pipeline_unregister(pipeline, wav_decoder);
        audio_pipeline_unregister(pipeline, resample_filter);
        audio_pipeline_unregister(pipeline, i2s_stream_writer);
        audio_pipeline_deinit(pipeline);
    }
    
    // Cleanup elements
    if (fatfs_reader) audio_element_deinit(fatfs_reader);
    if (mp3_decoder) audio_element_deinit(mp3_decoder);
    if (wav_decoder) audio_element_deinit(wav_decoder);
    if (resample_filter) audio_element_deinit(resample_filter);
    if (i2s_stream_writer) audio_element_deinit(i2s_stream_writer);
    if (evt) audio_event_iface_destroy(evt);
    
    // Free sound file mappings
    for (int i = 0; i < SOUND_EVENT_MAX; i++) {
        if (event_sound_files[i]) {
            free(event_sound_files[i]);
            event_sound_files[i] = NULL;
        }
    }
    
    is_initialized = false;
    ESP_LOGI(TAG, "Sound manager deinitialized");
    
    return ESP_OK;
}

esp_err_t sound_manager_play_file(const char *filename, sound_mode_t mode)
{
    if (!is_initialized) {
        ESP_LOGW(TAG, "Sound manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!filename) {
        ESP_LOGE(TAG, "Filename is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Stop current playback
    if (is_playing) {
        sound_manager_stop();
    }
    
    // Build full path
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", current_config.sound_dir, filename);
    
    ESP_LOGI(TAG, "Playing: %s (mode: %s)", filepath, mode == SOUND_MODE_LOOP ? "loop" : "once");
    
    // Determine decoder based on file extension
    audio_element_handle_t decoder = NULL;
    if (strstr(filename, ".mp3") || strstr(filename, ".MP3")) {
        decoder = mp3_decoder;
    } else if (strstr(filename, ".wav") || strstr(filename, ".WAV")) {
        decoder = wav_decoder;
    } else {
        ESP_LOGE(TAG, "Unsupported file format: %s", filename);
        return ESP_ERR_NOT_SUPPORTED;
    }
    
    // Reset pipeline
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);
    audio_pipeline_reset_ringbuffer(pipeline);
    audio_pipeline_reset_elements(pipeline);
    audio_pipeline_unregister(pipeline, mp3_decoder);
    audio_pipeline_unregister(pipeline, wav_decoder);
    audio_pipeline_unregister(pipeline, resample_filter);
    
    // Register elements: file -> decoder -> resample -> i2s
    audio_pipeline_register(pipeline, fatfs_reader, "file");
    audio_pipeline_register(pipeline, decoder, "dec");
    audio_pipeline_register(pipeline, resample_filter, "filter");
    audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");
    
    // Link elements in chain
    const char *link_tag[4] = {"file", "dec", "filter", "i2s"};
    audio_pipeline_link(pipeline, &link_tag[0], 4);
    
    // Set URI
    audio_element_set_uri(fatfs_reader, filepath);
    
    // Set playback mode
    current_mode = mode;
    
    // Start pipeline
    audio_pipeline_run(pipeline);
    is_playing = true;
    
    return ESP_OK;
}

esp_err_t sound_manager_play_event(sound_event_t event, sound_mode_t mode)
{
    if (event >= SOUND_EVENT_MAX) {
        ESP_LOGE(TAG, "Invalid event: %d", event);
        return ESP_ERR_INVALID_ARG;
    }
    
    const char *filename = event_sound_files[event];
    if (!filename) {
        ESP_LOGW(TAG, "No sound file mapped for event %d", event);
        return ESP_ERR_NOT_FOUND;
    }
    
    return sound_manager_play_file(filename, mode);
}

esp_err_t sound_manager_stop(void)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (is_playing) {
        ESP_LOGI(TAG, "Stopping playback");
        audio_pipeline_stop(pipeline);
        audio_pipeline_wait_for_stop(pipeline);
        audio_pipeline_terminate(pipeline);
        is_playing = false;
    }
    
    return ESP_OK;
}

esp_err_t sound_manager_set_volume(uint8_t volume)
{
    if (volume > 100) {
        volume = 100;
    }
    
    current_volume = volume;
    
    if (i2s_stream_writer) {
        // ESP-ADF volume control (0-100)
        audio_element_set_music_info(i2s_stream_writer, 48000, 2, 16);
    }
    
    ESP_LOGI(TAG, "Volume set to %d%%", volume);
    return ESP_OK;
}

int sound_manager_get_volume(void)
{
    return current_volume;
}

bool sound_manager_is_ready(void)
{
    return is_initialized;
}

esp_err_t sound_manager_set_event_file(sound_event_t event, const char *filename)
{
    if (event >= SOUND_EVENT_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (event_sound_files[event]) {
        free(event_sound_files[event]);
        event_sound_files[event] = NULL;
    }
    
    if (filename) {
        event_sound_files[event] = strdup(filename);
    }
    
    return ESP_OK;
}

const char* sound_manager_get_event_file(sound_event_t event)
{
    if (event >= SOUND_EVENT_MAX) {
        return NULL;
    }
    return event_sound_files[event];
}

esp_err_t sound_manager_save_config(void)
{
    nvs_handle_t handle;
    esp_err_t err;
    
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    // Save volume
    nvs_set_u8(handle, NVS_KEY_VOLUME, current_volume);
    
    // Save event mappings
    for (int i = 0; i < SOUND_EVENT_MAX; i++) {
        if (event_sound_files[i]) {
            char key[16];
            snprintf(key, sizeof(key), "%s%d", NVS_KEY_EVENT_PREFIX, i);
            nvs_set_str(handle, key, event_sound_files[i]);
        }
    }
    
    nvs_commit(handle);
    nvs_close(handle);
    
    ESP_LOGI(TAG, "Configuration saved to NVS");
    return ESP_OK;
}

esp_err_t sound_manager_load_config(void)
{
    nvs_handle_t handle;
    esp_err_t err;
    
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No saved configuration found");
        return err;
    }
    
    // Load volume
    uint8_t volume;
    if (nvs_get_u8(handle, NVS_KEY_VOLUME, &volume) == ESP_OK) {
        current_volume = volume;
    }
    
    // Load event mappings
    for (int i = 0; i < SOUND_EVENT_MAX; i++) {
        char key[16];
        snprintf(key, sizeof(key), "%s%d", NVS_KEY_EVENT_PREFIX, i);
        
        size_t required_size;
        if (nvs_get_str(handle, key, NULL, &required_size) == ESP_OK) {
            char *filename = malloc(required_size);
            if (filename && nvs_get_str(handle, key, filename, &required_size) == ESP_OK) {
                if (event_sound_files[i]) {
                    free(event_sound_files[i]);
                }
                event_sound_files[i] = filename;
            } else if (filename) {
                free(filename);
            }
        }
    }
    
    nvs_close(handle);
    ESP_LOGI(TAG, "Configuration loaded from NVS");
    
    return ESP_OK;
}

#else // !CONFIG_ENABLE_SOUND_MANAGER

// Stub implementations when sound manager is disabled
esp_err_t sound_manager_init(const sound_config_t *config) { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t sound_manager_deinit(void) { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t sound_manager_play_event(sound_event_t event, sound_mode_t mode) { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t sound_manager_play_file(const char *filename, sound_mode_t mode) { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t sound_manager_stop(void) { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t sound_manager_set_volume(uint8_t volume) { return ESP_ERR_NOT_SUPPORTED; }
int sound_manager_get_volume(void) { return -1; }
bool sound_manager_is_ready(void) { return false; }
esp_err_t sound_manager_set_event_file(sound_event_t event, const char *filename) { return ESP_ERR_NOT_SUPPORTED; }
const char* sound_manager_get_event_file(sound_event_t event) { return NULL; }
esp_err_t sound_manager_save_config(void) { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t sound_manager_load_config(void) { return ESP_ERR_NOT_SUPPORTED; }

#endif // CONFIG_ENABLE_SOUND_MANAGER
