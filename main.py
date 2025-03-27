import websocket
import matplotlib.pyplot as plt
import numpy as np
import csv
from collections import deque

# WebSocket server address (ESP32's AP default IP)
ESP32_IP = "192.168.4.1"
PORT = "81"

# Set up real-time plotting
buffer_size = 300  # Number of points to display
ecg_data = deque([0] * buffer_size, maxlen=buffer_size)

plt.ion()
fig, ax = plt.subplots()
line, = ax.plot(ecg_data)
ax.set_ylim(-100, 5000)  # Adjust for ADC 12-bit range
ax.set_title("Real-time ECG Signal")
ax.set_ylabel("ECG Value")  # Removed X-axis label

# Open CSV file to store ECG values
csv_file = open("ecg_data.csv", mode="w", newline="")
csv_writer = csv.writer(csv_file)
csv_writer.writerow(["ECG Value"])  # CSV header

def on_message(ws, message):
    try:
        value = int(message)
        ecg_data.append(value)

        # Save ECG value to CSV
        csv_writer.writerow([value])
        csv_file.flush()  # Ensure data is written immediately

        # Update plot
        line.set_ydata(ecg_data)
        plt.draw()
        plt.pause(0.01)
    except ValueError:
        pass  # Ignore invalid data

def on_error(ws, error):
    print(f"WebSocket Error: {error}")

def on_close(ws, close_status_code, close_msg):
    print("WebSocket Closed")
    csv_file.close()  # Close CSV file on exit

def on_open(ws):
    print("WebSocket Connected")

# Connect to ESP32 WebSocket
ws = websocket.WebSocketApp(f"ws://{ESP32_IP}:{PORT}", 
                            on_message=on_message, 
                            on_error=on_error, 
                            on_close=on_close)
ws.on_open = on_open

# Run WebSocket connection
ws.run_forever()
