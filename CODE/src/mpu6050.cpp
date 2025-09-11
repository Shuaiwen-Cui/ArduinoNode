#include "mpu6050.hpp"

static MPU6050 mpu(0x68);

void imu_init()
{
    Wire.begin();
    mpu.initialize();
    if (!mpu.testConnection())
    {
        Serial.println("[INIT] <MPU6050> Connection failed");
        while (1);  // Halt system if MPU not detected
    }

    mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
    Serial.println("[INIT] <MPU6050> Initialized successfully");
}

void imu_get_acceleration(int16_t &ax, int16_t &ay, int16_t &az)
{
    mpu.getAcceleration(&ax, &ay, &az);
}
