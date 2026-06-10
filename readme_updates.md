# 📘 Project Update Log
## Rejection Bin Interlocking System — Change Tracking

Use this file to track every important code change so the latest behavior is always documented.

---

## Entry Format
- **Date:** YYYY-MM-DD
- **Version/Tag:** optional
- **Type:** Fix / Improvement / Refactor / Docs / Test
- **Files Changed:** comma-separated list
- **Summary:** what changed
- **Why:** reason for change
- **Runtime Impact:** expected impact on stability/performance
- **Validation:** how it was checked

---

## Updates

### 2026-06-10 — Non-blocking serial command handling
- **Type:** Fix (High Priority Runtime)
- **Files Changed:** `serial_cmd.h`
- **Summary:** Replaced blocking serial command read (`readStringUntil('\n')`) with a non-blocking character-buffer parser.
- **Why:** Blocking reads can delay loop execution and reduce responsiveness during runtime.
- **Runtime Impact:** Better loop responsiveness and lower risk of delayed sensor/event handling when serial input is incomplete.
- **Validation:** No editor-reported compile/lint errors after change.

### 2026-06-10 — Added update tracking documentation
- **Type:** Docs
- **Files Changed:** `readme_updates.md`
- **Summary:** Added dedicated change-log README for tracking all future project updates.
- **Why:** Maintain clear history of latest changes and operational impact.
- **Runtime Impact:** Documentation only.
- **Validation:** File created successfully.

### 2026-06-10 — Web memory optimization (String pressure reduction)
- **Type:** Improvement (High Priority Runtime)
- **Files Changed:** `web_server.h`
- **Summary:** Reduced dynamic allocation pressure by reserving HTML `String` capacity in heavy handlers and replaced per-chunk `String` creation in `/downloadall` with direct byte streaming to client.
- **Why:** Repeated `String` growth and chunk conversions can fragment heap over long runtimes.
- **Runtime Impact:** Lower heap fragmentation risk and improved long-run stability of web endpoints.
- **Validation:** No editor-reported compile/lint errors after change.
