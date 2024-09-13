import re
import matplotlib.pyplot as plt
import time
import os

# File to monitor
log_file_path = "C:\\Julians\\Projects\\RCB WWVB\\Logs\\masterAnchor.txt"

# Regular expression pattern to match the exact line
pattern = re.compile(r"\*\*\*\*\*\*\*\*\*\*MasterAnchor wiwi_compute_phi_c_d unwrapped 2phi_C=([\d\.-]+)\s+2phi_D=([\d\.-]+)")

# Lists to store data points
phi_C_values = []
phi_D_values = []

# Create figure for plotting
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))  # Create two subplots (2 rows, 1 column)
line1, = ax1.plot([], [], label='2phi_C')
line2, = ax2.plot([], [], label='2phi_D')
ax1.set_ylabel('2phi_C (degrees)')
ax2.set_ylabel('2phi_D (degrees)')
ax2.set_xlabel('Data Points')
ax1.legend()
ax2.legend()

# Enable interactive mode in matplotlib to handle user input (drag, rescale, etc.)
plt.ion()

# Variable to control whether the program should continue running
program_running = True

# Event handler for closing the window
def handle_close(event):
    global program_running
    print("Plot window closed by user.")
    program_running = False

# Connect the close event to the handle_close function
fig.canvas.mpl_connect('close_event', handle_close)

# Wait for the log file to exist
while not os.path.exists(log_file_path):
    print(f"Waiting for log file '{log_file_path}' to be created...")
    time.sleep(1)  # Wait 1 second before checking again

# Open the log file
with open(log_file_path, 'r') as log_file:
    try:
        while program_running:
            # Try to read a new line
            line = log_file.readline()

            # If the line is empty, there are no new lines, wait for a short time and check again
            if not line:
                # Allow time for user interaction while waiting for new lines
                plt.pause(0.1)  # Small delay to handle user interaction
                continue

            # Check if the line matches the pattern
            match = pattern.search(line)
            if match:
                # Extract the values of 2phi_C and 2phi_D
                phi_C = float(match.group(1))
                phi_D = float(match.group(2))

                # Add them to the lists
                phi_C_values.append(phi_C)
                phi_D_values.append(phi_D)

                # Update the plots for each subplot
                line1.set_data(range(len(phi_C_values)), phi_C_values)
                line2.set_data(range(len(phi_D_values)), phi_D_values)

                # Autoscale both X and Y axes based on the new data
                ax1.relim()  # Recalculate the limits based on the new data
                ax1.autoscale_view(True, True, True)  # Autoscale X and Y axes
                ax2.relim()
                ax2.autoscale_view(True, True, True)

                # Redraw the plot and handle user interaction
                plt.draw()
                plt.pause(0.01)  # Small pause to allow plot update and interaction

    except KeyboardInterrupt:
        # Handle the Ctrl+C termination cleanly
        print("Program terminated by user.")

# Close the log file when done (optional)
print("Exiting program.")
log_file.close()
