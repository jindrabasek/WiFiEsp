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

#include <Arduino.h>
#include <avr/pgmspace.h>

#include "utility/EspDrv.h"
#include "utility/debug.h"


#define NUMESPTAGS 5

static const char TAG1[] PROGMEM = "\r\nOK\r\n";
static const char TAG2[] PROGMEM = "\r\nERROR\r\n";
static const char TAG3[] PROGMEM = "\r\nFAIL\r\n";
static const char TAG4[] PROGMEM = "\r\nSEND OK\r\n";
static const char TAG5[] PROGMEM = " CONNECT\r\n";

static const char* const ESPTAGS[] PROGMEM = { TAG1, TAG2, TAG3, TAG4, TAG5, };

static const char CWLAP_EXPECT[] PROGMEM = "+CWLAP:(";

typedef enum
{
	TAG_OK,
	TAG_ERROR,
	TAG_FAIL,
	TAG_SENDOK,
	TAG_CONNECT
} TagsEnum;


SerialHolder *EspDrv::espSerial = NULL;
int8_t EspDrv::resetPin = -1;

EspRingBuffer<EspDrv::RING_BUFFER_SIZE> EspDrv::ringBuf;

// Array of data to cache the information related to the networks discovered
char 	EspDrv::_networkSsid[][WL_SSID_MAX_LENGTH] = {{"1"},{"2"},{"3"},{"4"},{"5"}};
int32_t EspDrv::_networkRssi[WL_NETWORKS_LIST_MAXNUM] = { 0 };
uint8_t EspDrv::_networkEncr[WL_NETWORKS_LIST_MAXNUM] = { 0 };

// Cached values of retrieved data
char EspDrv::_ssid[] = {0};
uint8_t EspDrv::_bssid[] = {0};
uint8_t EspDrv::_mac[] = {0};
char EspDrv::fwVersion[] = {0};

long EspDrv::_bufPos=0;
uint8_t EspDrv::_connId=0;

uint16_t EspDrv::yield_every_n_chars = 0;

#ifndef YIELD_EVERY_N_MILLIS
#define YIELD_EVERY_N_MILLIS 4
#endif


void EspDrv::wifiDriverInit(SerialHolder *espSerial, unsigned long baudRate, int8_t resetPin, unsigned long originalBaudRate)
{
	LOGDEBUG(F("> wifiDriverInit"));

	EspDrv::espSerial = espSerial;
	EspDrv::resetPin = resetPin;
	EspDrv::yield_every_n_chars = baudRate * ((float)YIELD_EVERY_N_MILLIS/1000);

	espSerial->end();
	espSerial->begin(baudRate);
	delay(100);


	bool initOK = false;
	
	for(int i=0; i<5; i++)
	{
		if (sendCmd(F("AT")) == TAG_OK)
		{
			initOK=true;
			break;
		}
		delay(1000);
	}

	if (!initOK)
	{
		LOGERROR(F("Cannot initialize ESP module"));

		delay(2000);

		if (baudRate != originalBaudRate) {
			espSerial->end();
			espSerial->begin(originalBaudRate);
			delay(100);

			if (sendCmd(F("AT")) != TAG_OK) {
				LOGERROR(F("Cannot initialize ESP module using original baud rate"));
				return;
			} else {
				sendCmd(F("AT+UART_DEF=%d,8,1,0,0"), 5000, baudRate);

					espSerial->end();
					espSerial->begin(baudRate);

					delay(2000);

					if (sendCmd(F("AT")) != TAG_OK)
					{
						LOGERROR(F("Cannot initialize ESP module after baud rate change"));
						delay(2000);
						return;
					}
			}
		} else {
			return;
		}
	}

	reset();
}

// Folowing method is taken from Adafruit WiFi ESP8266 library
// https://github.com/adafruit/Adafruit_ESP8266/blob/master/Adafruit_ESP8266.cpp
// ESP8266 is reset by momentarily connecting RST to GND.  Level shifting is
// not necessary provided you don't accidentally set the pin to HIGH output.
// It's generally safe-ish as the default Arduino pin state is INPUT (w/no
// pullup) -- setting to LOW provides an open-drain for reset.
bool EspDrv::hardReset() {
  if (resetPin < 0) {
      return false;
  }

  if (espSerial != NULL) {
      espSerial->end();
  }

  digitalWrite(resetPin, LOW);
  pinMode(resetPin, OUTPUT); // Open drain; reset -> GND
  delay(20);                  // Hold a moment
  pinMode(resetPin, INPUT);  // Back to high-impedance pin state
  delay(5000);

  flushReceiveBuffer();
  _connId = 0;

  return true;
}

void EspDrv::reset()
{
	LOGDEBUG(F("> reset"));

	sendCmd(F("AT+RST"));
	delay(3000);
	espEmptyBuf(false);  // empty dirty characters from the buffer

	// disable echo of commands
	sendCmd(F("ATE0"));

	// set station mode
	sendCmd(F("AT+CWMODE=1"));
	delay(200);

	// set multiple connections mode
	sendCmd(F("AT+CIPMUX=1"));

	// Show remote IP and port with "+IPD"
	sendCmd(F("AT+CIPDINFO=1"));
	
	// Disable autoconnect
	// Automatic connection can create problems during initialization phase at next boot
	sendCmd(F("AT+CWAUTOCONN=0"));

	// enable DHCP
	sendCmd(F("AT+CWDHCP=1,1"));
	delay(200);

	flushReceiveBuffer();
	_connId = 0;
}

bool EspDrv::wifiConnect(const char* ssid, const char *passphrase)
{
	LOGDEBUG(F("> wifiConnect"));

	// TODO
	// Escape character syntax is needed if "SSID" or "password" contains
	// any special characters (',', '"' and '/')

    // connect to access point, use CUR mode to avoid connection at boot
	int ret = sendCmd(F("AT+CWJAP_CUR=\"%s\",\"%s\""), 20000, ssid, passphrase);

	if (ret==TAG_OK)
	{
		LOGINFO1(F("Connected to"), ssid);
		return true;
	}

	LOGWARN1(F("Failed connecting to"), ssid);

	// clean additional messages logged after the FAIL tag
	delay(1000);
	espEmptyBuf(false);

	return false;
}


bool EspDrv::wifiStartAP(char* ssid, const char* pwd, uint8_t channel, uint8_t enc, uint8_t espMode)
{
	LOGDEBUG(F("> wifiStartAP"));

	// set AP mode, use CUR mode to avoid automatic start at boot
    int ret = sendCmd(F("AT+CWMODE_CUR=%d"), 10000, espMode);
	if (ret!=TAG_OK)
	{
		LOGWARN1(F("Failed to set AP mode"), ssid);
		return false;
	}

	// TODO
	// Escape character syntax is needed if "SSID" or "password" contains
	// any special characters (',', '"' and '/')

	// start access point
	ret = sendCmd(F("AT+CWSAP_CUR=\"%s\",\"%s\",%d,%d"), 10000, ssid, pwd, channel, enc);

	if (ret!=TAG_OK)
	{
		LOGWARN1(F("Failed to start AP"), ssid);
		return false;
	}
	
	if (espMode==2)
		sendCmd(F("AT+CWDHCP_CUR=0,1"));    // enable DHCP for AP mode
	if (espMode==3)
		sendCmd(F("AT+CWDHCP_CUR=2,1"));    // enable DHCP for station and AP mode

	LOGINFO1(F("Access point started"), ssid);
	return true;
}


int8_t EspDrv::disconnect()
{
	LOGDEBUG(F("> disconnect"));

	if(sendCmd(F("AT+CWQAP"))==TAG_OK)
		return WL_DISCONNECTED;

	// wait and clear any additional message
	delay(2000);
	espEmptyBuf(false);

	return WL_DISCONNECTED;
}

void EspDrv::config(IPAddress ip)
{
	LOGDEBUG(F("> config"));

	// disable station DHCP
	sendCmd(F("AT+CWDHCP_CUR=1,0"));
	
	// it seems we need to wait here...
	delay(500);
	
	char buf[16];
	sprintf(buf, PSTR("%d.%d.%d.%d"), ip[0], ip[1], ip[2], ip[3]);

	int ret = sendCmd(F("AT+CIPSTA_CUR=\"%s\""), 2000, buf);
	delay(500);

	if (ret==TAG_OK)
	{
		LOGINFO1(F("IP address set"), buf);
	}
}

void EspDrv::configAP(IPAddress ip)
{
	LOGDEBUG(F("> config"));
	
    sendCmd(F("AT+CWMODE_CUR=2"));
	
	// disable station DHCP
	sendCmd(F("AT+CWDHCP_CUR=2,0"));
	
	// it seems we need to wait here...
	delay(500);
	
	char buf[16];
	sprintf(buf, PSTR("%d.%d.%d.%d"), ip[0], ip[1], ip[2], ip[3]);

	int ret = sendCmd(F("AT+CIPAP_CUR=\"%s\""), 2000, buf);
	delay(500);

	if (ret==TAG_OK)
	{
		LOGINFO1(F("IP address set"), buf);
	}
}

uint8_t EspDrv::getConnectionStatus()
{
	LOGDEBUG(F("> getConnectionStatus"));

/*
	AT+CIPSTATUS

	Response

		STATUS:<stat>
		+CIPSTATUS:<link ID>,<type>,<remote_IP>,<remote_port>,<local_port>,<tetype>

	Parameters

		<stat>
			2: Got IP
			3: Connected
			4: Disconnected
		<link ID> ID of the connection (0~4), for multi-connect
		<type> string, "TCP" or "UDP"
		<remote_IP> string, remote IP address.
		<remote_port> remote port number
		<local_port> ESP8266 local port number
		<tetype>
			0: ESP8266 runs as client
			1: ESP8266 runs as server
*/

	char buf[10];
	if(!sendCmdGet(F("AT+CIPSTATUS"), F("STATUS:"), F("\r\n"), buf, sizeof(buf)))
		return WL_NO_SHIELD;

	// 4: client disconnected
	// 5: wifi disconnected
	byte s = atoi(buf);
	if(s==2 or s==3 or s==4)
		return WL_CONNECTED;
	else if(s==5)
		return WL_DISCONNECTED;

	return WL_IDLE_STATUS;
}

uint8_t EspDrv::getClientState(uint8_t sock)
{
	LOGDEBUG1(F("> getClientState"), sock);

	char findBuf[20];
	sprintf_P(findBuf, PSTR("+CIPSTATUS:%d,"), sock);

	char buf[10];
	if (sendCmdGet(F("AT+CIPSTATUS"), findBuf, ",", buf, sizeof(buf)))
	{
		LOGDEBUG(F("Connected"));
		return true;
	}

	LOGDEBUG(F("Not connected"));
	return false;
}

uint8_t* EspDrv::getMacAddress()
{
	LOGDEBUG(F("> getMacAddress"));

	memset(_mac, 0, WL_MAC_ADDR_LENGTH);

	char buf[20];
	if (sendCmdGet(F("AT+CIFSR"), F(":STAMAC,\""), F("\""), buf, sizeof(buf)))
	{
		char* token;

		token = strtok(buf, ":");
		_mac[5] = (byte)strtol(token, NULL, 16);
		token = strtok(NULL, ":");
		_mac[4] = (byte)strtol(token, NULL, 16);
		token = strtok(NULL, ":");
		_mac[3] = (byte)strtol(token, NULL, 16);
		token = strtok(NULL, ":");
		_mac[2] = (byte)strtol(token, NULL, 16);
		token = strtok(NULL, ":");
		_mac[1] = (byte)strtol(token, NULL, 16);
		token = strtok(NULL, ":");
		_mac[0] = (byte)strtol(token, NULL, 16);
	}
	return _mac;
}


void EspDrv::getIpAddress(IPAddress& ip)
{
	LOGDEBUG(F("> getIpAddress"));

	char buf[20];
	if (sendCmdGet(F("AT+CIFSR"), F(":STAIP,\""), F("\""), buf, sizeof(buf)))
	{
		char* token;

		uint8_t  _localIp[WL_IPV4_LENGTH];
		token = strtok(buf, ".");
		_localIp[0] = atoi(token);
		token = strtok(NULL, ".");
		_localIp[1] = atoi(token);
		token = strtok(NULL, ".");
		_localIp[2] = atoi(token);
		token = strtok(NULL, ".");
		_localIp[3] = atoi(token);

		ip = _localIp;
	}
}

void EspDrv::getIpAddressAP(IPAddress& ip)
{
	LOGDEBUG(F("> getIpAddressAP"));

	char buf[20];
	if (sendCmdGet(F("AT+CIPAP?"), F("+CIPAP:ip:\""), F("\""), buf, sizeof(buf)))
	{
		char* token;

		uint8_t  _localIp[WL_IPV4_LENGTH];
		token = strtok(buf, ".");
		_localIp[0] = atoi(token);
		token = strtok(NULL, ".");
		_localIp[1] = atoi(token);
		token = strtok(NULL, ".");
		_localIp[2] = atoi(token);
		token = strtok(NULL, ".");
		_localIp[3] = atoi(token);

		ip = _localIp;
	}
}



char* EspDrv::getCurrentSSID()
{
	LOGDEBUG(F("> getCurrentSSID"));

	_ssid[0] = 0;
	sendCmdGet(F("AT+CWJAP?"), F("+CWJAP:\""), F("\""), _ssid, sizeof(_ssid));

	return _ssid;
}

uint8_t* EspDrv::getCurrentBSSID()
{
	LOGDEBUG(F("> getCurrentBSSID"));

	memset(_bssid, 0, WL_MAC_ADDR_LENGTH);

	char buf[20];
	if (sendCmdGet(F("AT+CWJAP?"), F(",\""), F("\","), buf, sizeof(buf)))
	{
		char* token;

		token = strtok(buf, ":");
		_bssid[5] = (byte)strtol(token, NULL, 16);
		token = strtok(NULL, ":");
		_bssid[4] = (byte)strtol(token, NULL, 16);
		token = strtok(NULL, ":");
		_bssid[3] = (byte)strtol(token, NULL, 16);
		token = strtok(NULL, ":");
		_bssid[2] = (byte)strtol(token, NULL, 16);
		token = strtok(NULL, ":");
		_bssid[1] = (byte)strtol(token, NULL, 16);
		token = strtok(NULL, ":");
		_bssid[0] = (byte)strtol(token, NULL, 16);
	}
	return _bssid;

}

int32_t EspDrv::getCurrentRSSI()
{
	LOGDEBUG(F("> getCurrentRSSI"));

    int ret=0;
	char buf[10];
	sendCmdGet(F("AT+CWJAP?"), F(",-"), F("\r\n"), buf, sizeof(buf));

	if (isDigit(buf[0])) {
      ret = -atoi(buf);
    }

    return ret;
}


uint8_t EspDrv::getScanNetworks()
{
    uint8_t ssidListNum = 0;
    int idx;

	espEmptyBuf();

	LOGDEBUG(F("----------------------------------------------"));
	LOGDEBUG(F(">> AT+CWLAP"));
	
	(*espSerial)->println(F("AT+CWLAP"));
	
    char cwlapExpect[sizeof(CWLAP_EXPECT)];
    strcpy_P(cwlapExpect, CWLAP_EXPECT);
	idx = readUntil(10000, cwlapExpect);
	
	while (idx == NUMESPTAGS)
	{
		_networkEncr[ssidListNum] = (*espSerial)->parseInt();
		
		// discard , and " characters
		readUntil(1000, "\"");

		idx = readUntil(1000, "\"", false);
		if(idx==NUMESPTAGS)
		{
			memset(_networkSsid[ssidListNum], 0, WL_SSID_MAX_LENGTH );
			ringBuf.getStrN(_networkSsid[ssidListNum], 1, WL_SSID_MAX_LENGTH-1);
		}
		
		// discard , character
		readUntil(1000, ",");
		
		_networkRssi[ssidListNum] = (*espSerial)->parseInt();
		
		char cwlapExpect[sizeof(CWLAP_EXPECT)];
		strcpy_P(cwlapExpect, CWLAP_EXPECT);
		idx = readUntil(1000, cwlapExpect);

		if(ssidListNum==WL_NETWORKS_LIST_MAXNUM-1)
			break;

		ssidListNum++;
	}
	
	if (idx==-1)
		return -1;

	LOGDEBUG1(F("---------------------------------------------- >"), ssidListNum);
	LOGDEBUG();
    return ssidListNum;
}

bool EspDrv::getNetmask(IPAddress& mask) {
  LOGDEBUG(F("> getNetmask"));

	char buf[20];
	if (sendCmdGet(F("AT+CIPSTA?"), F("+CIPSTA:netmask:\""), F("\""), buf, sizeof(buf)))
	{
    mask.fromString (buf);
    return true;
	}

  return false;
}

bool EspDrv::getGateway(IPAddress& gw) {
  LOGDEBUG(F("> getGateway"));

	char buf[20];
	if (sendCmdGet(F("AT+CIPSTA?"), F("+CIPSTA:gateway:\""), F("\""), buf, sizeof(buf)))
	{
    gw.fromString (buf);
    return true;
	}

  return false;
}

char* EspDrv::getSSIDNetoworks(uint8_t networkItem)
{
	if (networkItem >= WL_NETWORKS_LIST_MAXNUM)
		return NULL;

	return _networkSsid[networkItem];
}

uint8_t EspDrv::getEncTypeNetowrks(uint8_t networkItem)
{
	if (networkItem >= WL_NETWORKS_LIST_MAXNUM) {
		return 0;
	}

    return _networkEncr[networkItem];
}

int32_t EspDrv::getRSSINetoworks(uint8_t networkItem)
{
	if (networkItem >= WL_NETWORKS_LIST_MAXNUM) {
		return 0;
	}

    return _networkRssi[networkItem];
}

char* EspDrv::getFwVersion()
{
	LOGDEBUG(F("> getFwVersion"));

	fwVersion[0] = 0;

	sendCmdGet(F("AT+GMR"), F("SDK version:"), F("\r\n"), fwVersion, sizeof(fwVersion));

    return fwVersion;
}



bool EspDrv::ping(const char *host)
{
	LOGDEBUG(F("> ping"));

	int ret = sendCmd(F("AT+PING=\"%s\""), 2000, host);

	if (ret==TAG_OK)
	{
		return true;
	}

    return false;
}



// Start server TCP on port specified
bool EspDrv::startServer(uint16_t port)
{
	LOGDEBUG1(F("> startServer"), port);

	int ret = sendCmd(F("AT+CIPSERVER=1,%d"), 1000, port);

	return ret==TAG_OK;
}


bool EspDrv::startClient(const char* host, uint16_t port, uint8_t sock, uint8_t protMode)
{
	LOGDEBUG2(F("> startClient"), host, port);
	
	// TCP
	// AT+CIPSTART=<link ID>,"TCP",<remote IP>,<remote port>

	// UDP
	// AT+CIPSTART=<link ID>,"UDP",<remote IP>,<remote port>[,<UDP local port>,<UDP mode>]

	// for UDP we set a dummy remote port and UDP mode to 2
	// this allows to specify the target host/port in CIPSEND

	int ret = TAG_ERROR;
	if (protMode==TCP_MODE)
		ret = sendCmd(F("AT+CIPSTART=%d,\"TCP\",\"%s\",%u"), 5000, sock, host, port);
	else if (protMode==SSL_MODE)
	{
		// better to put the CIPSSLSIZE here because it is not supported before firmware 1.4
		sendCmd(F("AT+CIPSSLSIZE=4096"));
		ret = sendCmd(F("AT+CIPSTART=%d,\"SSL\",\"%s\",%u"), 5000, sock, host, port);
	}
	else if (protMode==UDP_MODE)
		ret = sendCmd(F("AT+CIPSTART=%d,\"UDP\",\"%s\",0,%u,2"), 5000, sock, host, port);

	return ret==TAG_OK;
}


// Start server TCP on port specified
void EspDrv::stopClient(uint8_t sock)
{
	LOGDEBUG1(F("> stopClient"), sock);

	sendCmd(F("AT+CIPCLOSE=%d"), 4000, sock);
    flushReceiveBuffer();
}

void EspDrv::flushReceiveBuffer()
{
    _bufPos = 0;
}


uint8_t EspDrv::getServerState(uint8_t sock)
{
    return 0;
}



////////////////////////////////////////////////////////////////////////////
// TCP/IP functions
////////////////////////////////////////////////////////////////////////////



uint16_t EspDrv::availData(uint8_t connId, uint16_t * _remotePort, uint8_t * _remoteIp)
{
    //LOGDEBUG1(F("Buffer poss availData"), _bufPos);

	// if there is data in the buffer
	if (_bufPos>0)
	{
		if (_connId==connId)
			return _bufPos;
		else if (_connId==0)
			return _bufPos;
	}

    if (readUntil(2000, "+IPD,", false))
    {
        // format is : +IPD,<id>,<len>:<data>
        // format is : +IPD,<ID>,<len>[,<remote IP>,<remote port>]:<data>

        _connId = (*espSerial)->parseInt();    // <ID>
        (*espSerial)->read();                  // ,
        _bufPos = (*espSerial)->parseInt();    // <len>
        (*espSerial)->read();                  // "
        _remoteIp[0] = (*espSerial)->parseInt();    // <remote IP>
        (*espSerial)->read();                  // .
        _remoteIp[1] = (*espSerial)->parseInt();
        (*espSerial)->read();                  // .
        _remoteIp[2] = (*espSerial)->parseInt();
        (*espSerial)->read();                  // .
        _remoteIp[3] = (*espSerial)->parseInt();
        (*espSerial)->read();                  // "
        (*espSerial)->read();                  // ,
        *_remotePort = (*espSerial)->parseInt();     // <remote port>

        (*espSerial)->read();                  // :

        LOGDEBUG();
        LOGDEBUG2(F("Data packet"), _connId, _bufPos);

        if(_connId==connId || connId==0)
            return _bufPos;
    }

	return 0;
}


bool EspDrv::getData(uint8_t connId, uint8_t *data, bool peek, bool* connClose)
{
	if (connId!=_connId)
		return false;


	// see Serial.timedRead

	long _startMillis = millis();
	do
	{
		if ((*espSerial)->available())
		{
			if (peek)
			{
				*data = (char)(*espSerial)->peek();
			}
			else
			{
				*data = (char)(*espSerial)->read();
				_bufPos--;
			}
			//Serial.print((char)*data);

			if (_bufPos == 0)
			{
				// after the data packet a ",CLOSED" string may be received
				// this means that the socket is now closed

                unsigned long start = millis();
                while ((millis() - start < 5)) {
                    if ((*espSerial)->available())
                    {
                        //LOGDEBUG(".2");
                        //LOGDEBUG((*espSerial)->peek());

                        // 48 = '0'
                        if ((*espSerial)->peek()==48+connId)
                        {
                            int idx = readUntil(1000, ",CLOSED\r\n", false);
                            if(idx!=NUMESPTAGS)
                            {
                                LOGERROR(F("Tag CLOSED not found"));
                            }

                            LOGDEBUG();
                            LOGDEBUG(F("Connection closed"));

                            *connClose=true;
                        }
                        break;
                    }
                }
			}

			return true;
		}else {
            yield();
        }
	} while(millis() - _startMillis < 2000);

    // timed out, reset the buffer
	LOGERROR1(F("TIMEOUT:"), _bufPos);

    flushReceiveBuffer();
	_connId = 0;
	*data = 0;
	
	return false;
}

/**
 * Receive the data into a buffer.
 * It reads up to bufSize bytes.
 * @return	received data size for success else -1.
 */
int EspDrv::getDataBuf(uint8_t connId, uint8_t *buf, uint16_t bufSize)
{
	if (connId!=_connId)
		return -1;

	if(_bufPos<bufSize)
		bufSize = _bufPos;
	
	for(uint16_t i=0; i<bufSize; i++)
	{
		int c = timedRead();
		//LOGDEBUG(c);
		if(c==-1)
			return -1;
		
		buf[i] = (char)c;
		_bufPos--;
	}

	return bufSize;
}


bool EspDrv::sendData(uint8_t sock, const uint8_t *data, uint16_t len)
{
	LOGDEBUG2(F("> sendData:"), sock, len);

	char cmdBuf[20];
	sprintf_P(cmdBuf, PSTR("AT+CIPSEND=%d,%u"), sock, len);
	(*espSerial)->println(cmdBuf);

	int idx = readUntil(2000, (char *)">", false);
	if(idx!=NUMESPTAGS)
	{
		LOGERROR(F("Data packet send error (1)"));
		return false;
	}

	(*espSerial)->write(data, len);

	idx = readUntil(3000);
	if(idx!=TAG_SENDOK)
	{
		LOGERROR(F("Data packet send error (2)"));
		return false;
	}

    return true;
}

bool EspDrv::sendDataEx(uint8_t sock, const uint8_t *data, uint16_t len, int & sendexBufferPosition, uint16_t* charsToYield)
{
    LOGDEBUG2(F("> sendDataEx:"), sock, len);

    uint16_t lenReamining = len;

    while (lenReamining > 0) {
        if (sendexBufferPosition == 0) {
            if (!beginPacket(sock)){
                return false;
            }
        }

        uint16_t lenSend = min(lenReamining, SENDEX_BUFFER_LENGTH - (uint16_t)sendexBufferPosition);
        lenReamining -= lenSend;
        sendexBufferPosition += lenSend;

        LOGDEBUG1(F("> sendDataEx lenSend:"), lenSend);
        LOGDEBUG1(F("> sendDataEx sendexBufferPosition:"), sendexBufferPosition);


        while (lenSend--) {
           if (!(*espSerial)->write(*data++)) {
               return false;
           }
           if (!(*charsToYield)--){
               // give time to other threads here
               LOGDEBUG(F("yield sendDataEx"));
               delay(3);
               (*charsToYield) = yield_every_n_chars;
           }
        }

        if (sendexBufferPosition >= SENDEX_BUFFER_LENGTH) {
            if (!endPacket(sendexBufferPosition)) {
                return false;
            }
        }
    }

    return true;
}

// Overridden sendData method for __FlashStringHelper strings
bool EspDrv::sendDataEx(uint8_t sock, const __FlashStringHelper *data, uint16_t len, int & sendexBufferPosition, uint16_t* charsToYield, bool appendCrLf)
{
    LOGDEBUG2(F("> sendDataEx:"), sock, len);

    uint16_t lenReamining = len;

    PGM_P p = reinterpret_cast<PGM_P>(data);
    while (lenReamining > 0) {
        if (sendexBufferPosition == 0) {
            if (!beginPacket(sock)){
                return false;
            }
        }

        uint16_t lenSend = min(lenReamining, SENDEX_BUFFER_LENGTH - (uint16_t)sendexBufferPosition);
        lenReamining -= lenSend;
        sendexBufferPosition += lenSend;

        LOGDEBUG1(F("> sendDataEx lenSend:"), lenSend);
        LOGDEBUG1(F("> sendDataEx sendexBufferPosition:"), sendexBufferPosition);

        while (lenSend--) {
            unsigned char c = pgm_read_byte(p++);
            if (!(*espSerial)->write(c)){
                return false;
            }
            if (!(*charsToYield)--){
                // give time to other threads here
                LOGDEBUG(F("yield sendDataEx flash str"));
                delay(3);
                (*charsToYield) = yield_every_n_chars;
            }
        }

        if (sendexBufferPosition >= SENDEX_BUFFER_LENGTH) {
            if (!endPacket(sendexBufferPosition)) {
                return false;
            }
        }
    }

    if (appendCrLf) {
        return sendDataEx(sock, F("\r\n"), 2, sendexBufferPosition, charsToYield, false);
    }

    return true;
}

bool EspDrv::beginPacket(uint8_t sock)
{
    LOGDEBUG1(F("> beginPacket:"), sock);

    char cmdBuf[25];
    sprintf_P(cmdBuf, PSTR("AT+CIPSENDEX=%d,%u"), sock, SENDEX_BUFFER_LENGTH);
    (*espSerial)->println(cmdBuf);

    int idx = readUntil(2000, (char *)">", false);
    if(idx!=NUMESPTAGS)
    {
        LOGERROR(F("Data packet begin error (1)"));
        return false;
    }
    return true;
}

bool EspDrv::endPacket(int & sendexBufferPosition)
{
    LOGDEBUG1(F("> endPacket:"), sendexBufferPosition);
    if (sendexBufferPosition < SENDEX_BUFFER_LENGTH) {
        (*espSerial)->write('\\');
        (*espSerial)->write('0');
        LOGDEBUG(F("> endPacket premature end"));
    }

    sendexBufferPosition = 0;

    int idx = readUntil(3000);
    if(idx!=TAG_SENDOK)
    {
        LOGERROR(F("Data packet end error (2)"));
        return false;
    }

    return true;
}

// Overridden sendData method for __FlashStringHelper strings
bool EspDrv::sendData(uint8_t sock, const __FlashStringHelper *data, uint16_t len, bool appendCrLf)
{
	LOGDEBUG2(F("> sendData:"), sock, len);

	char cmdBuf[20];
	uint16_t len2 = len + 2*appendCrLf;
	sprintf_P(cmdBuf, PSTR("AT+CIPSEND=%d,%u"), sock, len2);
	(*espSerial)->println(cmdBuf);

	int idx = readUntil(2000, (char *)">", false);
	if(idx!=NUMESPTAGS)
	{
		LOGERROR(F("Data packet send error (1)"));
		return false;
	}

	//(*espSerial)->write(data, len);
	PGM_P p = reinterpret_cast<PGM_P>(data);
	for (uint16_t i=0; i<len; i++)
	{
		unsigned char c = pgm_read_byte(p++);
		(*espSerial)->write(c);
	}
	if (appendCrLf)
	{
		(*espSerial)->write('\r');
		(*espSerial)->write('\n');
	}

	idx = readUntil(3000);
	if(idx!=TAG_SENDOK)
	{
		LOGERROR(F("Data packet send error (2)"));
		return false;
	}

    return true;
}

bool EspDrv::sendDataUdp(uint8_t sock, const char* host, uint16_t port, const uint8_t *data, uint16_t len)
{
	LOGDEBUG2(F("> sendDataUdp:"), sock, len);
	LOGDEBUG2(F("> sendDataUdp:"), host, port);

	char cmdBuf[40];
	sprintf_P(cmdBuf, PSTR("AT+CIPSEND=%d,%u,\"%s\",%u"), sock, len, host, port);
	//LOGDEBUG1(F("> sendDataUdp:"), cmdBuf);
	(*espSerial)->println(cmdBuf);

	int idx = readUntil(2000, (char *)">", false);
	if(idx!=NUMESPTAGS)
	{
		LOGERROR(F("Data packet send error (1)"));
		return false;
	}

	(*espSerial)->write(data, len);

	idx = readUntil(3000);
	if(idx!=TAG_SENDOK)
	{
		LOGERROR(F("Data packet send error (2)"));
		return false;
	}

    return true;
}


////////////////////////////////////////////////////////////////////////////
// Utility functions
////////////////////////////////////////////////////////////////////////////



/*
* Sends the AT command and stops if any of the TAGS is found.
* Extract the string enclosed in the passed tags and returns it in the outStr buffer.
* Returns true if the string is extracted, false if tags are not found of timed out.
*/
bool EspDrv::sendCmdGet(const __FlashStringHelper* cmd, const char* startTag, const char* endTag, char* outStr, int outStrLen)
{
    int idx;
	bool ret = false;

	outStr[0] = 0;

	espEmptyBuf();

	LOGDEBUG(F("----------------------------------------------"));
	LOGDEBUG1(F(">>"), cmd);

	// send AT command to ESP
	(*espSerial)->println(cmd);

	// read result until the startTag is found
	idx = readUntil(2000, startTag);

	if(idx==NUMESPTAGS)
	{
		// clean the buffer to get a clean string
		ringBuf.init();

		// start tag found, search the endTag
		idx = readUntil(1000, endTag);

		if(idx==NUMESPTAGS)
		{
		    int lenTag = strlen(endTag);
		    if( ringBuf.getLength() - lenTag <= outStrLen ) {
                // end tag found
                // copy result to output buffer avoiding overflow
                ringBuf.getStrN(outStr, lenTag, outStrLen-1);

                // read the remaining part of the response
                readUntil(3000);

                ret = true;
		    } else {
		        LOGERROR(F("Buffer overflow in sendCmdGet"));
		    }
		}
		else
		{
			LOGWARN(F("End tag not found"));
		}
	}
	else if(idx>=0 and idx<NUMESPTAGS)
	{
		// the command has returned but no start tag is found
		LOGDEBUG1(F("No start tag found:"), idx);
	}
	else
	{
		// the command has returned but no tag is found
		LOGWARN(F("No tag found"));
	}

	LOGDEBUG1(F("---------------------------------------------- >"), outStr);
	LOGDEBUG();

	return ret;
}

bool EspDrv::sendCmdGet(const __FlashStringHelper* cmd, const __FlashStringHelper* startTag, const __FlashStringHelper* endTag, char* outStr, int outStrLen)
{
	char _startTag[strlen_P((char*)startTag)+1];
	strcpy_P(_startTag,  (char*)startTag);

	char _endTag[strlen_P((char*)endTag)+1];
	strcpy_P(_endTag,  (char*)endTag);

	return sendCmdGet(cmd, _startTag, _endTag, outStr, outStrLen);
}


/*
* Sends the AT command and returns the id of the TAG.
* Return -1 if no tag is found.
*/
int EspDrv::sendCmd(const __FlashStringHelper* cmd, int timeout)
{
    espEmptyBuf();

	LOGDEBUG(F("----------------------------------------------"));
	LOGDEBUG1(F(">>"), cmd);

	(*espSerial)->println(cmd);

	int idx = readUntil(timeout);

	LOGDEBUG1(F("---------------------------------------------- >"), idx);
	LOGDEBUG();

    return idx;
}


/*
* Sends the AT command and returns the id of the TAG.
* The additional arguments are formatted into the command using sprintf.
* Return -1 if no tag is found.
*/
int EspDrv::sendCmd(const __FlashStringHelper* cmd, int timeout, ...)
{
	char cmdBuf[CMD_BUFFER_SIZE];

	va_list args;
	va_start (args, timeout);

#if defined (__arm__) && defined (__SAM3X8E__)
	vsnprintf (cmdBuf, CMD_BUFFER_SIZE, (char*)cmd, args);
#else
	vsnprintf_P (cmdBuf, CMD_BUFFER_SIZE, (char*)cmd, args);
#endif

	va_end (args);

	espEmptyBuf();

	LOGDEBUG(F("----------------------------------------------"));
	LOGDEBUG1(F(">>"), cmdBuf);

	(*espSerial)->println(cmdBuf);

	int idx = readUntil(timeout);

	LOGDEBUG1(F("---------------------------------------------- >"), idx);
	LOGDEBUG();

	return idx;
}


// Read from serial until one of the tags is found
// Returns:
//   the index of the tag found in the ESPTAGS array
//   -1 if no tag was found (timeout)
int EspDrv::readUntil(unsigned int timeout, const char* tag, bool findTags)
{
	ringBuf.reset();

	char c;
    unsigned long start = millis();
	int ret = -1;

	LOGDEBUG1(F("Read until: "), tag != NULL ? tag : "");

	while ((millis() - start < timeout) and ret<0)
	{
        if((*espSerial)->available())
		{
            c = (char)(*espSerial)->read();
			LOGDEBUG0(c);
			ringBuf.push(c);

			if (tag!=NULL)
			{
				if (ringBuf.endsWith(tag))
				{
					ret = NUMESPTAGS;
					//LOGDEBUG1("xxx");
				}
			}
			if(findTags)
			{
			    char buffer[12] = {0};
				for(byte i=0; i<NUMESPTAGS; i++)
				{
				    strcpy_P(buffer, (char*) pgm_read_word(&(ESPTAGS[i])));

					if (ringBuf.endsWith(buffer))
					{
						ret = i;
						break;
					}
				}
			}
		} else {
		    yield();

		    /*
		     * Big problems with responsiveness and threads switching make methods
		     * timedRead and timedPeek in Arduino Core file Stream.cpp
		     * There are one second long timeouts in these methods!
		     * It is necessary to add yield manually there every time you reinstall
		     * Arduino framework
		     *
		     *
		     * // private method to read stream with timeout
                int Stream::timedRead()
                {
                  int c;
                  _startMillis = millis();
                  do {
                    c = read();
                    if (c >= 0) return c;
                    yield();
                  } while(millis() - _startMillis < _timeout);
                  return -1;     // -1 indicates timeout
                }

                // private method to peek stream with timeout
                int Stream::timedPeek()
                {
                  int c;
                  _startMillis = millis();
                  do {
                    c = peek();
                    if (c >= 0) return c;
                    yield();
                  } while(millis() - _startMillis < _timeout);
                  return -1;     // -1 indicates timeout
                }
		     *
		     *
		     */
		}
    }

	if (millis() - start >= timeout)
	{
		LOGWARN1(F(">>> TIMEOUT >>>"), tag);
	}

    return ret;
}


void EspDrv::espEmptyBuf(bool warn)
{
    char c;
	int i=0;
	while((*espSerial)->available() > 0)
    {
		c = (*espSerial)->read();
		if (i>0 and warn==true)
			LOGDEBUG0(c);
		i++;
	}
	if (i>0 and warn==true)
    {
		LOGDEBUG(F(""));
		LOGDEBUG1(F("Dirty characters in the serial buffer! >"), i);
	}
}


// copied from Serial::timedRead
int EspDrv::timedRead()
{
  unsigned int _timeout = 1000;
  int c;
  long _startMillis = millis();
  do
  {
    c = (*espSerial)->read();
    if (c >= 0) return c;
  } while(millis() - _startMillis < _timeout);

  return -1; // -1 indicates timeout
}



EspDrv espDrv;
