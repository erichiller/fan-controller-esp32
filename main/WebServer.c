

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include "esp_event.h"

#include <string.h>

#include "WebServer.h"
#include "net_wifi.h"
#include "pwm.h"
#include "config.h"

#define WEB_SERVER_PORT 80
#define LOGT LOG_TAG_WEB_SERVER

#define WEB_SERVER_BUFFER_LEN 2048

#define SERVER_ACK "HTTP/1.1 200 OK\r\n"                               \
	               "Content-Type: text/html\r\n"                       \
	               "Content-Length: 98\r\n\r\n"                        \
	               "<html>\r\n"                                        \
	               "<head>\r\n"                                        \
	               "<title>WEBSERVER example</title></head><body>\r\n" \
	               "WEBSERVER server example!\r\n"                     \
	               "</body>\r\n"                                       \
	               "</html>\r\n"                                       \
	               "\r\n"

#define HEADER "HTTP/1.1 200 OK\r\n"         \
	           "Content-Type: text/html\r\n" \
	           "Content-Length: 225\r\n\r\n"  \
	           "<html>\r\n"                  \
	           "<head>\r\n"                  \
	           "<title>WEBSERVER example</title></head><body>\r\n"

#define FOOTER "</body>\r\n" \
	           "</html>\r\n" \
	           "\r\n"

void WebServerTask( void *arg ) {
	int                ret;
	int                sockfd, new_sockfd;
	socklen_t          addr_len;
	struct sockaddr_in sock_addr;
	char recv_buf[WEB_SERVER_BUFFER_LEN];

	xEventGroupWaitBits( wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY );


	ESP_LOGI( LOGT, "server create socket ......" );
	sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	if( sockfd < 0 ) {
		ESP_LOGI( LOGT, "failed" );
		vTaskDelete( NULL );
		return;
	}
	ESP_LOGI( LOGT, "OK" );


	ESP_LOGI( LOGT, "server socket bind ......" );
	memset( &sock_addr, 0, sizeof( sock_addr ) );
	sock_addr.sin_family      = AF_INET;
	sock_addr.sin_addr.s_addr = 0;
	sock_addr.sin_port        = htons( WEB_SERVER_PORT );
	ret                       = bind( sockfd, (struct sockaddr *)&sock_addr, sizeof( sock_addr ) );
	if( ret ) {
		ESP_LOGI( LOGT, "failed" );
		close( sockfd );
		sockfd = -1;
		vTaskDelete( NULL );
		return;
	}
	ESP_LOGI( LOGT, "OK" );


	ESP_LOGI( LOGT, "server socket listen ......" );
	ret = listen( sockfd, 32 );
	if( ret ) {
		ESP_LOGI( LOGT, "failed" );
		close( sockfd );
		sockfd = -1;
		vTaskDelete( NULL );
		return;
	}
	ESP_LOGI( LOGT, "OK" );

	do {
		ESP_LOGI( LOGT, "server socket accept client ......" );
		new_sockfd = accept( sockfd, (struct sockaddr *)&sock_addr, &addr_len );
		if( new_sockfd < 0 ) {
			ESP_LOGI( LOGT, "accept failed" );
		}
		ESP_LOGI( LOGT, "OK" );

		ESP_LOGI( LOGT, "server read message ......" );
		do {
			memset( recv_buf, 0, WEB_SERVER_BUFFER_LEN );
			ret = read( new_sockfd, recv_buf, WEB_SERVER_BUFFER_LEN - 1 );
			if( ret <= 0 ) {
				break;
			}
			ESP_LOGI( LOGT, "read: %s", recv_buf );
			if( strstr( recv_buf, "GET " ) &&
			    strstr( recv_buf, " HTTP/1.1" ) ) {
				ESP_LOGI( LOGT, "get matched message" );
				ESP_LOGI( LOGT, "write message" );
				ESP_LOGI( LOGT, "write header" );

				char header[] = HEADER;
				WebServer_Write( &new_sockfd, header, sizeof(header));


				if( strstr( recv_buf, "/increment_duty_1p" ) ) {
					set_duty_percent( PWM_PERCENT + 1 );
				}
				if( strstr( recv_buf, "/increment_duty_10p" ) ) {
					set_duty_percent( PWM_PERCENT + 10 );
				}
				if( strstr( recv_buf, "/decrement_duty_1p" ) ) {
					set_duty_percent( PWM_PERCENT - 1 );
				}
				if( strstr( recv_buf, "/decrement_duty_10p" ) ) {
					set_duty_percent( PWM_PERCENT - 10 );
				}

				WebServer_SendStatus( &new_sockfd );

				ESP_LOGI( LOGT, "write footer" );
				
				char footer[] = FOOTER;
				WebServer_Write( &new_sockfd, footer, sizeof(footer));

				break;
			}
		} while( 1 );
		close( new_sockfd );
		new_sockfd = -1;
	} while( 1 );
}

// void WebServer_SendHeader( int *sock ){
// 	char header[] = HEADER;
// 	WebServer_Write(sock, header, sizeof(header));
// }

// void WebServer_SendFooter( int *sock ){
// 	char footer[] = FOOTER;
// 	WebServer_Write(sock, footer, sizeof(footer));
// }

void WebServer_Write( int *sock, char *send_data, int send_bytes ) {
	int count_len, ret;
	ESP_LOGD( LOGT, "WebServer_Write :: sock=%i ;; data_length=%i ;; send_data=%s", *sock, send_bytes, send_data);
	while( ( ret = write( *sock, send_data, send_bytes ) ) <= 0 ) {
		if( ret != 0 ) {
			ESP_LOGE( LOGT, " failed\n  ! write returned %d\n\n", ret );
		}
		ESP_LOGV( LOGT, "write return=%i and errno=%i\n", ret, errno );
		count_len += ret;
	}
	if( ret > 0 ) {
		ESP_LOGI( LOGT, "OK, return=%i", ret )
	} else {
		ESP_LOGI( LOGT, "error, return=%i", ret )
	}
}

void WebServer_SendStatus( int *new_sockfd ) {
	ESP_LOGI( LOGT, "write Status" );
	char buf[WEB_SERVER_BUFFER_LEN];
	int len = sprintf( buf, "<b>STATUS:</b><br />DUTY: %i<br /><br /><a href=\"/H\">UP</a><a href=\"/HT\">+10</a><br /><br /><a href=\"/L\">DOWN</a><a href=\"/LT\">-10</a><br>", ledc_channel.duty );
	WebServer_Write(new_sockfd, buf, len);
}