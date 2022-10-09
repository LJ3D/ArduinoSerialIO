#include "arduinoSerial.h"

#include <stdio.h> 
#include <string.h>
#include <unistd.h>  
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#define BUFFERSIZE 1024 // Used by functions that are not provided a buffer by the user (e.g. readString()). This could be as small as 1, but i dont think a single kb is too much to ask for.

// Constructor
arduinoSerial::arduinoSerial(std::string port, bool debug){
    this->debug = debug;
    this->ttyName = port;
    if(this->debug){ std::cout << "aurdioSeral object created. ttyName = " << this->ttyName << "\n"; }
}

// Destructor
arduinoSerial::~arduinoSerial(){
    if(this->debug){ std::cout << "~arduinoSerial() called\n"; }
    this->end();
}

/*
    * Get the number of bytes (characters) available for reading from the serial port.
*/
unsigned int arduinoSerial::available(){
    int bytesAvailable;
    ioctl(this->fd, FIONREAD, &bytesAvailable);
    if(debug){ std::cout << "available(): Detected " << bytesAvailable << " bytes available\n"; }
    return bytesAvailable;
}

/*
    * Get the number of bytes (characters) available for writing in the serial buffer without blocking the write operation.
    * This isnt really something I need to implement, I dont think. I'll leave it here for now, but it may be removed in the future.
unsigned int arduinoSerial::availableForWrite(){
    std::cout << "AvailableForWrite(): Function not implemented, returning -1\n";
    return -1;
}
*/

void arduinoSerial::begin(unsigned long baudRate){
    // Check if the baud rate is valid:
    if(std::find(this->acceptableBaudRates.begin(), this->acceptableBaudRates.end(), baudRate) == this->acceptableBaudRates.end()){
        throw std::invalid_argument("begin(): Invalid baud rate - " + std::to_string(baudRate) + " is not a valid baud rate, see termios.h or termios-baud.h for a list of valid baud rate macros (Hint: format is B<baud rate>)");
        return;
    }
    this->fd = open(this->ttyName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY); // Open the file descriptor
    if(this->fd == -1){
        throw std::runtime_error("begin(): Unable to start the serial port " + this->ttyName);
    }
    if(this->debug){ std::cout << "begin(): Serial port " << this->ttyName << " opened\n"; }
    fcntl(this->fd, F_SETFL, O_NONBLOCK); // Set the file descriptor to nonblocking mode
    // !!! The following termios options may or may not be correct (I copied them from the internet :) )
    struct termios options;
    tcgetattr(this->fd, &options); // Get the current options for the port
    cfsetispeed(&options, baudRate); // Set the baud rates
    cfsetospeed(&options, baudRate);
    options.c_cflag |= (CLOCAL | CREAD); // Enable the receiver and set local mode
    options.c_cflag &= ~CSIZE; // Mask the character size bits
    options.c_cflag |= CS8; // Select 8 data bits
    options.c_cflag &= ~PARENB; // No parity
    options.c_cflag &= ~CSTOPB; // 1 Stop bit
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8; // 8 bits
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Raw input mode
    tcsetattr(this->fd, TCSANOW, &options); // Set the new options for the port
    sleep(1); // Wait for the Arduino to reset
    tcflush(this->fd, TCIFLUSH);
    if(this->debug){ std::cout << "begin(): Serial port " << this->ttyName << " configured. File descriptor: " << this->fd << "\n"; }
}

void arduinoSerial::end(){
    close(this->fd); // Close the file descriptor
    if(this->debug){ std::cout << "end(): Serial port " << this->ttyName << " closed\n"; }
}

/*
    * Reads data from the serial buffer intil the target string is found.
    * Returns true if the target string was found, false otherwise.
*/
bool arduinoSerial::find(char target){
    char c;
    while(1){
        c = this->read_s();
        if(c == target){
            if(this->debug){ std::cout << "find(): Found target '" << target << "in " << this->ttyName << "\n"; }
            return true;
        }
        else if(c == -1){
            if(this->debug){ std::cout << "find(): Did not find " << target << " in " << this->ttyName << "\n"; }
            return false;
        }
    }
    return false;
}

// WARNING: READS ENTIRE BUFFER, PROBABLY NOT A COPY OF HOW Serial.find() WORKS IN ARDUINO
bool arduinoSerial::find(std::string targetStr){
    std::string read = this->readString();
    if(read.find(targetStr) != std::string::npos){
        if(this->debug){ std::cout << "find(): Found target '" << targetStr << "' in " << this->ttyName << "\n"; }
        return true;
    }
    else{
        if(this->debug){ std::cout << "find(): Did not find '" << targetStr << "' in " << this->ttyName << "\n"; }
        return false;
    }
}

bool arduinoSerial::findUntil(char *target, char *terminator){
    return false; // Function not yet implemented
}

/*
    * Waits for the transmission of outgoing serial data to complete.
    * Might not implement this function.
void arduinoSerial::flush(){
    std::cout << "flush(): Function not implemented\n";
}
*/

/*
    * Looks for the next valid float in the incoming serial.
    * (based on ASCII digits, decimal point, and negative sign).
*/
float arduinoSerial::parseFloat(){
    return -1; // Function not yet implemented
}

/*
    * Looks for the next valid integer in the incoming serial. Will read until it finds a valid integer.
    * (based on ASCII digits, and negative sign).
*/
long arduinoSerial::parseInt(){
    long num = 0;
    char lastC; // Used to keep track of sign of number
    bool sign = true; // True if positive, false if negative
    while(1){
        char c = this->read_s();
        if(c == -1){
            if(debug){ std::cout << "pasrseInt(): Reached end of buffer, returning " << num << "\n"; }
            return num;
        }
        lastC = c;
        if(isdigit(c)){
            if(lastC == '-'){
                sign = false;
            }
            num = num * 10 + (c - '0');
        }else if(num != 0){
            return num;
        }
    }
    return num;
}

/*
    * Returns the next byte of incoming serial data without removing it from the internal serial buffer.
int arduinoSerial::peek(){
    return -1;
}
*/

/*
    * Prints data to the serial port (as human-readable ASCII text ?maybe?)
    * Will need to write a few overloaded functions for different data types
    * Could probably use templates
*/
void arduinoSerial::print(std::string str){
    write(this->fd, str.c_str(), str.length());
    if(this->debug){ std::cout << "print(): Wrote " << str.length() << " bytes to " << this->ttyName << "\n"; }
}
void arduinoSerial::print(char c){
    write(this->fd, &c, 1);
    if(this->debug){ std::cout << "print(): Wrote 1 byte to " << this->ttyName << "\n"; }
}
void arduinoSerial::print(int i){
    std::string str = std::to_string(i);
    write(this->fd, str.c_str(), str.length());
    if(this->debug){ std::cout << "print(): Wrote " << str.length() << " bytes to " << this->ttyName << "\n"; }
}
void arduinoSerial::print(float f){
    std::string str = std::to_string(f);
    write(this->fd, str.c_str(), str.length());
    if(this->debug){ std::cout << "print(): Wrote " << str.length() << " bytes to " << this->ttyName << "\n"; }
}

/*
    * Prints data to the serial port followed by a newline as human-readable ASCII text.
    * Will need to write a few overloaded functions for different data types
*/
void arduinoSerial::println(std::string str){
    int bytes_written =  write(this->fd, str.c_str(), str.length());
    bytes_written += write(this->fd, "\n\r", 2);
    if(this->debug){ std::cout << "println(): Wrote " << bytes_written << " bytes to " << this->ttyName << "\n"; }
}
void arduinoSerial::println(char c){
    int bytes_written = write(this->fd, &c, 1);
    bytes_written += write(this->fd, "\n\r", 2);
    if(this->debug){ std::cout << "println(): Wrote " << bytes_written << " bytes to " << this->ttyName << "\n"; }
}
void arduinoSerial::println(int i){
    std::string str = std::to_string(i);
    int bytes_written = write(this->fd, str.c_str(), str.length());
    bytes_written += write(this->fd, "\n\r", 2);
    if(this->debug){ std::cout << "println(): Wrote " << bytes_written << " bytes to " << this->ttyName << "\n"; }
}
void arduinoSerial::println(float f){
    std::string str = std::to_string(f);
    int bytes_written = write(this->fd, str.c_str(), str.length());
    bytes_written += write(this->fd, "\n\r", 2);
    if(this->debug){ std::cout << "println(): Wrote " << bytes_written << " bytes to " << this->ttyName << "\n"; }
}

/*
    * This function reads a single byte from the serial port.
    * It returns -1 if no data is available.
    * Function is called read_s() because read() is already taken by the C library.
*/
int arduinoSerial::read_s(){
    char byte;
    int bytesRead = read(this->fd, &byte, 1);
    if(bytesRead == -1){
        if(this->debug){ std::cout << "read_s(): Did not read from serial port " << this->ttyName << " (Returned -1) - Buffer is likely empty\n"; }
        return -1;
    }
    if(bytesRead == 0){
        if(this->debug){ std::cout << "read_s(): ERROR reading from serial port " << this->ttyName << " (Returned 0, EOF) - !!! /dev/ttyACM_ likely doesnt exist !!!\n"; }
        return -1;
    }
    return byte;
}

/*
    * Reads characters from the serial port into a buffer.
    * The function terminates if the terminator character is read, or if it times out.
    * Returns the number of bytes placed in the buffer (0 means no valid data found).
*/
size_t arduinoSerial::readBytes(char *buffer, size_t length){
    size_t bytesRead = 0;
    while(bytesRead < length){
        int byte = this->read_s();
        if(byte == -1){
            if(this->debug){ std::cout << "readBytes(): Finished reading from serial port " << this->ttyName << " (this->read_s() returned either -1 or 0) - Buffer is likely empty\n"; }
            break;
        }
        buffer[bytesRead] = byte;
        bytesRead++;
    }
    if(this->debug){ std::cout << "readBytes(): Read " << bytesRead << " bytes from serial port " << this->ttyName << "\n"; }
    return bytesRead;
}

/*
    * Reads characters from the serial port into a buffer.
    * The function terminates if the terminator character is read, or if it times out.
    * Returns the number of bytes placed in the buffer (0 means no valid data found).
*/
size_t arduinoSerial::readBytesUntil(char terminator, char *buffer, size_t length){
    size_t bytesRead = 0;
    while(bytesRead < length){ // Read until the desired number of bytes have been read
        int byte = this->read_s(); // Read the next byte in the serial port using the read_s() function from above
        if(byte == -1){ // -1 Means some error occurred (Such as no data available)
            if(this->debug){ std::cout << "readBytesUntil(): Finished reading from serial port " << this->ttyName << " (this->read_s() returned either -1 or 0) - Buffer is likely empty\n"; }
            break;
        }
        buffer[bytesRead] = byte;
        bytesRead++;
        if(byte == terminator){
            if(this->debug){ std::cout << "readBytesUntil(): Terminator character found, stopping read\n"; }
            break;
        }
    }
    if(this->debug){ std::cout << "readBytesUntil(): Read " << bytesRead << " bytes from serial port " << this->ttyName << "\n"; }
    return bytesRead;
}

/*
    * Reads characters from the serial port into a std::string.
    * The function terminates if it times out. (Not implemented yet, for now it just reads until /dev/ttyACM0 is empty)
*/
std::string arduinoSerial::readString(){
    char buffer[BUFFERSIZE]; // Create a buffer to store the data
    std::string str = ""; // Create a string to return
    while(1){
        size_t bytesRead = this->readBytes(buffer, BUFFERSIZE); // Read the data into the buffer local to this function
        for(size_t i = 0; i < bytesRead; i++){
            str += buffer[i]; // Add the data to the string
        }
        if(bytesRead < BUFFERSIZE){ // If the buffer is not full, then we have reached the end of the data
            break;
        }
    }
    if(this->debug){ std::cout << "readString(): Read std::string from serial port " << this->ttyName << ", bytes read: " << str.length() << "\n"; }
    return str;
}

/*
    * Reads characters from the serial port into a std::string.
    * The function terminates if the terminator character is read, or if it times out.
*/
std::string arduinoSerial::readStringUntil(char terminator){
    char buffer[BUFFERSIZE]; // Create a buffer to store the data
    std::string str = ""; // Create a string to return
    while(1){
        size_t bytesRead = this->readBytesUntil(terminator, buffer, BUFFERSIZE); // Read the data into the buffer local to this function
        for(size_t i = 0; i < bytesRead; i++){
            str += buffer[i]; // Add the data to the string
        }
        if(bytesRead < BUFFERSIZE){ // If the buffer is not full, then we have reached the end of the data
            break;
        }
    }
    if(this->debug){ std::cout << "readStringUntil(): Read std::string from serial port " << this->ttyName << ", bytes read: " << str.length() << "\n"; }
    return str;
}

/*
void arduinoSerial::setTimeout(unsigned long timeout){
    // Function not yet implemented
    // Print something because this function makes a huge difference to the program if it is not implemented
    std::cout << "setTimeout() not yet implemented, read arduinoSerial.h for more information.\n";
}
*/

/*
    * Writes binary data to the serial port.
    * Data is sent as a byte or series of bytes.
*/
size_t arduinoSerial::write_s(char byte){
    int bytesWritten = write(this->fd, &byte, 1);
    if(bytesWritten == -1 || bytesWritten == 0){
        if(this->debug){ std::cout << "write_s(): Error writing to serial port " << this->ttyName << " (bytesWritten == -1 or 0)\n"; }
        return -1;
    }
    if(this->debug){ std::cout << "write_s(): Wrote " << bytesWritten << " bytes to " << this->ttyName << "\n"; }
    return bytesWritten;
}

size_t arduinoSerial::write_s(char *buffer, size_t size){
    int bytesWritten = write(this->fd, buffer, size);
    if(bytesWritten == -1 || bytesWritten == 0){
        if(this->debug){ std::cout << "write_s(): Error writing to serial port " << this->ttyName << " (bytesWritten == -1 or 0)\n"; }
        return -1;
    }
    if(this->debug){ std::cout << "write_s(): Wrote " << bytesWritten << " bytes to " << this->ttyName << "\n"; }
    return bytesWritten;
}
