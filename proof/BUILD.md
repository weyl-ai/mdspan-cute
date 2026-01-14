# Building the Lean Proofs

For a comprehensive explanation of how the proofs work, how they connect to C++,
and the theorem reference, see **[PROOFS.md](PROOFS.md)**.

## Quick Start

```bash
cd proof

# 1. Get Mathlib cache (REQUIRED - don't build from source!)
lake exe cache get

# 2. Build
lake build
```

## If cache fails

The Mathlib cache must match your Lean version. If `lake exe cache get` fails:

```bash
# Update lake packages
lake update

# Try cache again
lake exe cache get

# Build
lake build
```

## Lean4web

If using lean4web.com, make sure it's using a Mathlib-enabled instance.
The standard lean4web may not have Mathlib pre-cached.

## Without Mathlib

If you just want to verify the core theorems without Mathlib dependencies,
use the standalone Core.lean which only uses built-in tactics:

```bash
lake env lean Razorgirl/Core.lean
```

## Troubleshooting

**"object file does not exist"**: Run `lake exe cache get` first

**Cache download fails**: Check your Lean version matches mathlib version

**Out of memory**: Mathlib is large. Need ~8GB RAM to build.
