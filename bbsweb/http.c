
#include "bbs.h"
#include "bbsweb.h"
#include "bbswebproto.h"
#include "log.h"

extern FILE *fp_in;

#ifdef TORNADO_OPTIMIZE
extern int isTORNADO;
#endif

extern int port;

#include "base64_decode.c"

#define MAX_HTTP_HEADER (30)

#ifdef KEEP_ALIVE
#define MAX_HTTP_REQUEST_PER_CHILD	(30)
#endif


static int
GetHttpRequestType(char *request)
{
	int type;
	static struct _http_request
	{
		char *method;
	} http_request[] =
	{
		{"GET"},
		{"POST"},
		{"HEAD"},
		{"CERTILOG"},		/* csbbs protocol */
		{"KMP"},
		{NULL}
	};


	for (type = 0; http_request[type].method; type++)
		if (!strcmp(request, http_request[type].method))
			break;

	return type;
}


int
httpRequest(char *line, REQUEST_REC *rq)
{
	char *p;

	if ((p = strtok(line, " \t\n")) == NULL)
		return -1;

	xstrncpy(rq->request_method, p, PROTO_LEN);
	rq->HttpRequestType = GetHttpRequestType(rq->request_method);

	if ((p = strtok(NULL, " \t\r\n")) == NULL)
		return -1;
	xstrncpy(rq->URI, p, URI_LEN);

	return 0;
}


/*******************************************************************
 *	check if browser send reload command (Pragma: no-cache)
 *
 *	......not work in IE 4........T_T
 *
 *******************************************************************/
BOOL
client_reload(char *pragma)
{
	return strcasecmp(pragma, "no-cache") ? FALSE : TRUE;
}


struct _mime_type
{
	char *ext;
	char *type;
} mime_type[] =
{
	{"html", "text/html; charset=big5"},
	{"htm", "text/html; charset=big5"},
	{"gif", "image/gif"},
	{"png", "image/png"},
	{"jpg", "image/jpeg"},
	{"jar", "text/plain"},
	{"txt", "text/plain; charset=big5"},
	{"js", "application/x-javascript"},
	{"cdf", "application/x-netcdf"},
	{"class", "application/octet-stream"},
	{"swf", "application/x-shockwave-flash"},
	{"ocx", "application/octet-stream"},
	{"cab", "application/octet-stream"},
	{"ccr", "application/octet-stream"},
	{NULL, NULL}
};

/*******************************************************************
 *	根據 副檔名 判斷 MIME type
 *
 *******************************************************************/
int
GetMimeType(char *ext)
{
	int typeindex;


	for (typeindex = 0; mime_type[typeindex].ext; typeindex++)
		if (!strcasecmp(mime_type[typeindex].ext, ext))
			return typeindex;

	return 0;		/* default: text/html */
}


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
} HTTP_HEADER;

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

HTTP_HEADER http_header[] =
{
	{HTTP_OK, TRUE, TRUE, FALSE, TRUE, TRUE, TRUE, FALSE, FALSE},
	{HTTP_MOVED_PERMANENTLY, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE},
	{HTTP_NOT_MODIFIED, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},
	{HTTP_BAD_REQUEST, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE},
	{HTTP_UNAUTHORIZED, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, FALSE},
	{HTTP_FORBIDDEN, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE},
	{HTTP_FILE_NOT_FOUND, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE},
	{HTTP_NOT_IMPLEMENTED, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE}
};


char *
GetHttpRespondCode(REQUEST_REC *r)
{
	static char status_code[4];

	xstrncpy(status_code, http_header[r->HttpRespondType].status_line + 9,
		sizeof(status_code));
	return status_code;
}


/*******************************************************************
 *	根據 HttpRespondType 及 檔案類型送出 HTTP Response Header
 *
 *	Response = Status-Line
 *				*( general-header
 *				| response-header
 *				| entity-header )
 *				CRLF
 *				[ message-body ] => PrintFile()
 *
 *	Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
 *
 *	files should not be cached: almost all WEBBBS files
 *******************************************************************/
void
httpResponseHeader(REQUEST_REC * r, const SKIN_FILE * sf)
{
	HTTP_HEADER *hh = &(http_header[r->HttpRespondType]);
	char timestr[30];

	strftime(timestr, sizeof(timestr), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&(r->atime)));

	/* ===== Primary Respond ===== */
	fprintf(fp_out, "%s\r\n", hh->status_line);		/* Status-Line */
	fprintf(fp_out, "Date: %s\r\n", timestr);
	fprintf(fp_out, "Server: %s/%s\r\n", WEB_SERVER_NAME, WEB_SERVER_VERSION);

/*
   According to RFC 2068 HTTP/1.1 January 1997
   To mark a response as "already expired," an origin server should use
   an Expires date that is equal to the Date header value.
   Note that HTTP/1.0 caches may not implement Cache-Control
   and may only implement Pragma: no-cache (see section 14.32).
 */
	if (hh->Expires && sf->expire)
		fprintf(fp_out, "Expires: %s\r\n", timestr);

	if (hh->Last_Modified && !sf->expire)
	{
		strftime(timestr, sizeof(timestr), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&(sf->mtime)));
		fprintf(fp_out, "Last-Modified: %s\r\n", timestr);
	}

/*
   NetscapeS 3.x cache html if "Pragma: no-cache" is absent
   so ...........
 */
	if (hh->Pragma && sf->expire)
		fprintf(fp_out, "Pragma: no-cache\r\n");

	if (hh->Content_Type)
		fprintf(fp_out, "Content-Type: %s\r\n", mime_type[sf->mime_type].type);

	if (hh->Content_Length && sf->mime_type > 1)
		fprintf(fp_out, "Content-Length: %d\r\n", sf->size);

	if (hh->Location)
		fprintf(fp_out, "Location: http://%s:%d%s\r\n", server->host_name, port, sf->filename);

	if (hh->WWW_Authenticate)
		fprintf(fp_out, "WWW-Authenticate: Basic realm=\"[Private Board Access]\"\r\n");

	if (hh->Allow)
		fprintf(fp_out, "Allow: GET, HEAD, POST\r\n");

#ifdef KEEP_ALIVE
	if (r->connection == TRUE
		&& (r->HttpRespondType == OK || r->HttpRespondType == NOT_MODIFIED)
	    && (MAX_HTTP_REQUEST_PER_CHILD - r->num_request >= 0)
	    && sf->mime_type > 1)
	{
		fprintf(fp_out, "Keep-Alive: timeout=%d, max=%d\r\n",
			WEB_KEEP_ALIVE_TIMEOUT,
			MAX_HTTP_REQUEST_PER_CHILD - r->num_request);
		fprintf(fp_out, "Connection: Keep-Alive\r\n");
	}
	else
	{
		fprintf(fp_out, "Connection: close\r\n");
		r->connection = FALSE;
	}
#else
	fprintf(fp_out, "Connection: close\r\n");
#endif

#if 0
      Set - Cookie:
#endif
	fprintf(fp_out, "\r\n");
	fflush(fp_out);
}


/*******************************************************************
 *	Read & Parse HTTP Request Header Field
 *
 *	The request-header fields allow the client to pass additional
 *	information about the request, and about the client itself, to the
 *	server. These fields act as request modifiers, with semantics
 *	equivalent to the parameters on a programming language method
 *	invocation.
 *
 *	return: none
 *******************************************************************/
int
httpGetHeader(REQUEST_REC * r, SERVER_REC * s)
{
	char *buffer;
	int count = 0;
	char req_buf[HTTP_REQUEST_LINE_BUF];


	r->connection = FALSE;

	while (++count < MAX_HTTP_HEADER)	/* prevent repeated header attack */
	{
		buffer = req_buf;
		if ((fgets(buffer, sizeof(req_buf), fp_in)) == NULL)
		{
			return WEB_ERROR;
		}

#if 0
		fprintf(fp_out, "len=[%d], buffer=[%s]", strlen(buffer), buffer);
		fflush(fp_out);
#endif
		*(buffer + strlen(buffer) - 2) = '\0';	/* strip '\r\n' */

		if (strlen(buffer) == 0)
		{
#ifdef DEBUG
#ifdef TORNADO_OPTIMIZE		/* skip debug log */
			if (!isTORNADO)
#endif
				weblog_line(s->debug_log, "%s", buffer);
			fflush(s->debug_log);
#endif
			return WEB_OK;
		}

		else if (!strncasecmp(buffer, "Cookie:", 7))
		{
			char pass[PASSLEN * 3];

			xstrncpy(r->cookie, buffer + 8, STRLEN * 2);
			GetPara2(username, "Name", buffer, sizeof(username), "");
			GetPara2(pass, "Password", buffer, PASSLEN * 3, "");
			Convert(pass, TRUE);
			xstrncpy(password, pass, PASSLEN);
#ifdef DEBUG
#ifdef TORNADO_OPTIMIZE		/* skip debug log */
			if (!isTORNADO)
#endif
				weblog_line(s->debug_log, "%s", buffer);
			fflush(s->debug_log);
#endif
		}
		else if (!strncasecmp(buffer, "X-Forwarded-For:", 16)
			 || !strncasecmp(buffer, "X-SeederNet-For:", 16))
		{
			/* record client ip [squid proxy] */
			int a1 = 0, a2 = 0, a3 = 0, a4 = 0;
			char x;

#ifdef DEBUG
#ifdef TORNADO_OPTIMIZE		/* skip debug log */
			if (!isTORNADO)
#endif
				weblog_line(s->debug_log, "%s", buffer);
			fflush(s->debug_log);
#endif

			buffer += 17;
			xstrncpy(r->x_forward_for, buffer, STRLEN * 2);
			strtok(buffer, ",\r\n");

			if (sscanf(buffer, "%d.%d.%d.%d%c", &a1, &a2, &a3, &a4, &x) != 4)
			{
				sscanf(r->fromhost, "%d.%d.%d.%d", &a1, &a2, &a3, &a4);
				sprintf(r->fromhost, "%d?%d?%d?%d", a1, a2, a3, a4);
			}
			else
			{
				xstrncpy(r->proxyhost, r->fromhost, HOSTLEN);
				xstrncpy(r->fromhost, buffer, HOSTLEN);
			}
		}
		else if (!strncasecmp(buffer, "If-Modified-Since:", 18))
		{
			buffer += 19;
			strtok(buffer, ";\r\n");

			r->if_modified_since = parseHTTPdate(buffer);
#ifdef DEBUG
#ifdef TORNADO_OPTIMIZE		/* skip debug log */
			if (!isTORNADO)
#endif
				weblog_line(s->debug_log, "%s", buffer);
			fflush(s->debug_log);
#endif
		}
		else if (!strncasecmp(buffer, "Referer:", 8))
		{
			/*
			   we log the client access from outside of this server
			   this can help us to know whose webpage has link to our webpage

			   Referer: http://bbs.irradiance.net/txtVersion/
			 */

			xstrncpy(r->referer, buffer + 9, STRLEN * 2);
		}
		else if (!strncasecmp(buffer, "Host:", 5))
		{
			GetPara2(r->host, "Host: ", buffer, sizeof(r->host), s->host_name);
#ifdef DEBUG
#ifdef TORNADO_OPTIMIZE		/* skip debug log */
			if (!isTORNADO)
#endif
				weblog_line(s->debug_log, "%s", buffer);
			fflush(s->debug_log);
#endif
		}
		else if (!strncasecmp(buffer, "Via:", 4))
		{
			/*
			   Via: 1.0 tpproxy2.hinet.net:80 (Squid/1.1.22), 1.0 proxy-root1.hinet.net:3128 (Squid/1.1.22)
			 */
			xstrncpy(r->via, buffer + 5, STRLEN * 3);
#ifdef DEBUG
#ifdef TORNADO_OPTIMIZE		/* skip debug log */
			if (!isTORNADO)
#endif
				weblog_line(s->debug_log, "%s", buffer);
			fflush(s->debug_log);
#endif
		}
		else if (!strncasecmp(buffer, "Connection:", 11)
			 || !strncasecmp(buffer, "Xonnection:", 11))
		{
			r->connection = !strcasecmp(buffer + 12, "close") ? FALSE : TRUE;
#ifdef DEBUG
#ifdef TORNADO_OPTIMIZE		/* skip debug log */
			if (!isTORNADO)
#endif
				weblog_line(s->debug_log, "%s", buffer);
			fflush(s->debug_log);
#endif
		}
		else if (!strncasecmp(buffer, "Content-length:", 15))
		{
			r->content_length = atoi(buffer + 16);
		}
		else if (!strncasecmp(buffer, "Content-Type:", 13))
		{
			xstrncpy(r->content_type, buffer + 14, STRLEN);
			/* application/x-www-form-urlencoded */
		}
		else if (!strncasecmp(buffer, "User-Agent:", 11))
		{
			xstrncpy(r->user_agent, buffer + 12, STRLEN * 2);
		}
		else if (!strncasecmp(buffer, "Pragma:", 7))
		{
			xstrncpy(r->pragma, buffer + 8, STRLEN);
#ifdef DEBUG
#ifdef TORNADO_OPTIMIZE		/* skip debug log */
			if (!isTORNADO)
#endif
				weblog_line(s->debug_log, "%s", buffer);
			fflush(s->debug_log);
#endif
		}
		else if (!strncasecmp(buffer, "Cache-Control:", 14))
		{
			xstrncpy(r->cache_control, buffer + 15, STRLEN);
#ifdef DEBUG
#ifdef TORNADO_OPTIMIZE		/* skip debug log */
			if (!isTORNADO)
#endif
				weblog_line(s->debug_log, "%s", buffer);
			fflush(s->debug_log);
#endif
		}
		else if (!strncasecmp(buffer, "Client-ip:", 10))
		{
			/* record client ip [?? proxy] */
			xstrncpy(r->x_forward_for, buffer + 11, STRLEN * 2);
			buffer += 11;
			strtok(buffer, ",\r\n");
			xstrncpy(r->fromhost, buffer, HOSTLEN);
		}
		else if (!strncasecmp(buffer, "Authorization:", 14))
		{
			/*
			   Authorization: Basic xxxxxxxxxxxxxxxxxxxx
			 */
			xstrncpy(r->authorization, buffer + 15, STRLEN);

			buffer += 21;
			strtok(buffer, "\r\n");
#if 0
			fprintf(fp_out, "ori=[%s]", buffer);
#endif
			base64_decode_str(buffer);
#if 0
			fprintf(fp_out, " , decode=[%s]\r\n", buffer);
			fflush(fp_out);
#endif
			xstrncpy(r->auth_code, buffer, sizeof(r->auth_code));
		}
#ifdef WEB_OTHERHEADER_LOG
		else if (!strncasecmp(buffer, "Proxy-Connection:", 17)
			 || !strncasecmp(buffer, "Xroxy-Connection:", 17))
		{
			/* Proxy-Connection: Keep-Alive */
		}
		else if (!strncasecmp(buffer, "Proxy-Authorization:", 20))
		{
		}
		else if (!strncasecmp(buffer, "Max-Forwards:", 13))
		{
		}
		else if (!strncasecmp(buffer, "Forwarded:", 10))
		{
			/*
			   Forwarded: by http://isvr.interior.com.tw:8080 (Netscape-Proxy/3.5)
			 */
		}
		else if (!strncasecmp(buffer, "Accept-Language:", 16))
		{
		}
		else if (!strncasecmp(buffer, "Accept-Charset:", 15))
		{
		}
		else if (!strncasecmp(buffer, "Accept:", 7))
		{
		}
		else if (!strncasecmp(buffer, "Accept-Encoding:", 16))
		{
		}
		else if (!strncasecmp(buffer, "Accept-Ranges:", 14))
		{
			/* Accept-Ranges: bytes */
		}
		else if (!strncasecmp(buffer, "Range:", 6))
		{
		}
		else if (!strncasecmp(buffer, "Cache-Info:", 11))
		{
		}
		else if (!strncasecmp(buffer, "UA-", 3))
		{
		}
		else if (!strncasecmp(buffer, "From:", 5))
		{
		}
		else if (!strncasecmp(buffer, "Extension:", 10))
		{
		}
		else
			/* log other unknow header */
		{
			weblog(buffer, WEB_OTHERHEADER_LOG);
		}
#endif /* WEB_OTHERHEADER_LOG */

	}

	return WEB_ERROR;
}
