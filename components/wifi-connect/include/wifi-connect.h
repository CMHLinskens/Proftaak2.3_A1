/* Common functions for protocol examples, to establish Wi-Fi or Ethernet connection.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

   Edited by A1
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "esp_netif.h"

/**
 * @brief Configure Wi-Fi and connect
 *
 * Used to make a connection to the internet.
 *
 * @return ESP_OK on successful connection
 */
esp_err_t wifi_connect(void);

/**
 * Counterpart to connect, de-initializes Wi-Fi
 *  
 * @return ESP_OK on successful disconnect
 */
esp_err_t wifi_disconnect(void);

/**
 * @brief Returns esp-netif pointer created by connect()
 *
 * @note If multiple interfaces active at once, this API return NULL
 * In that case the get_netif_from_desc() should be used
 * to get esp-netif pointer based on interface description
 * 
 * @return esp_netif_t
 */
esp_netif_t *get_netif(void);

/**
 * @brief Returns esp-netif pointer created by connect() described by
 * the supplied desc field
 *
 * @param desc Textual interface of created network interface, for example "sta"
 * indicate default WiFi station, "eth" default Ethernet interface.
 *
 * @return esp_netif_t from description
 */
esp_netif_t *get_netif_from_desc(const char *desc);

#ifdef __cplusplus
}
#endif
