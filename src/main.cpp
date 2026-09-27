#include <Arduino.h>

#define BUFFER_SIZE 500
#define SAMPLE_RATE 10000UL

#define MAGIC_1 0xAA
#define MAGIC_2 0x55

#define PROTOCOL_VERSION 0x01
#define PACKET_TYPE_CAPTURE 0x01


uint8_t buffer[BUFFER_SIZE];

volatile uint16_t sampleIndex = 0;
volatile bool captureComplete = false;


/*
 * Update CRC-16-CCITT with one byte
 */
uint16_t crcUpdate(uint16_t crc, uint8_t data)
{
    crc ^= (uint16_t)data << 8;

    for (uint8_t i = 0; i < 8; i++)
    {
        if (crc & 0x8000)
        {
            crc = (crc << 1) ^ 0x1021;
        }
        else
        {
            crc <<= 1;
        }
    }

    return crc;
}


/*
 * Timer1 sampling interrupt
 */
ISR(TIMER1_COMPA_vect)
{
    if (sampleIndex < BUFFER_SIZE)
    {
        buffer[sampleIndex] = PINB & 0b00001111;

        sampleIndex++;
    }
    else
    {
        captureComplete = true;

        TIMSK1 &= ~(1 << OCIE1A);
    }
}


/*
 * Send one complete capture packet
 */
void sendPacket()
{
    /*
     * Stop Timer1 sampling interrupt
     */
    TIMSK1 &= ~(1 << OCIE1A);


    /*
     * Start CRC
     */
    uint16_t crc = 0xFFFF;


    /*
     * -------------------------
     * MAGIC
     * -------------------------
     */

    Serial.write(MAGIC_1);
    Serial.write(MAGIC_2);


    /*
     * -------------------------
     * VERSION
     * -------------------------
     */

    uint8_t version = PROTOCOL_VERSION;

    Serial.write(version);

    crc = crcUpdate(crc, version);


    /*
     * -------------------------
     * PACKET TYPE
     * -------------------------
     */

    uint8_t packetType = PACKET_TYPE_CAPTURE;

    Serial.write(packetType);

    crc = crcUpdate(crc, packetType);


    /*
     * -------------------------
     * SAMPLE COUNT
     * -------------------------
     */

    uint16_t sampleCount = BUFFER_SIZE;

    uint8_t countLow = lowByte(sampleCount);
    uint8_t countHigh = highByte(sampleCount);

    Serial.write(countLow);
    Serial.write(countHigh);

    crc = crcUpdate(crc, countLow);
    crc = crcUpdate(crc, countHigh);


    /*
     * -------------------------
     * SAMPLE RATE
     * -------------------------
     */

    uint32_t sampleRate = SAMPLE_RATE;

    uint8_t rate0 = (uint8_t)(sampleRate);
    uint8_t rate1 = (uint8_t)(sampleRate >> 8);
    uint8_t rate2 = (uint8_t)(sampleRate >> 16);
    uint8_t rate3 = (uint8_t)(sampleRate >> 24);

    Serial.write(rate0);
    Serial.write(rate1);
    Serial.write(rate2);
    Serial.write(rate3);

    crc = crcUpdate(crc, rate0);
    crc = crcUpdate(crc, rate1);
    crc = crcUpdate(crc, rate2);
    crc = crcUpdate(crc, rate3);


    /*
     * -------------------------
     * SAMPLE PAYLOAD
     * -------------------------
     */

    Serial.write(buffer, BUFFER_SIZE);

    /*
     * Update CRC with every sample
     */
    for (uint16_t i = 0; i < BUFFER_SIZE; i++)
    {
        crc = crcUpdate(crc, buffer[i]);
    }


    /*
     * -------------------------
     * CRC
     * -------------------------
     */

    Serial.write(lowByte(crc));
    Serial.write(highByte(crc));


    /*
     * -------------------------
     * PREPARE NEXT CAPTURE
     * -------------------------
     */

    sampleIndex = 0;
    captureComplete = false;


    /*
     * Restart Timer1 interrupt
     */
    TIMSK1 |= (1 << OCIE1A);
}


void setup()
{
    Serial.begin(115200);


    /*
     * D8-D11 = INPUTS
     */
    DDRB &= 0b11110000;


    /*
     * D2-D5 = OUTPUTS
     */
    DDRD |= 0b00111100;


    /*
     * Start test outputs LOW
     */
    PORTD &= 0b11000011;


    /*
     * -------------------------
     * TIMER1 CONFIGURATION
     * -------------------------
     */

    TCCR1A = 0;
    TCCR1B = 0;


    /*
     * CTC mode
     */
    TCCR1B |= (1 << WGM12);


    /*
     * Prescaler = 8
     */
    TCCR1B |= (1 << CS11);


    /*
     * 16 MHz / 8 = 2 MHz
     *
     * 2 MHz / 200 = 10 kHz
     */
    OCR1A = 199;


    /*
     * Enable Timer1 Compare Match A interrupt
     */
    TIMSK1 |= (1 << OCIE1A);


    /*
     * Startup message
     *
     * IMPORTANT:
     * This is text.
     *
     * After this, everything is binary.
     */
    Serial.println("Rontogen Analyzer V0.5");
    Serial.println("Binary packet protocol");
}


void loop()
{
    static uint16_t counter = 0;


    /*
     * -------------------------
     * TEST SIGNAL GENERATOR
     * -------------------------
     */

    if (counter % 1 == 0)
        PORTD ^= 0b00000100;


    if (counter % 2 == 0)
        PORTD ^= 0b00001000;


    if (counter % 4 == 0)
        PORTD ^= 0b00010000;


    if (counter % 8 == 0)
        PORTD ^= 0b00100000;


    counter++;


    /*
     * -------------------------
     * SEND CAPTURE
     * -------------------------
     */

    if (captureComplete)
    {
        sendPacket();
    }
}