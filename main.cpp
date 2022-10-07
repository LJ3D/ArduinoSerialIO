#include <chrono>

#include "arduinoSerialIO.hpp"



int main(){
    ArduinoSerialIO arduino("/dev/ttyACM0");

    while(1){
        std::cout << "=====================\n";

        // Generate a random long string (only a-z)
        // 1600 seems to be close to the limit of what the arduino can send back in one go
        std::string randstr;
        for(int i = 0; i < 1600; i++){
            randstr += (rand() % 26) + 'a';
        }
        arduino.writeString(randstr); // Write a string to the serial port
        arduino.readUntilNewline(); // Read from the serial port until a newline character is encountered
        std::string received = arduino.getDataString(); // Converts the data in the class's data vector to a string and returns it into std::cout
        received = received.substr(0, received.length() - 2); // Remove the newline and carriage return characters
        std::cout << "Sent: " << randstr << '\n';
        std::cout << "Received: " << received << '\n';
        std::cout << "Match: " << (randstr == received ? "True" : "False") << '\n';
        arduino.clearDataVect(); // Clears the data vector
    }
    
    return 0;
}