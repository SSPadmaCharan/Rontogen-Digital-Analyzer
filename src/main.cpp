#include <Arduino.h>

#define BUFFER_SIZE 500

uint8_t buffer[BUFFER_SIZE];

volatile uint16_t sampleIndex = 0;
volatile bool captureComplete = false;


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


void setup()
{
    Serial.begin(115200);

    // D8-D11 = INPUT
    DDRB &= 0b11110000;

    // D2-D5 = OUTPUT
    DDRD |= 0b00111100;

    // Start outputs LOW
    PORTD &= 0b11000011;


    // -------------------------
    // TIMER1 CONFIGURATION
    // -------------------------

    TCCR1A = 0;
    TCCR1B = 0;

    // CTC mode
    TCCR1B |= (1 << WGM12);

    // Prescaler = 8
    TCCR1B |= (1 << CS11);

    // 10 kHz sampling
    OCR1A = 199;

    // Enable Timer1 Compare A interrupt
    TIMSK1 |= (1 << OCIE1A);


    Serial.println("Rontogen Analyzer V0.4");
    Serial.println("Binary capture");
}


void loop()
{
    // -------------------------
    // TEST SIGNAL GENERATOR
    // -------------------------

    static uint16_t counter = 0;

    if (counter % 1 == 0)
        PORTD ^= 0b00000100;

    if (counter % 2 == 0)
        PORTD ^= 0b00001000;

    if (counter % 4 == 0)
        PORTD ^= 0b00010000;

    if (counter % 8 == 0)
        PORTD ^= 0b00100000;

    counter++;


    // -------------------------
    // SEND CAPTURED DATA
    // -------------------------

    if (captureComplete)
    {
        // Stop Timer1 interrupt
        TIMSK1 &= ~(1 << OCIE1A);

        // Send raw binary data
        Serial.write(buffer, BUFFER_SIZE);

        // Reset capture
        sampleIndex = 0;
        captureComplete = false;

        // Start Timer1 interrupt again
        TIMSK1 |= (1 << OCIE1A);
    }
}