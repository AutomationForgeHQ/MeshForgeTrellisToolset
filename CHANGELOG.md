# MeshForgeTrellisToolset

Every released version of MeshForgeTrellisToolset, newest first. A release publishes **one** section of this
file — the one whose heading matches its tag — as its release notes; for an `open` plugin those
notes are posted to Discord `#releases` automatically. Write for someone who installs the plugin,
not for the commit log.

Headings are `## <x.y.z> — <date>`. Use `Added` / `Changed` / `Fixed` / `Compatibility` /
`Known issues`, only the ones that apply.

## 0.0.3 — 2026-09-21

### Changed
- The version this toolset reports to an agent is now read from the plugin's own
  descriptor rather than repeated in C++, so it can no longer answer a number the
  installed package does not carry.

### Compatibility
- Rebuilt against MeshForge 0.3.0 and MeshForgeTrellis 0.0.3, which it needs.

## 0.0.2 — 2026-09-08
- Packaging fix: the release now carries everything the register allows. `BuildPlugin`'s filter excludes `Config/` and every `public_extra` path, so earlier zips shipped without them.
- `GetToolsetVersion()` answers this plugin's real version; it had drifted from the descriptor.

## 0.0.1 — 2026-09-08
- First release, cut the day the plugin went `open` (no code change from the prior unreleased
  state — the tag simply marks the first version that could be mirrored).
- Docs/Support URLs pointed at the new GitHub mirror.
- MeshForge, and the model that only speaks in pictures.
- Do not advise somebody whose runner is working.
