#ifndef UWBPACKETCLASS_H
#define UWBPACKETCLASS_H

#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "rs232.h"

#define P_16 0xA001

#include <iostream>

/**
 * @file uwbpacketclass.h
 * @author  Peter Mikula <mikula.ptr@gmail.com>
 * @version 1.0
 * @brief Class is equipped with all stuff needed for generating and reading packets from uwb sensor network.
 *
 * @section DESCRIPTION
 *
 * Data in UWB sensor network are being sent into central unit via wireless connection. Central unit has
 * device (modul) for recieving such communication and sending recieved bytes through serial link into
 * computer. The software must be able read and write into such packets as well as send them via RS232.
 *
 * This class provides functionality for both, recieving and sending packets. You can comment one of that
 * functionality to reduce size of compiled program if you do not need it.
 *
 */


class uwbPacketTx
{
private:
    int radarID; ///< radar number identifier
    int comPort; ///< serial port which is aimed to be used for data sending. It is supposed that port is already opened!
    int packetCount; ///< packet number
    int radarTime; ///< ???

    void incrementPacketCount(void); ///< Increment packet count and reset to zero if 8 bit maximum value is overflown
    void decrementPacketCount(void); ///< Decrement packet count and reset to 255 if value is less then zero
    unsigned char makeCorrection(unsigned char ch); ///< Takes char and modifies it to 0-F form. According to asci table there is unwanted range between characters A-F and digits 0-9


    unsigned short update_crc_16( unsigned short crc, char c );
    void init_crc16_tab(void);
    unsigned short crc_tab16[256];
    int crc_tab16_init;


    const unsigned char endingChar; ///< Ending char. Last character sent after one complete packet
    const float rounder; ///< Multiplier used for conversion float to int value.

    unsigned char * packet; ///< Packet string to be sent
    int packetLength; ///< Stores the number of characters currently in packet

public:
    uwbPacketTx(int radarId, int port); ///< Constructor

    void generatePacket(float * data, int data_count); ///< Gets array of [x,y] values and converts them into '0'-'F' characters which are saved then into 'packet' string. Also adds radarID and packetCount into packet.
    void deleteLastPacket(void); ///< Deletes last packet and frees the memory
    bool sendPacket(void); ///< Takes 'packet' string and sends it via serial link. Returns true if success, false otherwise

    // functions for simple obtaining and changing common values
    void setRadarId(int id) { radarID = id; }
    void setComPort(int port) { comPort = port; }
    int getComPort(void) { return comPort; }
    int getRadarId(void) { return radarID; }
    unsigned char * getPacket(void) { return packet; }
    int getPacketLength(void) { return packetLength; }
};

class uwbPacketRx
{
private:
    int radarID; ///< radar number identifier
    int comPort; ///< serial port which is aimed to be used for data reading. It is supposed that port is already opened!
    int packetCount; ///< packet number
    int radarTime; ///< ???

    //void incrementPacketCount(void); ///< Increment packet count and reset to zero if 8 bit maximum value is overflown
    //void decrementPacketCount(void); ///< Decrement packet count and reset to 255 if value is less then zero
    int removeCorrection(unsigned char ch); ///< Takes char and modifies it into number representation which is the real number represented by obtained digit



    unsigned short update_crc_16( unsigned short crc, char c );
    void init_crc16_tab(void);
    unsigned short crc_tab16[256];
    int crc_tab16_init;

    unsigned char * buffer; ///< Original buffer used for holding lastly read values from link
    unsigned char * c_buffer; ///< Fake cyclic buffer for holding also old values. If packet is not completed in buffer, we can freely copy values here and buffer can be still overwritten
    const int buffer_size; ///< Maximum size of buffer
    const int c_buffer_size; ///< Cyclic buffer size

    int buffer_read_pointer; ///< Pointer to position where reading was lastly finished
    int buffer_stack_pointer; ///< Pointer to position where the last actual value was saved
    int buffer_rs232_read_size; ///< Number of bytes read from serial link during last recieving
    int buffer_packet_size; ///< Holds the packet length during reading from buffer. Ending char does not count.

    const unsigned char endingChar; ///< Ending char. Last character recieved after one complete packet
    const float rounder; ///< Multiplier used for conversion float to int value.

    unsigned char * packet; ///< Packet string that was recieved
    int packetLength; ///< Stores the number of characters currently in packet
    float * data; ///< Pointer to lastly recieved and parsed data (coordinates [x, y])
    int dataCount; ///< Specifies how many coordinates are in the data array

public:
    uwbPacketRx(int port); ///< Constructor

    void readPacket(void); ///< Reads the 'packet' array acoording to packet length and reads all information from it
    void deleteLastPacket(void); ///< Deletes last packet and frees the memory
    bool recievePacket(void); ///< Takes 'packet' string and sends it via serial link. Returns true if success, false otherwise

    // functions for simple obtaining and changing common values
    void setComPort(int port) { comPort = port; }
    int getComPort(void) { return comPort; }
    int getRadarId(void) { return radarID; }
    int getPacketCount(void) { return packetCount; }
    int getRadarTime(void) { return radarTime; }
    float * getData(void) { return data; }
    unsigned char * getPacket(void) { return packet; }
    int getDataCount(void) { return dataCount; }

    // TEMP
    void setPacketLength(int val) { packetLength = val; }
    void setPacket(unsigned char * pack) { packet = pack; }
};

#endif // UWBPACKETCLASS_H
