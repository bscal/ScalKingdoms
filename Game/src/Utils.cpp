#include "Utils.h"

#include "Core.h"

void
TextSplitBuffered(const char* text, char delimiter, int* _RESTRICT_ count,
	char* _RESTRICT_ buffer, int bufferLength,
	char** _RESTRICT_ splitBuffer, int splitBufferLength)
{
	SAssert(text);
	SAssert(count);
	SAssert(buffer);
	SAssert(bufferLength > 0);
	SAssert(splitBuffer);
	SAssert(splitBufferLength > 0);
	
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

void RemoveWhitespace(char* s)
{
	char* d = s;
	do
	{
		while (isspace(*d))
		{
			++d;
		}
		*s = *d;
		++s;
		++d;
	} while (*s);
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
