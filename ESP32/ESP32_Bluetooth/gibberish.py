import argparse
import numpy as np
import soundfile as sf
from scipy import signal


def to_mono(x):
    if x.ndim == 1:
        return x.astype(np.float32)
    return x.mean(axis=1).astype(np.float32)


def butter_band(y, sr, hp=250.0, lp=4200.0, order=4):
    sos_hp = signal.butter(order, hp, btype="highpass", fs=sr, output="sos")
    sos_lp = signal.butter(order, lp, btype="lowpass", fs=sr, output="sos")
    y = signal.sosfilt(sos_hp, y)
    y = signal.sosfilt(sos_lp, y)
    return y


def envelope_follower(y, sr, attack_ms=5.0, release_ms=90.0):
    """
    Smooth amplitude envelope (talking cadence).
    Attack fast, release slower.
    """
    x = np.abs(y)
    attack = np.exp(-1.0 / (sr * (attack_ms / 1000.0)))
    release = np.exp(-1.0 / (sr * (release_ms / 1000.0)))

    env = np.zeros_like(x)
    e = 0.0
    for i, v in enumerate(x):
        if v > e:
            e = attack * e + (1 - attack) * v
        else:
            e = release * e + (1 - release) * v
        env[i] = e

    # normalize envelope to ~0..1
    m = np.max(env) + 1e-12
    env = env / m
    return env


def bitcrush(y, bits=10):
    levels = 2 ** bits
    return np.round(y * (levels / 2)) / (levels / 2)


def sample_hold(y, factor=3):
    """Sample-rate reduction via zero-order hold."""
    if factor <= 1:
        return y
    out = y.copy()
    out[::factor] = y[::factor]
    for i in range(1, factor):
        out[i::factor] = out[0::factor]
    return out


def make_carrier(sr, n, seed=0):
    """
    Carrier = band-limited noise + a few drifting tones.
    This gives that 'robot language' feel.
    """
    rng = np.random.default_rng(seed)
    t = np.arange(n) / sr

    # Noise component
    noise = rng.normal(0, 1, n).astype(np.float32)
    noise = butter_band(noise, sr, hp=180, lp=3200)

    # Beep/tones component: a few sines with slow frequency drift
    tones = np.zeros(n, dtype=np.float32)
    base_freqs = [220, 330, 440, 660]  # tweak freely
    for f in base_freqs:
        drift = 1.0 + 0.03 * np.sin(2 * np.pi * (0.25 + rng.random()*0.35) * t + rng.random()*6.28)
        phase = 2 * np.pi * (f * drift) * t
        tones += np.sin(phase).astype(np.float32)

    tones /= (len(base_freqs) + 1e-12)
    carrier = 0.65 * noise + 0.35 * tones
    return carrier


def add_glitch_gates(y, sr, rate_hz=14.0, depth=0.35, seed=1):
    """
    Adds mild stutter/gate to feel 'digital' without losing cadence.
    """
    rng = np.random.default_rng(seed)
    n = len(y)
    t = np.arange(n) / sr

    # random-ish LFO gating signal
    lfo = 0.5 * (1.0 + np.sign(np.sin(2 * np.pi * rate_hz * t + rng.random()*6.28)))
    lfo = signal.savgol_filter(lfo, 501 if n > 501 else max(5, n//10 | 1), 2)
    g = (1.0 - depth) + depth * lfo
    return y * g.astype(np.float32)


def normalize(y, headroom_db=1.0):
    peak = np.max(np.abs(y)) + 1e-12
    target = 10 ** (-headroom_db / 20.0)
    return y * (target / peak)


def stray_gibberish(
    y, sr,
    attack_ms=4.0,
    release_ms=80.0,
    carrier_seed=0,
    hp=250.0,
    lp=4200.0,
    bits=10,
    hold=3,
    glitch_rate=14.0,
    glitch_depth=0.30
):
    # Envelope from your speech (keeps timing/emotion)
    env = envelope_follower(y, sr, attack_ms=attack_ms, release_ms=release_ms)

    # Build a synthetic carrier (gibberish source)
    carrier = make_carrier(sr, len(y), seed=carrier_seed)

    # Apply envelope -> "talking robot" but unintelligible
    out = carrier * (env ** 1.15)

    # Add mild digital gating/glitch
    out = add_glitch_gates(out, sr, rate_hz=glitch_rate, depth=glitch_depth, seed=carrier_seed + 10)

    # Small speaker band-limit + digital texture
    out = butter_band(out, sr, hp=hp, lp=lp)
    out = bitcrush(out, bits=bits)
    out = sample_hold(out, factor=hold)

    return normalize(out, headroom_db=1.0)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("input_wav")
    ap.add_argument("output_wav")
    ap.add_argument("--attack", type=float, default=4.0)
    ap.add_argument("--release", type=float, default=80.0)
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--hp", type=float, default=250.0)
    ap.add_argument("--lp", type=float, default=4200.0)
    ap.add_argument("--bits", type=int, default=10)
    ap.add_argument("--hold", type=int, default=3)
    ap.add_argument("--glitch_rate", type=float, default=14.0)
    ap.add_argument("--glitch_depth", type=float, default=0.30)
    args = ap.parse_args()

    x, sr = sf.read(args.input_wav, always_2d=True)
    y = to_mono(x)

    out = stray_gibberish(
        y, sr,
        attack_ms=args.attack,
        release_ms=args.release,
        carrier_seed=args.seed,
        hp=args.hp,
        lp=args.lp,
        bits=args.bits,
        hold=args.hold,
        glitch_rate=args.glitch_rate,
        glitch_depth=args.glitch_depth
    )

    sf.write(args.output_wav, out, sr)
    print(f"Wrote: {args.output_wav}")


if __name__ == "__main__":
    main()
