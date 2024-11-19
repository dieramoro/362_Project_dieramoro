from PIL import Image
import sys

jpg_path = "python/real_background.jpg"    # Replace with your jpg file name
c_path = "src/background.c"

try:
    # Open the image file
    with Image.open(jpg_path) as img:
        # Ensure the image is in RGB format
        img = img.convert('RGB')
        width, height = img.size
        bytes_per_pixel = 2  # RGB565

        # Convert RGB888 to RGB565
        pixel_data = []
        for y in range(height):
            for x in range(width):
                r, g, b = img.getpixel((x, y))
                # Convert RGB888 to RGB565
                rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                # Store as two bytes (big-endian)
                pixel_data.append((rgb565 >> 8) & 0xFF)
                pixel_data.append(rgb565 & 0xFF)
        pixel_data = bytes(pixel_data)

        # Write to the C source file
        with open(c_path, 'w') as c_file:
            c_file.write('const struct {\n')
            c_file.write(f'  unsigned int width;\n')
            c_file.write(f'  unsigned int height;\n')
            c_file.write(f'  unsigned int bytes_per_pixel; /* {bytes_per_pixel} */\n')
            c_file.write(f'  unsigned char pixel_data[{len(pixel_data)} + 1];\n')
            c_file.write('} real_background = {\n')
            c_file.write(f'  {width}, {height}, {bytes_per_pixel},\n')
            c_file.write('  {\n    ')

            # Write pixel data in a formatted way
            for i, byte in enumerate(pixel_data):
                c_file.write(f'0x{byte:02x}, ')
                if (i + 1) % 12 == 0:
                    c_file.write('\n    ')

            c_file.write('0x00\n  }\n};\n')

except Exception as e:
    print(f"An error occurred: {e}", file=sys.stderr)

# Example usage:
# jpg_to_c_source('path_to_image.jpg', 'output.c')
