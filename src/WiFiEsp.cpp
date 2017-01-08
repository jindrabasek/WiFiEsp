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

#include <IPAddress.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <utility/debug.h>
#include <utility/EspDrv.h>
#include <utility/SerialHolder.h>
#include <WiFiEsp.h>


int16_t 	WiFiEspClass::_state[MAX_SOCK_NUM] = { NA_STATE, NA_STATE, NA_STATE, NA_STATE };
uint16_t 	WiFiEspClass::_server_port[MAX_SOCK_NUM] = { 0, 0, 0, 0 };

uint8_t WiFiEspClass::espMode = 0;

WiFiEspClass::WiFiEspClass()
{

}

void WiFiEspClass::init(SerialHolder *espSerial, unsigned long baudRate,
                        int8_t chpdPin, int8_t resetPin,  unsigned long originalBaudRate)
{
    LOGINFO(F("Initializing ESP module"));
	EspDrv::wifiDriverInit(espSerial, baudRate, chpdPin,
	                       resetPin, originalBaudRate);

    for (uint8_t i = 0; i < MAX_SOCK_NUM; i++)
    {
        WiFiEspClass::_state[i] = NA_STATE;
    }
}



char* WiFiEspClass::firmwareVersion()
{
	return EspDrv::getFwVersion();
}


int WiFiEspClass::begin(const char* ssid, const char *passphrase)
{
    espMode = 1;
	if (EspDrv::wifiConnect(ssid, passphrase, wifiConnectionTimeout))
		return WL_CONNECTED;

	return WL_CONNECT_FAILED;
}


int WiFiEspClass::beginAP(char* ssid, uint8_t channel, const char* pwd, uint8_t enc, bool apOnly)
{
	if(apOnly)
        espMode = 2;
    else
        espMode = 3;
    
    if (EspDrv::wifiStartAP(ssid, pwd, channel, enc, espMode))
		return WL_CONNECTED;

	return WL_CONNECT_FAILED;
}

int WiFiEspClass::beginAP(char* ssid)
{
	return beginAP(ssid, 10, "", 0);
}

int WiFiEspClass::beginAP(char* ssid, uint8_t channel)
{
	return beginAP(ssid, channel, "", 0);
}


void WiFiEspClass::config(IPAddress ip)
{
	EspDrv::config(ip);
}

void WiFiEspClass::configAP(IPAddress ip)
{
	EspDrv::configAP(ip);
}



int WiFiEspClass::disconnect()
{
    return EspDrv::disconnect();
}

uint8_t* WiFiEspClass::macAddress(uint8_t* mac)
{
	// TODO we don't need _mac variable
	uint8_t* _mac = EspDrv::getMacAddress();
	memcpy(mac, _mac, WL_MAC_ADDR_LENGTH);
    return mac;
}

IPAddress WiFiEspClass::localIP()
{
	IPAddress ret;
	if(espMode==1)
		EspDrv::getIpAddress(ret);
	else
		EspDrv::getIpAddressAP(ret);
	return ret;
}

IPAddress WiFiEspClass::subnetMask()
{
	IPAddress mask;
	if(espMode==1)
    EspDrv::getNetmask(mask);
	return mask;
}

IPAddress WiFiEspClass::gatewayIP()
{
	IPAddress gw;
	if(espMode==1)
		EspDrv::getGateway(gw);
	return gw;
}


char* WiFiEspClass::SSID()
{
    return EspDrv::getCurrentSSID();
}

uint8_t* WiFiEspClass::BSSID(uint8_t* bssid)
{
	// TODO we don't need _bssid
	uint8_t* _bssid = EspDrv::getCurrentBSSID();
	memcpy(bssid, _bssid, WL_MAC_ADDR_LENGTH);
    return bssid;
}

int32_t WiFiEspClass::RSSI()
{
    return EspDrv::getCurrentRSSI();
}

int8_t WiFiEspClass::scanNetworks(char networkSsid[WL_NETWORKS_LIST_MAXNUM][WL_SSID_MAX_LENGTH],
                                  int32_t networkRssi[WL_NETWORKS_LIST_MAXNUM],
                                  wl_enc_type networkEncr[WL_NETWORKS_LIST_MAXNUM])
{
	return EspDrv::getScanNetworks(networkSsid, networkRssi, networkEncr);
}


uint8_t WiFiEspClass::status()
{
	return EspDrv::getConnectionStatus();
}



////////////////////////////////////////////////////////////////////////////
// Non standard methods
////////////////////////////////////////////////////////////////////////////

void WiFiEspClass::reset(void)
{
	EspDrv::reset();
	for (uint8_t i = 0; i < MAX_SOCK_NUM; i++)
    {
	    WiFiEspClass::_state[i] = NA_STATE;
    }
}



bool WiFiEspClass::hardReset(void)
{
    bool result = EspDrv::hardReset();
    for (uint8_t i = 0; i < MAX_SOCK_NUM; i++)
    {
        WiFiEspClass::_state[i] = NA_STATE;
    }
    return result;
}



bool WiFiEspClass::ping(const char *host)
{
	return EspDrv::ping(host);
}

uint8_t WiFiEspClass::getFreeSocket()
{
  // ESP Module assigns socket numbers in ascending order, so we will assign them in descending order
    for (int i = MAX_SOCK_NUM - 1; i >= 0; i--)
	{
      if (_state[i] == NA_STATE)
      {
          return i;
      }
    }
    return SOCK_NOT_AVAIL;
}

void WiFiEspClass::allocateSocket(uint8_t sock)
{
  _state[sock] = sock;
}

void WiFiEspClass::releaseSocket(uint8_t sock)
{
  _state[sock] = NA_STATE;
}


WiFiEspClass WiFi;
