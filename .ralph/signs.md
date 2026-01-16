# Signs (Lessons Learned)

## Parallel Agent Build Conflicts

**Issue:** Build fails with `Rename failed: ... No such file or directory`

**Root Cause:** Multiple Ralph agents or background processes running concurrent builds that delete/recreate the output/objs directory.

**Workaround:**
1. Kill all competing processes: `pkill -9 -f "make.*wordproc"; pkill -9 -f "clang.*wordproc"`
2. Wait a few seconds before building
3. Alternatively, build to /tmp: `mkdir -p /tmp/wp_build && clang++ ... -o /tmp/wp_build/...`

**Signs to watch for:**
- `Rename failed: ... No such file or directory`
- `error: unable to open output file ... No such file or directory`
- output/objs directory disappearing during compilation
