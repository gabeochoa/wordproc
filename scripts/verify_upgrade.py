import argparse
import json
import re
import shutil
import subprocess
import tempfile
import time
import xml.etree.ElementTree as ET
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--native", action="store_true")
parser.add_argument("--all", action="store_true")
parser.add_argument("--output", default="output/upgrade/verification")
parser.add_argument("--match", default="")
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
output = (root / args.output).resolve()
output.mkdir(parents=True, exist_ok=True)
selected = []
for path in sorted((root / "tests/e2e_scripts").rglob("*.e2e")):
    if path.name.startswith("fail_"):
        continue
    relevant = any(word in path.name for word in (
        "context_menu", "text_input", "file_dialog", "mouse", "double_click",
        "menu_over_toolbar", "toolbar_formatting", "copy_paste", "cut_paste",
        "shared_clipboard", "platform_services"))
    if (args.all or relevant) and args.match in str(path):
        selected.append(path)
if not selected:
    raise SystemExit("No matching scripts")
with tempfile.TemporaryDirectory(prefix="wordproc-upgrade-") as temporary:
    work = Path(temporary)
    (work / "resources").symlink_to(root / "resources", target_is_directory=True)
    shutil.copytree(root / "test_files", work / "test_files")
    (work / "output").mkdir()
    scripts = work / "scripts"
    scripts.mkdir()
    for path in selected:
        shutil.copy2(path, scripts / path.name)
    timeout = 900 if args.all else 240
    command = [str(root / "output/wordproc.exe"), "--test-mode",
               f"--test-script-dir={scripts}", f"--screenshot-dir={output}/screenshots",
               f"--e2e-timeout={timeout}"]
    if not args.native:
        command.append("--headless")
    started = time.monotonic()
    with (output / "e2e.log").open("w") as log:
        timed_out = False
        try:
            result = subprocess.run(command, cwd=work, stdout=log,
                                    stderr=subprocess.STDOUT, timeout=timeout + 30)
            exit_code = result.returncode
        except subprocess.TimeoutExpired:
            timed_out = True
            exit_code = 124
    log = (output / "e2e.log").read_text()
    log = re.sub(r"\x1b\[[0-9;]*m", "", log)
    passed = re.findall(r"\[PASS\] (\S+)", log)
    failed = re.findall(r"\[FAIL\] ([^\n]+)", log)
    geometry = {}
    for name, xml in re.findall(r"\[E2E\] dump_ui '(context_menu_\w+)':\n((?:[ \t]*<[^\n]*\n)+)", log):
        nodes = [n for n in ET.fromstring("<root>" + xml + "</root>").iter("ui")
                 if n.attrib.get("text") in {"Cut", "Copy", "Paste", "Select All"}]
        geometry[name] = len(nodes) == 4 and all(
            int(n.attrib["x"]) >= 0 and int(n.attrib["y"]) >= 0
            and int(n.attrib["x"]) + int(n.attrib["w"]) <= 800
            and int(n.attrib["y"]) + int(n.attrib["h"]) <= 600 for n in nodes)
    geometry_expected = any(p.stem == "pass_context_menu_edges" for p in selected)
    geometry_ok = not geometry_expected or (len(geometry) == 4 and all(geometry.values()))
    report = {"native": args.native, "seconds": time.monotonic() - started,
              "exit_code": exit_code, "timed_out": timed_out, "geometry": geometry, "expected": len(selected),
              "passed": passed, "failed": failed,
              "screenshots": len(list((output / "screenshots").glob("*.png")))}
    report["success"] = exit_code == 0 and len(passed) == len(selected) and not failed and geometry_ok
    (output / "verification.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    raise SystemExit(0 if report["success"] else 1)
