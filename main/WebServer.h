#ifndef WEB_SERVER_H
#define WEB_SERVER_H

void WebServerTask( void *arg );
void WebServer_SendStatus( int *new_sockfd );
void WebServer_Write( int *sock, char *send_data, int send_bytes );

#endif