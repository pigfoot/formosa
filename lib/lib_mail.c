#ifdef linux
#define _GNU_SOURCE
#endif
#include "bbs.h"
#include <ctype.h>
#include <stdlib.h>

char *cgetline(char *input,
			char **buf, size_t offset, size_t *buflen)
{
	int len;

	if (!input || !buf || !(*buf) || !buflen) {
		**buf = '\0';
		return NULL;
	}

	len = 0;
	while (1) {
		while (len + offset >= *buflen) {
			*buflen <<= 1;
			*buf = realloc(*buf, *buflen);
		}
		(*buf)[len + offset] = input[len];
		if (!input[len]) {
			break;
		} else if (input[len] == '\n') {
			(*buf)[++len + offset] = '\0';
			break;
		}
		++len;
	}
	return input + len;
}

static char *merge_input(char *input, char **buf, size_t *buflen)
{
	char *p, *ip;
	size_t offset = 0;

	offset = strlen(*buf);
	if (offset <= 0)
		return NULL;

	ip = input;
	while ((*buf)[offset - 1] == '\n' && (*ip == '\t' || *ip == ' ')) {
		while (*ip == '\t' || *ip == ' ')
			++ip;
		(*buf)[offset - 1] = ' ';
		p = cgetline(ip, buf, offset, buflen);
		if (!p)
			return NULL;
		if (p == ip)
			break;
		offset += p - ip;
		ip = p;
	}

	return ip;
}

static int parse_address(const char *buf, char *name, char *addr)
{
	const char *ptr, *p;
	char *str;
	size_t len;
	*name = *addr = '\0';

	if ((str = strchr(buf, '<')) && (ptr = strrchr(str, '>'))) {
		++str;
		while (isspace(*str))
			++str;
		while (isspace(*(ptr - 1)))
			--ptr;
		if (ptr <= str) {
			strcpy(addr, "NULL");
		} else {
			len = ptr - str;
			strncpy(addr, str, len);
			addr[len] = '\0';
		}
		if (!strchr(addr, '@')) {
			/*
			 * from local host
			 */
			strcat(addr, "@localhost");
		}

		ptr = buf;
		while (isspace(*ptr))
			++ptr;
		if (*ptr == '"') {
			/*
			 * handle "name" <address>
			 */
			if (!(p = strrchr(++ptr, '"'))) {
				/*
				 * handle "name <address>
				 * Which is error accutally...
				 */
				p = strchr(ptr, '<');
				while (isspace(*(p - 1)))
					--p;
			}
		} else {
			/*
			 * handle name <address>
			 */
			p = strchr(ptr, '<');
			while (isspace(*(p - 1)))
				--p;
		}

		if (p > ptr) {
			/*
			 * Have name
			 */
			len = p - ptr;
			strncpy(name, ptr, len);
			name[len] = '\0';
			str_decode((unsigned char *)name);
		}

	} else {
		/*
		 * Address only
		 * '<' and '>' used here is for missing each other to form a pair
		 */
		ptr = buf;
		while (*ptr && (isspace(*ptr) || *ptr == '<'))
			++ptr;

		p = ptr;
		while (*p != '\0' &&
		       !isspace(*p) &&
		       *p != '>' &&
		       *p != ',')
			++p;
		if (p <= ptr)
			return -1;

		len = p - ptr;
		strncpy(addr, ptr, len);
		addr[len] = '\0';

		str_trim(addr);
		if (!strlen(addr))
			return -1;

#if 0
		if (!strchr(addr, '@'))	/* ¥Ñ local host ±H«H */
			strcat(addr, "@localhost");
#endif
	}

	return 0;
}

static int mail_get_from(char *buf, struct MailHeader *mh)
{
	return parse_address(buf, mh->from_name, mh->from_addr);
}

static int mail_get_multi(char *buf,
			char name[MAX_ADDR_NR][NAME_LEN],
			char addr[MAX_ADDR_NR][ADDR_LEN])
{
	int n;
	char *token, *saveptr = NULL;

	token = strtok_r(buf, ",", &saveptr);
	for (n = 0 ; n < MAX_ADDR_NR && token ; ++n) {
		if (parse_address(token, name[n], addr[n]))
			break;
		dbg("ADDR: %s (%s)\n", addr[n], name[n]);
		token = strtok_r(NULL, ",", &saveptr);
	}

	if (n < MAX_ADDR_NR)
		name[n][0] = addr[n][0] = '\0';

	return 0;
}

static int mail_get_to(char *buf, struct MailHeader *mh)
{
	return mail_get_multi(buf, mh->to_name, mh->to_addr);
}

static int mail_get_cc(char *buf, struct MailHeader *mh)
{
	return mail_get_multi(buf, mh->cc_name, mh->cc_addr);
}

static int mail_get_subject(char *buf, struct MailHeader *mh)
{
	char *subject = mh->subject;

	while (isspace(*(buf)))
		++buf;

	str_notab(buf);
	str_trim(buf);
	str_decode((unsigned char *)buf);
	str_ansi(subject, buf, SUBJECT_LEN);

	return 0;
}

/*
 * Get a value ended with ';' or space or '\0'
 */
static int mail_get_val(const char *buf, char *val)
{
	const char *p;
	size_t len;

	while (isspace(*buf))
		++buf;

	p = buf;
	while (*p != '\0' && *p != '\n' && *p != '\r' &&
		!isspace(*p) && *p != ';') ++p;

	if (p <= buf)
		return -1;

	len = p - buf;
	strncpy(val, buf, len);
	val[len] = '\0';

	return 0;
}

static int mail_get_content_type(char *buf, struct MailHeader *mh)
{
	char *ptr;
	char *ct = mh->content_type;
	char *cs = mh->charset;
	char *bndy = mh->boundary;
	static const char *str_charset = "charset=";
	static const char *str_boundary = "boundary=";

	dbg("Getting content-type: %s\n", buf);

	if (mail_get_val(buf, ct))
		return -1;
	str_unquote(ct);

	if ((ptr = strcasestr(buf, str_charset)) != NULL) {
		if (mail_get_val(ptr + strlen(str_charset), cs))
			return -1;
		str_unquote(cs);
	}

	if ((ptr = strcasestr(buf, str_boundary)) != NULL) {
		if (mail_get_val(ptr + strlen(str_boundary), bndy))
			return -1;
		str_unquote(bndy);
	}

	return 0;
}

static int mail_get_transenc(char *buf, struct MailHeader *mh)
{
	return mail_get_val(buf, mh->transenc);
}

static int mail_get_xfrom(char *buf, struct MailHeader *mh)
{
	char unused[NAME_LEN];
	return parse_address(buf, unused, mh->xfrom);
}

static int mail_get_xorigto(char *buf, struct MailHeader *mh)
{
	char unused[NAME_LEN];
	return parse_address(buf, unused, mh->xorigto);
}

static int mail_get_xdelivto(char *buf, struct MailHeader *mh)
{
	char unused[NAME_LEN];
	return parse_address(buf, unused, mh->xdelivto);
}

int is_notmycharset(const char *charset)
{
	if (!strcasecmp(charset, MYCHARSET) ||
	    !strncasecmp(charset, "iso-8859-1", 10) ||
	    !strncasecmp(charset, "us-ascii", 8))
		return 0;

	return 1;
}

struct HeaderHandler {
	const char *header;
	char multiline;
	char casesence;
	int (*hdl)(char *buf, struct MailHeader *mh);
};
struct HeaderHandler hh[] = {
	{"From:",			1,	0,	mail_get_from},
	{"To:",				1,	0,	mail_get_to},
	{"Cc:",				1,	0,	mail_get_cc},
	{"Subject:",			1,	0,	mail_get_subject},
	{"Content-Type:",		1,	0,	mail_get_content_type},
	{"Content-Transfer-Encoding:",	1,	0,	mail_get_transenc},
	{"From",			0,	1,	mail_get_xfrom},
	{"X-Original-To:",		0,	1,	mail_get_xorigto},
	{"Delivered-To:",		0,	1,	mail_get_xdelivto},
	{NULL,				0,	0,	NULL},
};

/*
 * parse_header:
 * 	@input: Pointer to start of header
 * 	@mh:	Preallocated MailHeader structure for saving parsed information
 *
 * 	return value: Pointer to the end of header
 */
#define BUF_LEN 4096
char *parse_header(char *input, struct MailHeader *mh)
{
	static char *buf = NULL;
	static size_t buflen = BUF_LEN;
	char *next_line = NULL;
	int i;
	int (*cmpfun)(const char *, const char *, size_t);

	if (!input || !mh)
		return NULL;

	/*
	 * Set default value
	 */
	memset(mh, 0, sizeof(struct MailHeader));
	strcpy(mh->content_type, "text/plain");
	strcpy(mh->charset, "iso-8859-1");
	strcpy(mh->transenc, "7bit");

	if (!buf)
		buf = (char *)malloc(buflen);
	if (!buf)
		return NULL;
	memset(buf, 0, buflen);

	next_line = cgetline(input, &buf, 0, &buflen);
	while (buf[0] && buf[0] != '\n') {
		for (i = 0 ; hh[i].header ; ++i) {
			cmpfun = (hh[i].casesence) ? strncmp : strncasecmp;
			if (!cmpfun(buf, hh[i].header, strlen(hh[i].header))) {
				if (hh[i].multiline)
					next_line = merge_input(next_line, &buf, &buflen);
				if (hh[i].hdl(buf + strlen(hh[i].header) + 1, mh)) {
					next_line = NULL;
					dbg("Error at %s %30.30s\n", hh[i].header, buf + strlen(hh[i].header) + 1);
					goto out;
				}
				break;
			}
		}
		next_line = cgetline(next_line, &buf, 0, &buflen);
	}

out:
	return next_line;
}

enum print_simple_content_rt {
	ERROR = -1,
	ENDOFINPUT = 0,
	HIT_BOUNDARY = 1,
	HIT_END_BOUNDARY = 2,
};
static int print_simple_content(char *input, FILE *output, char *errmsg, struct MailHeader *mh, char **next_line)
{
	static char *buf = NULL;
	static size_t buflen = BUF_LEN;
	char *ptr;
	iconv_t tobig5 = (iconv_t) -1;
	size_t offset;
	unsigned char isbase64 = 0, isqp = 0;
	int rt = 0;

	dbg("In print_simple_content.\n");

	if (output) {
		if (is_notmycharset(mh->charset))
			tobig5 = iconv_open("big5//TRANSLIT//IGNORE", mh->charset);

		if (!strncasecmp(mh->transenc, "base64", 6))
			isbase64 = 1;
		else if (!strncasecmp(mh->transenc, "quoted-printable", 16))
			isqp = 1;
	}

	if (!buf)
		buf = malloc(buflen);
	if (!buf) {
		rt = ERROR;
		goto out;
	}

	*next_line = cgetline(input, &buf, 0, &buflen);
	while (buf[0]) {
		offset = strlen(mh->boundary);
		if (offset && (ptr = strstr(buf, mh->boundary)) != NULL) {
			if (ptr[offset] == '\n')
				rt = HIT_BOUNDARY;
			else
				rt = HIT_END_BOUNDARY;
			break;
		}

		if (isbase64) {
			offset = mmdecode((unsigned char *)buf, 'b', (unsigned char *)buf);
			buf[offset] = '\0';
		} else if (isqp) {
			str_deqp(buf, buf);
		}

		if (tobig5 != (iconv_t)-1) {
			if (!str_conv(tobig5, buf, buflen)) {
				sprintf(errmsg, "%s CONVERR(%s)", mh->charset, strerror(errno));
				rt = ERROR;
				break;
			}
		}
		if (output)
			fputs(buf, output);
		*next_line = cgetline(*next_line, &buf, 0, &buflen);
	}

	if (tobig5 != (iconv_t)-1)
		iconv_close(tobig5);

out:
	return rt;

}

int print_content(char *input, FILE *output, char *errmsg, struct MailHeader *mh, struct MailHeader *subhdr)
{
	int rt, n = 0;
	char buf[80], *next_line = input;

	memset(subhdr, 0, sizeof(struct MailHeader) * MAX_PART_NR);
	if (!strncasecmp(mh->content_type, "multipart/", 10)) {
		if (!*(mh->boundary)) {
			strcpy(errmsg, "MultipartNoBoundary");
			return -1;
		}

		dbg("bndy: \"%s\"\n", mh->boundary);

		rt = print_simple_content(next_line, output, errmsg, mh, &next_line);
		if (rt == ERROR)
			return ERROR;
		else if (rt == ENDOFINPUT)
			return 0;

		dbg("first rt: %d\n", rt);

		do {
			if (++n > MAX_PART_NR) {
				strcpy(errmsg, "PARTLIM");
				return ERROR;
			}

			next_line = parse_header(next_line, subhdr + (n - 1));

			dbg("header rt: %p\n", next_line);

			if (!next_line) {
				strcpy(errmsg, "ESUBHDR");
				return -1;
			}

			sprintf(buf, "\n============================== Part %d ==============================\n", n);
			fputs(buf, output);
			strcpy((subhdr + (n - 1))->boundary, mh->boundary);

			if (strncasecmp((subhdr + (n - 1))->content_type, "text/", 5)) {
			    	fputs("It's not a text file, ignored printing.\n", output);
				rt = print_simple_content(next_line, NULL, errmsg, subhdr + (n - 1), &next_line);
			} else {
				rt = print_simple_content(next_line, output, errmsg, subhdr + (n - 1), &next_line);
			}

			dbg("psc rt: %d\n", rt);
		} while (rt == HIT_BOUNDARY);

		if (rt == HIT_END_BOUNDARY) {
			sprintf(buf, "\n============================= Tail Part =============================\n");
			fputs(buf, output);
			rt = print_simple_content(next_line, output, errmsg, mh, &next_line);
			if (rt != ERROR)
				rt = n;
		}


		return rt;
	}

	if ((rt = print_simple_content(next_line, output, errmsg, mh, &next_line)) == ERROR)
		return -1;
	else
		return 0;
}

