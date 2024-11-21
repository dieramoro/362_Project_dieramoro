#!/usr/bin/env python3
import random

# Constants from track.h
MIDDLE_POS = 120
LEFT_POS = 50
RIGHT_POS = 190

DOWN_NOTE = 0
UP_NOTE = 1

NUMBER_OF_NOTES = 100
INTERVAL = 160

def generate_track():
    track = []
    current_position = 0
    
    # Generate notes with decreasing positions and enforced minimum intervals
    for i in range(NUMBER_OF_NOTES):
        # Calculate minimum gap as multiple of INTERVAL (40)
        gap = INTERVAL * random.randint(1, 2)  # Ensures minimum 40 unit gaps
        current_position -= gap  # Makes positions increasingly negative
        
        note = {
            "position": current_position,
            "string": random.choice([LEFT_POS, MIDDLE_POS, RIGHT_POS]),
            "dir": random.choice([UP_NOTE, DOWN_NOTE]),
            "played": 0
        }
        track.append(note)
    
    return track

def save_track_to_c_file():
    track = generate_track()
    
    with open("src/track.c", "w") as f:
        f.write('#include "track.h"\n\n')
        f.write("note Track[20] = {\n")
        
        for note in track:
            f.write(f"    {{ {note['position']}, {note['string']}, "
                   f"{note['dir']}, {note['played']} }},\n")
        
        f.write("};\n")

save_track_to_c_file()