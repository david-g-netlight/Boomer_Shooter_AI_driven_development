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


def bell(base_hz, seconds, volume=0.6):
    """Inharmonic partials with fast decay: a short 'ping' (the placeholder shot sound)."""
    count = int(seconds * SAMPLE_RATE)
    partials = [(1.0, 1.0), (2.76, 0.45), (5.4, 0.25), (8.93, 0.12)]   # (ratio, gain)
    out = []
    for i in range(count):
        t = i / SAMPLE_RATE
        tone = sum(g * math.sin(2.0 * math.pi * base_hz * r * t) * math.exp(-t * (4.0 + 3.0 * r)) for r, g in partials)
        out.append(volume * 0.55 * tone * min(1.0, t / 0.003))
    return out


NOTE_HZ = {  # equal temperament, A4 = 440
    "A2": 110.00, "C3": 130.81, "D3": 146.83, "E3": 164.81, "F3": 174.61, "G3": 196.00,
    "A3": 220.00, "B3": 246.94, "C4": 261.63, "D4": 293.66, "E4": 329.63, "F4": 349.23,
    "G4": 392.00, "A4": 440.00, "B4": 493.88, "C5": 523.25, "E5": 659.25,
}


def render_loop(chords, beats_per_chord, bpm, arpeggio, bass_every, seconds):
    """
    A seamless loop: each chord is held as a soft pad, an arpeggio plucks its notes on every beat
    subdivision and a bass note hits every `bass_every` beats. Length is a whole number of bars.
    """
    count = int(seconds * SAMPLE_RATE)
    beat = 60.0 / bpm
    out = [0.0] * count

    def add_note(start, length, freq, gain, decay, harmonics):
        first = int(start * SAMPLE_RATE)
        last = min(count, first + int(length * SAMPLE_RATE))
        for i in range(first, last):
            t = (i - first) / SAMPLE_RATE
            env = min(1.0, t / 0.005) * math.exp(-decay * t)
            out[i] += gain * env * sum(h_gain * math.sin(2.0 * math.pi * freq * h * t) for h, h_gain in harmonics)

    time = 0.0
    while time < seconds - 1e-6:
        for chord in chords:
            chord_length = beats_per_chord * beat
            # Pad: slow attack, held for the whole chord
            for name in chord:
                first = int(time * SAMPLE_RATE)
                last = min(count, first + int(chord_length * SAMPLE_RATE))
                freq = NOTE_HZ[name] / 2.0
                for i in range(first, last):
                    t = (i - first) / SAMPLE_RATE
                    env = min(1.0, t / 0.4) * min(1.0, (last - i) / (0.3 * SAMPLE_RATE))
                    out[i] += 0.05 * env * math.sin(2.0 * math.pi * freq * t)
            # Arpeggio
            steps = int(beats_per_chord * arpeggio)
            for k in range(steps):
                name = chord[k % len(chord)]
                add_note(time + k * beat / arpeggio, beat, NOTE_HZ[name], 0.12, 5.0, [(1, 1.0), (2, 0.3), (3, 0.1)])
            # Bass
            for k in range(0, beats_per_chord, bass_every):
                add_note(time + k * beat, beat * bass_every, NOTE_HZ[chord[0]] / 4.0, 0.22, 2.5, [(1, 1.0), (2, 0.5)])
            time += chord_length
            if time >= seconds - 1e-6:
                break

    peak = max(abs(v) for v in out) or 1.0
    return [0.6 * v / peak for v in out]


def main():
    random.seed(1234)
    print("Generating placeholder audio:")

    # Placeholder shot sound (SoundFXBank::PlayShot)
    write_wav("sfx_ping.wav", bell(1320.0, 0.6))

    # Themes (Unity ThemeMusicManager: MENU = 0, GAME_PLAY = 1). 16 s loops = whole bars
    write_wav("music_menu.wav", render_loop(
        chords=[["A3", "C4", "E4"], ["F3", "A3", "C4"], ["C4", "E4", "G4"], ["G3", "B3", "D4"]],
        beats_per_chord=4, bpm=60, arpeggio=2, bass_every=2, seconds=16.0))
    write_wav("music_game.wav", render_loop(
        chords=[["A3", "C4", "E4"], ["D4", "F4", "A4"], ["E4", "G4", "B4"], ["A3", "C4", "E4"]],
        beats_per_chord=8, bpm=120, arpeggio=2, bass_every=1, seconds=16.0))


if __name__ == "__main__":
    main()
