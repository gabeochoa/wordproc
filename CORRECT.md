# CORRECT — wordproc
Note: pre-existing uncommitted work in this tree was left untouched; all commits below add only new files.
(A) Brittle E2E — 881712a wait→wait_frames suite-wide, 1a9e144 pixel→click_text, e0f4296+5b989c7 inter-test state leak. Level: lint (runner already supports the right commands; copying old scripts is the failure). Fix 1ff5260 `scripts/check_e2e.py` (changed files). Proof: fails `1a9e144^:pass_file_menu_dropdown.e2e` (wait 5/click 28 34) + legacy tables `wait 2`, passes wait_frames files. Legacy tables/insert scripts still red if touched — convert, don't exempt.
(B) Direct raylib reintroduction — 6d38b79, 0981fdf, c09e4dc. Level: lint naming architecture. Fix a547ef9 `scripts/check_no_raylib.sh`. Proof: passes `src/`, fails `0981fdf^:src/main.cpp`.
(C) Duplicated ops/per-frame work — 2c2d203 unified DocumentCommand layer, 5617e24 god-object split, b794666 cached lookups, 901d296 shared ActionMap, 2327f32 border helper. Level: architecture (already built). No new guard: rule below is the fix; new ops go through the command layer/singleton, never a second copy in a system.
| Rule | Do | Never | Gate |
|---|---|---|---|
| E2E sync | `wait_frames N`, `click_text`/`click_ui` | `wait <sec>`, `click <x> <y>` | check_e2e.py |
| E2E isolation | rely on runner reset (e2e_runner) | assume prior test state | batch run |
| Backend | `afterhours::graphics`, `src/rl.h` shim | `raylib::` in src/ | check_no_raylib.sh |
| Doc ops | DocumentCommand layer, shared ActionMap/InputComponent | duplicate op / per-frame singleton hunt | review vs 2c2d203 |
