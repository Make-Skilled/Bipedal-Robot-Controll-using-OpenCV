import cv2
import mediapipe as mp
import serial
import time

# Initialize serial communication (adjust port as needed)
ser = serial.Serial('COM6', 115200, timeout=1)  # Replace 'COM3' with your Arduino port (e.g., '/dev/ttyUSB0' on Linux)
time.sleep(2)  # Wait for serial connection to establish

# Initialize MediaPipe Hands
mp_hands = mp.solutions.hands
hands = mp_hands.Hands(max_num_hands=1, min_detection_confidence=0.7, min_tracking_confidence=0.5)
mp_drawing = mp.solutions.drawing_utils

# Initialize webcam
cap = cv2.VideoCapture(0)

# Gesture detection variables
last_command = None
command_cooldown = 2.0  # Seconds between commands to avoid spamming
last_command_time = time.time()

def detect_gesture(landmarks):
    """Detect simple hand gestures based on landmarks."""
    # Extract key landmarks (fingertips and base points)
    thumb_tip = landmarks[4].y   # Thumb tip
    index_tip = landmarks[8].y   # Index finger tip
    middle_tip = landmarks[12].y # Middle finger tip
    ring_tip = landmarks[16].y   # Ring finger tip
    pinky_tip = landmarks[20].y  # Pinky tip
    wrist = landmarks[0].y       # Wrist

    # Gesture definitions (adjust thresholds as needed)
    if thumb_tip < wrist and index_tip > middle_tip and middle_tip > ring_tip and ring_tip > pinky_tip:
        return "G1"  # Open hand (Come on)
    elif index_tip < middle_tip and middle_tip < ring_tip and ring_tip < pinky_tip and thumb_tip > wrist:
        return "G2"  # Fist (Victory)
    elif index_tip < middle_tip and middle_tip < ring_tip and ring_tip > pinky_tip and thumb_tip < wrist:
        return "G3"  # Peace sign (Push-up)
    elif pinky_tip < ring_tip and ring_tip < middle_tip and middle_tip < index_tip and thumb_tip < wrist:
        return "G4"  # One finger up (One leg)
    elif thumb_tip < wrist and index_tip < middle_tip and middle_tip > ring_tip and ring_tip > pinky_tip:
        return "G5"  # Thumbs up (Tarzan)
    elif index_tip < middle_tip and middle_tip < ring_tip and ring_tip < pinky_tip and thumb_tip < index_tip:
        return "G6"  # Three fingers (Walking)
    elif middle_tip < index_tip and middle_tip < ring_tip and thumb_tip > wrist:
        return "G7"  # Horns (Hip-hop)
    return None

while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        break

    # Flip frame horizontally for a selfie-view display
    frame = cv2.flip(frame, 1)
    # Convert to RGB for MediaPipe
    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

    # Process frame with MediaPipe Hands
    results = hands.process(rgb_frame)

    if results.multi_hand_landmarks:
        for hand_landmarks in results.multi_hand_landmarks:
            # Draw landmarks on the frame
            mp_drawing.draw_landmarks(frame, hand_landmarks, mp_hands.HAND_CONNECTIONS)

            # Detect gesture
            gesture = detect_gesture(hand_landmarks.landmark)
            current_time = time.time()

            if gesture and gesture != last_command and (current_time - last_command_time) > command_cooldown:
                # Send gesture command to Arduino
                ser.write(f"{gesture}\n".encode())
                print(f"Sent command: {gesture}")
                last_command = gesture
                last_command_time = current_time

    # Display the frame
    cv2.imshow('Gesture Control', frame)

    # Exit on 'q' press
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# Cleanup
cap.release()
cv2.destroyAllWindows()
hands.close()
ser.close()