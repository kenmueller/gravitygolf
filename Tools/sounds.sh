#!/bin/sh
# Measures every sound the game makes. Nobody building this game can hear it, so this is how its sounds are judged:
# the game writes each one to a WAV file exactly as it would play it (console command GGSounds), and this prints
# its length, level, pitch and brightness, and flags anything clipped, too quiet or too loud.
#   Tools/sounds.sh [folder]      default: Saved/Sounds      (needs the editor open)
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-$ROOT/Saved/Sounds}"
LOG="$HOME/Library/Logs/Unreal Engine/GravityGolfEditor/GravityGolf.log"
mkdir -p "$OUT"
rm -f "$OUT"/*.wav
"$ROOT/Tools/play.sh" stop
"$ROOT/Tools/play.sh" 5 >/dev/null
MARK=$(($(wc -l <"$LOG") + 1))
"$ROOT/Tools/console.sh" "GGSounds $OUT"
sleep 2
"$ROOT/Tools/play.sh" stop
tail -n +"$MARK" "$LOG" | grep "LogTemp: Display: Sound " | sed 's/.*Display: Sound //' > "$OUT/peaks.txt"
python3 - "$OUT" <<'PY'
import cmath, glob, math, os, struct, sys, wave

folder = sys.argv[1]
peaks = {}
for line in open(os.path.join(folder, "peaks.txt")):
    name, rest = line.split(":", 1)
    peaks[name.strip()] = float(rest.split("peak before clipping")[1].split(",")[0])


def fft(values):
    n = len(values)
    if n == 1:
        return values
    even, odd = fft(values[0::2]), fft(values[1::2])
    out = [0j] * n
    for k in range(n // 2):
        twist = cmath.exp(-2j * math.pi * k / n) * odd[k]
        out[k], out[k + n // 2] = even[k] + twist, even[k] - twist
    return out


def db(value):
    return 20 * math.log10(max(value, 1e-6))


rows = []
for path in sorted(glob.glob(os.path.join(folder, "*.wav"))):
    with wave.open(path) as w:
        rate = w.getframerate()
        samples = [v / 32768.0 for v in struct.unpack("<%dh" % w.getnframes(), w.readframes(w.getnframes()))]
    name = os.path.basename(path)[:-4]
    peak = max(abs(v) for v in samples)
    window = int(rate * 0.1)
    loudest = max(math.sqrt(sum(v * v for v in samples[i:i + window]) / window) for i in range(0, max(1, len(samples) - window), window // 4))
    # How long until it has died away to a hundredth of its peak.
    last = max((i for i, v in enumerate(samples) if abs(v) > peak * 0.01), default=0)
    size = 16384
    start = max(range(0, max(1, len(samples) - size), 512), key=lambda i: sum(abs(v) for v in samples[i:i + size:16]), default=0)
    chunk = (samples[start:start + size] + [0.0] * size)[:size]
    spectrum = [abs(v) for v in fft([complex(v * (0.5 - 0.5 * math.cos(2 * math.pi * i / size))) for i, v in enumerate(chunk)])[:size // 2]]
    total = sum(spectrum) or 1.0
    centroid = sum(i * v for i, v in enumerate(spectrum)) * rate / size / total
    pitch = max(range(2, len(spectrum)), key=lambda i: spectrum[i]) * rate / size
    rows.append((name, last / rate, db(peak), db(loudest), pitch, centroid, peaks.get(name, 0.0)))

# The beds of air (bed_*) are meant to sit well under everything else; they are judged against their own target.
levels = sorted(r[3] for r in rows if not r[0].startswith("bed_"))
middle = levels[len(levels) // 2]
print(f"{'sound':16} {'dies away':>9} {'peak':>8} {'loudness':>9} {'pitch':>8} {'brightness':>10}   notes")
for name, length, peak, loud, pitch, centroid, raw in rows:
    notes = []
    if raw > 1.0:
        notes.append(f"CLIPS: its parts add up to {raw:.2f} of full scale before it is played")
    if name.startswith("bed_"):
        if not middle - 22 <= loud <= middle - 10:
            notes.append("a bed of air should sit 10 to 22 dB under the other sounds")
    elif loud < middle - 12:
        notes.append("much quieter than the rest")
    elif loud > middle + 9:
        notes.append("much louder than the rest")
    print(f"{name:16} {length:7.2f} s {peak:6.1f} dB {loud:7.1f} dB {pitch:6.0f} Hz {centroid:8.0f} Hz   {'; '.join(notes)}")
print(f"(loudness is the loudest tenth of a second; the middle sound is {middle:.1f} dB below full scale)")
PY
