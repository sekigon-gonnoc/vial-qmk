# Upstream policy

## Upstream

- Repository: `https://github.com/sekigon-gonnoc/vial-qmk`
- Reference branch at bootstrap time: `dev/ble-micro-pro`
- **Pinned upstream baseline commit:** `4de02c1aeec5a34f0744a97213499441c508d565`
- Reference target: `keyboards/sekigon/keyboard_quantizer/mini`

The pinned commit SHA is the authoritative baseline for this project. The upstream branch name is recorded for provenance, but it is mutable and must not be followed automatically during feature work.

The initial Feather bootstrap commit is a direct child of the pinned upstream baseline, so the project delta remains reviewable from that exact commit.

## Remote layout

Recommended local remotes:

```text
origin   -> Kota-Ueda/feather-keyboard-quantizer
upstream -> sekigon-gonnoc/vial-qmk
```

Verify with:

```bash
git remote -v
git show --no-patch --oneline 4de02c1aeec5a34f0744a97213499441c508d565
```

## Branch model

- integration branch: `feather-main`
- task branches: `task/<number>-<short-name>`
- upstream reference: pinned commit `4de02c1aeec5a34f0744a97213499441c508d565`

Do not develop directly on an upstream reference branch. Do not merge or rebase a newer upstream branch into a feature/task branch.

## Protected upstream areas

Unless a human explicitly approves a named file for a named task, do not modify:

```text
keyboards/sekigon/**
lib/**
platforms/**
quantum/**
tmk_core/**
.gitmodules
```

Do not update submodule pointers as incidental work.

## Why a separate target

Keeping the Feather implementation under `keyboards/feather_quantizer/**` makes the delta reviewable and gives CI a simple allowlist. It also prevents a Feather-specific board assumption from silently changing behavior for real Keyboard Quantizer Mini hardware.

## Upstream sync

Upstream synchronization is a separate maintenance task. Do not merge/rebase new upstream work during a feature task.

When an upstream sync is intentionally performed:

1. record the previous and new upstream commit IDs;
2. perform the sync on a dedicated branch;
3. review all upstream changes between those commits before adopting them;
4. build the original KQM target before and after;
5. build the Feather target;
6. review dependency/submodule changes explicitly;
7. update the pinned baseline in this document only after human approval.
