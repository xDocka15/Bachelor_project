#include "main_def.h"
#define CHUNK_SIZE 1024

volatile int number = 0;
volatile int led_state = 0;

static const char *TAG = "webserver";

void initi_web_page_buffer(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true};

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    // memset((void *)index_html, 0, sizeof(index_html));
    // struct stat st;
    // if (stat(INDEX_HTML_PATH, &st))
    // {
    //     ESP_LOGE(TAG, "index.html not found");
    //     return;
    // }

    // FILE *fp = fopen(INDEX_HTML_PATH, "r");
    // if (fread(index_html, st.st_size, 1, fp) == 0)
    // {
    //     ESP_LOGE(TAG, "fread failed");
    // }
    // fclose(fp);

}

// esp_err_t root_handler(httpd_req_t *req)
// {
//     // Open the index.html file for reading
//     const char* filename = INDEX_HTML_PATH; // Adjust the path as needed

//     FILE* file = fopen(filename, "r");

//     if (file == NULL) {
//         ESP_LOGE(TAG, "Failed to open file: %s", filename); // Log an error message
//         httpd_resp_send_404(req);
//         return ESP_OK;
//     }

//     // Calculate the file size
//     fseek(file, 0, SEEK_END);
//     size_t file_size = ftell(file);
//     fseek(file, 0, SEEK_SET);

//     // Read the file content into a buffer
//     char* file_content = (char*)malloc(file_size);
//     fread(file_content, 1, file_size, file);

//     // Send the file content as the HTTP response
//     httpd_resp_set_type(req, "text/html");
//     httpd_resp_send(req, file_content, file_size);

//     // Clean up
//     free(file_content);
//     fclose(file);

//     return ESP_OK;
// }
esp_err_t root_handler(httpd_req_t *req)
{
FILE *file = fopen(INDEX_HTML_PATH, "r");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open file");
        return ESP_FAIL;
    }

    char *chunk = malloc(CHUNK_SIZE);
    if (chunk == NULL) {
        fclose(file);
        ESP_LOGE(TAG, "Failed to allocate memory for chunk");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to allocate memory");
        return ESP_FAIL;
    }

    size_t read_bytes;
    do {
        read_bytes = fread(chunk, 1, CHUNK_SIZE, file);
        if (read_bytes > 0) {
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                ESP_LOGE(TAG, "File sending failed");
                free(chunk);
                fclose(file);
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);

    free(chunk);
    fclose(file);

    // Signal the end of the response
    httpd_resp_send_chunk(req, NULL, 0);
    
    return ESP_OK;
}

esp_err_t toggle_led_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Switching LED");

    // led_state = !led_state;
    // gpio_set_level(LED_GPIO_PIN, led_state);
    //return ESP_OK;

    gpio_isr_handler(NULL);

    char response[64];
    snprintf(response, sizeof(response), "{\"ledState\": %f}", speed/TIMER_SCALE);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

esp_err_t update_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Updating data");
    // Send the current LED state as a JSON response
    char response[64];
    snprintf(response, sizeof(response), "{\"ledState\": %f, \"TERM_TEMP_1\": %d, \"TERM_TEMP_2\": %d }", speed/TIMER_SCALE, TERM_TEMP_1, TERM_TEMP_2);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

void start_webserver(void)
{
    // Declare the HTTP server handle
    httpd_handle_t server = NULL;   
    // Configure the HTTP server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    // Start the HTTP server
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        static const httpd_uri_t root_uri = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = root_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &root_uri);
        static const httpd_uri_t toggle_led_uri = {
            .uri       = "/led",
            .method    = HTTP_GET,
            .handler   = toggle_led_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &toggle_led_uri);
        static const httpd_uri_t update_uri = {
            .uri       = "/upd",
            .method    = HTTP_GET,
            .handler   = update_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &update_uri);
        ESP_LOGI(TAG, "Server initialized!");
    } else {
    ESP_LOGI(TAG, "Error starting server!");
    }
}

// void app_main(void)
// {
//     initi_web_page_buffer();
//     start_webserver();

// }