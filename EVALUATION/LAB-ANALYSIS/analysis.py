import matplotlib.pyplot as plt
from matplotlib import rcParams
import numpy as np

# === Load Raw Data ===
ambient_vib_pcb = np.load("ambient_vib_pcb.npy")
ambient_vib_wsn = np.load("ambient_vib_wsn.npy")
free_vib_pcb = np.load("free_vib_pcb.npy")
free_vib_wsn = np.load("free_vib_wsn.npy")

# === Transform Function: Convert PCB to WSN-Aligned Coordinate System ===
def transform_pcb_to_wsn(pcb_data):
    transformed = np.empty_like(pcb_data)
    for sensor_index in range(4):
        base = sensor_index * 3
        pcb_x = pcb_data[:, base + 0]
        pcb_y = pcb_data[:, base + 1]
        pcb_z = pcb_data[:, base + 2]

        # Coordinate transformation:
        # WSN_X = -PCB_Y
        # WSN_Y =  PCB_X
        # WSN_Z =  PCB_Z
        transformed[:, base + 0] = -pcb_y  # WSN X
        transformed[:, base + 1] =  pcb_x  # WSN Y
        transformed[:, base + 2] =  pcb_z  # WSN Z
    return transformed

# === Generate Aligned Versions ===
am_pcb_aligned   = transform_pcb_to_wsn(ambient_vib_pcb)
am_wsn_aligned   = ambient_vib_wsn.copy()

free_pcb_aligned = transform_pcb_to_wsn(free_vib_pcb)
free_wsn_aligned = free_vib_wsn.copy()

# === (Optional) Confirm Shapes Match ===
print("am_pcb_aligned:   ", am_pcb_aligned.shape)
print("am_wsn_aligned:   ", am_wsn_aligned.shape)
print("free_pcb_aligned: ", free_pcb_aligned.shape)
print("free_wsn_aligned: ", free_wsn_aligned.shape)


# === Time Axes (independent) ===
time_pcb_am = np.arange(am_pcb_aligned.shape[0])
time_wsn_am = np.arange(am_wsn_aligned.shape[0])

# === Channel Labels ===
channel_labels = [
    "Sensor 1 - X", "Sensor 1 - Y", "Sensor 1 - Z",
    "Sensor 2 - X", "Sensor 2 - Y", "Sensor 2 - Z",
    "Sensor 3 - X", "Sensor 3 - Y", "Sensor 3 - Z",
    "Sensor 4 - X", "Sensor 4 - Y", "Sensor 4 - Z"
]

# === Plot ===
fig, axes = plt.subplots(12, 2, figsize=(16, 20), sharex=False)

for i in range(12):
    # Left: PCB (Aligned)
    axes[i, 0].plot(time_pcb_am, am_pcb_aligned[:, i], color='blue', linewidth=0.8)
    axes[i, 0].set_ylabel(channel_labels[i], fontsize=8)
    axes[i, 0].grid(True, linestyle='--', linewidth=0.4)
    if i == 0:
        axes[i, 0].set_title("PCB (Aligned to WSN)", fontsize=10)

    # Right: WSN
    axes[i, 1].plot(time_wsn_am, am_wsn_aligned[:, i], color='green', linewidth=0.8)
    axes[i, 1].grid(True, linestyle='--', linewidth=0.4)
    if i == 0:
        axes[i, 1].set_title("WSN", fontsize=10)

axes[-1, 0].set_xlabel("Sample Index", fontsize=10)
axes[-1, 1].set_xlabel("Sample Index", fontsize=10)

fig.suptitle("Ambient Vibration Comparison: PCB vs WSN (Aligned)", fontsize=14)
plt.tight_layout(rect=[0, 0, 1, 0.97])
plt.show()


# === Font Settings ===
rcParams['font.family'] = 'Times New Roman'
rcParams['axes.unicode_minus'] = False

# === Time Axes (independent) ===
time_pcb = np.arange(free_pcb_aligned.shape[0])
time_wsn = np.arange(free_wsn_aligned.shape[0])

# === Channel Labels ===
channel_labels = [
    "Sensor 1 - X", "Sensor 1 - Y", "Sensor 1 - Z",
    "Sensor 2 - X", "Sensor 2 - Y", "Sensor 2 - Z",
    "Sensor 3 - X", "Sensor 3 - Y", "Sensor 3 - Z",
    "Sensor 4 - X", "Sensor 4 - Y", "Sensor 4 - Z"
]

# === Global Y Axis Range for Consistency ===
y_min = min(free_pcb_aligned.min(), free_wsn_aligned.min())
y_max = max(free_pcb_aligned.max(), free_wsn_aligned.max())

# === Plot ===
fig, axes = plt.subplots(12, 2, figsize=(16, 20), sharex=False)

for i in range(12):
    # Left: PCB
    axes[i, 0].plot(time_pcb, free_pcb_aligned[:, i], color='blue', linewidth=0.8)
    axes[i, 0].set_ylabel(channel_labels[i], fontsize=8)
    axes[i, 0].set_ylim(y_min, y_max)
    axes[i, 0].grid(True, linestyle='--', linewidth=0.4)
    if i == 0:
        axes[i, 0].set_title("PCB (Aligned to WSN)", fontsize=10)

    # Right: WSN
    axes[i, 1].plot(time_wsn, free_wsn_aligned[:, i], color='green', linewidth=0.8)
    axes[i, 1].set_ylim(y_min, y_max)
    axes[i, 1].grid(True, linestyle='--', linewidth=0.4)
    if i == 0:
        axes[i, 1].set_title("WSN", fontsize=10)

axes[-1, 0].set_xlabel("Sample Index", fontsize=10)
axes[-1, 1].set_xlabel("Sample Index", fontsize=10)

fig.suptitle("Free Vibration Comparison: PCB vs WSN (Aligned)", fontsize=14)
plt.tight_layout(rect=[0, 0, 1, 0.97])
plt.show()


from scipy.signal import detrend

def preprocess(data):
    """
    Preprocess data by:
    1. Removing linear trend.
    2. Removing column-wise mean.
    """
    data_detrended = detrend(data, axis=0)
    data_zero_mean = data_detrended - np.mean(data_detrended, axis=0)
    return data_zero_mean


am_pcb_processed   = preprocess(am_pcb_aligned)
am_wsn_processed   = preprocess(am_wsn_aligned)
free_pcb_processed = preprocess(free_pcb_aligned)
free_wsn_processed = preprocess(free_wsn_aligned)

print("Processed Ambient PCB Shape:   ", am_pcb_processed.shape)
print("Processed Ambient WSN Shape:   ", am_wsn_processed.shape)
print("Processed Free PCB Shape:     ", free_pcb_processed.shape)
print("Processed Free WSN Shape:     ", free_wsn_processed.shape)

from scipy.signal import resample_poly

def downsample_signal(signal, orig_fs, target_fs):
    """
    Downsample a multi-channel signal to target_fs using polyphase filtering.
    
    Parameters:
        signal     : np.ndarray of shape (N, C) - N samples, C channels
        orig_fs    : original sampling frequency
        target_fs  : target sampling frequency (must be lower than orig_fs)
        
    Returns:
        np.ndarray of shape (M, C) - downsampled signal
    """
    if target_fs >= orig_fs:
        raise ValueError("Target sampling frequency must be less than original sampling frequency.")

    # Compute integer factors
    from math import gcd
    g = gcd(orig_fs, target_fs)
    up = target_fs // g
    down = orig_fs // g

    # Apply resample_poly on each column
    channels = []
    for i in range(signal.shape[1]):
        ch = resample_poly(signal[:, i], up, down)
        channels.append(ch)

    # Stack back to array
    return np.stack(channels, axis=1)

# Sampling rates
FS_PCB = 2048
FS_WSN = 100

# Apply downsampling
am_pcb_downsampled   = downsample_signal(am_pcb_processed, FS_PCB, FS_WSN)
free_pcb_downsampled = downsample_signal(free_pcb_processed, FS_PCB, FS_WSN)

# Check shapes
print("Downsampled Ambient PCB shape:", am_pcb_downsampled.shape)
print("Downsampled Free PCB shape:", free_pcb_downsampled.shape)


def normalize_signal(signal):
    """
    Normalize each column (channel) using Z-score normalization.
    Output has zero mean and unit variance per channel.
    """
    mean = np.mean(signal, axis=0)
    std = np.std(signal, axis=0)
    std[std == 0] = 1  # avoid division by zero
    return (signal - mean) / std

# normalize the data before time sync

am_pcb_normalized   = normalize_signal(am_pcb_downsampled)
am_wsn_normalized   = normalize_signal(am_wsn_processed)
free_pcb_normalized = normalize_signal(free_pcb_downsampled)
free_wsn_normalized = normalize_signal(free_wsn_processed)

# check shapes
print("Normalized Ambient PCB shape:", am_pcb_normalized.shape)
print("Normalized Ambient WSN shape:", am_wsn_normalized.shape)
print("Normalized Free PCB shape:", free_pcb_normalized.shape)
print("Normalized Free WSN shape:", free_wsn_normalized.shape)

def compute_energy_signals(signal_12ch):
    """
    Combine every 3 channels (X, Y, Z) of each sensor into a single energy signal (x^2 + y^2 + z^2).
    Input:  signal_12ch: shape (N, 12)
    Output: signal_combined: shape (N, 4)
    """
    N = signal_12ch.shape[0]
    signal_combined = np.zeros((N, 4))

    for sensor_idx in range(4):
        base = sensor_idx * 3
        x = signal_12ch[:, base + 0]
        y = signal_12ch[:, base + 1]
        z = signal_12ch[:, base + 2]
        signal_combined[:, sensor_idx] = x**2 + y**2 + z**2

    return signal_combined

am_pcb_energy   = compute_energy_signals(am_pcb_normalized)
am_wsn_energy   = compute_energy_signals(am_wsn_normalized)
free_pcb_energy = compute_energy_signals(free_pcb_normalized)
free_wsn_energy = compute_energy_signals(free_wsn_normalized)

print("Ambient PCB Energy shape:   ", am_pcb_energy.shape)
print("Ambient WSN Energy shape:   ", am_wsn_energy.shape)
print("Free PCB Energy shape:     ", free_pcb_energy.shape)
print("Free WSN Energy shape:     ", free_wsn_energy.shape)
