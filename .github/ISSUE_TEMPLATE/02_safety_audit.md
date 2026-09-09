---
name: Safety Audit Finding
about: Code paths with missing safety guards, potential crash vectors, or resource issues
title: "[SAFETY] "
labels: ["safety", "audit"]
assignees: []
---

**Affected file(s)**
```
src/path/to/file.cpp
```

**Issue type**
- [ ] Missing error handling — operation can fail without recovery
- [ ] Unsafe signal-slot chain — could trigger cascading failure
- [ ] Resource leak — VRAM, FD, thread, memory
- [ ] Thread safety — data race or missing synchronization
- [ ] Assumption about plasmashell lifecycle
- [ ] Other

**Description**
What's the code doing and why is it unsafe?

**Code reference**
```
paste relevant code block
```

**Impact assessment**
What's the worst case? Crash? Memory corruption? Silent data loss?

**Suggested fix direction**
High-level approach (defer detailed patch until Phase 1).
