#include <Arduino.h>
#include "Acurite5n1_Decoder.h";
//#include <Strings.h>

Acurite5n1_Decoder::Acurite5n1_Decoder() {

};

void Acurite5n1_Decoder::addData(const byte * buf, byte len){
    if (packetFill + len < sizeof(packetBuffer)) {
        memcpy(packetBuffer + packetFill, buf, len);
        packetFill += len;
    }
}

byte Acurite5n1_Decoder::removeData(byte *buf, byte len_requested){
  // If there's no data in the buffer, then just return
  if (packetFill == 0)
    return 0;
  
  // If the buffer is the same size as (or smaller than) what's requested, then return what's in the buffer
  if (packetFill <= len_requested) {
    memcpy(buf, packetBuffer, packetFill);
    len_requested = packetFill;
    packetFill = 0;
    return len_requested;
  }
  
  // The buffer must have more data in it than the request.
  //  copy the data to return in to the return buffer
  memcpy(buf, packetBuffer, len_requested);
  //  move the remaining data down in packetBuffer
  for (int i=0; i<packetFill - len_requested; i++) {
    packetBuffer[i] = packetBuffer[i + len_requested];
  }
  packetFill -= len_requested;
  //  return how many bytes we copied (same as the requested length)
  return len_requested;
}

byte Acurite5n1_Decoder::calcParity(byte b){
  byte result = 0;
  for (byte i=0; i<=6; i++) {
    result ^= (b & (1<<i)) ? 1 : 0;
  }
  return result ? 0x80 : 0x00;
}

float Acurite5n1_Decoder::kph2mph(float kmph) {
  return kmph / 1.609344;
}

float Acurite5n1_Decoder::getTemp(){
    return temp_F;
}

void Acurite5n1_Decoder::setTemp(uint8_t highByte, uint8_t lowByte){
    // range -40 to 158 F
    int highbits = (highByte & 0x0F) << 7 ;
    int lowbits = lowByte & 0x7F;
    int rawtemp = highbits | lowbits;
    temp_F = (rawtemp - 400) / 10.0;
}

float Acurite5n1_Decoder::getWindSpeed_kph(){
    return wind_speed_kph;
}

float Acurite5n1_Decoder::getWindSpeed_mph(){
    return kph2mph(getWindSpeed_kph());
}

void Acurite5n1_Decoder::setWindSpeed_kph (uint8_t highByte, uint8_t lowByte){
    // range: 0 to 159 kph
    // raw number is cup rotations per 4 seconds
    // http://www.wxforum.net/index.php?topic=27244.0 (found from weewx driver)
	int highbits = ( highByte & 0x1F) << 3;
    int lowbits = ( lowByte & 0x70 ) >> 4;
    int rawspeed = highbits | lowbits;
    wind_speed_kph = 0;
    if (rawspeed > 0) {
        wind_speed_kph = rawspeed * 0.8278 + 1.0;
    } 
}

uint8_t Acurite5n1_Decoder::getHumidity() {
    return humidity;
}

void Acurite5n1_Decoder::setHumidity(uint8_t byte){
    // range: 1 to 99 %RH
    humidity = byte & 0x7F;
}

int Acurite5n1_Decoder::getRainCounter() {
    return raincounter;
}

void Acurite5n1_Decoder::setRainCounter(uint8_t highByte, uint8_t lowByte){
    // range: 0 to 99.99 in, 0.01 in incr., rolling counter?
	raincounter = ((highByte & 0x7f) << 7) | (lowByte & 0x7F);
    if (priorRainCounter > 0) {
        // track rainfall difference after first run
        rainfall = (raincounter - priorRainCounter) * 0.01;
        if (raincounter < priorRainCounter) {
            priorRainCounter = raincounter;
        }
    } else {
        // prime it on first run
        priorRainCounter = raincounter;
    }
}

float Acurite5n1_Decoder::getRainFall(){
    return rainfall;
}

char Acurite5n1_Decoder::getChannel() {
    char chLetter[4] = {'C','E','B','A'}; // 'E' stands for error
    return chLetter[channelNum];
}

void Acurite5n1_Decoder::setChannel(uint8_t byte){
    channelNum = (byte & 0xC0) >> 6;
}

uint16_t Acurite5n1_Decoder::getSensorID(){
    return sensorID;
}

//
// 5-n-1 sensor ID is the last 12 bits of byte 0 & 1
// byte 0     | byte 1
// CC RR IIII | IIII IIII
void Acurite5n1_Decoder::setSensorID(uint8_t highByte, uint8_t lowByte){
    sensorID = ((highByte & 0x0f) << 8) | lowByte;
}

char * Acurite5n1_Decoder::getWindDirectionStr(){
    return acurite_5n1_winddirection_str[windDirectionIX];
}

float Acurite5n1_Decoder::getWindDirection(){
    return acurite_5n1_winddirections[windDirectionIX];
}

void Acurite5n1_Decoder::setWindDirection(uint8_t byte){
    windDirectionIX = byte & 0x0f;
}

uint8_t Acurite5n1_Decoder::getMessageType(){
    return messageType;
}

void Acurite5n1_Decoder::setMessageType(uint8_t byte) {
    messageType = byte & 0x3f;
}

uint8_t Acurite5n1_Decoder::getSequenceNum() {
    return sequenceNum;
}
// The sensor sends the same data three times, each of these have
// an indicator of which one of the three it is. This means the
// checksum and first byte will be different for each one.
// The bits 5,4 of byte 0 indicate which copy of the 65 bit data string
//  00 = first copy
//  01 = second copy
//  10 = third copy
//  1100 xxxx  = channel A 1st copy
//  1101 xxxx  = channel A 2nd copy
//  1110 xxxx  = channel A 3rd copy
void Acurite5n1_Decoder::setSequenceNum(uint8_t byte){
    sequenceNum = (byte & 0x30) >> 4;
}

int Acurite5n1_Decoder::getBatteryLow(){
    return batteryLow;
}

void Acurite5n1_Decoder::setBatteryLow(uint8_t byte) {
    batteryLow = (byte & 0x40) >> 6;
}

void Acurite5n1_Decoder::printSource(const byte *data){
    Serial.print("Source: ");
    unsigned long id = ((data[0] & 0x3f) << 7) | (data[1] & 0x7f);
    Serial.print(id);

    Serial.print("/");
    switch (data[0] & 0xC0) {
        case 0xc0:
            Serial.print("A");
            break;
        case 0x80:
            Serial.print("B");
            break;
        case 0x00:
            Serial.print("C");
            break;
        default:
            Serial.print("x");
            break;
    }
}

bool Acurite5n1_Decoder::isExpectedPacket(const byte *data) {
    bool goodPacket = true;
    // Check parity bits. (Byte 0 and byte 7 have no parity bits.)
    // ... Byte 1 might also not be parity-ing correctly. A new device
    // I bought in early 2018 has a parity error in byte 1, but
    // otherwise seems to work correctly.
    for (int i=2; i<=6; i++) {
        if (calcParity(data[i]) != (data[i] & 0x80)) {
#ifdef DEBUG
            Serial.print("Parity failure in byte ");
            Serial.println(i);
#endif
            goodPacket = false;
            return goodPacket;
        }
    }

    // Check mod-256 checksum of bytes 0 - 6 against byte 7
    unsigned char cksum = 0;
    for (int i=0; i<=6; i++) {
        cksum += data[i];
    }
    if ((cksum & 0xFF) != data[7]) {
        printSource(data);
        Serial.print(" checksum failure - calcd 0x");
        Serial.print(cksum, HEX);
        Serial.print(", expected 0x");
        Serial.println(data[6], HEX);
        goodPacket = false;
        return goodPacket;
    }

    uint8_t message_type = data[2] & 0x3f;
    if (message_type != acurite_msgtype_5n1_windspeed_winddir_railfall && 
        message_type != acurite_msgtype_5n1_windspeed_temp_humidity) {
        goodPacket = false;
    }
    return goodPacket;

}

bool Acurite5n1_Decoder::decodePacket(const byte *data){
    setChannel(data[0]);
    setSequenceNum(data[0]);
    setSensorID(data[0],data[1]);
    setMessageType(data[2]);
    setBatteryLow(data[2]);
    switch (getMessageType()) {
        case ACURITE_MSGTYPE_5N1_WINDSPEED_WINDDIR_RAINFALL:
            setWindSpeed_kph(data[3],data[4]);
            setWindDirection(data[4]);
            setRainCounter(data[5],data[6]);
            break;
        case ACURITE_MSGTYPE_5N1_WINDSPEED_TEMP_HUMIDITY:
            setWindSpeed_kph(data[3],data[4]);
            setTemp(data[4],data[5]);
            setHumidity(data[6]);
            break;
    }
}

bool Acurite5n1_Decoder::processMessage(){
    while (packetFill >= (acurite_5n1_bitlen / 8)) {
        byte dbuf[7];
        removeData(dbuf, (acurite_5n1_bitlen / 8));
        if (isExpectedPacket(dbuf)) {
            return decodePacket(dbuf);
        }
    }
    return false;
}
