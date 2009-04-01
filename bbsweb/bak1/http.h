#include <time.h>
#include "bbs.h"

/* 
	define HTTP Status-Line
	
 1xx: Informational - Request received, continuing process
 2xx: Success - The action was successfully received, understood, and accepted
 3xx: Redirection - Further action must be taken in order to complete the request
 4xx: Client Error - The request contains bad syntax or cannot be fulfilled
 5xx: Server Error - The server failed to fulfill an apparently valid request

	Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
*/
#define HTTP_OK 					"HTTP/1.0 200 OK"
#define HTTP_CREATED				"HTTP/1.0 201 Created"
#define HTTP_MOVED_PERMANENTLY		"HTTP/1.0 301 Moved Permanently"
#define HTTP_NOT_MODIFIED			"HTTP/1.0 304 Not Modified"
#define HTTP_BAD_REQUEST			"HTTP/1.0 400 Bad Request"
#define HTTP_UNAUTHORIZED			"HTTP/1.0 401 Authorization Required"
#define HTTP_FORBIDDEN				"HTTP/1.0 403 Forbidden"
#define HTTP_FILE_NOT_FOUND			"HTTP/1.0 404 File Not Found"
#define HTTP_LENGTH_REQUIRED		"HTTP/1.0 411 Length Required"
#define HTTP_ENTITY_TOO_LARGE		"HTTP/1.0 413 Request Entity Too Large"
#define HTTP_NOT_IMPLEMENTED 		"HTTP/1.0 501 Method Not Implemented"

#define MAX_HTTP_HEADER (30)
#define MAX_HTTP_REQUEST_PER_CHILD	(30)
#define URI_LEN						512					/* Bytes */
#define PROTO_LEN					32					/* Bytes */

typedef struct 
{
/* == Client private data  == */
	int URLParaType;
	int WebRespondType;
	int HttpRequestType;
	int HttpRespondType;
	int num_request;
	time_t atime;						/* time when access occurs */
	char fromhost[HOSTLEN];
	char proxyhost[HOSTLEN];
/* == HTTP request header field == */
	BOOL connection;
	int content_length;					/* form content length */
	time_t if_modified_since;
	char cookie[STRLEN*2];
	char authorization[STRLEN];
	char host[STRLEN];					/* host name string */
	char cache_control[STRLEN];
	char pragma[STRLEN];
	char content_type[STRLEN];
	char x_forward_for[STRLEN*2];
	char referer[STRLEN*2];
	char via[STRLEN*3];
	char user_agent[STRLEN*2];
	unsigned char mark1;
/* =============================== */
	char request_method[PROTO_LEN];
	char URI[URI_LEN];
	unsigned char mark2;
}REQUEST_REC;


enum http_request_method
{ 
	GET, POST, HEAD, CERTILOG, UNKNOW
};

typedef struct 
{
	char *method;
} HTTP_REQUEST;

typedef struct
{
	char *status_line;
	BOOL Expires;
	BOOL Pragma;
	BOOL Location;
	BOOL Content_Type;
	BOOL Content_Length;
	BOOL Last_Modified;
	BOOL WWW_Authenticate;
	BOOL Allow;
}HTTP_HEADER;


typedef struct 
{
	char *ext;
	char *type;

} MIME_TYPE;

enum http_respond_type
{
	OK 						, 
	MOVED_PERMANENTLY	 	, 
	NOT_MODIFIED 			, 
	BAD_REQUEST 			, 
	AUTHORIZATION_REQUIRED 	, 
	FORBIDDEN				,
	FILE_NOT_FOUND 			, 
	METHOD_NOT_IMPLEMENTED 	
};
