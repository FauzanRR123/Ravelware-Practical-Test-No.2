#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "esp_log.h"

#define SENSOR_COUNT 5
#define READ_TIME_MS 40
#define SEND_TIME_MS 40
#define TARGET_INTERVAL_MS 200

static const char *TAG = "SENSOR_SIM";


typedef struct {
    int sensor_id;
    int64_t timestamp_read;
} sensor_packet_t;


QueueHandle_t data_queue;

// Task 1: Membaca Sensor (Producer)
void read_sensors_task(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(TARGET_INTERVAL_MS);

    while (1) {
        for (int i = 1; i <= SENSOR_COUNT; i++) {
            vTaskDelay(pdMS_TO_TICKS(READ_TIME_MS));

            sensor_packet_t packet;
            packet.sensor_id = i;
            packet.timestamp_read = esp_timer_get_time() / 1000;

            xQueueSend(data_queue, &packet, 0);
        }
       
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Task 2: Mengirim Data (Consumer)
void send_data_task(void *pvParameters) {
    sensor_packet_t received_packet;
    
    while (1) {
 
        if (xQueueReceive(data_queue, &received_packet, portMAX_DELAY) == pdTRUE) {
            
      
            vTaskDelay(pdMS_TO_TICKS(SEND_TIME_MS));
            
            int64_t now = esp_timer_get_time() / 1000;
            
         
            ESP_LOGI(TAG, "[KIRIM] Sensor %d | Waktu Baca: %lld ms | Waktu Kirim: %lld ms", 
                     received_packet.sensor_id, 
                     received_packet.timestamp_read, 
                     now);
        }
    }
}

// Entry Point ESP-IDF
extern "C" void app_main(void) {
    ESP_LOGI(TAG, "=== Memulai Simulasi Pipelining 5 Sensor ===");

    // Buat antrean data (kapasitas 10 cukup untuk 5 sensor per siklus)
    data_queue = xQueueCreate(10, sizeof(sensor_packet_t));

    if (data_queue == NULL) {
        ESP_LOGE(TAG, "Gagal membuat queue!");
        return;
    }

    // Jalankan FreeRTOS Task

    xTaskCreate(read_sensors_task, "ReadTask", 4096, NULL, 5, NULL);
    xTaskCreate(send_data_task, "SendTask", 4096, NULL, 4, NULL);
}