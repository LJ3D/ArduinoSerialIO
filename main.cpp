#include<unistd.h> // Just for sleep()

#include <iostream>
#include <string>

#include "arduinoSerial.h"

int main(){
    arduinoSerial serial("/dev/ttyACM0", true);
    serial.begin(B9600);
    while(1){
        // Send a message to the Arduino
        serial.print("Hello Arduino!");
        // Wait for the Arduino to respond, currently manually waiting with sleep() is required as timeout is not yet implemented
        sleep(1);
        std::string response = serial.readString();
        std::cout << "Arduino says: " << response << "\n";
    }
    return 0;
}
