import re
# -*- coding: utf-8 -*-
"""Extract a single u8g2 font into a trimmed .c file (flash is only 64 KB)."""
import os

SRC = r"F:\deepseek_work\OLED_U8G2_T3\U8G2\csrc\u8g2_fonts.c"
DST = r"F:\deepseek_work\OLED_U8G2_T3\U8G2\csrc\u8g2_fonts_min.c"
FONT = "u8g2_font_6x10_tf"

data = open(SRC, "rb").read()
key = ("const uint8_t " + FONT + "[").encode()
i = data.find(key)
if i < 0:
    raise SystemExit("font not found")

# the data is stored as a list of string literals ending with  ";<newline>"
j = data.find(b'";\n', i)
if j < 0:
    raise SystemExit("end of array not found")
end = j + 3

block = data[i:end].decode("ascii")

header = (
    "/* Trimmed u8g2 font file: only %s is kept (about 2 KB).\n"
    " * The full u8g2_fonts.c is 30 MB and does not fit into the 64 KB flash of\n"
    " * STM32F103C8, so the library is trimmed for this project. */\n"
    "#include \"u8g2.h\"\n\n" % FONT
)

with open(DST, "w", encoding="ascii", newline="\n") as f:
    f.write(header + block + "\n")

print("wrote", DST, os.path.getsize(DST), "bytes")


