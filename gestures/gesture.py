import cv2
import mediapipe as mp
import pandas as pd

# Load MediaPipe hands tracking model
mp_hands = mp.solutions.hands
mp_drawing = mp.solutions.drawing_utils
mp_drawing_styles = mp.solutions.drawing_styles

# Load video capture (1 for external webcam, 0 for built-in)
cap = cv2.VideoCapture(0)

with mp_hands.Hands(
    min_detection_confidence=0.7,
    min_tracking_confidence=0.5,
    max_num_hands=2) as hands:  # You can adjust max_num_hands based on your needs
    
    while cap.isOpened():
        ret, frame = cap.read()

        if not ret:
            continue

        # Convert the image to RGB
        image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

        # Process the image to detect hands
        results = hands.process(image)

        # Convert the image back to BGR
        image = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)

        if results.multi_hand_landmarks:
            # Process each detected hand
            for hand_landmarks in results.multi_hand_landmarks:
                # Extract hand landmark coordinates
                landmarks = []
                for landmark in hand_landmarks.landmark:
                    landmarks.append(landmark.x)
                    landmarks.append(landmark.y)
                    landmarks.append(landmark.z)

                # Draw hand landmarks and connections
                mp_drawing.draw_landmarks(
                    image,
                    hand_landmarks,
                    mp_hands.HAND_CONNECTIONS,
                    landmark_drawing_spec=mp_drawing_styles.DrawingSpec(
                        color=(251, 245, 202), thickness=2, circle_radius=2),
                    connection_drawing_spec=mp_drawing_styles.DrawingSpec(
                        color=(251, 245, 202), thickness=3),
                )

                print(f"Number of coordinates: {len(landmarks)}")

                # Save hand tracking data to a CSV file
                data = str(landmarks)
                data = data[1:-1]  # removing square brackets of list
                with open('gesture7_horns.csv', 'a') as f:
                    f.write(str(data) + ',Hip Hop\n')

        cv2.imshow('Hand Gesture Tracking', image)

        # Press 'q' or ESC to exit
        if cv2.waitKey(10) in [27, ord('q')]:
            break

# Release resources
cap.release()
cv2.destroyAllWindows()