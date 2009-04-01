
/* define to enable log function */
/* log client access */
#define WEB_ACCESS_LOG				"log/web-access.log"

/* log special action */
#define WEB_EVENT_LOG				"log/web-access.log"

/* log error */
#define WEB_ERROR_LOG				"log/web-error.log"

/* log http header not recognized */
#define WEB_OTHERHEADER_LOG			"log/web_otherheader.log"

/* log file not send at http 304 respond */
#define WEB_304_LOG					"log/web_304.log"

/* log timeout connection */
#define WEB_TIMEOUT_LOG				"log/web_timeout.log"

/* log debug info */
#ifdef DEBUG
#define WEB_DEBUG_LOG				"log/web-debug.log"
#endif

#ifdef NSYSUBBS1
#undef WEB_OTHERHEADER_LOG
#undef WEB_304_LOG
#undef WEB_TIMEOUT_LOG
#endif

#ifdef NSYSUBBS3
#undef WEB_OTHERHEADER_LOG
#undef WEB_304_LOG
#undef WEB_TIMEOUT_LOG
#endif

#ifdef ULTRABBS

#endif

#ifndef NSYSUBBS
#undef WEB_OTHERHEADER_LOG
#undef WEB_304_LOG
#undef WEB_TIMEOUT_LOG
#endif
