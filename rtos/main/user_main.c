// /*
//    This example code is in the Public Domain (or CC0 licensed, at your option.)

//    Unless required by applicable law or agreed to in writing, this
//    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
//    CONDITIONS OF ANY KIND, either express or implied.
// */

// #include <stdio.h>

// #include "esp_system.h"

// #include "Serial.h"

// void app_main()
// {
//     uart_init_task(115200);
//     // Create a task to handler UART event from ISR
//     xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);
// }

/* Esptouch example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"

#include <sys/socket.h>
#include <netdb.h>
#include "lwip/apps/sntp.h"

#include "wolfssl/ssl.h"

#include "esp_system.h"

#include "cJSON.h"

#include "MQTTClient.h"

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "iotdev.clz.me"
#define WEB_PORT 443
#define WEB_URL "https://iotdev.clz.me/api/v1/user/device/diy/auth?authKey=bc5a991c7ec4"

#define REQUEST "GET " WEB_URL " HTTP/1.0\r\n" \
    "Host: "WEB_SERVER"\r\n" \
    "User-Agent: esp-idf/1.0 espressif\r\n" \
    "\r\n"

#define WOLFSSL_DEMO_THREAD_NAME        "wolfssl_client"
#define WOLFSSL_DEMO_THREAD_STACK_WORDS 8192
#define WOLFSSL_DEMO_THREAD_PRORIOTY    6

#define WOLFSSL_DEMO_SNTP_SERVERS       "pool.ntp.org"

#define MQTT_CLIENT_THREAD_NAME         "mqtt_client_thread"
#define MQTT_CLIENT_THREAD_STACK_WORDS  4096
#define MQTT_CLIENT_THREAD_PRIO         8

#define BLINKER_MQTT_BORKER_ALIYUN      "aliyun"
#define BLINKER_MQTT_ALIYUN_HOST        "public.iot-as-mqtt.cn-shanghai.aliyuncs.com"
#define BLINKER_MQTT_ALIYUN_PORT        1883

const char send_data[] = REQUEST;
const int32_t send_bytes = sizeof(send_data);
char recv_data[1024] = {0};
char http_data[1024] = {0};

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const char *TAG = "sc";

const int WIFI_CONNECTED_BIT = BIT0;

char*       MQTT_HOST_MQTT;
char*       MQTT_ID_MQTT;
char*       MQTT_NAME_MQTT;
char*       MQTT_KEY_MQTT;
char*       MQTT_PRODUCTINFO_MQTT;
char*       UUID_MQTT;
char*       DEVICE_NAME_MQTT;
char*       BLINKER_PUB_TOPIC_MQTT;
char*       BLINKER_SUB_TOPIC_MQTT;
uint16_t    MQTT_PORT_MQTT;

uint8_t isInit = 0;

void smartconfig_example_task(void * parm);

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    // case SYSTEM_EVENT_STA_START:
    //     xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
    //     break;
    // case SYSTEM_EVENT_STA_GOT_IP:
    //     xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    //     break;
    // case SYSTEM_EVENT_STA_DISCONNECTED:
    //     esp_wifi_connect();
    //     xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    //     break;
    // default:
    //     break;
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static void sc_callback(smartconfig_status_t status, void *pdata)
{
    switch (status) {
        case SC_STATUS_WAIT:
            ESP_LOGI(TAG, "SC_STATUS_WAIT");
            break;
        case SC_STATUS_FIND_CHANNEL:
            ESP_LOGI(TAG, "SC_STATUS_FINDING_CHANNEL");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            ESP_LOGI(TAG, "SC_STATUS_GETTING_SSID_PSWD");
            break;
        case SC_STATUS_LINK:
            ESP_LOGI(TAG, "SC_STATUS_LINK");
            wifi_config_t *wifi_config = pdata;
            ESP_LOGI(TAG, "SSID:%s", wifi_config->sta.ssid);
            ESP_LOGI(TAG, "PASSWORD:%s", wifi_config->sta.password);
            ESP_ERROR_CHECK( esp_wifi_disconnect() );
            ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config) );
            ESP_ERROR_CHECK( esp_wifi_connect() );
            break;
        case SC_STATUS_LINK_OVER:
            ESP_LOGI(TAG, "SC_STATUS_LINK_OVER");
            if (pdata != NULL) {
                uint8_t phone_ip[4] = { 0 };
                memcpy(phone_ip, (uint8_t* )pdata, 4);
                ESP_LOGI(TAG, "Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1], phone_ip[2], phone_ip[3]);
            }
            xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
            break;
        default:
            break;
    }
}

void smartconfig_example_task(void * parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    ESP_ERROR_CHECK( esp_smartconfig_start(sc_callback) );
    while (1) {
        uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY); 
        if(uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}

void wifi_init_sta()
{
    wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "mostfun",
            .password = "18038083873"
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    // ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
    //          EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

static void get_time()
{
    struct timeval now;
    int sntp_retry_cnt = 0;
    int sntp_retry_time = 0;

    sntp_setoperatingmode(0);
    sntp_setservername(0, WOLFSSL_DEMO_SNTP_SERVERS);
    sntp_init();

    while (1) {
        for (int32_t i = 0; (i < (SNTP_RECV_TIMEOUT / 100)) && now.tv_sec < 1525952900; i++) {
            vTaskDelay(100 / portTICK_RATE_MS);
            gettimeofday(&now, NULL);
        }

        if (now.tv_sec < 1525952900) {
            sntp_retry_time = SNTP_RECV_TIMEOUT << sntp_retry_cnt;

            if (SNTP_RECV_TIMEOUT << (sntp_retry_cnt + 1) < SNTP_RETRY_TIMEOUT_MAX) {
                sntp_retry_cnt ++;
            }

            printf("SNTP get time failed, retry after %d ms\n", sntp_retry_time);
            vTaskDelay(sntp_retry_time / portTICK_RATE_MS);
        } else {
            printf("SNTP get time success, time: %d\n", (int)now.tv_sec);
            break;
        }
    }
}

// static void wolfssl_client(void* pv)
static void wolfssl_client()
{
    int32_t ret = 0;

    const portTickType xDelay = 500 / portTICK_RATE_MS;
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;

    int32_t socket = -1;
    struct sockaddr_in sock_addr;
    struct hostent* entry = NULL;

    /* CA date verification need system time */
    get_time();

    time_t now;
    struct tm timeinfo;
    char strftime_buf[64];

    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();    

    time(&now);
    localtime_r(&now, &timeinfo);

    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
    printf("%d\n", timeinfo.tm_year);

    while (1) 
    {

        printf("Setting hostname for TLS session...\n");

        /*get addr info for hostname*/
        do {
            entry = gethostbyname(WEB_SERVER);
            vTaskDelay(xDelay);
        } while (entry == NULL);

        printf("Init wolfSSL...\n");
        ret = wolfSSL_Init();

        printf("heap:%d\n", esp_get_free_heap_size());

        if (ret != WOLFSSL_SUCCESS) {
            printf("Init wolfSSL failed:%d...\n", ret);
            goto failed1;
        }

        printf("Set wolfSSL ctx ...\n");
        ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());

        if (!ctx) {
            printf("Set wolfSSL ctx failed...\n");
            goto failed1;
        }

        printf("Creat socket ...\n");
        socket = socket(AF_INET, SOCK_STREAM, 0);

        if (socket < 0) {
            printf("Creat socket failed...\n");
            goto failed2;
        }

#if CONFIG_CERT_AUTH
        printf("Loading the CA root certificate...\n");
        ret = wolfSSL_CTX_load_verify_buffer(ctx, server_root_cert_pem_start, server_root_cert_pem_end - server_root_cert_pem_start, WOLFSSL_FILETYPE_PEM);

        if (WOLFSSL_SUCCESS != ret) {
            printf("Loading the CA root certificate failed...\n");
            goto failed3;
        }

        wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_PEER, NULL);
#else
        wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_NONE, NULL);
#endif

        memset(&sock_addr, 0, sizeof(sock_addr));
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_port = htons(WEB_PORT);
        sock_addr.sin_addr.s_addr = ((struct in_addr*)(entry->h_addr))->s_addr;

        printf("Connecting to %s:%d...\n", WEB_SERVER, WEB_PORT);
        ret = connect(socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr));

        if (ret) {
            printf("Connecting to %s:%d failed: %d\n", WEB_SERVER, WEB_PORT, ret);
            goto failed3;
        }

        printf("Create wolfSSL...\n");
        ssl = wolfSSL_new(ctx);

        if (!ssl) {
            printf("Create wolfSSL failed...\n");
            goto failed3;
        }

        wolfSSL_set_fd(ssl, socket);

        printf("Performing the SSL/TLS handshake...\n");
        ret = wolfSSL_connect(ssl);

        if (WOLFSSL_SUCCESS != ret) {
            printf("Performing the SSL/TLS handshake failed:%d\n", ret);
            goto failed4;
        }

        printf("Writing HTTPS request...\n");
        ret = wolfSSL_write(ssl, send_data, send_bytes);

        if (ret <= 0) {
            printf("Writing HTTPS request failed:%d\n", ret);
            goto failed5;
        }

        printf("Reading HTTPS response...\n");

        do {
            ret = wolfSSL_read(ssl, recv_data, sizeof(recv_data));


            if (ret <= 0) {
                printf("\nConnection closed\n");
                break;
            }

            /* Print response directly to stdout as it is read */
            for (int i = 0; i < ret; i++) {
                printf("%c", recv_data[i]);
            }

            printf("\n%d\n", ret);

            char _status[4];
            // for (int i = 9; i < 12; i++) {
            //     printf("%c", recv_data[i]);
            // }

            memcpy(_status, recv_data + 9, (12 - 9));

            int status_code = atoi(_status);
            printf("status_code: %d\n", status_code);

            // strcpy(http_data, strrchr(recv_data, "\n"));
            // printf("%s\n", strrchr(recv_data, "\n"));

            // for (int i = 0; i < strlen(http_data); i++) {
            //     printf("%c", http_data[i]);
            // }

            if (status_code == 200)
            {
                int num_r = 0;

                for (num_r = ret; num_r > 0; num_r--) {
                    // printf("%c", recv_data[i]);
                    if (recv_data[num_r - 1] == '\n')
                        break;
                }

                // for (int i = num_r ; i < ret; i++) {
                //     printf("%c", recv_data[i]);
                // }

                memcpy(http_data, recv_data + num_r, (ret - num_r));
            }
            else
            {
                memset(http_data, '\0', 1024)
            }
            
            // if (strcmp(recv_data, "\r\n") == 0)
            // {
            //     printf("\nget new line\n");
            // }
        } while (1);

        printf("heap: %d\n", esp_get_free_heap_size());

        printf("http_data: %s\n", http_data);

failed5:
        wolfSSL_shutdown(ssl);
failed4:
        wolfSSL_free(ssl);
failed3:
        close(socket);
failed2:
        wolfSSL_CTX_free(ctx);
failed1:
        wolfSSL_Cleanup();

        // strcpy(http_data, "{\"detail\":\"asd\"}");

        // const cJSON *pDetail = NULL;

        cJSON *pJsonRoot = cJSON_Parse(http_data);
        
        if (pJsonRoot) {
            printf("is Json!\n");

            cJSON *pDetail = cJSON_GetObjectItem(pJsonRoot, "detail");
            
            if (cJSON_IsString(pDetail) && (pDetail->valuestring != NULL))
            {
                printf("detail: %s\n", pDetail->valuestring);
            }
            else
            {
                printf("not String!\n");

                printf("detail: %s\n", cJSON_Print(pDetail));

                cJSON *_userID = cJSON_GetObjectItem(pDetail, "deviceName");
                cJSON *_userName = cJSON_GetObjectItem(pDetail, "iotId");
                cJSON *_key = cJSON_GetObjectItem(pDetail, "iotToken");
                cJSON *_productInfo = cJSON_GetObjectItem(pDetail, "productKey");
                cJSON *_broker = cJSON_GetObjectItem(pDetail, "broker");
                cJSON *_uuid = cJSON_GetObjectItem(pDetail, "uuid");

                if (strcmp(BLINKER_MQTT_BORKER_ALIYUN, _broker->valuestring) == 0)
                {
                    // printf("deviceName: %s\n", _userID->valuestring);
                    // printf("deviceName: %d\n", strlen(_userID->valuestring));

                    DEVICE_NAME_MQTT = (char*)malloc((strlen(_userID->valuestring)+1)*sizeof(char));
                    strcpy(DEVICE_NAME_MQTT, _userID->valuestring);
                    MQTT_ID_MQTT = (char*)malloc((strlen(_userID->valuestring)+1)*sizeof(char));
                    strcpy(MQTT_ID_MQTT, _userID->valuestring);
                    MQTT_NAME_MQTT = (char*)malloc((strlen(_userName->valuestring)+1)*sizeof(char));
                    strcpy(MQTT_NAME_MQTT, _userName->valuestring);
                    MQTT_KEY_MQTT = (char*)malloc((strlen(_key->valuestring)+1)*sizeof(char));
                    strcpy(MQTT_KEY_MQTT, _key->valuestring);
                    MQTT_PRODUCTINFO_MQTT = (char*)malloc((strlen(_productInfo->valuestring)+1)*sizeof(char));
                    strcpy(MQTT_PRODUCTINFO_MQTT, _productInfo->valuestring);
                    MQTT_HOST_MQTT = (char*)malloc((strlen(BLINKER_MQTT_ALIYUN_HOST)+1)*sizeof(char));
                    strcpy(MQTT_HOST_MQTT, BLINKER_MQTT_ALIYUN_HOST);

                    MQTT_PORT_MQTT = BLINKER_MQTT_ALIYUN_PORT;

                    UUID_MQTT = (char*)malloc((strlen(_uuid->valuestring)+1)*sizeof(char));
                    strcpy(UUID_MQTT, _uuid->valuestring);

                    printf("DEVICE_NAME_MQTT: %s\n", DEVICE_NAME_MQTT);
                    printf("MQTT_ID_MQTT: %s\n", MQTT_ID_MQTT);
                    printf("MQTT_NAME_MQTT: %s\n", MQTT_NAME_MQTT);
                    printf("MQTT_KEY_MQTT: %s\n", MQTT_KEY_MQTT);
                    printf("MQTT_PRODUCTINFO_MQTT: %s\n", MQTT_PRODUCTINFO_MQTT);
                    printf("MQTT_HOST_MQTT: %s\n", MQTT_HOST_MQTT); 
                    printf("MQTT_PORT_MQTT: %d\n", MQTT_PORT_MQTT);
                    printf("UUID_MQTT: %s\n", UUID_MQTT);

                    isInit = 1;
                }

                cJSON_Delete(pJsonRoot);

                break;
            }
        }
        else {
            printf("not Json!\n");

            cJSON_Delete(pJsonRoot);
        }

        for (int countdown = 10; countdown >= 0; countdown--) {
            printf("%d...\n", countdown);
            vTaskDelay(1000 / portTICK_RATE_MS);
        }

        printf("Starting again!\n");
    }
}

static void messageArrived(MessageData *data)
{
    ESP_LOGI(TAG, "Message arrived[len:%u]: %.*s", \
           data->message->payloadlen, data->message->payloadlen, (char *)data->message->payload);
    ESP_LOGI(TAG, "data from topic: %s", data->topicName->cstring);
}

static void mqtt_client_thread(void *pvParameters)
{
    wolfssl_client();

    while(isInit == 0)
    {
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    char *payload = NULL;
    MQTTClient client;
    Network network;
    int rc = 0;
    char clientID[32] = {0};
    uint32_t count = 0;

    ESP_LOGI(TAG, "ssid:%s passwd:%s sub:%s qos:%u pub:%s qos:%u pubinterval:%u payloadsize:%u",
             CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD, CONFIG_MQTT_SUB_TOPIC,
             CONFIG_DEFAULT_MQTT_SUB_QOS, CONFIG_MQTT_PUB_TOPIC, CONFIG_DEFAULT_MQTT_PUB_QOS,
             CONFIG_MQTT_PUBLISH_INTERVAL, CONFIG_MQTT_PAYLOAD_BUFFER);

    ESP_LOGI(TAG, "ver:%u clientID:%s keepalive:%d username:%s passwd:%s session:%d level:%u",
             CONFIG_DEFAULT_MQTT_VERSION, CONFIG_MQTT_CLIENT_ID,
             CONFIG_MQTT_KEEP_ALIVE, CONFIG_MQTT_USERNAME, CONFIG_MQTT_PASSWORD,
             CONFIG_DEFAULT_MQTT_SESSION, CONFIG_DEFAULT_MQTT_SECURITY);

    ESP_LOGI(TAG, "broker:%s port:%u", CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT);

    ESP_LOGI(TAG, "sendbuf:%u recvbuf:%u sendcycle:%u recvcycle:%u",
             CONFIG_MQTT_SEND_BUFFER, CONFIG_MQTT_RECV_BUFFER,
             CONFIG_MQTT_SEND_CYCLE, CONFIG_MQTT_RECV_CYCLE);

    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

    NetworkInit(&network);

    if (MQTTClientInit(&client, &network, 0, NULL, 0, NULL, 0) == false) {
        ESP_LOGE(TAG, "mqtt init err");
        vTaskDelete(NULL);
    }

    payload = malloc(CONFIG_MQTT_PAYLOAD_BUFFER);

    if (!payload) {
        ESP_LOGE(TAG, "mqtt malloc err");
    } else {
        memset(payload, 0x0, CONFIG_MQTT_PAYLOAD_BUFFER);
    }

    for (;;) {
        ESP_LOGI(TAG, "wait wifi connect...");
        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

        if ((rc = NetworkConnect(&network, CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT)) != 0) {
            ESP_LOGE(TAG, "Return code from network connect is %d", rc);
            continue;
        }

        connectData.MQTTVersion = CONFIG_DEFAULT_MQTT_VERSION;

        // sprintf(clientID, "%s_%u", MQTT_ID_MQTT, esp_random());

        connectData.clientID.cstring = MQTT_ID_MQTT;
        connectData.keepAliveInterval = CONFIG_MQTT_KEEP_ALIVE;

        connectData.username.cstring = MQTT_NAME_MQTT;
        connectData.password.cstring = MQTT_KEY_MQTT;

        connectData.cleansession = CONFIG_DEFAULT_MQTT_SESSION;

        ESP_LOGI(TAG, "MQTT Connecting");

        if ((rc = MQTTConnect(&client, &connectData)) != 0) {
            ESP_LOGE(TAG, "Return code from MQTT connect is %d", rc);
            network.disconnect(&network);
            continue;
        }

        ESP_LOGI(TAG, "MQTT Connected");

        printf("heap:%d\n", esp_get_free_heap_size());

#if defined(MQTT_TASK)

        if ((rc = MQTTStartTask(&client)) != pdPASS) {
            ESP_LOGE(TAG, "Return code from start tasks is %d", rc);
        } else {
            ESP_LOGI(TAG, "Use MQTTStartTask");
        }

#endif

        if ((rc = MQTTSubscribe(&client, CONFIG_MQTT_SUB_TOPIC, CONFIG_DEFAULT_MQTT_SUB_QOS, messageArrived)) != 0) {
            ESP_LOGE(TAG, "Return code from MQTT subscribe is %d", rc);
            network.disconnect(&network);
            continue;
        }

        ESP_LOGI(TAG, "MQTT subscribe to topic %s OK", CONFIG_MQTT_SUB_TOPIC);

        for (;;) {
            if (MQTTIsConnected(&client) == 0)
            {
                break;
            }
            // MQTTMessage message;

            // message.qos = CONFIG_DEFAULT_MQTT_PUB_QOS;
            // message.retained = 0;
            // message.payload = payload;
            // sprintf(payload, "message number %d", ++count);
            // message.payloadlen = strlen(payload);

            // if ((rc = MQTTPublish(&client, CONFIG_MQTT_PUB_TOPIC, &message)) != 0) {
            //     ESP_LOGE(TAG, "Return code from MQTT publish is %d", rc);
            // } else {
            //     ESP_LOGI(TAG, "MQTT published topic %s, len:%u heap:%u", CONFIG_MQTT_PUB_TOPIC, message.payloadlen, esp_get_free_heap_size());
            // }

            // if (rc != 0) {
            //     break;
            // }

            vTaskDelay(1000 / portTICK_RATE_MS);
        }

        network.disconnect(&network);
    }

    ESP_LOGW(TAG, "mqtt_client_thread going to be deleted");
    vTaskDelete(NULL);
    return;
}

void blinker_run(void* pv)
{
    wolfssl_client();

    // while(1)
    // {
    //     vTaskDelay(1000 / portTICK_RATE_MS);
    // }
    vTaskDelete(NULL);

    // ret = 
    // xTaskCreate(&mqtt_client_thread,
    //             MQTT_CLIENT_THREAD_NAME,
    //             MQTT_CLIENT_THREAD_STACK_WORDS,
    //             NULL,
    //             MQTT_CLIENT_THREAD_PRIO,
    //             NULL);

    // if (ret != pdPASS)  {
    //     ESP_LOGE(TAG, "mqtt create client thread %s failed", MQTT_CLIENT_THREAD_NAME);
    // }

    return;
}

void app_main()
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK( nvs_flash_init() );
    // initialise_wifi();

    wifi_init_sta();

    // wolfssl_client();

    // xTaskCreate(blinker_run,
    //             WOLFSSL_DEMO_THREAD_NAME,
    //             WOLFSSL_DEMO_THREAD_STACK_WORDS,
    //             NULL,
    //             WOLFSSL_DEMO_THREAD_PRORIOTY,
    //             NULL);

    ret = xTaskCreate(&mqtt_client_thread,
                      MQTT_CLIENT_THREAD_NAME,
                      MQTT_CLIENT_THREAD_STACK_WORDS,
                      NULL,
                      MQTT_CLIENT_THREAD_PRIO,
                      NULL);

    if (ret != pdPASS)  {
        ESP_LOGE(TAG, "mqtt create client thread %s failed", MQTT_CLIENT_THREAD_NAME);
    }
}
