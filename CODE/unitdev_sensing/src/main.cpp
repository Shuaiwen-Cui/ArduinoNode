#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu(0x68);

// Sampling parameters
const uint16_t sample_rate_hz = 100;
const uint32_t sample_duration_sec = 16;
const uint32_t num_channels = 4;  // timestamp, ax, ay, az
const uint32_t trigger_window_ms = 5000;  // Allowable trigger window after scheduled time

// Time control
uint64_t unix_time_base_ms = 0;              // base time in ms (to be synced in future)
uint64_t scheduled_start_time_ms = 10000;    // start sensing at t = 10s (ms)
unsigned long millis_at_base = 0;
unsigned long last_sample_time = 0;

// Sampling buffer
int32_t* sensor_data = nullptr;
uint32_t num_samples = 0;
uint32_t sample_interval_ms = 0;
uint32_t sample_index = 0;

bool sensing_started = false;
bool countdown_displayed = false;

// Get current synchronized Unix time in ms
uint64_t get_current_unix_time_ms() {
  return unix_time_base_ms + (millis() - millis_at_base);
}

// Start sampling setup
void sensing_start() {
  sample_index = 0;
  last_sample_time = 0;

  Serial.println(">>> Sampling started.");
  Serial.print("Scheduled start (ms): "); Serial.println(scheduled_start_time_ms);

  num_samples = sample_rate_hz * sample_duration_sec;
  sample_interval_ms = 1000 / sample_rate_hz;

  sensor_data = (int32_t*)malloc(num_samples * num_channels * sizeof(int32_t));
  if (!sensor_data) {
    Serial.println("Memory allocation failed");
    while (1);
  }
}

// Perform sampling during loop
void sensing_loop() {
  if (sample_index >= num_samples) return;

  unsigned long now = millis();
  if (now - last_sample_time >= sample_interval_ms) {
    last_sample_time = now;

    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);

    uint64_t timestamp = get_current_unix_time_ms();
    uint32_t base = sample_index * num_channels;

    sensor_data[base + 0] = (int32_t)(timestamp & 0xFFFFFFFF);  // use lower 32 bits
    sensor_data[base + 1] = ax;
    sensor_data[base + 2] = ay;
    sensor_data[base + 3] = az;

    sample_index++;

    if (sample_index >= num_samples) {
      Serial.println(">>> Sampling completed. Outputting data:");
      for (uint32_t i = 0; i < num_samples; ++i) {
        uint32_t base = i * num_channels;
        Serial.print("Unix(ms): "); Serial.print(sensor_data[base + 0]);
        Serial.print(" X: "); Serial.print(sensor_data[base + 1]);
        Serial.print(" Y: "); Serial.print(sensor_data[base + 2]);
        Serial.print(" Z: "); Serial.println(sensor_data[base + 3]);
      }

      free(sensor_data);
      sensing_started = false;  // prevent reentry
    }
  }
}

void print_config() {
  Serial.println("======== Sensing Configuration ========");
  Serial.print("Sample rate     : "); Serial.print(sample_rate_hz); Serial.println(" Hz");
  Serial.print("Duration        : "); Serial.print(sample_duration_sec); Serial.println(" s");
  Serial.print("Start time (ms) : "); Serial.println(scheduled_start_time_ms);
  Serial.println("=======================================");
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Wire.begin();
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
    while (1);
  }
  Serial.println("MPU6050 connected");

  // Simulated synchronized time setup
  unix_time_base_ms = 0;           // Replace with actual NTP/master sync in the future
  millis_at_base = millis();       // Save when base time was set

  print_config();
  Serial.println("Waiting for scheduled sensing start...");
}

void loop() {
  uint64_t current_time = get_current_unix_time_ms();

  // Display countdown if close to scheduled start
  if (!countdown_displayed &&
      current_time >= scheduled_start_time_ms - 3000 &&
      current_time < scheduled_start_time_ms) {
    countdown_displayed = true;
    for (int i = 3; i > 0; --i) {
      Serial.print("Sampling in "); Serial.print(i); Serial.println("...");
      delay(1000);
    }
  }

  // Trigger sensing only within allowed time window
  if (!sensing_started &&
      current_time >= scheduled_start_time_ms &&
      current_time <= scheduled_start_time_ms + trigger_window_ms) {
    sensing_started = true;
    sensing_start();
  }

  if (sensing_started) {
    sensing_loop();
  }
}
