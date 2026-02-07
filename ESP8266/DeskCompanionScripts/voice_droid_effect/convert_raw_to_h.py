import sys
from pathlib import Path

def main():
    if len(sys.argv) < 3:
        print("Usage: python raw_to_h.py input.raw output.h [array_name]")
        sys.exit(1)

    in_path = Path(sys.argv[1])
    out_path = Path(sys.argv[2])
    name = sys.argv[3] if len(sys.argv) >= 4 else in_path.stem

    data = in_path.read_bytes()

    with out_path.open("w", encoding="utf-8") as f:
        f.write("#pragma once\n")
        f.write("#include <Arduino.h>\n\n")
        f.write(f"const uint8_t {name}[] PROGMEM = {{\n")

        # 16 bytes per line
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            f.write("  " + ", ".join(f"0x{b:02X}" for b in chunk))
            f.write(",\n" if i + 16 < len(data) else "\n")

        f.write("};\n\n")
        f.write(f"const uint32_t {name}_len = {len(data)};\n")

    print(f"OK: wrote {len(data)} bytes to {out_path} as {name}[]")

if __name__ == "__main__":
    main()
