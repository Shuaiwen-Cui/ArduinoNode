import numpy as np
from scipy.signal import detrend
import matplotlib.pyplot as plt
from matplotlib import rcParams

# === Font Settings ===
rcParams['font.family'] = 'Times New Roman'
rcParams['axes.unicode_minus'] = False

# === Load Raw Data ===
ambient_vib_pcb = np.load("ambient_vib_pcb.npy")
ambient_vib_wsn = np.load("ambient_vib_wsn.npy")
free_vib_pcb = np.load("free_vib_pcb.npy")
free_vib_wsn = np.load("free_vib_wsn.npy")

# === Coordinate Transformation Function (PCB → WSN-aligned) ===
def transform_pcb_to_wsn(pcb_data):
    transformed = np.empty_like(pcb_data)
    for sensor_index in range(4):
        base = sensor_index * 3
        pcb_x = pcb_data[:, base + 0]
        pcb_y = pcb_data[:, base + 1]
        pcb_z = pcb_data[:, base + 2]
        # Apply coordinate transformation rules
        transformed[:, base + 0] = -pcb_y  # WSN X
        transformed[:, base + 1] =  pcb_x  # WSN Y
        transformed[:, base + 2] =  pcb_z  # WSN Z
    return transformed

# === Apply transformation to PCB data, WSN is copied directly ===
am_pcb_aligned   = transform_pcb_to_wsn(ambient_vib_pcb)
am_wsn_aligned   = ambient_vib_wsn.copy()

free_pcb_aligned = transform_pcb_to_wsn(free_vib_pcb)
free_wsn_aligned = free_vib_wsn.copy()

# === Optional: Print Shape Info ===
print("am_pcb_aligned:   ", am_pcb_aligned.shape)
print("am_wsn_aligned:   ", am_wsn_aligned.shape)
print("free_pcb_aligned: ", free_pcb_aligned.shape)
print("free_wsn_aligned: ", free_wsn_aligned.shape)


# === Preprocessing Function ===
def preprocess_signal(signal_data):
    """
    Apply linear detrending and mean removal to each column (channel) of the input signal matrix.

    Parameters:
        signal_data (np.ndarray): Input signal, shape (N, 12)

    Returns:
        np.ndarray: Preprocessed signal of same shape
    """
    processed = np.empty_like(signal_data)
    for i in range(signal_data.shape[1]):
        # Linear detrend
        detrended = detrend(signal_data[:, i], type='linear')
        # Remove mean
        demeaned = detrended - np.mean(detrended)
        processed[:, i] = demeaned
    return processed

# === Apply to both ambient and free vibration data (PCB and WSN) ===
am_pcb_processed   = preprocess_signal(am_pcb_aligned)
am_wsn_processed   = preprocess_signal(am_wsn_aligned)
free_pcb_processed = preprocess_signal(free_pcb_aligned)
free_wsn_processed = preprocess_signal(free_wsn_aligned)

# === Confirm shapes match ===
print("am_pcb_processed:   ", am_pcb_processed.shape)
print("am_wsn_processed:   ", am_wsn_processed.shape)
print("free_pcb_processed: ", free_pcb_processed.shape)
print("free_wsn_processed: ", free_wsn_processed.shape)


# === Channel Labels (for y-axis)
channel_labels = [
    "Sensor 1 - X", "Sensor 1 - Y", "Sensor 1 - Z",
    "Sensor 2 - X", "Sensor 2 - Y", "Sensor 2 - Z",
    "Sensor 3 - X", "Sensor 3 - Y", "Sensor 3 - Z",
    "Sensor 4 - X", "Sensor 4 - Y", "Sensor 4 - Z"
]

# === Time Vectors
time_pcb = np.arange(am_pcb_processed.shape[0])
time_wsn = np.arange(am_wsn_processed.shape[0])

# === Plot Figure 1 ===
fig, axes = plt.subplots(12, 2, figsize=(16, 30), sharex=False)

for i in range(12):
    # Left column: PCB
    axes[i, 0].plot(time_pcb, am_pcb_processed[:, i], color='blue', linewidth=0.8)
    axes[i, 0].set_ylabel(channel_labels[i], fontsize=8)
    axes[i, 0].grid(True, linestyle='--', linewidth=0.4)
    if i == 0:
        axes[i, 0].set_title("PCB", fontsize=10)

    # Right column: WSN
    axes[i, 1].plot(time_wsn, am_wsn_processed[:, i], color='green', linewidth=0.8)
    axes[i, 1].grid(True, linestyle='--', linewidth=0.4)
    if i == 0:
        axes[i, 1].set_title("WSN", fontsize=10)

axes[-1, 0].set_xlabel("Sample Index", fontsize=10)
axes[-1, 1].set_xlabel("Sample Index", fontsize=10)

fig.suptitle("Ambient Vibration Time History (Processed)", fontsize=14)
plt.tight_layout(rect=[0, 0, 1, 0.97])
plt.show()

# === Time Vectors for free vibration ===
time_pcb_free = np.arange(free_pcb_processed.shape[0])
time_wsn_free = np.arange(free_wsn_processed.shape[0])

# === Channel Labels ===
channel_labels = [
    "Sensor 1 - X", "Sensor 1 - Y", "Sensor 1 - Z",
    "Sensor 2 - X", "Sensor 2 - Y", "Sensor 2 - Z",
    "Sensor 3 - X", "Sensor 3 - Y", "Sensor 3 - Z",
    "Sensor 4 - X", "Sensor 4 - Y", "Sensor 4 - Z"
]

# === Plot Free Vibration Time History ===
fig, axes = plt.subplots(12, 2, figsize=(14, 20), sharex=False)

for i in range(12):
    # Left column: PCB
    axes[i, 0].plot(time_pcb_free, free_pcb_processed[:, i], color='blue', linewidth=0.8)
    axes[i, 0].set_ylabel(channel_labels[i], fontsize=8)
    axes[i, 0].grid(True, linestyle='--', linewidth=0.4)
    if i == 0:
        axes[i, 0].set_title("PCB", fontsize=10)

    # Right column: WSN
    axes[i, 1].plot(time_wsn_free, free_wsn_processed[:, i], color='green', linewidth=0.8)
    axes[i, 1].grid(True, linestyle='--', linewidth=0.4)
    if i == 0:
        axes[i, 1].set_title("WSN", fontsize=10)

axes[-1, 0].set_xlabel("Sample Index", fontsize=10)
axes[-1, 1].set_xlabel("Sample Index", fontsize=10)

fig.suptitle("Free Vibration Time History (Processed)", fontsize=14)
plt.tight_layout(rect=[0, 0, 1, 0.97])
plt.show()
