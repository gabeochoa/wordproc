#!/usr/bin/env python3
"""Gate changed .e2e files: no wall-clock waits, no pixel clicks.
Past: 881712a wait->wait_frames (suite-wide), 1a9e144 click coords->click_text, e0f4296/5b989c7 state leaks (reset is in e2e_runner, not scripts)."""
import re,subprocess,sys
files=sys.argv[1:]
if not files: files=[f for f in subprocess.run(["git","diff","--name-only","HEAD"],capture_output=True,text=True).stdout.splitlines() if f.endswith(".e2e")]
bad=[]
for f in files:
 for i,l in enumerate(open(f,errors="ignore"),1):
  if re.match(r"\s*wait\s+\d",l): bad.append(f"{f}:{i}: '{l.strip()}' use wait_frames N")
  if re.match(r"\s*click\s+\d+\s+\d+",l): bad.append(f"{f}:{i}: '{l.strip()}' use click_text/click_ui, not pixels")
print("\n".join(bad) if bad else "check_e2e: ok"); sys.exit(1 if bad else 0)
