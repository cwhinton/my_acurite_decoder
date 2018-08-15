
#include <Arduino.h>

class RFM69_Helper
{
    public:
        RFM69_Helper (int selPin);
        uint8_t read(uint8_t reg);
        void write(uint8_t reg, uint8_t val);

        void initOOK();
    private:
        int _selPin = -1;
        //
        // talk to the RFM69
        uint16_t xfer16(uint16_t cmd);
};
