# AGENTS.md — Feather Keyboard Quantizer

## Mission

Develop a Feather RP2040 USB Host implementation of Keyboard Quantizer functionality while keeping the `sekigon-gonnoc/vial-qmk` upstream code as unchanged as possible.

The primary target is ELECOM HUGE PLUS; ELECOM DEFT is the initial test device.

## Read before working

Before changing anything, read:

1. `FEATHER_QUANTIZER.md`
2. `docs/feather-quantizer/spec.md`
3. `docs/feather-quantizer/upstream-policy.md`
4. `docs/feather-quantizer/guardrails.md`
5. the task-specific prompt or issue

Then inspect `git status --short` and the existing diff.

## Source-of-truth rule

Existing KQM Mini code under `keyboards/sekigon/**` is upstream reference code. Treat it as read-only unless the human explicitly authorizes a specific upstream-path edit in the current task.

Do not update QMK, Vial, TinyUSB, Pico-PIO-USB, pico-sdk, ChibiOS, submodule revisions, or toolchain versions as part of an implementation task unless the task explicitly says to do so.

## Default writable scope

For firmware implementation tasks, changes are limited to:

- `keyboards/feather_quantizer/**`
- `docs/feather-quantizer/**`
- task-specific files under `prompts/**` only when explicitly requested

Do not change repository control files such as `AGENTS.md`, `.github/workflows/**`, `.gitmodules`, or upstream source paths unless explicitly requested by the human.

## Minimal-diff rules

- Do not perform unrelated formatting or cleanup.
- Do not replace a whole upstream file when a small local implementation or wrapper is sufficient.
- Prefer a new Feather-specific file/target over modifying shared upstream behavior.
- Preserve upstream copyright and SPDX headers in any code derived from upstream.
- Do not rename or reorganize upstream directories.
- Do not introduce a new dependency when existing repository facilities can implement the task.
- Keep one task to one coherent concern.

## Task boundaries

If a task says read-only analysis, do not modify firmware source. A designated report file may be created if the task explicitly names it.

Do not flash hardware, issue destructive Git commands, force-push, rewrite history, merge branches, or publish releases unless the human explicitly asks for that action.

## Verification

Before completing a code task:

1. run the smallest relevant build/checks;
2. run `git diff --check`;
3. inspect `git status --short`;
4. inspect `git diff --stat` and `git diff --name-only`;
5. confirm every changed file is within the authorized task scope.

If hardware verification is required but hardware is unavailable, state exactly what remains unverified. Never claim real-device behavior from compilation alone.

## Reporting

At the end of a task, report:

- files changed
- why each file changed
- commands/tests run and their outcomes
- assumptions and unresolved risks
- whether real hardware was tested
- the smallest recommended next task

For analysis tasks, cite repository file paths, functions, macros, and relevant line ranges or symbols so the conclusions can be reviewed by a human.
