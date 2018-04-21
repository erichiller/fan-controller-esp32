

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include "esp_event.h"

#include <string.h>

#include "TelnetServer.h"
#include "net_wifi.h"
#include "pwm.h"
#include "temp.h"
#include "config.h"
#include "ctype.h"


#define WELCOME "***********************************************\r\n"        \
	            "**  __    __     _                           **\r\n"        \
	            "** / / /\\ \\ \\___| | ___ ___  _ __ ___   ___  **\r\n"     \
	            "** \\ \\/  \\/ / _ \\ |/ __/ _ \\| '_ ` _ \\ / _ \\ **\r\n" \
	            "**  \\  /\\  /  __/ | (_| (_) | | | | | |  __/ **\r\n"      \
	            "**   \\/  \\/ \\___|_|\\___\\___/|_| |_| |_|\\___| **\r\n"  \
	            "***********************************************\r\n"        \
	            "***************  Fan Controller ***************\r\n"        \
	            "***********************************************\r\n"        \
	            "\r\n"

#define TELNET_SERVER_PORT 23
#define LOGT LOG_TAG_TELNET_SERVER

#define TELNET_SERVER_BUFFER_LEN 2048



telnet_server_err_t TelnetServer_cmdPrintHelp( int *socket, char *read_buf, int read_buf_len, char *command, telnet_server_command_dir_t *curr_cmd_dir ) {
	ESP_LOGD( LOGT, "TelnetServer_cmdPrintHelp" );
	char  write_buf[1024];
	int   buf_at      = 0;
	char *buf_pointer = &write_buf[buf_at];
	int   d, c;
	buf_at += sprintf( buf_pointer, "**** %s Directory Information ****\r\n", curr_cmd_dir->name );
	buf_pointer = &write_buf[buf_at];
	buf_at += sprintf( buf_pointer, "Child Directories:\r\n" );
	buf_pointer = &write_buf[buf_at];
	for( d = 0; d < curr_cmd_dir->child_count; d++ ) {
		buf_at += sprintf( buf_pointer, "\t%s", curr_cmd_dir->children[d]->name );
		buf_pointer = &write_buf[buf_at];
	}
	for( c = 0; c < curr_cmd_dir->command_count; c++ ) {
		buf_at += sprintf( buf_pointer, "\t%s", curr_cmd_dir->commands[c].name );
		buf_pointer = &write_buf[buf_at];
	}
	return TelnetServer_Write( socket, write_buf, buf_at );
}

telnet_server_err_t TelnetServer_cmdSystemStatus( int *socket, char *read_buf, int read_buf_len, char *command, telnet_server_command_dir_t *curr_cmd_dir ) {
	ESP_LOGD( LOGT, "TelnetServer_cmdStatus" );
	char buf[128];
	int  new_pwm_percent = atoi( strrchr( read_buf, ' ' ) );
	int  len             = sprintf( buf, "STATUS:\r\n\tDUTY:        %i/%i\r\n\tPERCENT:     %i\r\n\tTEMP:        %f F, %f C\r\n", ledc_channel.duty, ledc_timer.duty_resolution, PWM_PERCENT, farenheit, centigrade );

	return TS_OK;
}


telnet_server_err_t TelnetServer_cmdDutySet( int *socket, char *read_buf, int read_buf_len, char *command, telnet_server_command_dir_t *curr_cmd_dir ) {
	ESP_LOGD( LOGT, "TelnetServer_cmdDutySet" );
	char buf[64];
	int  new_pwm_percent = atoi( strrchr( read_buf, ' ' ) );
	int  len             = sprintf( buf, "Setting new duty cycle percentage to: %i", new_pwm_percent );
	ESP_LOGI( LOGT, "Setting new duty cycle percentage to: %i", new_pwm_percent );
	set_duty_percent( new_pwm_percent );
	return TelnetServer_Write( socket, buf, len );
}
telnet_server_err_t TelnetServer_cmdDutyGet( int *socket, char *read_buf, int read_buf_len, char *command, telnet_server_command_dir_t *curr_cmd_dir ) {
	ESP_LOGD( LOGT, "TelnetServer_cmdDutyGet" );
	char buf[64];
	int  new_pwm_percent = atoi( strrchr( read_buf, ' ' ) );
	int  len             = sprintf( buf, "STATUS:\r\n\tDUTY:        %i/%i\r\n\tPERCENT:     %i\r\n\t", ledc_channel.duty, ledc_timer.duty_resolution, PWM_PERCENT );
	return TelnetServer_Write( socket, buf, len );
}

/* clang-format off */

telnet_server_command_dir_t ts_cmd_dir_duty = {
	.name = "duty",
	.command_count = 3,
	.commands = {
					{
						.name = "get", 
						.command = TelnetServer_cmdDutyGet
					},
					{
						.name = "set", 
						.command = TelnetServer_cmdDutySet
					},
					{
						.name = "help", 
						.command = TelnetServer_cmdPrintHelp
					}
				},
	.child_count = 0
};

telnet_server_command_dir_t ts_cmd_dir_system = {
	.name = "system",
	.command_count = 2,
	.commands = {                                                           // with its own set of commands within
					{
						.name = "help", 
						.command = TelnetServer_cmdPrintHelp
					},    // each with defined named and handlers
					{
						.name = "status", 
						.command = TelnetServer_cmdSystemStatus
					}
				},
	.child_count = 0
};

// init
// https://stackoverflow.com/questions/1761809/nested-struct-variable-initialization
// hierarachy of commands
// commands in this level, can be a list of up to size defined in COMMAND_MAX
telnet_server_command_dir_t ts_cmds = {
	.name = "top",
	.command_count = 2,
    .commands = {
                 {
                     .name    = "help",                      // command name
                     .command = TelnetServer_cmdPrintHelp    // function handler for command
                 },
				 {
					 .name    = "echo",
					 .command = TelnetServer_EchoCommand
				 },
	},
	.child_count = 1,
    .children     = { &ts_cmd_dir_duty, &ts_cmd_dir_system }
};
/* clang-format on */

void TelnetServerTask( void *arg ) {
	int                ret;
	int                sockfd, new_sockfd;
	socklen_t          addr_len;
	struct sockaddr_in sock_addr;
	char               recv_buf[TELNET_SERVER_BUFFER_LEN];
	char *             buf_ptr = &recv_buf[0];
	int                buf_len = 0;

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
	sock_addr.sin_port        = htons( TELNET_SERVER_PORT );
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
		ESP_LOGI( LOGT, "write welcome" );

		char welcome[] = WELCOME;
		TelnetServer_Write( &new_sockfd, welcome, sizeof( welcome ) );

		ESP_LOGI( LOGT, "server read message ......" );
		do {
			ret = read( new_sockfd, buf_ptr, TELNET_SERVER_BUFFER_LEN - 1 - buf_len );
			if( ret <= 0 ) {
				break;
			}
			buf_len += ret;

			if( buf_len > 1024 ) {
				ESP_LOGW( LOGT, "the buffer is currently %i bytes, this seems excessive", buf_len );
			}

			ESP_LOGI( LOGT, "read <%i bytes,buf@%i>: %s", ret, buf_len, recv_buf );
			// char *nptr = strpbrk(recv_buf, ";\n\r");
			int nchar = strcspn( recv_buf, ";\n\r" );
			if( nchar < buf_len ) {
				ESP_LOGI( LOGT, "ending char @%i found during read.", strcspn( recv_buf, ";\n\r" ) );
				ESP_LOGI( LOGT, "proceeding to process command" );
				recv_buf[nchar] = '\0';
				TelnetServer_CommandParse( &new_sockfd, recv_buf, buf_len );
				// reset buf_len and ptr
				buf_len = 0;
				buf_ptr = &recv_buf[0];
				memset( recv_buf, 0, TELNET_SERVER_BUFFER_LEN );
			} else {
				ESP_LOGD( LOGT, "INCREMENTING buf_ptr" );
				buf_ptr = &recv_buf[buf_len];
			}
		} while( 1 );
		close( new_sockfd );
		new_sockfd = -1;
	} while( 1 );
}


telnet_server_err_t TelnetServer_Write( int *socket, char *send_data, int send_bytes ) {
	int count_len, ret;
	ESP_LOGD( LOGT, "TelnetServer_Write :: socket=%i ;; data_length=%i ;; send_data=%s", *socket, send_bytes, send_data );
	while( ( ret = write( *socket, send_data, send_bytes ) ) <= 0 ) {
		if( ret != 0 ) {
			ESP_LOGE( LOGT, " failed\n  ! write returned %d\n\n", ret );
		}
		ESP_LOGV( LOGT, "write return=%i and errno=%i\n", ret, errno );
		count_len += ret;
	}
	if( ret > 0 ) {
		ESP_LOGI( LOGT, "OK, return=%i", ret )
	} else {
		ESP_LOGE( LOGT, "error, return=%i", ret )
		return (telnet_server_err_t)ret;
	}
	return TS_OK;
}

telnet_server_err_t TelnetServer_CommandCall( int *socket, char *commandBuf, int commandBuf_len, char *command, telnet_server_command_dir_t *curr_cmd_dir ) {
	int i, c, d;
	ESP_LOGD( LOGT, "TelnetServer_CommandCall: processing command: '%s'\n\t strlen command: %i\n\t current dir: '%s'", command, strlen( command ), curr_cmd_dir->name );
	ESP_LOGD( LOGT, "Length of (curr_cmd_dir->children) = %i", ( sizeof( curr_cmd_dir->children ) / sizeof( *curr_cmd_dir->children ) ) );
	ESP_LOGD( LOGT, "Length of (curr_cmd_dir->*commands) = %i", ( sizeof( curr_cmd_dir->commands ) / sizeof( *curr_cmd_dir->commands ) ) );

	for( i = 0; i < curr_cmd_dir->command_count + curr_cmd_dir->child_count; i++ ) {
		// ALWAYS try to match dirs first. So that cmds like: dir cmd dir do not cause cmd to exec
		for( d = 0; d < curr_cmd_dir->child_count; d++ ) {
			ESP_LOGD( LOGT, "TelnetServer_CommandCall: checking against directory #%i", d );
			ESP_LOGD( LOGT, "TelnetServer_CommandCall: checking '%s' against directory: '%s'", command, curr_cmd_dir->children[d]->name );

			if( strcmp( curr_cmd_dir->children[d]->name, command ) == 0 ) {
				ESP_LOGI( LOGT, "TelnetServer_CommandCall: entering directory command: '%s'", curr_cmd_dir->children[d]->name );
				curr_cmd_dir = curr_cmd_dir->children[d];
				return (telnet_server_err_t)TS_OK;
			}
		}
		for( c = 0; c < curr_cmd_dir->command_count; c++ ) {
			ESP_LOGD( LOGT, "TelnetServer_CommandCall: checking against command #%i", c );
			ESP_LOGD( LOGT, "TelnetServer_CommandCall: checking '%s' against command: '%s'", command, curr_cmd_dir->commands[c].name );

			if( strcmp( curr_cmd_dir->commands[c].name, command ) == 0 ) {
				ESP_LOGI( LOGT, "TelnetServer_CommandCall: calling command: '%s'", curr_cmd_dir->commands[c].name );
				return curr_cmd_dir->commands[c].command( socket, commandBuf, commandBuf_len, command, curr_cmd_dir );
			}
		}
	}
	return (telnet_server_err_t)TS_INVALID_COMMAND;
}


void TelnetServer_CommandParse( int *socket, char *commandBuf, int commandBuf_len ) {
	ESP_LOGD( LOGT, "parsing command: %s", commandBuf );
	char *token, *str, *tofree;
	tofree = str                             = strdup( tolower( commandBuf ) );
	telnet_server_command_dir_t curr_cmd_dir = ts_cmds;
	ESP_LOGD( LOGT, "str: %s", str );
	while( ( token = strsep( &str, " " ) ) ) {
		ESP_LOGD( LOGT, "Token from strsep: %s", token );
		if( TelnetServer_CommandCall( socket, commandBuf, commandBuf_len, token, &curr_cmd_dir ) != (telnet_server_err_t)TS_OK ) {
			ESP_LOGE( LOGT, "Error while processing command: %s", commandBuf );
		}
	}
	free( tofree );
}


telnet_server_err_t TelnetServer_EchoCommand( int *socket, char *recv_buf, int recv_len, char *command, telnet_server_command_dir_t *curr_cmd_dir ) {
	const int line_len   = 84;
	const int line_count = 4;
	const int buf_len    = ( line_len + 2 ) * line_count;
	// write 4 lines at a time
	// i
	// HH
	// ddd
	// c   |
	// consumes 4 chars
	// 4 lines + CR + NL at each end
	char buf[buf_len];
	int  buf_idx = 0;

	sprintf( &buf[( ( line_len + 2 ) * 1 ) - 2], "%s", "&\r\n%" );
	sprintf( &buf[( ( line_len + 2 ) * 2 ) - 2], "%s", "&\r\n%" );
	sprintf( &buf[( ( line_len + 2 ) * 3 ) - 2], "%s", "&\r\n%" );
	sprintf( &buf[( ( line_len + 2 ) * 4 ) - 2], "%s", "&\r\n%" );
	for( int i = 0; i < recv_len; i++ ) {
		for( buf_idx = 0; buf_idx <= line_len - 6 && i < recv_len; buf_idx += 6, i++ ) {
			sprintf( &buf[buf_idx], "%-3i.  ", i );
			sprintf( &buf[buf_idx + ( ( line_len + 2 ) * 1 )], "%#-2x  ", recv_buf[i] );
			sprintf( &buf[buf_idx + ( ( line_len + 2 ) * 2 )], "%-6u.", recv_buf[i] );
			if( recv_buf[i] >= 32 && recv_buf[i] < 127 ) {
				sprintf( &buf[buf_idx + ( ( line_len + 2 ) * 3 )], "%-6c.", recv_buf[i] );
			} else {
				sprintf( &buf[buf_idx + ( ( line_len + 2 ) * 3 )], "noop  " );
			}
		}
		// fill out buff len
		for( ; buf_idx <= line_len - 6; buf_idx += 6 ) {
			memset( &buf[buf_idx], 32, ( line_len + 2 ) - buf_idx );
			memset( &buf[( ( ( line_len + 2 ) * 1 ) - 2 )], 13, 1 );
			memset( &buf[( ( ( line_len + 2 ) * 1 ) - 1 )], 10, 1 );

			memset( &buf[buf_idx + ( ( line_len + 2 ) * 1 )], 32, ( line_len + 2 ) - buf_idx );
			memset( &buf[( ( ( line_len + 2 ) * 2 ) - 2 )], 13, 1 );
			memset( &buf[( ( ( line_len + 2 ) * 2 ) - 1 )], 10, 1 );

			memset( &buf[buf_idx + ( ( line_len + 2 ) * 2 )], 32, ( line_len + 2 ) - buf_idx );
			memset( &buf[( ( ( line_len + 2 ) * 3 ) - 2 )], 13, 1 );
			memset( &buf[( ( ( line_len + 2 ) * 3 ) - 1 )], 10, 1 );

			memset( &buf[buf_idx + ( ( line_len + 2 ) * 3 )], 32, ( line_len + 2 ) - buf_idx );
			memset( &buf[( ( ( line_len + 2 ) * 4 ) - 2 )], 13, 1 );
			memset( &buf[( ( ( line_len + 2 ) * 4 ) - 1 )], 10, 1 );

			// sprintf( &buf[buf_idx], "%s", "      " );
			// sprintf( &buf[buf_idx + ( ( line_len * 1 ) + 2 )], "%s", "      " );
			// sprintf( &buf[buf_idx + ( ( line_len * 2 ) + 4 )], "%s", "      " );
			// sprintf( &buf[buf_idx + ( ( line_len * 3 ) + 6 )], "%s", "      " );
		}
		return TelnetServer_Write( socket, buf, buf_len );
	}
	return TS_OK;
}
