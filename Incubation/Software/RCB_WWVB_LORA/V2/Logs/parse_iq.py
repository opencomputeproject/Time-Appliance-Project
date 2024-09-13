import re
import numpy as np
import matplotlib.pyplot as plt

# Function to parse the IQ data from the log as 13-bit signed values
def parse_iq_data(file_path):
    with open(file_path, 'r') as file:
        data = file.readlines()

    iq_dumps = []
    current_dump = []
    in_dump = False

    for line in data:
        if '*******Printing IQ' in line:
            if current_dump:  # Save the previous dump if exists
                iq_dumps.append(current_dump)
                current_dump = []
            in_dump = True  # Start new dump
        elif in_dump:
            match = re.match(r'I=(\d+), I=0x([0-9a-f]+), Q=0x([0-9a-f]+)', line, re.IGNORECASE)
            if match:
                index = int(match.group(1))  # Capture the index from the log file
                
                # Parse I and Q as 13-bit signed integers
                i_val = int(match.group(2), 16) & 0x1FFF  # Keep only the lower 13 bits
                q_val = int(match.group(3), 16) & 0x1FFF  # Keep only the lower 13 bits
                
                # Sign extend the 13-bit values to signed 16-bit
                if i_val >= 0x1000:
                    i_val -= 0x2000  # Convert to negative if the 13th bit is set
                if q_val >= 0x1000:
                    q_val -= 0x2000  # Convert to negative if the 13th bit is set
                
                current_dump.append((index, i_val, q_val))
    
    if current_dump:  # Append the last dump
        iq_dumps.append(current_dump)

    return iq_dumps

# Function to detect the end of the packet based on amplitude drop
def find_packet_end(I_values, Q_values):
    #amplitude = np.sqrt(I_values**2 + Q_values**2)
    amplitude = abs(I_values) + abs(Q_values)
    #amplitude = I_values**2 + Q_values**2
    initial_amplitude = amplitude[0]
    threshold = initial_amplitude / 3

    # Find the index where amplitude drops below the threshold
    for i in range(len(amplitude)):
        if amplitude[i] < threshold:
            return i, amplitude
    return None, amplitude  # In case no drop is detected

# Function to apply a moving average filter to smooth the phase
def moving_average(signal, window_size=4):
    return np.convolve(signal, np.ones(window_size)/window_size, mode='valid')

# Function to calculate the slope and detect the local minimum on the smoothed phase
def find_phase_flattening(I_values, Q_values, packet_end_index, slope_window_size=10, avg_window_size=4):
    # Calculate the phase and unwrap it
    phase = np.arctan2(Q_values, I_values)
    unwrapped_phase = np.unwrap(phase)

    # Apply a moving average filter to smooth the unwrapped phase with smaller window size (4 samples)
    smoothed_phase = moving_average(unwrapped_phase, window_size=avg_window_size)

    # Calculate slope over a window of 10 samples on the smoothed phase
    slope = np.convolve(np.diff(smoothed_phase), np.ones(slope_window_size)/slope_window_size, mode='valid')

    # Look for the change in slope direction (from negative to positive)
    for i in range(packet_end_index - slope_window_size - 1, 0, -1):
        if slope[i] > 0 and slope[i - 1] < 0:  # Local minimum (slope changes from negative to positive)
            return i, phase, unwrapped_phase, smoothed_phase
    return None, phase, unwrapped_phase, smoothed_phase  # In case no flattening is detected

# Function to plot the results
def plot_results(indices, I_values, Q_values, amplitude, phase, unwrapped_phase, smoothed_phase, packet_end_index=None, phase_flattening_index=None, dump_index=None):
    plt.figure(figsize=(12, 16))  # Increased figure size to accommodate more subplots

    # Plot I values
    plt.subplot(6, 1, 1)  # Adjust subplot layout to fit all 6 plots
    plt.plot(indices, I_values, label='I Values', color='blue')
    plt.title(f'I Values (Dump {dump_index})')
    plt.xlabel('Index')
    plt.ylabel('I')
    plt.legend()

    # Plot Q values
    plt.subplot(6, 1, 2)
    plt.plot(indices, Q_values, label='Q Values', color='orange')
    plt.title(f'Q Values (Dump {dump_index})')
    plt.xlabel('Index')
    plt.ylabel('Q')
    plt.legend()

    # Plot amplitude
    plt.subplot(6, 1, 3)
    plt.plot(indices, amplitude, label='Amplitude', color='green')
    if packet_end_index is not None:
        plt.axvline(x=indices[packet_end_index], color='red', linestyle='--', label='Packet End')
    plt.title(f'Amplitude (Dump {dump_index})')
    plt.xlabel('Index')
    plt.ylabel('Amplitude')
    plt.legend()

    # Plot direct (wrapped) phase
    plt.subplot(6, 1, 4)
    plt.plot(indices, phase, label='Wrapped Phase', color='orange')
    if packet_end_index is not None:
        plt.axvline(x=indices[packet_end_index], color='red', linestyle='--', label='Packet End')
    if phase_flattening_index is not None:
        plt.axvline(x=indices[phase_flattening_index], color='blue', linestyle='--', label='Phase Flattening')
    plt.title(f'Wrapped Phase (Dump {dump_index})')
    plt.xlabel('Index')
    plt.ylabel('Phase (radians)')
    plt.legend()

    # Plot unwrapped phase
    plt.subplot(6, 1, 5)
    plt.plot(indices[:len(unwrapped_phase)], unwrapped_phase, label='Unwrapped Phase', color='purple')
    if packet_end_index is not None:
        plt.axvline(x=indices[packet_end_index], color='red', linestyle='--', label='Packet End')
    if phase_flattening_index is not None:
        plt.axvline(x=indices[phase_flattening_index], color='blue', linestyle='--', label='Phase Flattening')
    plt.title(f'Unwrapped Phase (Dump {dump_index})')
    plt.xlabel('Index')
    plt.ylabel('Phase (radians)')
    plt.legend()

    # Plot smoothed phase
    plt.subplot(6, 1, 6)
    plt.plot(indices[:len(smoothed_phase)], smoothed_phase, label='Smoothed Phase', color='blue')
    if packet_end_index is not None:
        plt.axvline(x=indices[packet_end_index], color='red', linestyle='--', label='Packet End')
    if phase_flattening_index is not None:
        plt.axvline(x=indices[phase_flattening_index], color='blue', linestyle='--', label='Phase Flattening')
    plt.title(f'Smoothed Phase (Dump {dump_index})')
    plt.xlabel('Index')
    plt.ylabel('Phase (radians)')
    plt.legend()

    plt.tight_layout()
    plt.show()

# Main function to process IQ dumps from the log and perform the calculations
def main(file_path):
    # Parse the IQ data dumps from the log file
    iq_dumps = parse_iq_data(file_path)
    
    # Iterate through each IQ dump and process
    for dump_index, dump in enumerate(iq_dumps):
        indices = np.array([iq[0] for iq in dump])  # Extract the index for X-axis
        I_values = np.array([iq[1] for iq in dump])
        Q_values = np.array([iq[2] for iq in dump])

        # Step 1: Find the end of the packet based on amplitude drop
        packet_end_index, amplitude = find_packet_end(I_values, Q_values)
        
        # Step 2: Find the phase flattening point using the smoothed phase
        phase_flattening_index, phase, unwrapped_phase, smoothed_phase = find_phase_flattening(I_values, Q_values, packet_end_index if packet_end_index is not None else len(I_values) - 1)

        # Step 3: Plot the results
        plot_results(indices, I_values, Q_values, amplitude, phase, unwrapped_phase, smoothed_phase, packet_end_index, phase_flattening_index, dump_index)

if __name__ == '__main__':
    file_path = 'masterAnchor.txt'  # Replace this with the actual path to your log file
    #file_path = 'clientAnchor0.txt'
    main(file_path)
