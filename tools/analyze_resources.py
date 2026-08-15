from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8", errors="replace")

print("STM32 target: STM32F103C8, Cortex-M3, IRAM 0x5000 (20,480 bytes), IROM 0x10000 (65,536 bytes)")
print()

# PoseStep has uint8_t angle[4] followed by uint32_t duration_ms; ARM ABI aligns the uint32_t at offset 4.
step_counts = {}
action = read("User/ActionTask.c")
for name, count in re.findall(r"static const PoseStep (g_[A-Za-z0-9_]+)_steps\[\] =\s*\{(.*?)\n\};", action, re.S):
    step_counts[name] = len(re.findall(r"\{\s*\{", count))
print("PoseStep scripts:")
for name, count in step_counts.items():
    print(f"  {name}: {count} steps, estimated {count * 8} bytes")
print(f"  total: {sum(step_counts.values())} steps, estimated {sum(step_counts.values()) * 8} bytes read-only")
print()

items = [
    ("OLED_DisplayBuf", 8 * 128, "Hardware/OLED.c"),
    ("AudioTask frame", 50, "User/AudioTask.c"),
    ("USART1 ByteRing", 16 + 3, "System/usart1.c/System/ByteRing.h"),
    ("USART3 ByteRing", 16 + 3, "System/usart3.c/System/ByteRing.h"),
    ("Timebase counter", 4, "System/Timebase.c"),
    ("Safety flags", 2, "User/SafetyTask.c"),
    ("Serial_Printf local buffer", 100, "Hardware/Serial.c"),
    ("OLED_Printf local buffer", 30, "Hardware/OLED.c"),
    ("SYN6288 local frame", 50, "Hardware/syn6288.c"),
]
print("Known memory objects:")
for name, size, source in items:
    print(f"  {name}: {size} bytes ({source})")
print(f"  known static/task-local subtotal: {sum(size for _, size, _ in items)} bytes")
print()

app = read("User/AppTask.c")
periods = re.findall(r"\{\s*AppTask_[A-Za-z0-9_]+,\s*([0-9]+)U", app)
print("AppTask periods (ms):", ", ".join(periods))
print("AppTask slots:", len(periods), "; estimated 12 bytes/slot on 32-bit ARM =", len(periods) * 12, "bytes")
print()

# Blocking / potentially high-cost operations by source occurrence.
patterns = {
    "Delay_ms/us/s calls": r"\bDelay_(?:ms|us|s)\s*\(",
    "busy-wait while loops": r"\bwhile\s*\(",
    "blocking USART sends": r"USART[123]_SendString|Serial_SendByte1",
    "OLED full refreshes": r"OLED_Update\s*\(",
    "float operations": r"\bfloat\b|\bdouble\b",
}
for label, pattern in patterns.items():
    hits = []
    for rel in ["User", "System", "Hardware"]:
        for path in (ROOT / rel).rglob("*.c"):
            text = path.read_text(encoding="utf-8", errors="replace")
            hits.extend((str(path.relative_to(ROOT)), i + 1) for i, line in enumerate(text.splitlines()) if re.search(pattern, line))
    print(f"{label}: {len(hits)} occurrences")
    for path, line in hits[:20]:
        print(f"  {path}:{line}")
    if len(hits) > 20:
        print("  ...")
