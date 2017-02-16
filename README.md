# UWB coordinate reader

UWB (Ultra Wideband bandwidth) coordinate reader is a simple helper program designed to read
a synthetic UWB packet data from text files and convert them into UWB packet format which can
be then transmitted through windows named pipe or serial port within time periods specified.

The main idea of this program is to emulate data incoming from UWB sensor network monitoring
the area in order to detect moving/living targets through a wall. To be able do some development
without real sensors connected to the PC, I wrote a simple coordinate generator to generate synthetic
data as [x, y] targets positions. Number of UWB sensors as well as targets count is not limited. Data
are exported into text files in agreed format. You can find this generator here:

https://github.com/Gresthorn/UWB_COORDINATE_GENERATOR

To read those data, this program was written. It provides simple GUI to select source files to read,
select method and parameters of transmitting. Program will automatically convert all the data into
UWB packet format.

# Usage
  - When program starts, user is asked to select source files to be read by program
  - Only files reasonable for reading are coordinate source files (from generator exported as Radar_X.txt)
  - If no specified otherwise, program will be sending all measurements from all sensors with frequency 13 times per second
  - Speed of transmitting data can be set manually, however, it is recommended to use speed supposed by generator (data are generated as sequence of measurements simulating this speed)
  - You can set speed of transmitting by entering the frequency manually or give the program a setup.txt file for automatical extraction
  - If Use serial checkbox is unmarked, windows named pipe will be used to transfer data (this is usefull if no serial cabels/comports are available)
  - Pipe is configured as: CreateNamedPipeA("\\\\.\\pipe\\uwb_pipe", PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 0, 0, 0, NULL) - you will need to configure reciever side accordingly
  - Example of client pipe configuration: CreateFileA("\\\\.\\pipe\\uwb_pipe", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)
  - If Use serial checbox is marked, you will need to select COM port, Baudrate and Mode in order to transmit data correctly
  - Mode is in text format: DATA BITS TO TRANSMIT (8, 7, 6 or 5), PARITY CONTROL (N=none, O=odd, E=even), STOP BITS (1 or 2)
  - For more information about used RS232 library visit: http://www.teuniz.net/RS-232/

# Packet Format

The packet transmitted from the softvare is of undefined end. Each packet contains one ending character: $
Packets can therefore contain random number of [x, y] coordinates. Exact packet format is as the following:
  - 8 bits : radar id
  - 8 bits : radar time (not used for any purpose yet, not defined time format)
  - 8 bits : packet number (incremented for each next packet per sensor)
  - 12 bits per coordinate : pairs [x, y] have therefore 24 bits together
    - each target has assigned [x, y] position, position of [x, y] pair for specific target does not change between the packets from single sensor
    - current implementation of generator is even exporting each target coordinate pairs for each sensor at the same position in packet (e.g. target X position will occure as N-th [x, y] pair in each packet for every sensor)
    - The functionality may be enhanced by messing this order up in later versions to test the association algorithms properly
  - 16 bits CRC - CRC plynomial used is P_16 0xA001
  - IMPORTANT: Each 4 bits of transmitted data are extracted and converted to hexadecimal character according to numeric meaning of those 4 bits. EXAMPLE:
    - To transmit number 12.45, this is multiplied by 100 -> 12.45 x 100 = 1245
    - 1245 on 12 bits is 010011011101 (first bit is sign)
    - we will divide these to 4 bits bulks: 0100 1101 1101
    - 4 bits bulks have the following hexadecimal expression: 4 D D
    - these values are converted to the character 8 bit values (unsigned char) and transmitted in following order: 4 D D (LSB transmitted last)
    - the same convenction is used for radar id as well as to packet number and CRC. 4 bits are obviously enhanced to 8 bits.
  - last transmitted character is $. This character is transmitted as-is.

# Directory structure

  - ClientExampleSource : example of client side windows named pipe application, can be run with coordinate reader
  - Sources: latest source files for the project
  - v1.1 : fully compiled application, ready to run on windows platform (includes precompiled client example)
  - v1.1/TestCoordinateFiles : test coordinate files exported from synthetic data generator for 3 radars (+ operator radar) and 4 targets

# Compilation

The compiled package was built with Qt Creator based on Qt 5.4.1 (MSVC 2010, 32 bit), revision 567c6eb875
Later versions of Qt should be also able to build the project. In case something goes wrong, try to use the
exact version as mentioned above. 

Qt Archive: https://download.qt.io/archive/qt/5.4/5.4.1/
Direct link for Qt Creator: https://download.qt.io/archive/qt/5.4/5.4.1/qt-opensource-windows-x86-mingw491_opengl-5.4.1.exe

Install the package, run the Qt Creator and open the *.pro file from the Sources directory. Go to Build menu
and click the "Build Project" button or click the "Hammer" icon in the bottom left corner. To run the application,
click the green triangle or click the "Build->Run" button, or press "CTRL+R"

# See also

UWB packet generator - https://github.com/Gresthorn/UWB_COORDINATE_GENERATOR
UWB packet generator exported files format documentation - https://github.com/Gresthorn/UWB_COORDINATE_GENERATOR/blob/master/v1.0/Documentation/doc.pdf
RS232 library - http://www.teuniz.net/RS-232/

---
License
----
MIT