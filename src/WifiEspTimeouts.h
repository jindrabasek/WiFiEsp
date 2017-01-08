/*
 * WifiEspTimeouts.h
 *
 *  Created on: 8. 1. 2017
 *      Author: jindra
 */

#ifndef LIBRARIES_WIFIESP_SRC_WIFIESPTIMEOUTS_H_
#define LIBRARIES_WIFIESP_SRC_WIFIESPTIMEOUTS_H_

#include <stdint.h>
#include <utility/EspDrv.h>

class WifiEspTimeouts {
public:
    uint16_t getConnectionTimeout() const {
        return connectionTimeout;
    }

    WifiEspTimeouts* setConnectionTimeout(uint16_t connectionTimeout =
            EspDrv::DEFAULT_CONNECTION_TIMEOUT) {
        this->connectionTimeout = connectionTimeout;
        return this;
    }

    uint16_t getConnectionTimeoutTcp() const {
        return connectionTimeoutTcp;
    }

    WifiEspTimeouts* setConnectionTimeoutTcp(uint16_t connectionTimeoutTcp =
            EspDrv::DEFAULT_TCP_CONNECTION_TIMEOUT) {
        this->connectionTimeoutTcp = connectionTimeoutTcp;
        return this;
    }

    uint16_t getConnectionTimeoutUdp() const {
        return connectionTimeoutUdp;
    }

    WifiEspTimeouts* setConnectionTimeoutUdp(uint16_t connectionTimeoutUdp =
            EspDrv::DEFAULT_UDP_CONNECTION_TIMEOUT) {
        this->connectionTimeoutUdp = connectionTimeoutUdp;
        return this;
    }

    uint16_t getReadTimeout() const {
        return readTimeout;
    }

    WifiEspTimeouts* setReadTimeout(uint16_t readTimeout = EspDrv::DEFAULT_READ_TIMEOUT) {
        this->readTimeout = readTimeout;
        return this;
    }

private:
    uint16_t connectionTimeout = EspDrv::DEFAULT_CONNECTION_TIMEOUT;
    uint16_t connectionTimeoutTcp = EspDrv::DEFAULT_TCP_CONNECTION_TIMEOUT;
    uint16_t connectionTimeoutUdp = EspDrv::DEFAULT_UDP_CONNECTION_TIMEOUT;
    uint16_t readTimeout = EspDrv::DEFAULT_READ_TIMEOUT;
};

#endif /* LIBRARIES_WIFIESP_SRC_WIFIESPTIMEOUTS_H_ */
