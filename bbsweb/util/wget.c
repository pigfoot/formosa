#include "bbs.h"

#if 0
char *hostname="bbs.nsysu.edu.tw";
char *hostip = "140.117.11.2";
char *uri = "/Data/forecast/W01.txt";
char *uri = "/";
#endif


int main(int argc, char *argv[])
{
	char *hostip, *uri;
	char temp[2048];
	int sd;

	if(argc != 3)
	{
		fprintf(stderr, "Usage: %s hostip uri\n", argv[0]);
		return -1;
	}

	hostip = argv[1];
	uri = argv[2];

	if((sd = ConnectServer(hostip, 80)) > 0)
	{
		FILE *fin, *fout, *data;


		fin = fdopen(sd, "r");
		fout = fdopen(sd, "w");

		fprintf(fout, "GET %s HTTP/1.0\r\n", uri);
		fprintf(fout, "Host: %s\r\n\r\n", hostip);
		fflush(fout);


		if((data = fopen("data.txt", "w"))!=NULL)
		{
			while(fgets(temp, sizeof(temp), fin))
			{
				char *p;
				if((p = strrchr(temp, '\r')) != NULL)
				{
					*p = '\n';
					*(p+1) = 0x00;
				}
				fputs(temp, data);

			}

			fclose(data);
		}

		close(sd);
	}
	else
	{
		fprintf(stderr, "Connect Server %s error...\n", hostip);
	}

	return 0;
}
