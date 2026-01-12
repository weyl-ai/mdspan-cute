import Lake
open Lake DSL

package «villa-straylight» where
  leanOptions := #[
    ⟨`pp.unicode.fun, true⟩,
    ⟨`autoImplicit, false⟩
  ]

-- Mathlib 4.15.0 matches lean4:v4.15.0
require mathlib from git
  "https://github.com/leanprover-community/mathlib4" @ "v4.15.0"

@[default_target]
lean_lib VillaStraylight where
  roots := #[`VillaStraylight]
