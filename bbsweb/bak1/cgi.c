
#if 0
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
	char host[STRLEN];
	char cache_control[STRLEN];
	char pragma[STRLEN];
	char content_type[STRLEN];
	char x_forward_for[STRLEN*2];
	char referer[STRLEN*2];
	char via[STRLEN*3];
	char user_agent[STRLEN*2];
/* =============================== */
	char request_method[32];
	char URI[URI_LEN];
}REQUEST_REC;
#endif


int
cgi_handler(REQUEST_REC *r)
{
	struct stat st;

	if ((argv0 = strrchr(r->URI, '/')) != NULL)
		argv0++;
	else
		argv0 = r->URI;
	/* todo:Åv­­ºÞ²z */
	if (stat(argv0, &st) != 0 || !S_IEXEC(st.st_mode) || S_ISDIR(st.st_mode))
		return -1;
	
	{
		"Transfer-Encoding"
		"Content-Length"
		
	}
}
