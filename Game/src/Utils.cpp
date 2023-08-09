#include "Utils.h"

void
TextSplitBuffered(const char* text, char delimiter, int* _RESTRICT_ count,
	char* _RESTRICT_ buffer, int bufferLength,
	char** _RESTRICT_ splitBuffer, int splitBufferLength)
{
	SASSERT(text);
	SASSERT(count);
	SASSERT(buffer);
	SASSERT(bufferLength > 0);
	SASSERT(splitBuffer);
	SASSERT(splitBufferLength > 0);
	
	splitBuffer[0] = buffer;
	int counter = 0;

	if (text != NULL)
	{
		counter = 1;

		// Count how many substrings we have on text and point to every one
		for (int i = 0; i < bufferLength; ++i)
		{
			buffer[i] = text[i];
			if (buffer[i] == '\0') 
				break;

			else if (buffer[i] == delimiter)
			{
				buffer[i] = '\0';   // Set an end of string at this point
				splitBuffer[counter] = buffer + i + 1;
				counter++;

				if (counter == splitBufferLength) break;
			}
		}
	}

	*count = counter;
}

STR2INT 
Str2Int(int* out, const char* s, int base)
{
	char* end;
	if (s[0] == '\0' || isspace(s[0]))
		return STR2INT_INCONVERTIBLE;
	errno = 0;
	long l = strtol(s, &end, base);
	/* Both checks are needed because INT_MAX == LONG_MAX is possible. */
	if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX))
		return STR2INT_OVERFLOW;
	if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN))
		return STR2INT_UNDERFLOW;
	if (*end != '\0')
		return STR2INT_INCONVERTIBLE;
	*out = l;
	return STR2INT_SUCCESS;
}

STR2INT 
Str2UInt(u32* out, const char* s, int base)
{
	char* end;
	if (s[0] == '\0' || isspace(s[0]))
		return STR2INT_INCONVERTIBLE;
	errno = 0;
	u32 l = strtoul(s, &end, base);
	if (*end != '\0')
		return STR2INT_INCONVERTIBLE;
	*out = l;
	return STR2INT_SUCCESS;
}


int 
FastAtoi(const char* str)
{
	SASSERT(str);
	int val = 0;
	for (; *str; ++str)
	{
		val = val * 10 + (*str - '0');
	}
	return val;
}

Color IntToColor(int colorInt)
{
	Color c;
	c.r = (i8)(colorInt >> 24);
	c.g = (i8)(colorInt >> 16);
	c.b = (i8)(colorInt >> 8);
	c.a = (i8)colorInt;
	return c;
}
