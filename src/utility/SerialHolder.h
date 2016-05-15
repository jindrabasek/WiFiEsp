/*
 * SerialHolder.h
 *
 *  Created on: 3. 5. 2016
 *      Author: jindra
 */

#ifndef LIBRARIES_WIFIESP_SRC_UTILITY_SERIALHOLDER_H_
#define LIBRARIES_WIFIESP_SRC_UTILITY_SERIALHOLDER_H_

#include <stddef.h>
#include <Stream.h>

// Do not define virtual destructor on purpose - class
// and its children is not expected to need destructors,
// it saves a lot of SRAM otherwise occupied by VTABLE
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"

class SerialHolder {
public:
    SerialHolder(){
    }
    virtual void begin(unsigned long baud) =0;
    virtual void end() =0;
    virtual Stream* getSerial() =0;

    Stream& operator* () {
        return *getSerial();
    }
    Stream* operator-> (){
        return getSerial();
    }
};

template <typename SerialClass>
class SerialHolderT : public SerialHolder {
private:
    SerialClass * serial;
public:
    SerialHolderT(SerialClass * serial) : serial(serial) {
    }

    virtual void begin(unsigned long baud) {
        serial->begin(baud);
    }

    virtual void end() {
        serial->end();
    }

    virtual Stream* getSerial() {
        return serial;
    }
};

#pragma GCC diagnostic pop

#endif /* LIBRARIES_WIFIESP_SRC_UTILITY_SERIALHOLDER_H_ */
