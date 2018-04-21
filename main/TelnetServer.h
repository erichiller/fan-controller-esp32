
#ifndef TELNET_H
#define TELNET_H

#define DIR_MAX 8
#define COMMAND_MAX 8


/*************
** Commands
**************/
typedef enum
{
	TS_WRITE_FAILED      = -1,
	TS_OK                = 0,
	TS_UNDEFINED_VALUE   = 1,
	TS_INVALID_SET_VALUE = 2,
	TS_GET_VALUE_NOT_SET = 3,
	TS_INVALID_COMMAND   = 4
} telnet_server_err_t;

// telnet_server_command_cb
// pass
typedef telnet_server_err_t ( *telnet_server_command_cb )( int *socket, char *read_buf, int read_buf_len, char *command, telnet_server_command_dir_t *curr_cmd_dir );

typedef struct
{
	char                     name[16];
	telnet_server_command_cb command;
} telnet_server_command_t;


// typedef union
// {
// 	telnet_server_command_t command;
// 	telnet_server_command_t commands[COMMAND_MAX];
// } telnet_server_command_node_t;



struct telnet_server_command_dir_t
{
	char                                name[16];
	int                                 command_count;
	telnet_server_command_t             commands[COMMAND_MAX];
	int                                 child_count;
	struct telnet_server_command_dir_t *children[DIR_MAX];
	struct telnet_server_command_dir_t *parent;    // set as hierarchy is traversed
};

typedef struct telnet_server_command_dir_t telnet_server_command_dir_t;

void TelnetServerTask( void *arg );

telnet_server_err_t TelnetServer_Write( int *socket, char *send_data, int send_bytes );

telnet_server_err_t TelnetServer_CommandCall( int *socket, char *commandBuf, int commandBuf_len, char *command, telnet_server_command_dir_t *curr_cmd_dir );

void TelnetServer_CommandParse( int *socket, char *commandBuf, int commandBuf_len );

telnet_server_err_t TelnetServer_EchoCommand( int *socket, char *recv_buf, int recv_len, char *command, telnet_server_command_dir_t *curr_cmd_dir );

#endif
