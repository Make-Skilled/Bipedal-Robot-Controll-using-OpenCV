import cv2
import mediapipe as mp
import pandas as pd
import pickle
import serial
import time
import os

# Disable OpenCV GUI to avoid Qt display errors (for headless Raspberry Pi)
cv2.imshow = lambda *args, **kwargs: None  

# Detect Serial Port for Raspberry Pi (adjust if needed)
SERIAL_PORT = '/dev/ttyUSB0'  # Change to '/dev/ttyAMA0' if necessary

try:
    ser = serial.Serial(SERIAL_PORT, 115200, timeout=1)
    time.sleep(2)  # Allow serial connection to establish
    print(f"Connected to Arduino on {SERIAL_PORT}")
except serial.SerialException:
    print(f"Error: Could not connect to {SERIAL_PORT}")
    ser = None  # Prevent sending serial commands if connection fails

# Load the pre-trained model
model_path = 'model.pkl'
if os.path.exists(model_path):
    model = pickle.load(open(model_path, 'rb'))
    print("Model loaded successfully.")
else:
    print(f"Error: Model file '{model_path}' not found.")
    exit(1)

# Initialize MediaPipe Hands
mp_hands = mp.solutions.hands
mp_drawing = mp.solutions.drawing_utils
mp_drawing_styles = mp.solutions.drawing_styles

# Gesture detection variables
last_command = None
command_cooldown = 2.0  # Cooldown in seconds to avoid command spam
last_command_time = time.time()

# Gesture mapping to match Arduino code
gesture_map = {
    "Open Hand": "G1",    # Come on
    "Fist": "G2",         # Victory
    "Peace Sign": "G3",   # Push-up
    "One Finger": "G4",   # One leg
    "Thumbs Up": "G5",    # Tarzan
    "Three Fingers": "G6",# Walking
    "Hip Hop": "G7"       # Hip-hop
}

# Initialize webcam
cap = cv2.VideoCapture(0)  # Use 1 for an external webcam if needed

if not cap.isOpened():
    print("Error: Could not open webcam.")
    exit(1)

with mp_hands.Hands(
    min_detection_confidence=0.7,
    min_tracking_confidence=0.5,
    max_num_hands=1) as hands:
    
    while cap.isOpened():
        ret, frame = cap.read()
        if not ret:
            print("Error: Failed to capture frame.")
            continue

        # Flip frame horizontally for a selfie-view display
        frame = cv2.flip(frame, 1)
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

        # Process frame with MediaPipe Hands
        results = hands.process(rgb_frame)

        # Convert back to BGR for OpenCV
        frame = cv2.cvtColor(rgb_frame, cv2.COLOR_RGB2BGR)

        if results.multi_hand_landmarks:
            hand_landmarks = results.multi_hand_landmarks[0]
            
            # Extract hand landmark coordinates
            landmarks = []
            for landmark in hand_landmarks.landmark:
                landmarks.extend([landmark.x, landmark.y, landmark.z])

            # Draw hand landmarks
            mp_drawing.draw_landmarks(
                frame,
                hand_landmarks,
                mp_hands.HAND_CONNECTIONS,
                landmark_drawing_spec=mp_drawing_styles.DrawingSpec(
                    color=(251, 245, 202), thickness=2, circle_radius=2),
                connection_drawing_spec=mp_drawing_styles.DrawingSpec(
                    color=(251, 245, 202), thickness=3),
            )

            # Predict gesture
            gesture_name = model.predict([landmarks])[0]  # Get the predicted gesture name
            current_time = time.time()

            # Display gesture name on the frame
            font = cv2.FONT_HERSHEY_TRIPLEX
            frame = cv2.putText(frame, gesture_name, (50, 50), font, 1, (255, 125, 135), 2, cv2.LINE_4)

            # Send only the mapped command to Arduino
            if gesture_name in gesture_map:
                command = gesture_map[gesture_name]
                if command != last_command and (current_time - last_command_time) > command_cooldown:
                    if ser:  # Only send if serial connection exists
                        ser.write(f"{command}\n".encode("UTF-8"))
                        print(f"Sent command to Arduino: {command}")
                    last_command = command
                    last_command_time = current_time

        # Save frame instead of displaying (for debugging in headless mode)
        cv2.imwrite("output.jpg", frame)

        # Exit on 'q' or ESC press
        if cv2.waitKey(10) in [27, ord('q')]:
            break

# Cleanup
cap.release()
cv2.destroyAllWindows()
if ser:
    ser.close()
