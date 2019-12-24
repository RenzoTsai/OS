#include "string.h"

int strlen(char *src)
{
	int i;
	for (i = 0; src[i] != '\0'; i++)
	{
	}
	return i;
}

void memcpy(uint8_t *dest, uint8_t *src, uint32_t len)
{
	for (; len != 0; len--)
	{
		*dest++ = *src++;
	}
}

void memset(void *dest, uint8_t val, uint32_t len)
{
	uint8_t *dst = (uint8_t *)dest;

	for (; len != 0; len--)
	{
		*dst++ = val;
	}
}

void bzero(void *dest, uint32_t len)
{
	memset(dest, 0, len);
}

// int strcmp(char *str1, char *str2)
// {
// 	while (*str1 && *str2 && (*str1++ == *str2++))
// 	{
// 	};

// 	return (*str1) - (*str2);
// }
int strcmp(char *str1, char *str2)
{
	while (*str1 && *str2 && (*str1 == *str2))
	{
		str1++;
		str2++;
		//printk("hh\n");
	}
 	return (*str1) - (*str2);
}

int memcmp(char *str1, char *str2, uint32_t size)
{
	int i;

	for (i = 0; i < size; i++)
	{
		if (str1[i] > str2[i])
		{
			return 1;
		}
		else if (str1[i] < str2[i])
		{
			return -1;
		}
	}
	return 0;
}

void strcpy(char *dest, char *src)
{
	int l = strlen(src);
	int i;

	for (i = 0; i < l; i++)
	{
		dest[i] = src[i];
	}

	dest[i] = '\0';
}

int atoi(char *str)
{
	int len = strlen(str);
	int num = 0, i;

	for(i = 0; i < len; i++)
		num = num*10 + str[i]-'0';
	
	return num;
}