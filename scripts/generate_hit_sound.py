"""Generate the owned 160 ms hit thump before running setup_damage_death.py."""
import math
import random
import struct
import wave

rng = random.Random(41)
rate = 44100
low = 0.0
frames = []
for i in range(int(rate * .16)):
    t = i / rate
    low = .78 * low + .22 * rng.uniform(-1, 1)
    value = (.45 * math.sin(2 * math.pi * (100 * t + 35 * t * t)) * math.exp(-30 * t)
             + .55 * low * math.exp(-25 * t) + .15 * rng.uniform(-1, 1) * math.exp(-110 * t))
    frames.append(struct.pack('<h', int(max(-1, min(1, value)) * 26000)))
with wave.open('/private/tmp/ER_Hit_Impact.wav', 'wb') as output:
    output.setparams((1, 2, rate, len(frames), 'NONE', 'not compressed'))
    output.writeframes(b''.join(frames))
