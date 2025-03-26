import sensor
import time
import image
import network
import socket
from machine import I2C
from vl53l1x import VL53L1X

# Wi-Fi settings
SSID = "Conn"
PASSWORD = "12345678"
SERVER_IP = "192.168.154.23"
SERVER_PORT = 80

# Connect Nicla Vision to Wi-Fi
nicla_wifi = network.WLAN(network.STA_IF)
nicla_wifi.active(True)
nicla_wifi.connect(SSID, PASSWORD)

while not nicla_wifi.isconnected():
    time.sleep(1)
    print("Connecting to WiFi...")

print("Connected to WiFi. IP:", nicla_wifi.ifconfig())

# Initialize camera
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QQVGA)
sensor.set_windowing((240, 240))
sensor.skip_frames(time=2000)

# Initialize ToF sensor
tof = VL53L1X(I2C(2))

# Single socket instance
sock = None

def connect_to_arduino():
    global sock
    if sock:
        sock.close()
        sock = None

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(2)
        sock.connect((SERVER_IP, SERVER_PORT))
        print("Connected to Arduino")
    except MemoryError:
        print("Memory Error! Restarting Nicla Vision...")
        import machine
        machine.reset()
    except Exception as e:
        print(f"Failed to connect: {e}")
        sock = None

connect_to_arduino()

def send_data_to_arduino(command):
    global sock
    if sock is None:
        connect_to_arduino()
        if sock is None:
            return

    try:
        data = f"{command}\n".encode()
        sock.sendall(data)
        print(f"Sent command: {command}")

        sock.settimeout(1)
        response = sock.recv(1024)
        print("Response:", response.decode())

    except MemoryError:
        print("Memory Error while sending data! Restarting...")
        import machine
        machine.reset()
    except Exception as e:
        print(f"Send error: {e}")
        sock.close()
        sock = None
        time.sleep(1)

# Red color detection thresholds
red_thresholds = [
    (30, 70, 40, 70, -20, 20),
    (15, 50, 50, 90, 0, 20),
    (50, 80, 20, 50, -10, 10),
    (30, 60, 20, 50, 30, 70),
    (10, 40, 40, 70, -10, 30),
    (30, 70, 40, 80, -20, 20)
]

# Cache last ToF reading
last_distance = None
distance_update_time = time.ticks_ms()

# Main loop
while True:
    try:
        img = sensor.snapshot()

        # Update ToF distance every 500ms
        if time.ticks_diff(time.ticks_ms(), distance_update_time) > 500:
            try:
                last_distance = tof.read()
                if last_distance is None:
                    last_distance = 9999  # Assume far distance if sensor fails
            except Exception as e:
                print("ToF sensor error:", e)
                last_distance = 9999  # Default distance if error
            distance_update_time = time.ticks_ms()

        # Detect red blobs
        blobs = img.find_blobs(red_thresholds, pixels_threshold=100, area_threshold=200)

        if not blobs:
            send_data_to_arduino(0)  # Stop motors if no red detected
            continue

        # **MERGE BLOBS INTO ONE SINGLE BOUNDING BOX**
        min_x = min(blob.x() for blob in blobs)
        min_y = min(blob.y() for blob in blobs)
        max_x = max(blob.x() + blob.w() for blob in blobs)
        max_y = max(blob.y() + blob.h() for blob in blobs)

        merged_width = max_x - min_x
        merged_height = max_y - min_y
        merged_cx = min_x + merged_width // 2
        merged_cy = min_y + merged_height // 2

        # Draw merged blob
        img.draw_rectangle(min_x, min_y, merged_width, merged_height, color=(255, 0, 0))
        img.draw_cross(merged_cx, merged_cy, color=(0, 255, 0))

        # Send movement commands based on merged blob
        command = 2 if last_distance < 200 else 1
        send_data_to_arduino(command)

        print(f"Merged Red Blob at ({merged_cx}, {merged_cy}), Size=({merged_width}, {merged_height}), Distance={last_distance}mm, Command={command}")

    except MemoryError:
        print("Critical Memory Error! Restarting device...")
        import machine
        machine.reset()
    except Exception as e:
        print("Error in loop:", e)
        time.sleep(0.5)  # Prevent crashing
