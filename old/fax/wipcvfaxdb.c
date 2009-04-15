
/*********************************************************************
                        Copyright (c) 1994
               New Technology Information Systems, Inc.
                Unpublished and Not for Publication
                        All Rights Reserved

                        NTIS - PROPRIETARY

THIS DOCUMENT CONTAINS PROPRIETARY INFORMATION OF NTIS AND IS NOT TO
BE DISCLOSED OR USED EXCEPT IN ACCORDANCE WITH APPLICABLE AGREEMENTS.

Program Name:   %M%
Version:        %W%
Date:           %H%
Unit:
Author:
Descriptions:
Input:
Output:
Constraints:
Change History:
*************************************************************************/
/*************************************************************
 This Program is the wipc server for faxnet billing system
 *************************************************************/
#include "wipc.h"
#include "wipcvfaxdb.h"

/*************************************************************
 *  Add New customer                                         *
 *************************************************************/
int
Wipc_cusadd(buffer)
char buffer[WIPC_MAX_MESSAGE_SIZE];
{
  int cc,i,ch,flag,DONE;
  char buf[15][80],passwd[5],chmodmsg[50];
  char *token ;
  FILE *fp,*sysfd;
  struct keydesc ckey;

  if ((sysfd=fopen("/home/workfile/errlog","a+")) == NULL)
    printf("vircusadd:Open errlog Failed !\n");

  i=0;
  token = strtok(buffer,tokensep);
  while (token != NULL)
  {
    sprintf(buf[i],"%s",token);
    i++;
    token = strtok(NULL,tokensep);
  }

  passwd[0]=buf[2][3] - 10;
  passwd[1]=buf[2][1] - 10;
  passwd[2]=buf[2][2] - 10;
  passwd[3]=buf[2][0] - 10;
  passwd[4]='\0';

  fdcus=cc=isopen("/DB/vircust",ISMANULOCK+ISINOUT);
  if (cc < 0)
  {
    if (iserrno == 2)
    {
        /* Set up Customer Key (accno) in vircust.dat file*/
       	ckey.k_flags = ISNODUPS;
        ckey.k_nparts = 1;
  	ckey.k_part[0].kp_start = 0;
  	ckey.k_part[0].kp_leng = 20;
  	ckey.k_part[0].kp_type = CHARTYPE;

  	fdcus = cc = isbuild("/DB/vircust", 600, &ckey,ISINOUT + ISEXCLLOCK);
  	if (cc < SUCCESS)
  	{
    		printf("isbuild error %d for vircust file\n",iserrno);
                fclose(sysfd);
        	return FALSE;
        }
        else
        {       sprintf(chmodmsg,"chmod 777 /DB/vircust.*");
                system(chmodmsg);
        }
    }
    else
    {
    	fprintf(sysfd,"vircus_add:isopen error %d for vircust file\n",iserrno);
    	fclose(sysfd);
    	return FALSE;
    }
  }

  /**************************
   * Putting data to record *
   **************************/
  stchar(buf[0],&cusrec[0],20);		/* account */
  readit:
  cc = isread (fdcus,cusrec,ISEQUAL);
  stchar(buf[1],&cusrec[20],40);	/* company */
  stchar(passwd,&cusrec[60],4);         /* passwd */
  stchar(buf[3],&cusrec[64],10);	/* idno */
  stchar(buf[4],&cusrec[74],15);	/* phone */
  stchar(buf[5],&cusrec[89],80);	/* address */
  stchar(buf[6],&cusrec[169],20);       /* customer_name */
  stchar(buf[7],&cusrec[190],10);	/* zip */
  stchar(buf[8],&cusrec[200],80);	/* invo_addr */
  stchar(buf[9],&cusrec[280],60);	/* memo */
  stchar(buf[10],&cusrec[340],6);	/* begin_day */
  stchar(buf[11],&cusrec[346],40);	/* email */
  stchar(buf[12],&cusrec[386],10);	/* end_date */
  stchar(buf[13],&cusrec[396],10);	/* start_date*/
  stchar("          ",&cusrec[406],10); /* end_date2 */
  stchar(buf[14],&cusrec[416],5);       /* sales */

  if (cc == 0)
  {
    islock(fdcus);
    cc = isrewrite(fdcus,cusrec);
    if (cc != 0)
    {
      fprintf(sysfd,"vircusadd:isrewrite error %d for vircust file\n",iserrno);
      isunlock(fdcus);
      isclose(fdcus);
      fclose(sysfd);
      return FALSE;
    }
    isunlock(fdcus);
  }
  else if (iserrno == ENOREC)
  {
    islock(fdcus);
    cc = iswrite(fdcus,cusrec);
    if (cc != 0)
    {
      fprintf(sysfd,"vircusadd:iswrite error %d for vircust file\n",iserrno);
      isunlock(fdcus);
      isclose(fdcus);
      fclose(sysfd);
      return FALSE;
    }
    isunlock(fdcus);
  }
  else if (iserrno == ELOCKED || iserrno == EFLOCKED)
  {
    sleep(1);
    goto readit;
  }
  isclose(fdcus);
  fclose(sysfd);
  return TRUE;
}

/*************************************************************
 *  Delete the customer record                               *
 *************************************************************/
int
Wipc_cusdel(buffer)
char buffer[WIPC_MAX_MESSAGE_SIZE];
{
  int cc,i,ch,flag,DONE;
  char *token,chmodmsg[50];
  FILE *fp,*sysfd;
  struct keydesc ckey;

  if ((sysfd=fopen("/home/workfile/errlog","a+")) == NULL)
    printf("vircusdel:Open errlog Failed !\n");


  fdcus=cc=isopen("/DB/vircust",ISMANULOCK+ISINOUT);
  if (cc < 0)
  {
    if (iserrno == 2)
    {
        /* Set up Customer Key (accno) in vircust.dat file*/
        ckey.k_flags = ISNODUPS;
        ckey.k_nparts = 1;
        ckey.k_part[0].kp_start = 0;
        ckey.k_part[0].kp_leng = 20;
        ckey.k_part[0].kp_type = CHARTYPE;

        fdcus = cc = isbuild("/DB/vircust", 600, &ckey,ISINOUT + ISEXCLLOCK);
        if (cc < SUCCESS)
        {
                printf("isbuild error %d for vircust file\n",iserrno);
                fclose(sysfd);
        	return FALSE;
        }
        else
        {       sprintf(chmodmsg,"chmod 777 /DB/vircust.*");
                system(chmodmsg);
        }
    }
    else
    {
        fprintf(sysfd,"vircusdel:isopen error %d for vircust file\n",iserrno);
        fclose(sysfd);
        return FALSE;
    }
  }
  /* Using faxno to find the record ,if this record was locked,try again */
  stchar(buffer,&cusrec[0],20);
  readit:
  cc = isread(fdcus,cusrec,ISEQUAL);
  if (cc == 0)
  {
    islock(fdcus);
    cc = isdelete(fdcus,cusrec);
    if (cc != 0)
    {
      fprintf(sysfd,"vircusdel:isdelete error %d for vircust file\n",iserrno);
      isunlock(fdcus);
      isclose(fdcus);
      fclose(sysfd);
      return FALSE;
    }
    isunlock(fdcus);
  }
  else
  {
    if (iserrno == ELOCKED || iserrno == EFLOCKED)
    {
      sleep(1);
      goto readit;
    }
    else
    {
      if (iserrno == ENOREC)
	fprintf(sysfd,"vircusdel:NO This Record[%s] EXIST\n",buffer);
      else
        fprintf(sysfd,"vircusdel:isread error %d for vircust file\n",iserrno);
      isclose(fdcus);
      fclose(sysfd);
      return FALSE;
    }
  }
  isclose(fdcus);
  fclose(sysfd);
  return TRUE;
}

/*************************************************************
 *  Check if the customer exists or not                      *
 *************************************************************/
int
Wipc_cusmod(buffer)
char buffer[WIPC_MAX_MESSAGE_SIZE];
{
  int cc,i,ch,flag,DONE;
  char passwd[6],buf[15][80],chmodmsg[50];
  char *token;
  FILE *fp,*sysfd;
  struct keydesc ckey;


  if ((sysfd=fopen("/home/workfile/errlog","a+")) == NULL)
    printf("vircusmod:Open errlog Failed !\n");

  i=0;
  token = strtok(buffer,tokensep);
  while (token != NULL)
  {
    sprintf(buf[i],"%s",token);
    i++;
    token = strtok(NULL,tokensep);
  }

  passwd[0]=buf[2][3] - 10;
  passwd[1]=buf[2][1] - 10;
  passwd[2]=buf[2][2] - 10;
  passwd[3]=buf[2][0] - 10;
  passwd[4]='\0';

  fdcus=cc=isopen("/DB/vircust",ISMANULOCK+ISINOUT);
  if (cc < 0)
  {
    if (iserrno == 2)
    {
        /* Set up Customer Key (accno) in vircust.dat file*/
        ckey.k_flags = ISNODUPS;
        ckey.k_nparts = 1;
        ckey.k_part[0].kp_start = 0;
        ckey.k_part[0].kp_leng = 20;
        ckey.k_part[0].kp_type = CHARTYPE;

        fdcus = cc = isbuild("/DB/vircust", 600, &ckey,ISINOUT + ISEXCLLOCK);
        if (cc < SUCCESS)
        {
                printf("isbuild error %d for vircust file\n",iserrno);
                fclose(sysfd);
        	return FALSE;
        }
        else
        {       sprintf(chmodmsg,"chmod 777 /DB/vircust.*");
                system(chmodmsg);
        }
    }
    else
    {
        fprintf(sysfd,"vircusmod:isopen error %d for vircust file\n",iserrno);
        fclose(sysfd);
        return FALSE;
    }
  }

  /* Using faxno to find the record ,if this record was locked,try again */
  stchar(buf[0],&cusrec[0],20);
  readit:
  cc = isread(fdcus,cusrec,ISEQUAL);
  /* Putting the modify data to record */
  /**************************
   * Putting data to record *
   **************************/
  stchar(buf[1],&cusrec[20],40);        /* company */
  stchar(passwd,&cusrec[60],4);         /* password */
  stchar(buf[3],&cusrec[64],10);        /* idno */
  stchar(buf[4],&cusrec[74],15);        /* phone */
  stchar(buf[5],&cusrec[89],80);        /* address */
  stchar(buf[6],&cusrec[169],20);       /* customer_name */
  stchar(buf[7],&cusrec[190],10);       /* zip */
  stchar(buf[8],&cusrec[200],80);       /* invo_addr */
  stchar(buf[9],&cusrec[280],60);       /* memo */
  stchar(buf[10],&cusrec[340],6);       /* begin_day */
  stchar(buf[11],&cusrec[346],40);      /* email */
  stchar(buf[12],&cusrec[386],10);      /* end_date */
  stchar(buf[13],&cusrec[396],10);      /* start_date*/
  stchar("          ",&cusrec[406],10); /* end_date2 */
  stchar(buf[14],&cusrec[416],5);       /* sales */

  if (cc == 0)
  {
    islock(fdcus);
    cc = isrewrite(fdcus,cusrec);
    if (cc != 0)
    {
      fprintf(sysfd,"vircusmod:isrewrite error %d for vircust file\n",iserrno);
      isunlock(fdcus);
      isclose(fdcus);
      fclose(sysfd);
      return FALSE;
    }
    isunlock(fdcus);
  }
  else if (iserrno == ENOREC)
  {
    islock(fdcus);
    cc = iswrite(fdcus,cusrec);
    if (cc != 0)
    {
      fprintf(sysfd,"vircusmod:iswrite error %d for vircust file\n",iserrno);
      isunlock(fdcus);
      isclose(fdcus);
      fclose(sysfd);
      return FALSE;
    }
    isunlock(fdcus);
  }
  else if (iserrno == ELOCKED || iserrno == EFLOCKED)
  {
    sleep(1);
    goto readit;
  }
  isclose(fdcus);
  fclose(sysfd);
  return TRUE;
}

/**********************************************************
 * customer change the start_date and end_date            *
 **********************************************************/
int
Wipc_moddate(buffer)
char buffer[WIPC_MAX_MESSAGE_SIZE];
{
        int cc,i;
        char buf[6][30];
        char *token;
        FILE *fp,*sysfd;

        if ((sysfd=fopen("/home/workfile/errlog","a+")) == NULL)
                printf("moddate:Open errlog Failed !\n");

        fdcus=cc=isopen("/DB/vircust",ISMANULOCK+ISINOUT);
        if (cc < 0)
        {
                fprintf(sysfd,"cusmoddate:isopen error %d for vircust file\n",iserrno);
                fclose(sysfd);
                return;
        }
        i=0;
        token=strtok(buffer,tokensep);
        while (token !=NULL)
        {
                sprintf(buf[i],"%s",token);
                i++;
                token=strtok(NULL,tokensep);
        }
        /* Using faxno to find the record , if this record was locked, try again */
        stchar(buf[0],&cusrec[0],20);
        readit:
        cc=isread(fdcus,cusrec,ISEQUAL);
        if (cc == 0)
        {
                /* Putting the modify data to record */
                stchar(buf[1],&cusrec[396],10);
                stchar(buf[2],&cusrec[386],10);
                islock(fdcus);
                cc=isrewrite(fdcus,cusrec);
                if (cc != 0)
                {
                         fprintf(sysfd,"cusmoddate:isrewrite error %d for vircust file\n",iserrno);
                         isunlock(fdcus);
                         isclose(fdcus);
                         fclose(sysfd);
                         return FALSE;
                }
                isunlock(fdcus);
        }
        else
        {
                if (iserrno == ELOCKED || iserrno == EFLOCKED)
                {
                        sleep(1);
                        goto readit;
                }
                else
                {
                        if (iserrno == ENOREC)
                                fprintf(sysfd,"cusmoddate:NO This Record[%s] EXIST\n",buf[0]);
                        else
                                fprintf(sysfd,"cusmoddate:isread error %d for vircust file\n",iserrno);
                        isclose(fdcus);
                        fclose(sysfd);
                        return FALSE;
                }
        }
        isclose(fdcus);
        fclose(sysfd);
        return TRUE;
}

/*************************************************************
 *  Add the New sales record                                 *
 *************************************************************/
int
Wipc_saleadd(buffer)
char buffer[WIPC_MAX_MESSAGE_SIZE];
{
  int cc,i,ch,flag,DONE;
  char buf[8][80],passwd[5],chmodmsg[50];
  char *token;
  FILE *sysfd;


  if ((sysfd=fopen("/home/workfile/errlog","a+")) == NULL)
    printf("virsaleadd:Open errlog Failed !\n");

  i=0;
  /* parsing the buffer to each field data */
  token = strtok(buffer,tokensep);
  while (token != NULL)
  {
    sprintf(buf[i],"%s",token);
    i++;
    token = strtok(NULL,tokensep);
  }

  passwd[0] = buf[1][3] - 10;
  passwd[1] = buf[1][1] - 10;
  passwd[2] = buf[1][2] - 10;
  passwd[3] = buf[1][0] - 10;
  passwd[4] = '\0';

  fdsales=cc=isopen("/DB/virsales",ISMANULOCK+ISINOUT);
  if (cc < 0)
  {
    if (iserrno == 2)
    {
	/* Set up Sales Key (salesno) in virsales.dat file*/
  	skey.k_flags = ISNODUPS;
  	skey.k_nparts = 1;
  	skey.k_part[0].kp_start = 0;
  	skey.k_part[0].kp_leng = 5;
  	skey.k_part[0].kp_type = CHARTYPE;

  	fdsales = cc = isbuild("/DB/virsales",300 , &skey,ISINOUT + ISEXCLLOCK);
  	if (cc < SUCCESS)
  	{
    		printf("isbuild error %d for virsales file\n",iserrno);
                fclose(sysfd);
                return FALSE;
        }
        else
        {       sprintf(chmodmsg,"chmod 777 /DB/virsales.*");
                system(chmodmsg);
        }
    }
    else
    {
    	fprintf(sysfd,"virsaleadd:isopen error %d for virsales file\n",iserrno);
    	fclose(sysfd);
    	return FALSE;
    }
  }
  /**************************
   * Putting data to record *
   **************************/
  stchar(buf[0],&salesrec[0],5);
  readit:
  cc = isread(fdsales,salesrec,ISEQUAL);
  stchar(passwd,&salesrec[5],4);
  stchar(buf[2],&salesrec[9],40);
  stchar(buf[3],&salesrec[49],80);
  stchar(buf[4],&salesrec[129],15);
  stchar(buf[5],&salesrec[144],15);
  stchar(buf[6],&salesrec[159],20);
  stchar(buf[7],&salesrec[179],1);
  if (cc == 0)
  {
    islock(fdsales);
    cc = isrewrite(fdsales,salesrec);
    if (cc != 0)
    {
      fprintf(sysfd,"virsaleadd:isrewrite error %d for sales file\n",iserrno);
      isunlock(fdsales);
      isclose(fdsales);
      fclose(sysfd);
      return FALSE;
    }
  }
  else if (iserrno == ENOREC)
  {
    islock(fdsales);
    cc = iswrite(fdsales,salesrec);
    if (cc != 0)
    {
      fprintf(sysfd,"virsaleadd:iswrite error %d for sales file\n",iserrno);
      isunlock(fdsales);
      isclose(fdsales);
      fclose(sysfd);
      return FALSE;
    }
  }
  else if (iserrno == ELOCKED || iserrno == EFLOCKED)
  {
    sleep(1);
    goto readit;
  }
  isunlock(fdsales);
  isclose(fdsales);
  fclose(sysfd);
  return TRUE;
}

/*************************************************************
 * Delete the sales record                                   *
 *************************************************************/
int
Wipc_saledel(buffer)
char buffer[WIPC_MAX_MESSAGE_SIZE];
{
  int cc,i,ch,flag,DONE;
  char chmodmsg[50];
  char *token;
  FILE *sysfd;


  if ((sysfd=fopen("/home/workfile/errlog","a+")) == NULL)
    printf("virsaledel:Open errlog Failed !\n");

  fdsales=cc=isopen("/DB/virsales",ISMANULOCK+ISINOUT);
  if (cc < 0)
  {
    if (iserrno == 2)
    {
        /* Set up Sales Key (salesno) in virsales.dat file*/
        skey.k_flags = ISNODUPS;
        skey.k_nparts = 1;
        skey.k_part[0].kp_start = 0;
        skey.k_part[0].kp_leng = 5;
        skey.k_part[0].kp_type = CHARTYPE;

        fdsales = cc = isbuild("/DB/virsales",300 , &skey,ISINOUT + ISEXCLLOCK);
        if (cc < SUCCESS)
        {
                printf("isbuild error %d for virsales file\n",iserrno);
                fclose(sysfd);
                return FALSE;
        }
        else
        {       sprintf(chmodmsg,"chmod 777 /DB/virsales.*");
                system(chmodmsg);
        }
    }
    else
    {
        fprintf(sysfd,"virsaleadd:isopen error %d for virsales file\n",iserrno);
        fclose(sysfd);
        return FALSE;
    }
  }
  stchar(buffer,&salesrec[0],5);
  readit:
  cc = isread(fdsales,salesrec,ISEQUAL);
  if (cc == 0)
  {
    islock(fdsales);
    cc = isdelete(fdsales,salesrec);
    if (cc != 0)
    {
      fprintf(sysfd,"virsaledel:isdelete error %d for virsales file\n",iserrno);
      isunlock(fdsales);
      isclose(fdsales);
      fclose(sysfd);
      return FALSE;
    }
    isunlock(fdsales);
  }
  else
  {
    if (iserrno == ELOCKED || iserrno == EFLOCKED)
    {
      sleep(1);
      goto readit;
    }
    else
    {
      if (iserrno == ENOREC)
        fprintf(sysfd,"virsaledel:NO this sales[%s] EXIST\n",buffer);
      else
        fprintf(sysfd,"virsaledel:isread error %d for sales file\n",iserrno);
      isclose(fdsales);
      fclose(sysfd);
      return FALSE;
    }
  }
  isclose(fdsales);
  fclose(sysfd);
  return TRUE;
}

/*************************************************************
 *  Modify the sales record                                  *
 *************************************************************/
int
Wipc_salemod(buffer)
char buffer[WIPC_MAX_MESSAGE_SIZE];
{
  int cc,i,ch,flag,DONE;
  char buf[8][80],passwd[5],chmodmsg[50];
  char *token;
  FILE *sysfd;


  if ((sysfd=fopen("/home/workfile/errlog","a+")) == NULL)
    printf("virsalemod:Open errlog Failed !\n");

  i=0;
  token = strtok(buffer,tokensep);
  while (token != NULL)
  {
    sprintf(buf[i],"%s",token);
    i++;
    token = strtok(NULL,tokensep);
  }

  passwd[0] = buf[1][3] - 10;
  passwd[1] = buf[1][1] - 10;
  passwd[2] = buf[1][2] - 10;
  passwd[3] = buf[1][0] - 10;
  passwd[4] = '\0';

  fdsales=cc=isopen("/DB/virsales",ISMANULOCK+ISINOUT);
  if (cc < 0)
  {
  if (iserrno == 2)
    {
        /* Set up Sales Key (salesno) in virsales.dat file*/
        skey.k_flags = ISNODUPS;
        skey.k_nparts = 1;
        skey.k_part[0].kp_start = 0;
        skey.k_part[0].kp_leng = 5;
        skey.k_part[0].kp_type = CHARTYPE;

        fdsales = cc = isbuild("/DB/virsales",300 , &skey,ISINOUT + ISEXCLLOCK);
        if (cc < SUCCESS)
        {
                printf("isbuild error %d for virsales file\n",iserrno);
                fclose(sysfd);
                return FALSE;
        }
        else
        {       sprintf(chmodmsg,"chmod 777 /DB/virsales.*");
                system(chmodmsg);
        }
    }
    else
    {
        fprintf(sysfd,"virsaleadd:isopen error %d for virsales file\n",iserrno);
        fclose(sysfd);
        return FALSE;
    }
  }
  /****************************************
   * Using Primary key to find the record *
   ****************************************/
  stchar(buf[0],&salesrec[0],5);
  readit:
  cc = isread(fdsales,salesrec,ISEQUAL);
  /**************************
   * Putting data to record *
   **************************/
  stchar(buf[0],&salesrec[0],5);
  stchar(passwd,&salesrec[5],4);
  stchar(buf[2],&salesrec[9],40);
  stchar(buf[3],&salesrec[49],80);
  stchar(buf[4],&salesrec[129],15);
  stchar(buf[5],&salesrec[144],15);
  stchar(buf[6],&salesrec[159],20);
  stchar(buf[7],&salesrec[179],1);

  if (cc == 0)
  {
    islock(fdsales);
    cc = isrewrite(fdsales,salesrec);
    if (cc != 0)
    {
      fprintf(sysfd,"virsaleadd:isrewrite error %d for sales file\n",iserrno);
      isunlock(fdsales);
      isclose(fdsales);
      fclose(sysfd);
      return FALSE;
    }
  }
  else if (iserrno == ENOREC)
  {
    islock(fdsales);
    cc = iswrite(fdsales,salesrec);
    if (cc != 0)
    {
      fprintf(sysfd,"virsaleadd:iswrite error %d for sales file\n",iserrno);
      isunlock(fdsales);
      isclose(fdsales);
      fclose(sysfd);
      return FALSE;
    }
  }
  else if (iserrno == ELOCKED || iserrno == EFLOCKED)
  {
    sleep(1);
    goto readit;
  }
  fclose(sysfd);
  isclose(fdsales);
  return TRUE;
}

/*************************************************************
 *  update the customer password                             *
 *************************************************************/
int
Wipc_updateps(buffer)
char buffer[WIPC_MAX_MESSAGE_SIZE];
{
  int cc,i,ch,flag,DONE;
  char buf[3][80],passwd[5];
  char *token;
  FILE *fp,*sysfd;


  if ((sysfd=fopen("/home/workfile/errlog","a+")) == NULL)
    printf("virupdateps:Open errlog Failed !\n");

  i=0;
  token = strtok(buffer,tokensep);
  while (token != NULL)
  {
    sprintf(buf[i],"%s",token);
    i++;
    token = strtok(NULL,tokensep);
  }
  passwd[0] = buf[1][3] - 10;
  passwd[1] = buf[1][1] - 10;
  passwd[2] = buf[1][2] - 10;
  passwd[3] = buf[1][0] - 10;
  passwd[4] = '\0';

  fdcus=cc=isopen("/DB/vircust",ISMANULOCK+ISINOUT);
  if (cc < 0)
  {
    fprintf(sysfd,"virupdateps:isopen error %d for vircust file\n",iserrno);
    fclose(sysfd);
    return FALSE;
  }
  /* Using faxno to find the record ,if this record was locked,try again */
  stchar(buf[0],&cusrec[0],20);
  readit:
  cc = isread(fdcus,cusrec,ISEQUAL);
  if (cc == 0)
  {
    /* Putting the modify data to record */
    stchar(passwd,&cusrec[60],4);
    /**************************
     * Putting data to record *
     **************************/
    islock(fdcus);
    cc = isrewrite(fdcus,cusrec);
    if (cc != 0)
    {
      fprintf(sysfd,"virupdateps:isrewrite error %d for vircust file\n",iserrno);
      isunlock(fdcus);
      isclose(fdcus);
      fclose(sysfd);
      return FALSE;
    }
    isunlock(fdcus);
  }
  else
  {
    if (iserrno == ELOCKED || iserrno == EFLOCKED)
    {
      sleep(1);
      goto readit;
    }
    else
    {
      if (iserrno == ENOREC)
        fprintf(sysfd,"virupdateps:NO this accno[%s] EXIST\n",buf[0]);
      else
        fprintf(sysfd,"virupdateps:isread error %d for vircust file\n",iserrno);
      isclose(fdcus);
      fclose(sysfd);
      return FALSE;
    }
  }
  isclose(fdcus);
  fclose(sysfd);
  return TRUE;
}

