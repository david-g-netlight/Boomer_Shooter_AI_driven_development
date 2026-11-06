#!/usr/bin/env python3
"""
Generates placeholder audio into assets/audio/ (16-bit mono WAV, 22050 Hz).
Deterministic: the same script always writes the same files.

These stand in for the game jam's sounds and music until real assets are dropped in
with the same file names (see assets/audio/CREDITS.txt).

Usage:  python3 tools/generate_placeholder_audio.py
"""
import math
import random
import struct
import wave
from pathlib import Path

SAMPLE_RATE = 22050
OUT_DIR = Path(__file__).resolve().parent.parent / "assets" / "audio"


def write_wav(name, samples):
    """samples: floats in [-1, 1]"""
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    path = OUT_DIR / name
    frames = b"".join(struct.pack("<h", int(max(-1.0, min(1.0, s)) * 32767)) for s in samples)
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)
        wav.writeframes(frames)
    print(f"  {path.relative_to(OUT_DIR.parent.parent)}  ({len(samples) / SAMPLE_RATE:.2f} s)")


def envelope(i, count, attack=0.01, decay=6.0):
    """Short linear attack, exponential decay."""
    t = i / SAMPLE_RATE
    attack_gain = min(1.0, t / attack) if attack > 0 else 1.0
    return attack_gain * math.exp(-decay * t / (count / SAMPLE_RATE))


def chirp(start_hz, end_hz, seconds, volume=0.6):
    """Frequency sweep with a 2nd harmonic: the 'stretch' / 'shrink' scaling sound."""
    count = int(seconds * SAMPLE_RATE)
    phase = 0.0
    out = []
    for i in range(count):
        k = i / count
        freq = start_hz * (end_hz / start_hz) ** k          # exponential sweep
        phase += 2.0 * math.pi * freq / SAMPLE_RATE
        tone = math.sin(phase) + 0.35 * math.sin(2.0 * phase) + 0.15 * math.sin(3.0 * phase)
        out.append(volume * 0.66 * tone * envelope(i, count, decay=3.0))
    return out


def main():
    random.seed(1234)
    print("Generating placeholder audio:")

    # Scaling sounds (Unity: ScalingAudioFX). Played with pitch = scale factor +- 0.02
    write_wav("sfx_stretch.wav", chirp(320.0, 960.0, 0.35))
    write_wav("sfx_shrink.wav", chirp(960.0, 280.0, 0.35))


if __name__ == "__main__":
    main()
