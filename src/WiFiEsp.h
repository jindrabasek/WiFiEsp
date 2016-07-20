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

#ifndef WiFiEsp_h
#define WiFiEsp_h

#include <IPAddress.h>
#include <stdbool.h>
#include <stdint.h>
#include <utility/EspDrv.h>
#include <utility/SerialHolder.h>


class WiFiEspClass
{

public:

	static int16_t _state[MAX_SOCK_NUM];
	static uint16_t _server_port[MAX_SOCK_NUM];

	WiFiEspClass();


	/**
	* Initialize the ESP module.
	*
	* param espSerial: the serial interface (HW or SW) used to communicate with the ESP module
	*/
	static void init(SerialHolder *espSerial, unsigned long baudRate,
	                 int8_t resetPin = -1, unsigned long originalBaudRate = EspDrv::DEFAULT_ORIGINAL_BAUD_RATE);


	/**
	* Get firmware version
	*/
	static char* firmwareVersion();


	// NOT IMPLEMENTED
	//int begin(char* ssid);

	// NOT IMPLEMENTED
	//int begin(char* ssid, uint8_t key_idx, const char* key);


	/**
	* Start Wifi connection with passphrase
	* the most secure supported mode will be automatically selected
	*
	* param ssid: Pointer to the SSID string.
	* param passphrase: Passphrase. Valid characters in a passphrase
	*		  must be between ASCII 32-126 (decimal).
	*/
	int begin(const char* ssid, const char* passphrase);


	/**
	* Change Ip configuration settings disabling the DHCP client
	*
	* param local_ip:	Static ip configuration
	*/
	void config(IPAddress local_ip);


	// NOT IMPLEMENTED
	//void config(IPAddress local_ip, IPAddress dns_server);

	// NOT IMPLEMENTED
	//void config(IPAddress local_ip, IPAddress dns_server, IPAddress gateway);

	// NOT IMPLEMENTED
	//void config(IPAddress local_ip, IPAddress dns_server, IPAddress gateway, IPAddress subnet);

	// NOT IMPLEMENTED
	//void setDNS(IPAddress dns_server1);

	// NOT IMPLEMENTED
	//void setDNS(IPAddress dns_server1, IPAddress dns_server2);

	/**
	* Disconnect from the network
	*
	* return: one value of wl_status_t enum
	*/
	int disconnect(void);

	/**
	* Get the interface MAC address.
	*
	* return: pointer to uint8_t array with length WL_MAC_ADDR_LENGTH
	*/
	uint8_t* macAddress(uint8_t* mac);

	/**
	* Get the interface IP address.
	*
	* return: Ip address value
	*/
	IPAddress localIP();


	/**
	* Get the interface subnet mask address.
	*
	* return: subnet mask address value
	*/
	IPAddress subnetMask();

	/**
	* Get the gateway ip address.
	*
	* return: gateway ip address value
	*/
   IPAddress gatewayIP();

	/**
	* Return the current SSID associated with the network
	*
	* return: ssid string
	*/
	char* SSID();

	/**
	* Return the current BSSID associated with the network.
	* It is the MAC address of the Access Point
	*
	* return: pointer to uint8_t array with length WL_MAC_ADDR_LENGTH
	*/
	uint8_t* BSSID(uint8_t* bssid);


	/**
	* Return the current RSSI /Received Signal Strength in dBm)
	* associated with the network
	*
	* return: signed value
	*/
	int32_t RSSI();


	/**
	* Return Connection status.
	*
	* return: one of the value defined in wl_status_t
	*         see https://www.arduino.cc/en/Reference/WiFiStatus
	*/
	uint8_t status();


    /*
      * Return the Encryption Type associated with the network
      *
      * return: one value of wl_enc_type enum
      */
    //uint8_t	encryptionType();

    /*
     * Start scan WiFi networks available
     *
     * return: Number of discovered networks
     */
    int8_t scanNetworks(char networkSsid[WL_NETWORKS_LIST_MAXNUM][WL_SSID_MAX_LENGTH],
                        int32_t networkRssi[WL_NETWORKS_LIST_MAXNUM],
                        wl_enc_type networkEncr[WL_NETWORKS_LIST_MAXNUM]);


	// NOT IMPLEMENTED
	//int hostByName(const char* aHostname, IPAddress& aResult);



	////////////////////////////////////////////////////////////////////////////
	// Non standard methods
	////////////////////////////////////////////////////////////////////////////

	/**
	* Start the ESP access point.
	*
	* param ssid: Pointer to the SSID string.
	* param channel: WiFi channel (1-14)
	* param pwd: Passphrase. Valid characters in a passphrase
	*		  must be between ASCII 32-126 (decimal).
	* param enc: encryption type (enum wl_enc_type)
	* param apOnly: Set to false if you want to run AP and Station modes simultaneously
	*/
	int beginAP(char* ssid, uint8_t channel, const char* pwd, uint8_t enc, bool apOnly=true);

	/*
	* Start the ESP access point with open security.
	*/
	int beginAP(char* ssid);
	int beginAP(char* ssid, uint8_t channel);

	/**
	* Change IP address of the AP
	*
	* param ip:	Static ip configuration
	*/
	void configAP(IPAddress ip);



	/**
	* Restart the ESP module.
	*/
	void reset();

    /**
    * Restart the ESP module using hardware pin.
    * This method does not initialize ESP
    */
    bool hardReset();

	/**
	* Ping a host.
	*/
	bool ping(const char *host);


	friend class WiFiEspClient;
	friend class WiFiEspServer;

private:
	static uint8_t espMode;
};

extern WiFiEspClass WiFi;

#endif
