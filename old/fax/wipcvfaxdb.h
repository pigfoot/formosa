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
#include <isam.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SUCCESS 0
#define False 0
#define True  1
#define WHOLEKEY 0

/*    Globe  Declare    */
char line[82];
char tokensep[] = "!!";
int finished = False;
int cus_eof = False;

char cusrec[600],raterec[20],trarec[150];/* Record size in File */
char salesrec[480],telrate[20];
struct keydesc ckey,key,tkey,skey,rkey,tkey,bkey;  /* C-ISAM File Structure */
int   fdcus,fdrate,fdtran,fdsales;   /* C-ISAM File Descriptor */


