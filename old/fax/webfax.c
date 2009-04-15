/*****************************************************
    Formosa Web Server い闽 fax 场だ
******************************************************/

#include <stdlib.h>

#if 0
/***********************************************************
*   眔獺絚fax计ヘ
************************************************************/
int GetFaxNum()
{
    char maildirect[STRLEN];
    int num=0, fd;
    struct fileheader fh;

    if (strlen(username) == 0)
        num = 0;
    else
        sprintf(maildirect,"mail/%c/%-s/%-s",tolower(username[0]),
                username, DIR_REC);

    if ((fd = open (maildirect, O_RDWR)) < 0)
        return 0;

    num = 0;
    while (read (fd, &fh, FH_SIZE) == FH_SIZE)
        if (fh.accessed & FILE_FAX)
            num++;
    return num;
}
#endif

/***********************************************************
* BBS_FaxList_Read BBS_FaxList_num BBS_FaxList_Sender
* BBS_FaxList_Date BBS_FaxList_Subject
*
* BBS_VoiceList_Read BBS_VoiceList_num BBS_VoiceList_Sender
* BBS_VoiceList_Date BBS_VoiceList_Subject
*
* tag 癳秈ㄓㄏノセ┮把计
* fax
************************************************************/
ShowFaxList (tag)
char *tag;
{
    int fd, number, mailindex, flag;
    struct fileheader fh;
    char *request, mail_state, ReadFlag[255], buffer[10];
    char boardhtml[255], before[255], after[255], maildirect[STRLEN], chdate[6];
    int haveBoardhtml, index;
    time_t date;
	char check_str[20];

    /* 耞盞絏琌タ絋 */
    if (!CheckUser(username, password, &curuser))
        return FALSE;

	if (strstr(tag, "BBS_FaxList") != NULL)
	{
		flag = FILE_FAX;
		strcpy(check_str, "ReadFax");
	}
	else
	{
		flag = FILE_ANS;
		strcpy(check_str, "ReadVoice");
	}

    sprintf(maildirect, "mail/%c/%-s/%-s", tolower(curuser.userid[0]),
                    curuser.userid, DIR_REC);
                /* 盚ン﹎ */
    if ((strstr (tag, "BBS_FaxList_Sender") != NULL) ||
		(strstr (tag, "BBS_VoiceList_Sender") != NULL))
        request = fh.owner;

    /* 耞琌惠璶hyperlink */
    GetPara (boardhtml, "boardhtml", tag);
    if (*boardhtml != 0)
        haveBoardhtml = TRUE;
    else
        haveBoardhtml = FALSE;

    GetPara (before, "before", tag);
    GetPara (after, "after", tag);
    GetPara(buffer, "number", tag);
    number = atoi(buffer);


    if ((fd = open (maildirect, O_RDWR)) < 0)
        return;
    index = 1;

    /* Τ﹚number把计 */
    if (number > 0)
    {
        mailindex = 0;
        while (read(fd, &fh, FH_SIZE) == FH_SIZE)
        {
            if (fh.accessed & flag)
                mailindex++;
            if (mailindex == number)
            {
                if (lseek(fd, -FH_SIZE, SEEK_CUR) == -1)
                {
                    close(fd);
                    return;
                }
                break;
            }
        }
        index = number;
    }

    while (read (fd, &fh, FH_SIZE) == FH_SIZE)
    {
        if (fh.accessed & flag)
        {
            if (strstr(tag, "Num") != NULL)
            {
                if (haveBoardhtml)
                    printf ("%s<A HREF=\"%s?%s=%d\">%d</A><br>%s\r\n",
                            before, boardhtml, check_str, index, index, after);
                else
                    printf ("%s%d<br>%s\r\n", before, index, after);
            }
            else if (strstr(tag, "Date") != NULL)
            {
                date = atol((fh.filename) + 2);
                strftime(chdate, 6, "%m/%d", localtime(&date));
                if (haveBoardhtml)
                    printf ("%s<A HREF=\"%s?%s=%d\">%s</A><br>%s\r\n",
                            before, boardhtml, check_str, index, chdate, after);
                else
                    printf ("%s%s<br>%s\r\n", before, chdate, after);
            }
            else if (strstr(tag, "Read") != NULL)
            {
                if (fh.accessed & FILE_DELE)
                    NULL;
                else if (fh.accessed & FILE_READ)
                    GetPara (ReadFlag, "old", tag);
                else
                    GetPara (ReadFlag, "new", tag);
                if (haveBoardhtml)
                    printf ("%s<A HREF=\"%s?%s=%d\">%s</A><br>%s\r\n",
                        before, boardhtml, check_str, index, ReadFlag, after);
                else
                    printf ("%s%s<br>%s\r\n", before, ReadFlag, after);
            }
			else if (strstr(tag, "Subject") != NULL)
			{
				if (haveBoardhtml)
                    printf ("%s<A HREF=\"%s?%s=%d\">材%d</A><br>%s\r\n",
                            before, boardhtml, check_str, index, index, after);
                else
                    printf ("%s%d<br>%s\r\n", before, index, after);
			}
            else
            {
                if (haveBoardhtml)
                    printf ("%s<A HREF=\"%s?%s=%d\">%s</A><br>%s\r\n",
                        before, boardhtml, check_str, index, request, after);
                else
                    printf ("%s%s<br>%s\r\n", before, request, after);
            }
        }
        if (number > 0)
            break;
        index++;
    }
    fflush (stdout);
    close (fd);
}

/***********************************************************
*       BBS_FaxGet_Sender BBS_FaxGet_Date
*       BBS_FaxGet_Subject BBS_FaxGet_Content
*
*		BBS_VoiceGet_Sender BBS_VoiceGet_Date
*       BBS_VoiceGet_Subject BBS_VoiceGet_Content
*       tag 癳秈ㄓㄏノセ┮把计
*       ╭FAXず甧
************************************************************/
void
ShowFax (tag)
     char *tag;
{
    char buffer[256], *p, *p2, maildirect[256], tifname[20];
    int idx, num, fd, faxindex, pid, type, flag;
    struct fileheader fileinfo;
    char MailName[STRLEN];
    time_t date;
    USEREC curuser;
	FILE *fp;
	char check_str[20];

    /* 耞盞絏琌タ絋 */
    if (!CheckUser(username, password, &curuser))
        return;

	if (strstr(URLPara, "ReadFax") != NULL)
	{
		strcpy(check_str, "ReadFax");
		type = 2;
		flag = FILE_FAX;
	}
	else if (strstr(URLPara, "ReadVoice") != NULL)
	{
		strcpy(check_str, "ReadVoice");
		type = 3;
		flag = FILE_ANS;
	}

    sprintf(maildirect, "mail/%c/%-s/%-s", tolower(curuser.userid[0]),
            curuser.userid, DIR_REC);

    p = strstr (URLPara, check_str);
    p += strlen (check_str)+1;
    strcpy (buffer, p);
    p2 = strrchr (buffer, '/');
    if (p2 != NULL)
        *p2 = '\0';
    idx = atoi(p);

    if (strstr (URLPara, "Last") != NULL)
        idx--;
    else if (strstr (URLPara, "Next") != NULL)
        idx++;

	num = GetMailNum(type);
    if (idx > num)
        return;

    if ((fd = open (maildirect, O_RDWR)) < 0)
        return;

	faxindex = 0;
	while (read (fd, &fileinfo, FH_SIZE) == FH_SIZE)
	{
        if (fileinfo.accessed & flag)
            faxindex++;
		if (faxindex == idx)
			break;
	}

    if (fileinfo.accessed & FILE_DELE)
    {
        close (fd);
        printf ("<form name=\"%s\">\r\n", check_str);
        printf ("<Input type=\"hidden\" Name=\"number\" value=\"%d\">\r\n",
            idx);
        printf ("</form>\r\n");
        return;
    }

    if (lseek(fd, -FH_SIZE, SEEK_CUR) == -1)
    {
        close(fd);
        return;
    }

    fileinfo.accessed |= FILE_READ;
    write(fd, &fileinfo, FH_SIZE);
    close(fd);

    strcpy (MailName, maildirect);
    p = strrchr (MailName, '/') + 1;
    strcpy (p, fileinfo.filename);

    if (strstr (tag, "Sender") != NULL)
        printf ("%s\r\n", fileinfo.owner);
    else if (strstr (tag, "Date") != NULL)
    {
        date = atol ((fileinfo.filename) + 2);
        printf ("%s\r\n", ctime (&date));
    }
    else if (strstr (tag, "Subject") != NULL)
        printf ("%s\r\n", fileinfo.title);
    else if (strstr (tag, "Content") != NULL)
    {
#if 0
		sprintf(tifname, "tmp/%d.tif", time(0));
		if ((fp = fopen(MailName, "r")) == NULL)
			return;
		while (fgets(buffer, 255, fp) != NULL)
		{
			if (strstr(buffer, "Content-Disposition:") != NULL)
			{
				if ((p = strstr(buffer, "filename=")) != NULL)
				{
					p += strlen("filename=") + 1;
					strcpy(tifname, "tmp/");
					strcat(tifname, p);
					if (*(p+1) == '"')
						sprintf(tifname, "tmp/%d.tif", time(0));
					p = strchr(tifname, '"');
					*p = '\0';
					break;
				}
			}
		}
		fclose(fp);

		sprintf(para1, "metamail");
		sprintf(para2, "%s", MailName);
		sprintf(para3, "-t");
		sprintf(para4, "%s", tifname);
		paras[0] = para1;
		paras[1] = para2;
		paras[2] = para3;
		paras[3] = para4;
		paras[4] = NULL;

		pid = fork();
		if (pid == 0)
		{
			execv("bin/metamail", paras);
			printf("ERROR");
		}
		else if (pid < 0)
			return;
		else
			wait();
#endif
		SendArticle(MailName, tifname);
/*
		printf("<a href=\"%s\">hyperlink</a>\r\n", tifname);
*/

        printf ("<form name=\"%s\">\r\n", check_str);
        printf ("<Input type=\"hidden\" Name=\"number\" value=\"%d\">\r\n",
            idx);
        printf ("</form>\r\n");

    }
}
