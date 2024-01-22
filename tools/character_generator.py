from PIL import Image, ImageDraw, ImageFont

def render_font_to_byte_arrays(font_path, font_size, pixel_size):
    font = ImageFont.truetype(font_path, font_size)
    char_byte_arrays = []

    for char_code in range(33, 127):
        char = chr(char_code)
        
        width, height = font.getbbox(char)[2:]
        image = Image.new('L', (width, height), 0)
        draw = ImageDraw.Draw(image)
        draw.text((0, 0), char, font=font, fill=255)

        new_height = pixel_size
        new_width = 2*(int(pixel_size * (float(width) / height)) // 2)
        
        resized_image = image.resize((new_width, new_height))
        
        byte_array = bytearray()
        byte_array.append(new_width)
        for y in range(new_height):
            for x in range(0, new_width, 2):
                pixel1 = resized_image.getpixel((x, y)) // 16
                pixel2 = resized_image.getpixel((x + 1, y)) // 16
                byte_value = (pixel1 << 4) | pixel2
                byte_array.append(byte_value)
        
        char_byte_arrays.append(byte_array)

    byteCount = 0
    out_str = "const char FONT[] = {\n"
    char_dict = "const int CHAR_DICT[] = {"
    i = 33
    for character in char_byte_arrays:
        char_dict += "%i," % byteCount
        out_str = out_str+"\n\t // %s" % chr(i) 
        out_str = out_str+"\n\t"
        j = 0
        for byte in character:
            out_str = out_str+("0x%X," % byte)
            byteCount += 1
            j += 1
            if j % 22 == 0:
                out_str += "\n\t"
        
        i += 1
        
        # out_str = out_str+"],\n"
    out_str = out_str+"\n};\n"
    char_dict += "};\n"
    out_str += char_dict

    return out_str

font_path = "./inter.ttf"
font_size = 28
pixel_size = 20

char_byte_arrays = render_font_to_byte_arrays(font_path, font_size, pixel_size)
print(char_byte_arrays)