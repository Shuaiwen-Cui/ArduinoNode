# Development Framework

!!! tip "Development Framework"
    For simplicity, this project does not use an RTOS but is developed directly based on the basic model of "initialization" and "loop" of the microcontroller. On this basis, a state machine model is introduced to manage different states. The core of the framework is the `main.cpp` file, which contains the basic logic for initialization and looping. Functions for sensing, communication, storage, etc., are encapsulated for easy invocation as needed.

## Initialization Part

Initialization is responsible for setting up hardware, configuring peripherals, and initializing various modules. In this project, initialization mainly includes the following aspects:

- Hardware configuration: Setting up GPIO, UART, SPI, and other peripheral pins and parameters.
- Module initialization: Initializing sensors, communication modules (such as Wi-Fi, Bluetooth, etc.), storage modules, etc.
- System settings: Clock synchronization, system parameter configuration, etc.

## Loop Part

The loop part is the core of the program, responsible for continuously executing tasks such as sensing, processing, and communication. In this project, the loop part mainly manages different states through a state machine by monitoring a series of flags. It primarily switches between sensing and communication states, with storage operations included in the sensing operations for simplicity.
