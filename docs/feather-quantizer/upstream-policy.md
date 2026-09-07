# Upstream policy

## Upstream

- Repository: `https://github.com/sekigon-gonnoc/vial-qmk`
- Base branch: `bmp-vial-1.0.6`
- Reference target: `keyboards/sekigon/keyboard_quantizer/mini`

The project should be created as a GitHub fork so Git ancestry and upstream comparison remain available.

## Remote layout

Recommended local remotes:

```text
origin   -> your GitHub fork
upstream -> sekigon-gonnoc/vial-qmk
```

Verify with:

```bash
git remote -v
```

## Branch model

- default integration branch: `feather-main`
- task branches: `task/<number>-<short-name>`
- never develop directly on `bmp-vial-1.0.6`

The upstream branch should remain a clean reference.

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
3. build the original KQM target before and after;
4. build the Feather target;
5. review dependency/submodule changes explicitly.
