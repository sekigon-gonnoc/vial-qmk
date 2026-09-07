# AI implementation guardrails

These controls exist to keep Codex changes narrow, reviewable, and reversible.

## G0 — task scope is authoritative

Every task must identify whether it is analysis-only, documentation, board port, HID parser, gesture logic, or another single concern.

Do not expand the task because an adjacent improvement looks useful.

## G1 — upstream is read-only by default

`keyboards/sekigon/**` and shared framework/dependency paths are not implementation areas for this project.

If the Feather cannot be implemented without editing shared upstream code, stop the implementation at that boundary and document:

- exact file/function requiring change
- why local implementation is insufficient
- proposed smallest shared change
- compatibility risk to KQM Mini

Human approval is required before that shared edit.

## G2 — dependency freeze

No opportunistic updates to:

- QMK/Vial base
- TinyUSB
- Pico-PIO-USB
- pico-sdk
- ChibiOS
- Git submodules
- compiler/toolchain

A build failure is evidence to diagnose, not permission to upgrade dependencies.

## G3 — allowed implementation path

Normal firmware code changes are restricted to:

```text
keyboards/feather_quantizer/**
```

Documentation is restricted to:

```text
docs/feather-quantizer/**
```

## G4 — control-plane files are human-owned

Do not modify without explicit human instruction:

```text
AGENTS.md
.github/workflows/**
.gitmodules
```

Do not create `AGENTS.override.md` unless the human explicitly asks for it.

## G5 — no unrelated diff

Forbidden:

- repository-wide formatting
- line-ending normalization
- mass include sorting
- comment rewrites unrelated to the task
- generated-file churn unrelated to the task
- renaming upstream symbols solely for style

## G6 — diff budget

Aim for less than 500 changed non-documentation lines per task.

Hard review threshold: 1,500 changed non-documentation lines in a pull request. Split larger work unless the human explicitly approves the larger diff.

## G7 — test evidence

Compilation proves compilation only.

Do not claim:

- DEFT compatibility
- HUGE PLUS compatibility
- gesture feel/latency
- Vial persistence on hardware
- sleep/resume stability

until each item has been tested on actual hardware.

## G8 — no destructive operations

Without explicit human instruction, do not:

- `git reset --hard`
- force push
- rewrite history
- delete branches/tags
- flash a connected board
- create a release
- merge a PR

## G9 — completion gate

Before calling a task complete:

```bash
git diff --check
git status --short
git diff --stat
git diff --name-only
```

Run the smallest relevant build/check and report its exact outcome.
