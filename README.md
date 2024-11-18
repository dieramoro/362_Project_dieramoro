Game Flow 
Initialization:

Initialize GPIO, Timer, and LCD.
Display the start screen.
Gameplay:

Every 50ms (via Timer):
Move all active notes down 1px.
Erase and redraw notes.
Every 2000ms:
Generate a new note (if not already at the limit).
Check for player input:
If the strum button is pressed:
Check if a note is in the strum zone.
Compare the player’s button input with the note’s configuration.
Provide feedback (green for correct, red for missed).
Game End:

When all notes have exited the screen:
Display performance summary.
Wait for the strum button to restart.
Restart:

Reset all counters and states.
Start the game from the beginning.
