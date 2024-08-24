#include <cstdint>
#include <iostream>
#include <sstream>


int startByte = 0x5F;
int key = 0x21BF32C8;

int shiftLeft(int byte);
int shiftRight(int byte);
int getKeyByte(int key, int index);
void decode(uint8_t* originalArray, int key);
int parseHex(const std::string& hexStr) {
    std::stringstream ss;
    ss << std::hex << hexStr;
    int hexValue;
    ss >> hexValue;
    return hexValue;
}




int main(){
    uint8_t originalData[8];
    std::cout << "Enter data to decode: ";
    std::string hexStr;
    for(int i = 0; i < 8; i++){
        std::cin >> hexStr;
        if(hexStr.length() > 2){
            for(int j = 0; j < 16; j+=2){
                originalData[j/2] = parseHex(hexStr.substr(j, 2));
            }
            break;
        }
        originalData[i] = parseHex(hexStr);
    }

    decode(originalData, key);
    for(int i = 0; i < 8; i++){
        if(originalData[i] < 0x10){
            std::cout << "0" << std::hex << int(originalData[i]) << " ";
        }else{
            std::cout << std::hex << int(originalData[i]) << " ";        
        }
    } 
    std::cout << "\n";
    system("timeout /t 6");
    return 0;
}
int shiftLeft(int byte){
    if((byte & 0x80) >> 7){
        byte = byte << 1;
        byte = byte | 1;
    }else{
        byte = byte << 1;
    }
    return byte;
}
int shiftRight(int byte){
    if((byte & 1)){
        byte = byte >> 1;
        byte = byte | 0x80;
    }else{
        byte = byte >> 1;
    }    
    return byte;
}
int getKeyByte(int key, int index){
    return (key >> index*8 & 0xFF);
}
void decode(uint8_t* originalArray, int key){
    uint8_t tempByte;
    tempByte = originalArray[0];
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 2);
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 3);
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 0);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 1);
    originalArray[0] = tempByte;

    tempByte = originalArray[1];
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 3);
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 0);
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 1);
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 2);
    originalArray[1] = tempByte;

    //third byte skipped
    //fourth byte skipped
    
    tempByte = originalArray[4];
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 2);
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 3);
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 0);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 1);
    originalArray[4] = tempByte;

    tempByte = originalArray[5];
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 3);
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 0);
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 1);
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 2);
    originalArray[5] = tempByte;

    tempByte = originalArray[6];
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 0);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 0);
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 1);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 2);
    originalArray[6] = tempByte;

    tempByte = originalArray[7];
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 0);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 1);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 2);
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 3);
    originalArray[7] = tempByte;

}