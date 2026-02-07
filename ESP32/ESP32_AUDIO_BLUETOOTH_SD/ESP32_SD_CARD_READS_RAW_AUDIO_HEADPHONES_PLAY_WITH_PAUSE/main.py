import argparse
import numpy as np
import soundfile as sf
import librosa
from scipy import signal


def band_limit(y, sr, hp_hz=380.0, lp_hz=4200.0, order=4):
    # Butterworth high-pass + low-pass
    sos_hp = signal.butter(order, hp_hz, btype="highpass", fs=sr, output="sos")
    sos_lp = signal.butter(order, lp_hz, btype="lowpass", fs=sr, output="sos")
    y = signal.sosfilt(sos_hp, y)
    y = signal.sosfilt(sos_lp, y)
    return y


def soft_clip(y, drive=1.35):
    # Gentle saturation; drive ~ 1.1–1.8
    return np.tanh(y * drive)


def bitcrush(y, bits=12):
    # Quantize amplitude to simulate bit depth
    levels = 2 ** bits
    return np.round(y * (levels / 2)) / (levels / 2)


def downsample_crush(y, factor=2):
    # Sample-rate reduction: hold every N samples
    if factor <= 1:
        return y
    y2 = y.copy()
    y2[::factor] = y[::factor]
    # hold value (zero-order hold)
    for i in range(1, factor):
        y2[i::factor] = y2[0::factor]
    return y2


def tiny_room_reverb(y, sr, mix=0.08, decay=0.35):
    """
    Very small, cheap reverb: multi-tap comb-like echoes.
    mix: 0.03–0.15 usually enough
    decay: 0.2–0.6
    """
    delays_ms = [18, 33, 55, 79]
    out = y.astype(np.float32).copy()
    for i, d in enumerate(delays_ms):
        delay = int(sr * (d / 1000.0))
        if delay <= 0 or delay >= len(y):
            continue
        gain = (decay ** (i + 1))
        out[delay:] += y[:-delay] * gain
    # wet/dry mix
    return (1.0 - mix) * y + mix * out


def normalize(y, headroom_db=1.0):
    peak = np.max(np.abs(y)) + 1e-12
    target = 10 ** (-headroom_db / 20.0)
    return y * (target / peak)


def droidify(
    y, sr,
    pitch_semitones=5.0,
    formant_hint="none",
    hp_hz=380.0, lp_hz=4200.0,
    drive=1.35,
    bits=12,
    crush_factor=2,
    reverb_mix=0.08
):
    """
    formant_hint: placeholder for now (true formant shifting is non-trivial).
    You still get a very close vibe with pitch + EQ + texture.
    """
    # 1) Pitch shift (keeps timing; good for voice)
    y = librosa.effects.pitch_shift(y, sr=sr, n_steps=pitch_semitones)

    # 2) Band-limit to feel like small speaker / droid body
    y = band_limit(y, sr, hp_hz=hp_hz, lp_hz=lp_hz)

    # 3) Add subtle digital texture
    y = soft_clip(y, drive=drive)
    y = bitcrush(y, bits=bits)
    y = downsample_crush(y, factor=crush_factor)

    # 4) Optional tiny reverb to feel "inside a shell"
    if reverb_mix > 0:
        y = tiny_room_reverb(y, sr, mix=reverb_mix, decay=0.35)

    # 5) Normalize
    y = normalize(y, headroom_db=1.0)
    return y


def main():
    p = argparse.ArgumentParser()
    p.add_argument("input_wav", help="Input voice WAV (mono or stereo)")
    p.add_argument("output_wav", help="Output WAV")
    p.add_argument("--pitch", type=float, default=5.0, help="Pitch shift in semitones (typ. +3 to +7)")
    p.add_argument("--hp", type=float, default=380.0, help="High-pass Hz (typ. 300–500)")
    p.add_argument("--lp", type=float, default=4200.0, help="Low-pass Hz (typ. 3000–5000)")
    p.add_argument("--drive", type=float, default=1.35, help="Saturation drive (typ. 1.1–1.8)")
    p.add_argument("--bits", type=int, default=12, help="Bit depth for crush (typ. 10–14)")
    p.add_argument("--crush", type=int, default=2, help="Sample-rate crush factor (typ. 1–4)")
    p.add_argument("--reverb", type=float, default=0.08, help="Reverb mix 0..0.2")
    args = p.parse_args()

    y, sr = sf.read(args.input_wav, always_2d=True)
    # convert to mono (droid voices often are)
    y = y.mean(axis=1).astype(np.float32)

    out = droidify(
        y, sr,
        pitch_semitones=args.pitch,
        hp_hz=args.hp,
        lp_hz=args.lp,
        drive=args.drive,
        bits=args.bits,
        crush_factor=args.crush,
        reverb_mix=args.reverb
    )

    sf.write(args.output_wav, out, sr)
    print(f"Wrote: {args.output_wav}")


if __name__ == "__main__":
    main()
