/***************************
 **** WiFi
 * See also:
 * http://esp-idf.readthedocs.io/en/latest/api-reference/wifi/esp_wifi.html
 ***************************/

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_event_loop.h"

#include "net_wifi.h"
#include "secrets.h"
#include "config.h"

#define LOGT "WiFi"
#define LOGT_LEVEL ESP_LOG_DEBUG

EventGroupHandle_t wifi_event_group = NULL;

int CONNECTED_BIT = BIT0;

void log_wifi_init( ) {
	esp_log_level_set( LOGT, LOGT_LEVEL );
}


int net_wifi_connect( ) {

	tcpip_adapter_init( );
	ESP_LOGI( LOGT, "tcpip adapter initialized" );

	wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK( esp_event_loop_init( WiFiEvent, NULL ) );
	ESP_LOGI( LOGT, "WIFI event loop initialized" );

	wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT( );
	
	ESP_LOGI( LOGT, "WIFI config created" );

	ESP_ERROR_CHECK( esp_wifi_init( &wifi_config ) );
	ESP_LOGI( LOGT, "WIFI config initialized" );

	// ESP_ERROR_CHECK( esp_wifi_set_storage( WIFI_STORAGE_RAM ) );
	// ESP_LOGI( LOGT, "WIFI storage set to RAM" );



	ESP_LOGI(LOGT, "Setting WiFi configuration SSID %s...", sta_config.sta.ssid);
	ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ) );
	ESP_LOGI( LOGT, "WIFI station mode set" );

	ESP_ERROR_CHECK( esp_wifi_set_config( WIFI_IF_STA, &sta_config ) );
	ESP_LOGI( LOGT, "WIFI config attached" );

	ESP_ERROR_CHECK( esp_wifi_start( ) );
	ESP_LOGI( LOGT, "WIFI interface started" );

	return 0;
}


uint32_t wifi_get_local_ip( void ) {
	tcpip_adapter_if_t		ifx = TCPIP_ADAPTER_IF_STA;
	tcpip_adapter_ip_info_t ip_info;
	wifi_mode_t				mode;

	esp_wifi_get_mode( &mode );
	if( WIFI_MODE_STA == mode ) {
		tcpip_adapter_get_ip_info( ifx, &ip_info );
		return ip_info.ip.addr;
	}
	return 0;
}

void wifi_get_local_ip_str( char* buf_ip_addr_string ) {
	tcpip_adapter_if_t		ifx = TCPIP_ADAPTER_IF_STA;
	tcpip_adapter_ip_info_t ip_info;
	wifi_mode_t				mode;
	// uint32_t				ip;

	esp_wifi_get_mode( &mode );
	if( WIFI_MODE_STA == mode ) {
		tcpip_adapter_get_ip_info( ifx, &ip_info );
		sprintf(buf_ip_addr_string, "%s", ip4addr_ntoa(&ip_info.ip));
	} else {
		buf_ip_addr_string = "0";
	}	
}

/**
 * Event Descriptions Here
 * http://esp-idf.readthedocs.io/en/latest/api-guides/wifi.html
 * 
**/
esp_err_t WiFiEvent( void *ctx, system_event_t *event ) {
	printf( "[WiFi-event] event: %i\n", (int)event->event_id );
	switch( (int)event->event_id ) {
		case SYSTEM_EVENT_WIFI_READY:	// ESP32 WiFi ready
			ESP_LOGI( LOGT, "SYSTEM_EVENT_WIFI_READY" );
			return ESP_OK;
			break;
		case SYSTEM_EVENT_SCAN_DONE:	// ESP32 finish scanning AP
			ESP_LOGI( LOGT, "SYSTEM_EVENT_SCAN_DONE" );
			return ESP_OK;
			break;
		case SYSTEM_EVENT_STA_START:	// ESP32 station start
			ESP_LOGI( LOGT, "SYSTEM_EVENT_STA_START" );
			ESP_ERROR_CHECK( esp_wifi_connect( ) );
			ESP_LOGI( LOGT, "WIFI interface being connected" );
			return ESP_OK;
			break;
		case SYSTEM_EVENT_STA_STOP:	// ESP32 station stop
			ESP_LOGI( LOGT, "SYSTEM_EVENT_STA_STOP" );
			return ESP_OK;
			break;
		case SYSTEM_EVENT_STA_CONNECTED:	// ESP32 station connected to AP
			ESP_LOGI( LOGT, "SYSTEM_EVENT_STA_CONNECTED" );
			return ESP_OK;
			break;
		case SYSTEM_EVENT_STA_DISCONNECTED:	// ESP32 station disconnected from AP
			ESP_LOGI( LOGT, "SYSTEM_EVENT_STA_DISCONNECTED" );
			
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);	
			ESP_ERROR_CHECK( esp_wifi_connect( ) );
			//return ESP_FAIL;
			return ESP_OK;
			break;
		case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:	// the auth mode of AP connected by ESP32 station changed
			ESP_LOGW( LOGT, "SYSTEM_EVENT_STA_AUTHMODE_CHANGE" );
			return event->event_id;
		/**
		 * RECEIVED IP
		 * Good to go ahead and do things
		 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		 **/
		case SYSTEM_EVENT_STA_GOT_IP:	// ESP32 station got IP from connected AP
			ESP_LOGI( LOGT, "SYSTEM_EVENT_STA_GOT_IP" );
			printf( "got ip:%s\n",
					ip4addr_ntoa( &event->event_info.got_ip.ip_info.ip ) );
			xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
			return ESP_OK;
			break;
		case SYSTEM_EVENT_STA_LOST_IP:	// ESP32 station lost IP and the IP is reset to 0
			ESP_LOGE( LOGT, "SYSTEM_EVENT_STA_LOST_IP" );
			//return ESP_FAIL;
			return ESP_OK;
			break;
		case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:	// ESP32 station wps succeeds in enrollee mode	// ESP32 station wps fails in enrollee mode
			ESP_LOGI( LOGT, "SYSTEM_EVENT_STA_WPS_ER_SUCCESS" );
			return ESP_OK;
			break;
		case SYSTEM_EVENT_STA_WPS_ER_FAILED:	// ESP32 station wps fails in enrollee mode
			ESP_LOGE( LOGT, "SYSTEM_EVENT_STA_WPS_ER_FAILED" );
			//return ESP_FAIL;
			return ESP_OK;
			break;
		case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:	// ESP32 station wps timeout in enrollee mode
			ESP_LOGE( LOGT, "SYSTEM_EVENT_STA_WPS_ER_TIMEOUT" );
			//return ESP_FAIL;
			return ESP_OK;
			break;
		case SYSTEM_EVENT_STA_WPS_ER_PIN:	// ESP32 station wps pin code in enrollee mode
			ESP_LOGE( LOGT, "SYSTEM_EVENT_ETH_DISCONNECTED" );
			//return ESP_FAIL;
			return ESP_OK;
			break;
		case SYSTEM_EVENT_AP_START:	// ESP32 soft-AP start
			ESP_LOGI( LOGT, "SYSTEM_EVENT_AP_START" );
			return ESP_OK;
			break;
		case SYSTEM_EVENT_AP_STOP:	// ESP32 soft-AP stop
			ESP_LOGI( LOGT, "SYSTEM_EVENT_AP_STOP" );
			return ESP_OK;
			break;
		case SYSTEM_EVENT_AP_STACONNECTED:	// a station connected to ESP32 soft-AP
			ESP_LOGI( LOGT, "SYSTEM_EVENT_AP_STACONNECTED" );
			return ESP_OK;
			break;
		case SYSTEM_EVENT_AP_STADISCONNECTED:	// a station disconnected from ESP32 soft-AP
			ESP_LOGI( LOGT, "SYSTEM_EVENT_AP_STADISCONNECTED" );
			return ESP_OK;
			break;
		case SYSTEM_EVENT_AP_PROBEREQRECVED:	// Receive probe request packet in soft-AP interface
			ESP_LOGI( LOGT, "SYSTEM_EVENT_AP_PROBEREQRECVED" );
			return ESP_OK;
			break;
		case SYSTEM_EVENT_GOT_IP6:	// ESP32 station or ap or ethernet interface v6IP addr is preferred
			ESP_LOGI( LOGT, "SYSTEM_EVENT_GOT_IP6" );
			return ESP_OK;
			break;
		case SYSTEM_EVENT_ETH_START:	// ESP32 ethernet start
			ESP_LOGI( LOGT, "SYSTEM_EVENT_ETH_START" );
			return ESP_OK;
			break;
		case SYSTEM_EVENT_ETH_STOP:	// ESP32 ethernet stop
			ESP_LOGI( LOGT, "SYSTEM_EVENT_ETH_STOP" );
			return ESP_OK;
			break;
		case SYSTEM_EVENT_ETH_CONNECTED:	// ESP32 ethernet phy link up
			ESP_LOGI( LOGT, "SYSTEM_EVENT_ETH_CONNECTED" );
			return ESP_OK;
			break;
		case SYSTEM_EVENT_ETH_DISCONNECTED:	// ESP32 ethernet phy link down
			ESP_LOGE( LOGT, "SYSTEM_EVENT_ETH_DISCONNECTED" );
			//return ESP_FAIL;
			return ESP_OK;
			break;
		case SYSTEM_EVENT_ETH_GOT_IP:	// ESP32 ethernet got IP from connected AP
			ESP_LOGI( LOGT, "SYSTEM_EVENT_ETH_GOT_IP" );
			return ESP_OK;
			break;
		case SYSTEM_EVENT_MAX:
			ESP_LOGE( LOGT, "SYSTEM_EVENT_MAX" );
			//return ESP_FAIL;
			return ESP_OK;
			break;
		default:
			ESP_LOGE( LOGT, "UNKNOWN WiFi EVENT HAS OCCURRED" );
			//return ESP_FAIL;
			return ESP_OK;
			break;
	}
}