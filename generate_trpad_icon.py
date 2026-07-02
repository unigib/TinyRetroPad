from pathlib import Path

width = 16
height = 16
header = b"\x00\x00\x01\x00\x01\x00"
dib = (
    (40).to_bytes(4, 'little') +
    width.to_bytes(4, 'little') +
    (height * 2).to_bytes(4, 'little') +
    (1).to_bytes(2, 'little') +
    (32).to_bytes(2, 'little') +
    (0).to_bytes(4, 'little') +
    (width * height * 4).to_bytes(4, 'little') +
    (0).to_bytes(4, 'little') +
    (0).to_bytes(4, 'little') +
    (0).to_bytes(4, 'little') +
    (0).to_bytes(4, 'little')
)

pixels = bytearray()
for y in range(height):
    for x in range(width):
        if 4 <= x < 12 and 4 <= y < 12:
            pixels.extend((0xFF, 0x00, 0x00, 0xFF))
        else:
            pixels.extend((0x00, 0x00, 0x00, 0x00))

mask = bytearray()
for _ in range(height):
    mask.extend((0x00, 0x00))
    mask.extend((0x00, 0x00))

image_data = dib + pixels + mask
entry = (
    (width if width < 256 else 0).to_bytes(1, 'little') +
    (height if height < 256 else 0).to_bytes(1, 'little') +
    (0).to_bytes(1, 'little') +
    (0).to_bytes(1, 'little') +
    (1).to_bytes(2, 'little') +
    (32).to_bytes(2, 'little') +
    len(image_data).to_bytes(4, 'little') +
    (6 + 16).to_bytes(4, 'little')
)

Path('trpad.ico').write_bytes(header + entry + image_data)
print('wrote trpad.ico', Path('trpad.ico').stat().st_size)
