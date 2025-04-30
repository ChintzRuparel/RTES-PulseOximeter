#include "mbed.h" // Include MBED OS core libraries

// === CONFIGURATION ===
#define SIMULATION_MODE 1  // Set to 0 for real hardware, 1 for testing without MAX30100

// === INTERFACES ===
// Define I2C and interrupt only if using real hardware
#if SIMULATION_MODE == 0
I2C i2c(PB_7, PB_6);           // SDA = PB_7, SCL = PB_6 (adjust pins as needed)
InterruptIn maxInt(PA_0);      // Interrupt pin from MAX30100 sensor connected to PA_0
#endif

DigitalOut led(LED1);          // Onboard user LED (blinks on interrupt or simulated event)
const int MAX30100_I2C_ADDR = 0xAE; // 8-bit I2C address for MAX30100 (write address)

// === SENSOR INITIALIZATION ===
// Initializes MAX30100 registers: interrupt enable, mode, and SpO2 configuration
#if SIMULATION_MODE == 0
void init_sensor() {
    char data[2];

    // Enable SPO2_RDY interrupt only (write 0x10 to register 0x01)
    data[0] = 0x01;  // Address of interrupt enable register
    data[1] = 0x10;  // Enable only SPO2_RDY interrupt
    i2c.write(MAX30100_I2C_ADDR, data, 2);

    // Set mode to SpO2 mode (write 0x03 to register 0x06)
    data[0] = 0x06;  // Mode configuration register
    data[1] = 0x03;  // SpO2 mode
    i2c.write(MAX30100_I2C_ADDR, data, 2);

    // Configure SpO2: 100 samples per second, 16-bit ADC resolution (write 0x47 to 0x07)
    data[0] = 0x07;  // SpO2 configuration register
    data[1] = 0x47;  // HI_RES_EN = 1, SR = 100sps, PW = 1600us
    i2c.write(MAX30100_I2C_ADDR, data, 2);
}
#endif

// === FETCH DATA FUNCTION ===
// Reads (or simulates) Red and IR LED values from MAX30100 FIFO buffer
void GetData() {
    int ir_val, red_val;

#if SIMULATION_MODE == 1
    // In simulation mode, generate random values to mimic sensor behavior
    ir_val = 25000 + rand() % 1000;   // Simulate IR LED ADC value
    red_val = 26000 + rand() % 1000;  // Simulate Red LED ADC value
#else
    // In hardware mode, read 4 bytes from FIFO data register (0x05)
    char reg = 0x05;
    char data[4];

    i2c.write(MAX30100_I2C_ADDR, &reg, 1, true);       // Point to FIFO register
    i2c.read(MAX30100_I2C_ADDR | 0x01, data, 4);       // Read 4 bytes (2 for IR, 2 for RED)

    // Combine two bytes to form 16-bit values
    ir_val  = (data[0] << 8) | data[1];
    red_val = (data[2] << 8) | data[3];
#endif

    // Print IR and RED values to serial terminal
    printf("IR Value: %d, RED Value: %d\r\n", ir_val, red_val);
}

// === INTERRUPT HANDLER ===
// This function is triggered on SPO2_RDY interrupt (or simulated interrupt)
void handle_interrupt() {
    led = !led;      // Toggle the onboard LED to indicate interrupt received
    GetData();       // Read sensor data and print to terminal
}

// === MAIN FUNCTION ===
int main() {
    // Print startup message to terminal
    printf("Starting MAX30100 Pulse Oximeter (%s mode)...\r\n",
           SIMULATION_MODE ? "SIMULATION" : "HARDWARE");

#if SIMULATION_MODE == 0
    init_sensor();                       // Initialize MAX30100 sensor settings
    maxInt.rise(&handle_interrupt);      // Attach interrupt handler to INT pin
#endif

    // Main execution loop
    while (true) {
#if SIMULATION_MODE == 1
        // In simulation mode, trigger a "fake interrupt" every 1 second
        ThisThread::sleep_for(1s);
        handle_interrupt();
#else
        // In real mode, just wait for real interrupts (from MAX30100)
        ThisThread::sleep_for(500ms); // Idle delay
#endif
    }
}
