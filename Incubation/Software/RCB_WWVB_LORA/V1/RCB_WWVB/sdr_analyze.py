import numpy as np
from scipy.signal import decimate, butter, sosfilt
import matplotlib.pyplot as plt
from scipy.signal import firwin, lfilter
from scipy.signal import decimate, cheby1, sosfilt
from numpy.fft import fft, fftfreq
from scipy.signal import butter, sosfilt
from scipy.signal import lfilter, find_peaks

# Initialize lists to hold the I and Q values
I_values = []
Q_values = []

# Initialize lists to hold the hexadecimal values for I and Q
I_hex_strings = []
Q_hex_strings = []


    
# Open and read the file named 'com20.txt'
with open('com20.txt', 'r') as file:
    # Loop over each line in the file
    for line in file:
        # Split the line by spaces
        parts = line.strip().split(' ')
        
        # Loop over each part
        for part in parts:
            # Check if the part starts with 'I=' or 'Q='
            if part.startswith('I='):
                # Find the hexadecimal string following '0x' and before '=>'
                hex_str = part.split('0x')[1].split('=>')[0]
                # Append the hexadecimal string to I_hex_strings
                I_hex_strings.append(hex_str)
                    
            elif part.startswith('Q='):
                # Find the hexadecimal string following '0x' and before '=>'
                hex_str = part.split('0x')[1].split('=>')[0]
                # Append the hexadecimal string to Q_hex_strings
                Q_hex_strings.append(hex_str)



def print_hex_values_with_index(I_hex_strings, Q_hex_strings, entries_per_line=5):
    # Determine the number of lines needed
    num_lines = max(len(I_hex_strings), len(Q_hex_strings)) // entries_per_line + 1
    
    # Loop through each line
    for line in range(num_lines):
        # Start and end indices for this line
        start_idx = line * entries_per_line
        end_idx = start_idx + entries_per_line
        
        # Extract the subset of values for this line
        I_line_values = I_hex_strings[start_idx:end_idx]
        Q_line_values = Q_hex_strings[start_idx:end_idx]
        
        # Construct the line string with index number at the beginning
        line_str = f"{start_idx * entries_per_line}: " + ', '.join([f"Q=0x{Q_line_values[i]} I=0x{I_line_values[i]}" for i in range(min(len(I_line_values), len(Q_line_values)))])
        
        # Print the constructed line
        print(line_str)

def hex_strings_to_bit_list(hex_strings, lsb_first=True):
    bit_list = []
    for hex_str in hex_strings:
        # Convert hex string to binary string, remove the '0b' prefix, and ensure it's 32 bits
        binary_str = bin(int(hex_str, 16))[2:].zfill(32)
        # If LSB should come first, reverse the order of bits in the binary string
        if lsb_first:
            binary_str = binary_str[::-1]
        # Extend the bit_list with each bit in the (possibly reversed) binary_str
        bit_list.extend([int(bit) for bit in binary_str])
    return bit_list

# Function to apply a Butterworth low-pass filter
def butter_lowpass_filter(data, cutoff, fs, order=5):
    nyq = 0.5 * fs
    normal_cutoff = cutoff / nyq
    b, a = butter(order, normal_cutoff, btype='low', analog=False)
    y = lfilter(b, a, data)
    return y

# Calculate the amplitude from I and Q values
def calculate_amplitude(I, Q):
    return np.sqrt(I**2 + Q**2)

# Function to plot FFT of the signal
def plot_fft(signal, fs, ax, title='FFT'):
    N = len(signal)
    fft_signal = np.fft.fft(signal)
    fft_freq = np.fft.fftfreq(N, 1/fs)
    
    # Only plot the positive frequencies
    n = N // 2
    ax.plot(fft_freq[:n], 20 * np.log10(np.abs(fft_signal[:n])), linestyle='--')
    ax.set_title(title)
    ax.set_xlabel('Frequency (Hz)')
    ax.set_ylabel('Magnitude (dB)')
    ax.grid(True)
    

def sinc_filter_decimation(bitstream, decimation_factor):
    # Convert bitstream to bipolar (-1, 1)
    bipolar_stream = np.array(bitstream) * 2 - 1
    
    # Apply sinc filter - using a simple boxcar window for illustration
    sinc_filtered = np.convolve(bipolar_stream, np.ones(decimation_factor), mode='valid') / decimation_factor
    
    # Decimate
    decimated_signal = decimate(sinc_filtered, decimation_factor, zero_phase=True)
    
    return decimated_signal
    
    
def fft_and_find_peaks(signal, fs):
    # Perform FFT
    fft_signal = np.fft.fft(signal)
    fft_freq = np.fft.fftfreq(len(signal), 1/fs)
    
    # Calculate magnitude spectrum and find peaks
    magnitude_spectrum = np.abs(fft_signal)
    peaks, _ = find_peaks(magnitude_spectrum)

    # Filter out negative frequencies and peaks too close to 0 frequency
    positive_freq_peaks = peaks[fft_freq[peaks] > 0]

    # Get frequencies and magnitudes of the positive frequency peaks
    peak_freqs = fft_freq[positive_freq_peaks]
    peak_mags = magnitude_spectrum[positive_freq_peaks]

    # Sort peaks by magnitude and select the top 5
    top_peaks_indices = np.argsort(peak_mags)[-5:][::-1]  # Indices of top 5 peaks, sorted by magnitude
    top_peak_freqs = peak_freqs[top_peaks_indices]
    top_peak_mags = peak_mags[top_peaks_indices]

    # Print the top 5 peaks
    print("Top 5 Peaks at frequencies (Hz):", top_peak_freqs)
    print("Top 5 Peak magnitudes:", top_peak_mags)


# Use the function to print the hex values with index numbers
#print_hex_values_with_index(I_hex_strings, Q_hex_strings)

# Convert I and Q hex strings to bit lists
I_bit_stream = hex_strings_to_bit_list(I_hex_strings, lsb_first=True)
Q_bit_stream = hex_strings_to_bit_list(Q_hex_strings, lsb_first=True)

# Print the first 64 bits of the I and Q bit streams
#print("First 64 bits of I bit stream:", I_bit_stream[:64])
#print("First 64 bits of Q bit stream:", Q_bit_stream[:64])

# Print the last 64 bits of the I and Q bit streams
#print("Last 64 bits of I bit stream:", I_bit_stream[-64:])
#print("Last 64 bits of Q bit stream:", Q_bit_stream[-64:])


# Convert bit streams to bipolar format (-1 for 0, and 1 for 1)
I_bipolar = np.array(I_bit_stream) * 2 - 1
Q_bipolar = np.array(Q_bit_stream) * 2 - 1

# Sampling parameters
fs = 32e6  # Sampling frequency: 32 MHz
cutoff = 500e3  # Cutoff frequency: 500 kHz

# Apply low-pass filtering
I_filtered = butter_lowpass_filter(I_bipolar, cutoff, fs)
Q_filtered = butter_lowpass_filter(Q_bipolar, cutoff, fs)

# Decimation
# Calculate the decimation factor to match the Nyquist rate of the filtered signal or to achieve the desired resolution
# This is an example value; the exact factor depends on the requirements and the oversampling ratio
decimation_factor = int(fs / (2 * cutoff))  # Simplified calculation for demonstration purposes

I_decimated = decimate(I_filtered, decimation_factor, zero_phase=True)
Q_decimated = decimate(Q_filtered, decimation_factor, zero_phase=True)

# Parameters
decimation_factor = 32  # Example decimation factor, adjust based on your specific requirements

# Apply sinc filter and decimation to I and Q bitstreams
I_decimated = sinc_filter_decimation(I_bit_stream, decimation_factor)
Q_decimated = sinc_filter_decimation(Q_bit_stream, decimation_factor)

# Calculate amplitude of the decimated signals
amplitude_decimated = np.sqrt(I_decimated**2 + Q_decimated**2)






# Sampling parameters for decimated signal
fs_decimated = 1e6  # Decimated sampling frequency (1 MHz as an example)

# Perform FFT and find peaks for I, Q, and combined signals
print("I Channel Peaks:")
fft_and_find_peaks(I_decimated, fs_decimated)

print("\nQ Channel Peaks:")
fft_and_find_peaks(Q_decimated, fs_decimated)

combined_signal = I_decimated + 1j * Q_decimated
print("\nCombined I+Q Signal Peaks:")
fft_and_find_peaks(combined_signal, fs_decimated)




# Plotting the amplitude and FFTs for I, Q, and total signal
fig, axs = plt.subplots(3, 2, figsize=(14, 12))

# Plot amplitude and FFT for I
axs[0, 0].plot(I_decimated, label='I Amplitude')
axs[0, 0].set_title('I Amplitude')
axs[0, 0].set_xlabel('Sample Index')
axs[0, 0].set_ylabel('Amplitude')
axs[0, 0].legend()
plot_fft(I_decimated, fs/decimation_factor, axs[0, 1], 'FFT of I')

# Plot amplitude and FFT for Q
axs[1, 0].plot(Q_decimated, label='Q Amplitude')
axs[1, 0].set_title('Q Amplitude')
axs[1, 0].set_xlabel('Sample Index')
axs[1, 0].set_ylabel('Amplitude')
axs[1, 0].legend()
plot_fft(Q_decimated, fs/decimation_factor, axs[1, 1], 'FFT of Q')

# Plot amplitude and FFT for total signal
axs[2, 0].plot(amplitude_decimated, label='Total Amplitude', color='purple')
axs[2, 0].set_title('Total Signal Amplitude')
axs[2, 0].set_xlabel('Sample Index')
axs[2, 0].set_ylabel('Amplitude')
axs[2, 0].legend()
plot_fft(amplitude_decimated, fs/decimation_factor, axs[2, 1], 'FFT of Total Signal')

plt.tight_layout()
plt.show()




