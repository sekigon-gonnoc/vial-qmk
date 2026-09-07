# Bootstrap instructions

Use this bundle **on top of a fork of `sekigon-gonnoc/vial-qmk`**. Do not create an unrelated empty Git repository for the firmware source.

## 1. Fork upstream on GitHub

Fork:

`sekigon-gonnoc/vial-qmk`

Recommended repository name after forking:

`feather-keyboard-quantizer`

Keep the fork relationship intact.

## 2. Clone your fork

```bash
git clone --recurse-submodules <YOUR_FORK_URL> feather-keyboard-quantizer
cd feather-keyboard-quantizer
```

Add the original repository as `upstream`:

```bash
git remote add upstream https://github.com/sekigon-gonnoc/vial-qmk.git
git fetch upstream --prune
```

## 3. Create the integration branch from the KQM branch

```bash
git switch -c feather-main upstream/bmp-vial-1.0.6
git submodule update --init --recursive
```

Confirm that there are no local changes:

```bash
git status --short
git remote -v
```

## 4. Copy this bootstrap bundle into the repository root

Copy all files from the bundle root into the cloned fork root.

These files are intentionally additive. Do not overwrite the upstream root `README.md`.

Commit the bootstrap only:

```bash
git add AGENTS.md FEATHER_QUANTIZER.md BOOTSTRAP.md docs/feather-quantizer prompts scripts/feather-quantizer .github/workflows/feather-guardrails.yml .github/workflows/feather-build.yml .github/PULL_REQUEST_TEMPLATE/feather-quantizer.md keyboards/feather_quantizer/.gitkeep
git commit -m "chore: bootstrap Feather Quantizer agent guardrails"
git push -u origin feather-main
```

## 5. GitHub repository settings

Set `feather-main` as the default branch.

Recommended repository description:

`RP2040 USB HID proxy for QMK/Vial remapping and MX-style trackball gestures on Adafruit Feather USB Host.`

If branch rules/rulesets are available on your plan, require the following checks before merge:

- `Feather Quantizer - scope guardrails / changed-file-scope`
- `Feather Quantizer - firmware build / baseline-kqm`
- `Feather Quantizer - firmware build / feather-target` once the target exists

Disallow force pushes to `feather-main`.

## 6. Run Task 001

Create a task branch:

```bash
git switch -c task/001-upstream-analysis
```

Give Codex the contents of:

`prompts/task-001-upstream-analysis.md`

Task 001 must leave firmware source untouched and may only update:

`docs/feather-quantizer/research/task-001-upstream-analysis.md`
