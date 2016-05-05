/*--------------------------------------------------------------------
This file is part of the Arduino WiFiEsp library.

The Arduino WiFiEsp library is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

The Arduino WiFiEsp library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with The Arduino WiFiEsp library.  If not, see
<http://www.gnu.org/licenses/>.
--------------------------------------------------------------------*/

#ifndef RingBuffer_h
#define RingBuffer_h

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

template <uint8_t size>
class EspRingBuffer
{
public:
	EspRingBuffer()
	{
	    ringBufEnd = &ringBuf[size];
	    reset();
	}

	void reset()
	{
	    ringBufP = ringBuf;
	}

	void init()
    {
        reset();
        memset(ringBuf, 0, size+1);
    }

	void push(char c)
	{
	    *ringBufP = c;
	    ringBufP++;
	    if (ringBufP>=ringBufEnd)
	        ringBufP = ringBuf;
	}

	bool endsWith(const char* str)
	{
	    int findStrLen = strlen(str);

	    // b is the start position into the ring buffer
	    char* b = ringBufP-findStrLen;
	    if(b < ringBuf) {
	        b += size;
	    }

	    char *p1 = (char*)&str[0];
	    char *p2 = p1 + findStrLen;

	    for(char *p=p1; p<p2; p++)
	    {
	        if(*p != *b)
	            return false;

	        b++;
	        if (b == ringBufEnd)
	            b=ringBuf;
	    }

	    return true;
	}

	void getStr(char * destination, unsigned int skipChars)
	{
	    int len = ringBufP-ringBuf-skipChars;

	    // copy buffer to destination string
	    strncpy(destination, ringBuf, len);

	    // terminate output string
	    //destination[len]=0;
	}

	void getStrN(char * destination, unsigned int skipChars, unsigned int num)
	{
	    unsigned int len = ringBufP-ringBuf-skipChars;

	    if (len>num)
	        len=num;

	    // copy buffer to destination string
	    strncpy(destination, ringBuf, len);

	    // terminate output string
	    //destination[len]=0;
	}

	int getLength() {
	   return ringBufP - ringBuf;
	}

private:
	// add one char to terminate the string
	char ringBuf[size + 1] = {0};
	char* ringBufEnd;
	char* ringBufP;
};

#endif
