# Person Detection for OpenMV Cam on Nicla Vision
# This is an extension to blob detection in blob.py + TOF sensor
# Based on the FOMO example from OpenMV. # other models not loading for now

import sensor
import time
import ml
from ml.utils import NMS
import math
import image
import network # Import module for Wi-Fi networking
import socket # Import module for socket communication
from machine import I2C
from vl53l1x import VL53L1X



# Wi-Fi 
ssid = "Conn"
password = "12345678"
server_ip = "192.168.154.79"  # The IP address of the Wi-Fi shield
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

# Initialize camera
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)  # Set frame size to QVGA (320x240)
sensor.set_windowing((240, 240))   # Set 240x240 window
sensor.skip_frames(time=2000)      # Let the camera adjust

#tof sensor
tof = VL53L1X(I2C(2))

# Set detection parameters
min_confidence = 0.4
threshold_list = [(math.ceil(min_confidence * 255), 255)]

# Define detection colors
colors = [
    (255, 0, 0),    # Red - background/no detection
    (0, 255, 0),    # Green - person
]

try:
    # Try built-in models first
    model = ml.Model("person_detection")
    print("Using built-in person detection model")
except:
    try:
        # Try to load from filesystem
        model = ml.Model("person_detection_nicla.tflite", load_to_fb=True)
        print("Using custom person detection model")
    except:
        # If both fail, try FOMO face detection as fallback
        model = ml.Model("fomo_face_detection")
        print("Using FOMO face detection as fallback")

print("Model info:", model)

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

# Post-processing function for FOMO models
def fomo_post_process(model, inputs, outputs):
    n, oh, ow, oc = model.output_shape[0]
    nms = NMS(ow, oh, inputs[0].roi)
    for i in range(oc):
        img = image.Image(outputs[0][0, :, :, i] * 255)
        blobs = img.find_blobs(
            threshold_list, x_stride=1, area_threshold=1, pixels_threshold=1
        )
        for b in blobs:
            rect = b.rect()
            x, y, w, h = rect
            score = (
                img.get_statistics(thresholds=threshold_list, roi=rect).l_mean() / 255.0
            )
            nms.add_bounding_box(x, y, x + w, y + h, score, i)
    return nms.get_bounding_boxes()

frame_center_x = 320 // 2  # Center of the frame in the x-axis for QQVGA (160 width)

# Setup clock for FPS calculation
clock = time.clock()

# Main loop
while True:
    clock.tick()
    img = sensor.snapshot()

    try:
        # Run inference with the post-processing callback
        detection_lists = model.predict([img], callback=fomo_post_process)

        # Process results
        for i, detection_list in enumerate(detection_lists):
            if i == 0:
                continue  # Skip background class

            if len(detection_list) == 0:
                continue  # No detections for this class

            # Print class name
            print("********** %s **********" % (model.labels[i] if hasattr(model, 'labels') else f"Class {i}"))

            # Process each detection
            for (x, y, w, h), score in detection_list:
                # Calculate center point
                center_x = math.floor(x + (w / 2))
                center_y = math.floor(y + (h / 2))

                if center_x < frame_center_x:
                    position = "Left"
                else:
                    position = "Right"

                # Print detection details
                print(f"x {center_x}\ty {center_y}\tscore {score}")

                # Draw detection visualization
                img.draw_rectangle((x, y, w, h), color=colors[i % len(colors)])
                img.draw_circle((center_x, center_y, 12), color=colors[i % len(colors)])
                img.draw_string(x, y, f"{score:.2f}", color=colors[i % len(colors)], scale=2)
                print(f"Distance: {tof.read()}mm")
                time.sleep_ms(50)

                data = f"Blob Center: ({center_x},{center_y}), Position: {position} , Distance: {tof.read()}mm"

                send_data_to_arduino(data)

    except Exception as e:
        # Handle errors
        img.draw_string(0, 0, f"Error: {type(e).__name__}", color=(255, 0, 0), scale=2)
        print("Error:", e)

    # Print FPS
    print(f"FPS: {clock.fps():.2f}")
