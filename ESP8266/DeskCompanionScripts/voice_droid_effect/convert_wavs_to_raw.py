#!/usr/bin/env python3
"""Convert .wav files to raw PCM by stripping the WAV header.

Usage:
  python convert_wavs_to_raw.py "Droid Vocalization Sound Pack"

This script:
- scans the given folder for .wav/.WAV files (non-recursive)
- replaces whitespace in names with underscores
- removes square brackets '[' and ']' from filenames
- writes raw PCM frame data to a .raw file next to the source file
"""
from pathlib import Path
import argparse
import sys
import re
import shutil
import subprocess


def sanitize(name: str) -> str:
    name = re.sub(r"\s+", "_", name)
    name = name.replace("[", "").replace("]", "")
    return name


def convert_folder(folder: Path, dry_run: bool = False) -> int:
    folder = Path(folder)
    if not folder.exists():
        print(f"Folder not found: {folder}", file=sys.stderr)
        return 2

    wavs = sorted(folder.glob("*.wav")) + sorted(folder.glob("*.WAV"))
    if not wavs:
        print(f"No WAV files found in: {folder}")
        return 0

    for wav in wavs:
        out_name = sanitize(wav.stem) + ".raw"
        out_path = wav.parent / out_name

        ffmpeg = shutil.which("ffmpeg")
        if not ffmpeg:
            print("ffmpeg not found in PATH. Install ffmpeg to perform conversions.", file=sys.stderr)
            return 3

        cmd = [
            ffmpeg,
            "-y",
            "-i",
            str(wav),
            "-ac",
            "2",
            "-ar",
            "44100",
            "-f",
            "s16le",
            str(out_path),
        ]

        if dry_run:
            print(f"[DRY] {' '.join(cmd)}")
            continue

        try:
            res = subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            print(f"Wrote: {out_path}")
        except subprocess.CalledProcessError as e:
            err = e.stderr.decode(errors="replace").strip()
            print(f"ffmpeg failed for {wav.name}: {err}", file=sys.stderr)
        except Exception as e:
            print(f"Unexpected error for {wav}: {e}", file=sys.stderr)

    return 0


def main() -> int:
    p = argparse.ArgumentParser(description="Convert .wav files to .raw (strip WAV header)")
    p.add_argument("folder", nargs="?", default="Droid Vocalization Sound Pack", help="Folder with .wav files")
    p.add_argument("--dry-run", action="store_true", help="Show what would be converted without writing files")
    args = p.parse_args()

    return convert_folder(Path(args.folder), args.dry_run)


if __name__ == "__main__":
    raise SystemExit(main())
