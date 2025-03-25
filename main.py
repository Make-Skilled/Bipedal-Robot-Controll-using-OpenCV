import cv2
import mediapipe as mp
import pandas as pd
import pickle
import serial
import time

# Initialize serial communication (adjust port as needed)
ser = serial.Serial('COM6', 115200, timeout=1)  # Replace 'COM6' with your Arduino port
time.sleep(2)  # Wait for serial connection to establish

# Load the pre-trained model (assumes model predicts these specific gestures)
model = pickle.load(open('model.pkl', 'rb'))

# Initialize MediaPipe Hands
mp_hands = mp.solutions.hands
mp_drawing = mp.solutions.drawing_utils
mp_drawing_styles = mp.solutions.drawing_styles

# Gesture detection variables
last_command = None
command_cooldown = 2.0  # Seconds between commands to avoid spamming
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
cap = cv2.VideoCapture(0)  # 1 for external webcam, 0 for built-in

with mp_hands.Hands(
    min_detection_confidence=0.7,
    min_tracking_confidence=0.5,
    max_num_hands=1) as hands:
    
    while cap.isOpened():
        ret, frame = cap.read()
        if not ret:
            continue

        # Flip frame horizontally for a selfie-view display
        frame = cv2.flip(frame, 1)
        # Convert to RGB for MediaPipe
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

        # Process frame with MediaPipe Hands
        results = hands.process(rgb_frame)

        # Convert back to BGR for OpenCV
        frame = cv2.cvtColor(rgb_frame, cv2.COLOR_RGB2BGR)

        if results.multi_hand_landmarks:
            # Process the first detected hand
            hand_landmarks = results.multi_hand_landmarks[0]
            
            # Extract hand landmark coordinates
            landmarks = []
            for landmark in hand_landmarks.landmark:
                landmarks.append(landmark.x)
                landmarks.append(landmark.y)
                landmarks.append(landmark.z)

            # Draw hand landmarks and connections
            mp_drawing.draw_landmarks(
                frame,
                hand_landmarks,
                mp_hands.HAND_CONNECTIONS,
                landmark_drawing_spec=mp_drawing_styles.DrawingSpec(
                    color=(251, 245, 202), thickness=2, circle_radius=2),
                connection_drawing_spec=mp_drawing_styles.DrawingSpec(
                    color=(251, 245, 202), thickness=3),
            )

            # Predict gesture using the loaded model
            gesture_name = model.predict([landmarks])[0]  # Get the predicted gesture name
            current_time = time.time()

            # Display full gesture name on the frame
            font = cv2.FONT_HERSHEY_TRIPLEX
            org = (50, 50)
            fontScale = 1
            color = (255, 125, 135)  # Blue, Green, Red
            thickness = 2
            frame = cv2.putText(
                frame, gesture_name, org, font, fontScale, color, thickness, cv2.LINE_4)

            # Send only the G# command to Arduino
            if gesture_name in gesture_map:
                command = gesture_map[gesture_name]
                if command != last_command and (current_time - last_command_time) > command_cooldown:
                    ser.write(f"{command}\n".encode())
                    print(f"Sent command to Arduino: {command}")
                    last_command = command
                    last_command_time = current_time

        # Display the frame
        cv2.imshow('Hand Gesture Tracking', frame)

        # Exit on 'q' or ESC press
        if cv2.waitKey(10) in [27, ord('q')]:
            break

# Cleanup
cap.release()
cv2.destroyAllWindows()
ser.close()