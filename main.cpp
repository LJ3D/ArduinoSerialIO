#include <chrono>

#include "arduinoSerialIO.hpp"



int main(){
    ArduinoSerialIO arduino("/dev/ttyACM0");
    
    while(1){
        arduino.writeString("HELLO WORLD");
        arduino.readUntilNewline();
        std::cout << arduino.getDataString();
        arduino.clearDataVect();
    }
    
    return 0;
}