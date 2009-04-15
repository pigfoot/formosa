
char inalphabet[256];
char decoder[256];
unsigned char alphabet[64] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int
main()
{
	int i;


	for (i = (sizeof alphabet) - 1; i >= 0 ; i--)
	{
		inalphabet[alphabet[i]] = 1;
		decoder[alphabet[i]] = i;
	}

	printf("\nchar inalphabet[] = \n{");
	for (i = 0; i <= 255; i++)
	{
		printf("%d", inalphabet[i]);
		if (i != 255)
			printf(", ");
		if (i % 20 == 19)
			printf("\n");
	}
	printf("};\n");
	printf("\nchar decoder[] = \n{");
	for (i = 0; i <= 255; i++)
	{
		printf("%d", decoder[i]);
		if (i != 255)
			printf(", ");
		if (i % 20 == 19)
			printf("\n");
	}
	printf("};\n");
}
