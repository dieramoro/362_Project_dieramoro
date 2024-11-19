#!/usr/bin/env python3
from PIL import Image
import numpy as np

width = 240
height = 320
name = 'test_background'

# Initialize to zero
arr = [ [0 for x in range(0,height)] for y in range(0,width) ]
img_arr = np.zeros((height, width, 3), dtype=np.uint8)


black = 0x0000
white = 0xffff

black_rgb = (0, 0, 0)
white_rgb = (255, 255, 255)

# create background fretboard
for x in range(0, width):
    for y in range(0, height):
        # Set default background to black
        arr[x][y] = black
        img_arr[y, x] = black_rgb

        # Define string positions first (we'll need these for fret boundaries)
        string_positions = [50, width//2, width-50]  # Left, middle, and right strings
        string_thickness = 3  # Increased thickness
        
        # Draw three vertical lines (strings)
        for string_x in string_positions:
            if abs(x - string_x) <= string_thickness:
                arr[x][y] = white
                img_arr[y, x] = white_rgb
        
        # Draw horizontal fret lines, but only between the leftmost and rightmost strings
        if x >= string_positions[0] and x <= string_positions[2]:  # Only between first and last string
            fret_spacing = height // 8  # 12 frets
            if y % fret_spacing == 7:  # Draw horizontal lines at fret positions
                arr[x][y] = white
                img_arr[y, x] = white_rgb

#img = Image.fromarray(img_arr)
#img.save('python/background.jpg')

out = open('src/{name}.c','w')

# Print out the header
out.write("const struct {\n")
out.write("  unsigned int width;\n")
out.write("  unsigned int height;\n")
out.write("  unsigned int bytes_per_pixel;\n")
out.write("  unsigned char pixel_data[%d * %d * 2 + 1];\n" % (width,height))
out.write("} %s = {\n" % name)
out.write("  %d, %d, 2,\n" % (width,height))

# Print out one row at a time from top to bottom
for y in range(0, height):
  out.write('  "')
  for x in range(0, width):
    # Print each 16-bit value in little-endian format
    out.write('\\%03o\\%03o' % (arr[x][y]&0xff, arr[x][y]>>8))
  out.write('"\n')

out.write("};\n")
