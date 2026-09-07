# Bootstrap instructions

This repository is a fork of `sekigon-gonnoc/vial-qmk` and keeps the fork relationship intact.

## Current authoritative baseline

The Feather project is pinned to upstream commit:

```text
4de02c1aeec5a34f0744a97213499441c508d565
```

This commit was the tip of upstream `dev/ble-micro-pro` when the project was bootstrapped. The commit SHA, not the mutable branch name, is authoritative.

The first Feather bootstrap commit is a direct child of that pinned upstream commit.

## Local setup

```bash
git clone --recurse-submodules https://github.com/Kota-Ueda/feather-keyboard-quantizer.git
cd feather-keyboard-quantizer

git remote add upstream https://github.com/sekigon-gonnoc/vial-qmk.git
git fetch upstream --prune

git switch feather-main
git submodule update --init --recursive
```

Verify the baseline exists locally:

```bash
git show --no-patch --oneline 4de02c1aeec5a34f0744a97213499441c508d565
git log --oneline --decorate -n 3
git status --short
git remote -v
```

Do not merge or rebase a newer upstream branch during normal feature tasks.

## Repository policy

Project-specific additions live under:

```text
AGENTS.md
FEATHER_QUANTIZER.md
docs/feather-quantizer/**
prompts/**
scripts/feather-quantizer/**
keyboards/feather_quantizer/**
.github/workflows/feather-*.yml
.github/PULL_REQUEST_TEMPLATE/feather-quantizer.md
```

Existing `keyboards/sekigon/**` code is upstream reference code and remains read-only unless a human explicitly authorizes a named upstream edit.

## GitHub repository settings

Use `feather-main` as the integration/default branch for Feather development.

Recommended description:

`RP2040 USB HID proxy for QMK/Vial remapping and MX-style trackball gestures on Adafruit Feather USB Host.`

Recommended required checks once they have run successfully on a PR:

- `Feather Quantizer - scope guardrails / changed-file-scope`
- `Feather Quantizer - firmware build / baseline-kqm`
- `Feather Quantizer - firmware build / feather-target` once the target exists

Disallow force pushes to `feather-main`.

The fork also inherits upstream workflows. They are not Feather project guardrails and may fail independently of this project's code. Do not make broad upstream CI changes merely to silence inherited failures.

## Run Task 001

Create the task branch from `feather-main`:

```bash
git switch feather-main
git pull --ff-only origin feather-main
git switch -c task/001-upstream-analysis
```

Give Codex the contents of:

`prompts/task-001-upstream-analysis.md`

Task 001 is analysis-only. It may only create or update:

`docs/feather-quantizer/research/task-001-upstream-analysis.md`
