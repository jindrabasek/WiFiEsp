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
#include <stddef.h>
#include <stdint.h>
#include <utility/debug.h>
#include <utility/EspDrv.h>
#include <WifiEspTimeouts.h>
#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include <WiFiEspServer.h>


WiFiEspServer::WiFiEspServer(uint16_t port, WifiEspTimeouts* timeouts) : _port(port), _sock(NO_SOCKET_AVAIL), timeouts(timeouts)
{
}

void WiFiEspServer::begin()
{
	LOGDEBUG(F("Starting server"));

	/* The ESP Module only allows socket 1 to be used for the server */
	_sock = WiFiEspClass::getFreeSocket();
	if (_sock == SOCK_NOT_AVAIL || _sock != 1)
    {
        LOGERROR(F("No socket available for server"));
        return;
    }

	WiFiEspClass::allocateSocket(_sock);

	_started = EspDrv::startServer(_port, _sock);

	if (_started)
	{
		LOGINFO1(F("Server started on port"), _port);
	}
	else
	{
		LOGERROR(F("Server failed to start"));
	}
}

WiFiEspClient WiFiEspServer::available(byte* status)
{
	// TODO the original method seems to handle automatic server restart

    uint16_t _remotePort = 0;
    uint8_t  _remoteIp[WL_IPV4_LENGTH] = {0};
    int bytes;
    if (timeouts != NULL) {
        bytes = EspDrv::availData(0, &_remotePort, _remoteIp, timeouts->getReadTimeout());
    } else {
        bytes = EspDrv::availData(0, &_remotePort, _remoteIp);
    }

	if (bytes>0)
	{
		LOGINFO1(F("New client"), EspDrv::_connId);
        WiFiEspClass::allocateSocket(EspDrv::_connId);
		return WiFiEspClient(EspDrv::_connId, _remotePort, _remoteIp);
	}

    return WiFiEspClient(255);
}

uint8_t WiFiEspServer::status()
{
    return EspDrv::getServerState(0);
}

size_t WiFiEspServer::write(uint8_t b)
{
    return write(&b, 1);
}

size_t WiFiEspServer::write(const uint8_t *buffer, size_t size)
{
	size_t n = 0;

    for (int sock = 0; sock < MAX_SOCK_NUM; sock++)
    {
        if (WiFiEspClass::_state[sock] != 0)
        {
        	WiFiEspClient client(sock);
            n += client.write(buffer, size);
        }
    }
    return n;
}
