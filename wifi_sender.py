import pyb # Import module for board related functions
import sensor # Import the module for sensor related functions
import image # Import module containing machine vision algorithms
import time # Import module for tracking elapsed time
import network # Import module for Wi-Fi networking
import socket # Import module for socket communication

# Initialize sensor settings
sensor.reset() # Resets the sensor
sensor.set_pixformat(sensor.RGB565) # Sets the sensor to RGB
sensor.set_framesize(sensor.QQVGA) # Sets the resolution to 160x120 px for QQVGA
sensor.set_hmirror(True) # Mirrors the image horizontally
sensor.skip_frames(time=2000) # Skip some frames to let the image stabilize

# LAB thresholds for detecting various shades of red
thresholdsRedBlob = (30, 70, 40, 70, -20, 20) # Red blob
thresholdsDeepRed = (15, 50, 50, 90, 0, 20)  # Deep red
thresholdsPinkRed = (50, 80, 20, 50, -10, 10)  # Pinkish red
thresholdsOrangeRed = (30, 60, 20, 50, 30, 70)  # Orangish red
thresholdsDarkRed = (10, 40, 40, 70, -10, 30)  # Darker red
thresholdsBrightRed = (30, 70, 40, 80, -20, 20)  # Bright red

ledRed = pyb.LED(1)  # Red LED
ledGreen = pyb.LED(2)  # Green LED

frame_center_x = 160 // 2  # Center of the frame in the x-axis for QQVGA (160 width)

clock = time.clock()  # Instantiates a clock object

# Wi-Fi connection details
ssid = "SSID"
password = "PASSWD"
server_ip = "Ip"  # The IP address of the Wi-Fi shield
server_port = 80  # The port to communicate with the Arduino

# Connect Nicla Vision to the Wi-Fi network
nicla_wifi = network.WLAN(network.STA_IF)
nicla_wifi.active(True)
nicla_wifi.connect(ssid, password)

# Wait for the Wi-Fi connection to be established
while not nicla_wifi.isconnected():
    time.sleep(1)
    print("Connecting to WiFi...")

print("Connected to WiFi. IP:", nicla_wifi.ifconfig())

# Create a function to maintain a continuous connection and send data to the server
def send_data_to_arduino(data):
    try:
        addr = (server_ip, server_port)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  # Create a TCP socket
        sock.connect(addr)  # Connect to the Arduino server
        sock.send(data.encode() + b'\n')  # Send data with newline
        print("Data sent:", data)

        # Optionally, receive a response from the server
        response = sock.recv(1024)
        print("Response from server:", response.decode())

        sock.close()  # Close the socket after communication
    except Exception as e:
        print(f"Failed to send data: {e}")

# Main loop for image processing and data transmission
while(True):
    clock.tick()  # Advances the clock
    img = sensor.snapshot()  # Take a snapshot and save it in memory

    # Find blobs of different shades of red
    blobs = img.find_blobs([thresholdsBrightRed, thresholdsDarkRed, thresholdsOrangeRed, thresholdsPinkRed, thresholdsDeepRed], area_threshold=2500, merge=True)

    # Check if any blobs were found
    if blobs:
        for blob in blobs:
            # Draw a rectangle around the blob and a cross at the blob's center
            img.draw_rectangle(blob.rect(), color=(0, 255, 0))
            img.draw_cross(blob.cx(), blob.cy(), color=(0, 255, 0))

            # Get the center coordinates of the blob
            cx = blob.cx()
            cy = blob.cy()

            # Determine if the blob is to the left or right of the frame
            if cx < frame_center_x:
                position = "Left"
            else:
                position = "Right"

            # Prepare the data to send (coordinates and position)
            data = f"Blob Center: ({cx},{cy}), Position: {position}"

            # Send the data to the Arduino over Wi-Fi
            send_data_to_arduino(data)

        # Turn on the green LED if any blob is found
        ledGreen.on()
        ledRed.off()
    else:
        # Turn on the red LED if no blob is found
        ledGreen.off()
        ledRed.on()

    pyb.delay(50)  # Pause the execution for 50ms
