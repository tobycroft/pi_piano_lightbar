"""Test script: send MIDI Note On/Off to Pico LED controller."""
import time
import rtmidi

out = rtmidi.MidiOut()
ports = out.get_ports()
target = None
for i, p in enumerate(ports):
    if "Piano" in p or "88" in p:
        target = i
        print(f"Found: [{i}] {p}")
        break

if target is None:
    print("Pico MIDI device not found!")
    exit(1)

out.open_port(target)

def note_on(note, velocity=127, channel=0):
    msg = [0x90 | (channel & 0x0F), note, velocity]
    out.send_message(msg)
    print(f"Note ON:  ch={channel}, note={note}, vel={velocity}")

def note_off(note, channel=0):
    msg = [0x80 | (channel & 0x0F), note, 0]
    out.send_message(msg)
    print(f"Note OFF: ch={channel}, note={note}")

print("\n=== Testing MIDI Note On/Off ===")
print("LED mapping: note 21=A0 -> LED[0], note 108=C8 -> LED[87]")
print()

# Test a few notes across the range
test_notes = [21, 36, 48, 60, 72, 84, 96, 108]

for note in test_notes:
    idx = note - 21
    print(f"\n--- Note {note} (LED[{idx}]) ---")
    note_on(note)
    time.sleep(1.0)
    note_off(note)
    time.sleep(0.3)

print("\n=== Test complete! ===")
out.close_port()