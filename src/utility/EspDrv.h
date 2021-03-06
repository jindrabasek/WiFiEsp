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

#ifndef EspDrv_h
#define EspDrv_h

#include <IPAddress.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <utility/EspRingBuffer.h>
#include <utility/SerialHolder.h>
#include <WString.h>



// Maximum size of a SSID
#define WL_SSID_MAX_LENGTH 32

// Size of a MAC-address or BSSID
#define WL_MAC_ADDR_LENGTH 6

// Size of a MAC-address or BSSID
#define WL_IPV4_LENGTH 4

// Maximum size of a SSID list
#define WL_NETWORKS_LIST_MAXNUM	10

// Maxmium number of socket
#define	MAX_SOCK_NUM		4

// Socket not available constant
#define SOCK_NOT_AVAIL  255

// Default state value for Wifi state field
#define NA_STATE -1

#define WL_FW_VER_LENGTH 6

#define NO_SOCKET_AVAIL 255


// maximum size of AT command
#define CMD_BUFFER_SIZE 200

typedef enum eProtMode {TCP_MODE, UDP_MODE, SSL_MODE} tProtMode;


typedef enum {
        WL_FAILURE = -1,
        WL_SUCCESS = 1,
} wl_error_code_t;

/* Authentication modes */
enum wl_auth_mode {
        AUTH_MODE_INVALID,
        AUTH_MODE_AUTO,
        AUTH_MODE_OPEN_SYSTEM,
        AUTH_MODE_SHARED_KEY,
        AUTH_MODE_WPA,
        AUTH_MODE_WPA2,
        AUTH_MODE_WPA_PSK,
        AUTH_MODE_WPA2_PSK
};


typedef enum {
	WL_NO_SHIELD = 255,
	WL_IDLE_STATUS = 0,
	//WL_NO_SSID_AVAIL,
	//WL_SCAN_COMPLETED,
	WL_CONNECTED,
	WL_CONNECT_FAILED,
	//WL_CONNECTION_LOST,
	WL_DISCONNECTED
} wl_status_t;

/* Encryption modes */
enum wl_enc_type {
	ENC_TYPE_NONE = 0,
	ENC_TYPE_WEP = 1,
	ENC_TYPE_WPA_PSK = 2,
	ENC_TYPE_WPA2_PSK = 3,
	ENC_TYPE_WPA_WPA2_PSK = 4
};


enum wl_tcp_state {
	CLOSED      = 0,
	LISTEN      = 1,
	SYN_SENT    = 2,
	SYN_RCVD    = 3,
	ESTABLISHED = 4,
	FIN_WAIT_1  = 5,
	FIN_WAIT_2  = 6,
	CLOSE_WAIT  = 7,
	CLOSING     = 8,
	LAST_ACK    = 9,
	TIME_WAIT   = 10
};



class EspDrv
{

public:

	static const unsigned long DEFAULT_ORIGINAL_BAUD_RATE = 115200;
	static const int SENDEX_BUFFER_LENGTH = 2048;

	static const uint16_t DEFAULT_WIFI_CONNECTION_TIMEOUT = 30000;
	static const uint16_t DEFAULT_CONNECTION_TIMEOUT = 5000;
	static const uint16_t DEFAULT_TCP_CONNECTION_TIMEOUT = 2000;
	static const uint16_t DEFAULT_UDP_CONNECTION_TIMEOUT = 2000;
	static const uint16_t DEFAULT_READ_TIMEOUT = 3000;

    static void wifiDriverInit(SerialHolder *espSerial, unsigned long baudRate,
                               int8_t chpdPin = -1, int8_t resetPin = -1,
                               unsigned long originalBaudRate = DEFAULT_ORIGINAL_BAUD_RATE);


    /* Start Wifi connection with passphrase
     *
     * param ssid: Pointer to the SSID string.
     * param passphrase: Passphrase. Valid characters in a passphrase must be between ASCII 32-126 (decimal).
     */
    static bool wifiConnect(const char* ssid, const char *passphrase, uint16_t wifiConnectionTimeout = DEFAULT_WIFI_CONNECTION_TIMEOUT);


    /*
	* Start the Access Point
	*/
	static bool wifiStartAP(char* ssid, const char* pwd, uint8_t channel, uint8_t enc, uint8_t espMode);


    /*
	 * Set ip configuration disabling dhcp client
	 */
    static void config(IPAddress local_ip);

    /*
	 * Set ip configuration disabling dhcp client
	 */
    static void configAP(IPAddress local_ip);


    /*
     * Disconnect from the network
     *
     * return: WL_SUCCESS or WL_FAILURE
     */
    static int8_t disconnect();

    /*
     *
     *
     * return: one value of wl_status_t enum
     */
    static uint8_t getConnectionStatus();

    /*
     * Get the interface MAC address.
     *
     * return: pointer to uint8_t array with length WL_MAC_ADDR_LENGTH
     */
    static uint8_t* getMacAddress();

    /*
     * Get the interface IP address.
     *
     * return: copy the ip address value in IPAddress object
     */
    static void getIpAddress(IPAddress& ip);

	static void getIpAddressAP(IPAddress& ip);

    /*
     * Get the interface IP netmask.
     * This can be used to retrieve settings configured through DHCP.
     *
     * return: true if successful
     */
    static bool getNetmask(IPAddress& mask);

    /*
     * Get the interface IP gateway.
     * This can be used to retrieve settings configured through DHCP.
     *
     * return: true if successful
     */
    static bool getGateway(IPAddress& mask);

    /*
     * Return the current SSID associated with the network
     *
     * return: ssid string
     */
    static char* getCurrentSSID();

    /*
     * Return the current BSSID associated with the network.
     * It is the MAC address of the Access Point
     *
     * return: pointer to uint8_t array with length WL_MAC_ADDR_LENGTH
     */
    static uint8_t* getCurrentBSSID();

    /*
     * Return the current RSSI /Received Signal Strength in dBm)
     * associated with the network
     *
     * return: signed value
     */
    static int32_t getCurrentRSSI();

    /*
     * Get the networks available
     *
     * param networkSsid: array of ssids of size [WL_NETWORKS_LIST_MAXNUM][WL_SSID_MAX_LENGTH]
     * param networkRssi: array of rssi of size [WL_NETWORKS_LIST_MAXNUM]
     * param networkEncr: array of encryption type of size [WL_NETWORKS_LIST_MAXNUM]
     *
     * return: Number of discovered networks
     */
    static uint8_t getScanNetworks(char networkSsid[WL_NETWORKS_LIST_MAXNUM][WL_SSID_MAX_LENGTH],
                                   int32_t networkRssi[WL_NETWORKS_LIST_MAXNUM],
                                   wl_enc_type networkEncr[WL_NETWORKS_LIST_MAXNUM]);

    /*
     * Get the firmware version
     */
    static char* getFwVersion();

    static bool hardReset();
    static void reset();


	////////////////////////////////////////////////////////////////////////////
	// Client/Server methods
	////////////////////////////////////////////////////////////////////////////


    static bool startServer(uint16_t port, uint8_t sock);
    static bool startClient(const char* host, uint16_t port, uint8_t sock, uint8_t protMode, uint16_t connectionTimeout = DEFAULT_CONNECTION_TIMEOUT);
    static void stopClient(uint8_t sock);
    static uint8_t getServerState(uint8_t sock);
    static uint8_t getClientState(uint8_t sock);
    static bool getData(uint8_t connId, uint8_t *data, bool peek, bool* connClose);
    static int getDataBuf(uint8_t connId, uint8_t *buf, uint16_t bufSize);
    static bool sendData(uint8_t sock, const uint8_t *data, uint16_t len, uint16_t connectionTimeoutTcp = DEFAULT_TCP_CONNECTION_TIMEOUT);
    static bool sendData(uint8_t sock, const __FlashStringHelper *data, uint16_t len, bool appendCrLf=false, uint16_t connectionTimeoutTcp = DEFAULT_TCP_CONNECTION_TIMEOUT);
    static bool sendDataEx(uint8_t sock, const uint8_t *data, uint16_t len, int & sendexBufferPosition, uint16_t* charsToYield, uint16_t connectionTimeoutTcp = DEFAULT_TCP_CONNECTION_TIMEOUT);
    static bool sendDataEx(uint8_t sock, const __FlashStringHelper *data, uint16_t len, int & sendexBufferPosition, uint16_t* charsToYield, bool appendCrLf=false, uint16_t connectionTimeoutTcp = DEFAULT_TCP_CONNECTION_TIMEOUT);
    static bool beginPacket(uint8_t sock, uint16_t connectionTimeoutTcp);
    static bool endPacket(int & sendexBufferPosition);

	static bool sendDataUdp(uint8_t sock, const char* host, uint16_t port, const uint8_t *data, uint16_t len, uint16_t connectionTimeoutUdp = DEFAULT_UDP_CONNECTION_TIMEOUT);
    static uint16_t availData(uint8_t connId, uint16_t * _remotePort, uint8_t * _remoteIp, uint16_t readTimeout = DEFAULT_READ_TIMEOUT);


	static bool ping(const char *host);

    static void flushReceiveBuffer();


////////////////////////////////////////////////////////////////////////////////

private:
	static SerialHolder *espSerial;

	static int8_t resetPin;
    static int8_t chpdPin;
	static long _bufPos;
	static uint8_t _connId;

	// firmware version string
	static char 	fwVersion[WL_FW_VER_LENGTH];

	// settings of current selected network
	static char 	_ssid[WL_SSID_MAX_LENGTH];
	static uint8_t 	_bssid[WL_MAC_ADDR_LENGTH];
	static uint8_t 	_mac[WL_MAC_ADDR_LENGTH];

	static const uint8_t RING_BUFFER_SIZE = 32;
	// the ring buffer is used to search the tags in the stream
	static EspRingBuffer<RING_BUFFER_SIZE> ringBuf;

	static uint16_t yield_every_n_chars;


	//static int sendCmd(const char* cmd, int timeout=1000);
	static int sendCmd(const __FlashStringHelper* cmd, int timeout=1000);
	static int sendCmd(const __FlashStringHelper* cmd, int timeout, ...);

	static bool sendCmdGet(const __FlashStringHelper* cmd, const char* startTag, const char* endTag, char* outStr, int outStrLen);
	static bool sendCmdGet(const __FlashStringHelper* cmd, const __FlashStringHelper* startTag, const __FlashStringHelper* endTag, char* outStr, int outStrLen);

	static int readUntil(unsigned int timeout, const char* tag=NULL, bool findTags=true, bool showWarn = true);

	static void espEmptyBuf(bool warn=true);

	static int timedRead();


	friend class WiFiEsp;
	friend class WiFiEspServer;
	friend class WiFiEspClient;
	friend class WiFiEspUdp;
};

extern EspDrv espDrv;

#endif
