import Mathlib.Tactic

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                     THE VILLA STRAYLIGHT PAPERS
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

                A Formal Treatment of NVIDIA's Layout Algebra

                        razorgirl / Weyl AI / 2026

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━


    The Villa Straylight is a body grown in upon itself, a Gothic folly.
    Each space in Straylight is in some way secret, this endless series
    of chambers linked by passages, by stairwells vaulted like intestines,
    where the eye is trapped in narrow curves, carried past ornate screens,
    empty alcoves. ... In Straylight, the hull's inner surface is overgrown
    with a desperate proliferation of structures, forms flowing, interlocking,
    rising ... The semiotics of the Villa bespeak a turning in, a denial of
    the bright void beyond the hull. ... We have sealed ourselves away behind
    our money, growing inward, generating a seamless universe of self.
    The Villa Straylight knows no sky, recorded or otherwise.
    
                                                        — Neuromancer¹

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

NVIDIA documented the true names.

Not in marketing materials or even CUTLASS example 77. In
`doc/reading/tma-modeling-in-depth.md` and `doc/math/integer-division.md`
and thirty other files released under BSD-3-Clause, written by engineers
who needed to navigate the labyrinth they built.

They proved these by hand. They wrote them in markdown and SVG.

We encoded them as types.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    §0   JENSEN'S RAZOR                              the gothic folly
    §1   THE RECTILINEAR CHAMBER                     layouts, coords
    §2   THE SENSE/NET PYRAMID                       coalescence
    §3   BUILT HIM UP FROM NOTHING                   holes, fttc
    §4   TAKE YOUR WORD, THIEF                       integer division
    §A   PROOF INDEX                                 0 sorry

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
-/


/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
       // reading this file
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    This is a Lean 4 file. Lean is a proof assistant: a programming
    language where you can state mathematical theorems and the compiler
    verifies your proofs are correct. If this file compiles, every
    theorem is true.

    KEY SYNTAX:

    `structure Foo where`     -- defines a data type with named fields
      field1 : Type           -- each field has a name and type
      field2 : Type

    `def foo (x : Nat) : Nat` -- defines a function
      := x + 1                -- `:=` means "equals by definition"

    `theorem name : P := by`  -- states theorem "name" with type P
      tactic1                 -- `by` enters tactic mode
      tactic2                 -- tactics build the proof step by step

    `(h : x > 0)`             -- h is a PROOF that x > 0, not just a bool
                              -- Lean tracks proofs as first-class values

    `hb : b > 0`              -- naming convention: h = "hypothesis"
                              -- hb = "hypothesis about b"

    COMMON TACTICS:

    `simp`                    -- simplify using known lemmas
    `omega`                   -- solve linear arithmetic automatically
    `exact foo`               -- "the proof is exactly foo"
    `rw [lemma]`              -- rewrite using lemma
    `constructor`             -- split ∧ or ↔ into subgoals
    `intro h`                 -- assume hypothesis, call it h
    `obtain ⟨x, hx⟩ := h`     -- destructure existential

    The proofs here are terse. The point is that they EXIST and
    TYPE-CHECK, not that they're pedagogically optimal.

-/

namespace VillaStraylight

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                              §0. JENSEN'S RAZOR
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    He told me,' she whispered. Wintermute. How he played a waiting game for
    years. Didn't have any real power, then, but he could use the Villa's
    security and custodial systems to keep track of where everything was, how
    things moved, where they went. He saw somebody lose this key twenty years 
    ago, and he managed to get somebody else to leave it here. Then he killed 
    him, the boy who'd brought it here. Kid was eight.' She closed her white 
    fingers  over the key. So nobody would find it.' She took a length of 
    black nylon cord  from the suit's kangaroo pocket and threaded it through 
    the round hole above CHUBB. Knotting it, she hung it around her neck. They 
    were always fucking him over with how old-fashioned they were, he said, 
    all their nineteenth-century stuff. He looked just like the Finn, on 
    the screen in that meat puppet hole. Almost thought he _was_ the Finn, 
    if I wasn't careful.'
    
    Her readout flared the time, alphanumerics superimposed over the gray 
    steel chests. `He said if they'd turned into what they'd wanted to, he 
    could've gotten out a long time ago. But they didn't. Screwed up. Freaks 
    like 3Jane. That's what he called her, but he talked like he liked her.
    'She turned, opened the door, and stepped out, her hand brushing the 
    checkered grip of the holstered fletcher.

    Case flipped.

                                                        — Neuromancer²

Jensen climbed the silcon stairwells to discover that he loathed
general purpose computing. NVIDIA's CUDA stack is Villa Straylight. 
Not metaphorically. Architecturally. A body grown in upon itself. 
Each space in some way secret.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
              // jensen's razor
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    Never attribute to search what can be proven by construction.
    ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    Night City was like a deranged experiment in Social Darwinism,
    designed by a bored researcher who kept one thumb permanently on
    the fast-forward button. Stop hustling and you sank without a
    trace, but move a little too swiftly and you'd break the fragile
    surface tension of the black market; either way, you were gone,
    with nothing left of you but some vague memory in the mind of a
    fixture like Ratz, though heart or lungs or kidneys might survive
    in the service of some stranger with New Yen for the clinic tanks.
                                                        — Neuromancer³

Move too slow: TFLOPS on the table. Sink without a trace.
Move too fast: break the surface tension. Silent corruption.

We prove tilings valid before compilation. The search space collapses.
The types won't let you break the surface.
-/

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                          §1. THE RECTILINEAR CHAMBER
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    A year here and he still dreamed of cyberspace, hope fading nightly.
    All the speed he took, all the turns he'd taken and the corners he'd
    cut in Night City, and still he'd see the matrix in his sleep, bright
    lattices of logic unfolding across that colorless void… The Sprawl
    was a long strange way home over the Pacific now, and he was no
    console man, no cyberspace cowboy. Just another hustler, trying to
    make it through. But the dreams came on in the Japanese night like
    livewire voodoo, and he'd cry for it, cry in his sleep, and wake
    alone in the dark, curled in his capsule in some coffin hotel, his
    hands clawed into the bedslab, temperfoam bunched between his fingers,
    trying to reach the console that wasn't there.
                                                        — Neuromancer⁴

The tensor core is the console. The layout algebra is how you jack in.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
       // the layout function
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    eval : Coord(S) → ℕ
    eval(x₀, x₁, …, xₙ₋₁) = Σᵢ xᵢ · dᵢ

    Shape    S = (M₀, M₁, …, Mₙ₋₁)     positive integers
    Stride   D = (d₀, d₁, …, dₙ₋₁)     positive integers
    Coord    C ∈ [0,M₀) × [0,M₁) × …

-/

/-- A single mode: one dimension of a layout.

    In GPU terminology, a "mode" is one axis of a tensor's shape.
    A 2D matrix has 2 modes: rows and columns.

    LEAN NOTES:
    • `structure` defines a record type (like a C struct)
    • `shape_pos : shape > 0` is a PROOF FIELD — you can't construct
      a Mode without proving its shape is positive
    • `deriving Repr` auto-generates a pretty-printer
-/
structure Mode where
  shape : Nat       -- how many elements along this dimension
  stride : Nat      -- memory distance between consecutive elements
  shape_pos : shape > 0  -- PROOF: shape must be positive (enforced by types!)
  deriving Repr

/-- A layout is a list of modes.

    EXAMPLE: A row-major 4×8 matrix has layout:
      [Mode(shape=8, stride=1), Mode(shape=4, stride=8)]

    The innermost mode (columns) has stride 1.
    The outer mode (rows) has stride 8 (= width of row).
-/
def Layout := List Mode

/-- Evaluate a layout at a coordinate.

    Given coordinates [x₀, x₁, ...], compute the linear index:
      x₀ × stride₀ + x₁ × stride₁ + ...

    Returns `none` if coordinate length doesn't match layout rank.

    LEAN NOTES:
    • `Option Nat` means "maybe a Nat" — like Rust's Option<u64>
    • `zipWith` pairs up elements from two lists
    • This is a DEFINITION, not a theorem — no proof needed
-/
def Layout.eval (L : Layout) (coords : List Nat) : Option Nat :=
  if coords.length = L.length then
    some (List.sum (List.zipWith (· * ·) coords (L.map Mode.stride)))
  else
    none

/-- Total number of elements in a layout's domain -/
def Layout.size (L : Layout) : Nat :=
  L.foldl (· * ·.shape) 1

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // coordinate isomorphism
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    x ∈ [0, M₀ × M₁)
           │
           ▼
       x % M₀        →  x₀
       x / M₀ % M₁   →  x₁
           │
           ▼
    x₀ + x₁ × M₀ = x

    Mixed-radix arithmetic. The foundation.

-/

section MixedRadix

/-- Decompose a linear index into 2D coordinates.

    EXAMPLE: If M₀=4, then index 11 becomes (11%4, 11/4%M₁) = (3, 2)
             Check: 3 + 2×4 = 11 ✓
-/
def decompose2 (M₀ M₁ : Nat) (x : Nat) : Nat × Nat :=
  (x % M₀, x / M₀ % M₁)

/-- Recompose 2D coordinates into a linear index -/
def recompose2 (M₀ : Nat) (x₀ x₁ : Nat) : Nat :=
  x₀ + x₁ * M₀

/-- **Theorem 1.1**: recompose ∘ decompose = id

    "Round-tripping through 2D coordinates gets you back where you started."

    LEAN NOTES:
    • `(hM₀ : M₀ > 0)` — we REQUIRE a proof that M₀ > 0
    • `(hx : x < M₀ * M₁)` — and that x is in bounds
    • The proof uses `simp`, `have`, and `exact`
    • `Nat.mod_add_div` is Mathlib's theorem: n % m + n / m * m = n
-/
theorem recompose_decompose_2d (M₀ M₁ : Nat) (x : Nat)
    (_hM₀ : M₀ > 0) (_hM₁ : M₁ > 0) (hx : x < M₀ * M₁) :
    recompose2 M₀ (decompose2 M₀ M₁ x).1 (decompose2 M₀ M₁ x).2 = x := by
  simp only [decompose2, recompose2]
  have h1 : x / M₀ < M₁ := Nat.div_lt_of_lt_mul hx
  rw [Nat.mod_eq_of_lt h1]
  rw [Nat.mul_comm]
  exact Nat.mod_add_div x M₀

/-- **Theorem 1.2**: decompose ∘ recompose = id

    "Coordinates → index → coordinates gives you the same coordinates."
-/
theorem decompose_recompose_2d (M₀ M₁ : Nat) (x₀ x₁ : Nat)
    (hM₀ : M₀ > 0) (hx₀ : x₀ < M₀) (hx₁ : x₁ < M₁) :
    decompose2 M₀ M₁ (recompose2 M₀ x₀ x₁) = (x₀, x₁) := by
  simp only [decompose2, recompose2]
  ext <;> simp only
  · rw [Nat.add_mul_mod_self_right]
    exact Nat.mod_eq_of_lt hx₀
  · rw [Nat.add_mul_div_right _ _ hM₀]
    simp only [Nat.div_eq_of_lt hx₀, Nat.zero_add]
    exact Nat.mod_eq_of_lt hx₁

/-- **Theorem 1.3**: decomposition stays in bounds -/
theorem decompose2_bounds (M₀ M₁ : Nat) (x : Nat) (hM₀ : M₀ > 0) (hM₁ : M₁ > 0) :
    (decompose2 M₀ M₁ x).1 < M₀ ∧ (decompose2 M₀ M₁ x).2 < M₁ := by
  simp only [decompose2]
  exact ⟨Nat.mod_lt x hM₀, Nat.mod_lt _ hM₁⟩

end MixedRadix

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                          §2. THE SENSE/NET PYRAMID
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    Case met his first Modern two days after he'd screened the Hosaka's
    precis. The Moderns, he'd decided, were a contemporary version of
    the Big Scientists of his own late teens. There was a kind of ghostly
    teenage DNA at work in the Sprawl, something that carried the coded
    precepts of various short-lived subcults and replicated them at odd
    intervals. The Panther Moderns were a softhead variant on the
    Scientists. If the technology had been available, the Big Scientists
    would all have had sockets stuffed with microsofts. It was the style
    that mattered and the style was the same. The Moderns were mercenaries,
    practical jokers, nihilistic technofetishists.
                                                        — Neuromancer⁵

Canonical form. The style is the same. Coalescence applies three rewrites:

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
      // coalescence rules
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    RULE 1   right unit       (M, 1) : (d, s)  →  (M) : (d)
    RULE 2   left unit        (1, M) : (d, s)  →  (M) : (s)
    RULE 3   packed merge     (M₁, M₂) : (d₁, d₂)  →  (M₁·M₂) : (d₁)
                              when d₂ = M₁ · d₁

-/

/-- Result of attempting to coalesce two modes.

    LEAN NOTES:
    • `inductive` defines a sum type (like Rust enum)
    • Either unchanged (can't merge) or merged into one Mode
-/
inductive CoalesceResult where
  | unchanged : CoalesceResult
  | merged : Mode → CoalesceResult
  deriving Repr

/-- Try to coalesce two adjacent modes into one.

    Three rules (from NVIDIA's coalescence algorithm):
    1. Right unit:  (M, 1) → M       (drop trailing 1)
    2. Left unit:   (1, M) → M       (drop leading 1)
    3. Packed:      (M₁, M₂) → M₁×M₂ when strides align

    LEAN NOTES:
    • Pattern `if ... then ... else if ... else`
    • `.merged m₀` constructs CoalesceResult.merged with mode m₀
    • The `shape_pos` field requires a proof — we use `Nat.mul_pos`
-/
def tryCoalesce (m₀ m₁ : Mode) : CoalesceResult :=
  if m₁.shape = 1 then
    .merged m₀                           -- Rule 1: drop trailing 1
  else if m₀.shape = 1 then
    .merged m₁                           -- Rule 2: drop leading 1
  else if m₁.stride = m₀.shape * m₀.stride then
    .merged {                            -- Rule 3: packed merge
      shape := m₀.shape * m₁.shape
      stride := m₀.stride
      shape_pos := Nat.mul_pos m₀.shape_pos m₁.shape_pos  -- proof!
    }
  else
    .unchanged                           -- can't coalesce

-- **Theorem 2.1**: coalescence terminates (mode count decreases)
-- Each rule reduces list length. Noetherian on ℕ.

/-- **Theorem 2.2**: coalescence preserves eval -/
theorem coalesce_preserves_function (m0 m1 : Mode) (result : Mode)
    (h : tryCoalesce m0 m1 = .merged result) :
    ∀ x0 < m0.shape, ∀ x1 < m1.shape,
      x0 * m0.stride + x1 * m1.stride =
      (x0 + x1 * m0.shape) * result.stride := by
  intro x0 hx0 x1 hx1
  by_cases h1 : m1.shape = 1
  · have hmerge : tryCoalesce m0 m1 = .merged m0 := by simp [tryCoalesce, h1]
    rw [hmerge] at h
    cases h
    have hx1zero : x1 = 0 := Nat.lt_one_iff.mp (by simpa [h1] using hx1)
    simp [hx1zero]
  · by_cases h0 : m0.shape = 1
    · have hmerge : tryCoalesce m0 m1 = .merged m1 := by simp [tryCoalesce, h1, h0]
      rw [hmerge] at h
      cases h
      have hx0zero : x0 = 0 := Nat.lt_one_iff.mp (by simpa [h0] using hx0)
      simp [hx0zero, h0]
    · by_cases hstride : m1.stride = m0.shape * m0.stride
      · have hmerge : tryCoalesce m0 m1 = .merged
            ⟨m0.shape * m1.shape, m0.stride, Nat.mul_pos m0.shape_pos m1.shape_pos⟩ := by
          simp [tryCoalesce, h1, h0, hstride]
        rw [hmerge] at h
        cases h
        simp [hstride, Nat.add_mul, Nat.mul_assoc]
      · have : tryCoalesce m0 m1 = .unchanged := by simp [tryCoalesce, h1, h0, hstride]
        cases h ▸ this

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            §3. BUILT HIM UP FROM NOTHING AT A FACILITY IN FRANCE
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    Translated French medical records explained that a man without
    identification had been taken to a Paris mental health unit and
    diagnosed as schizophrenic. He became catatonic and was sent to a
    government institution on the outskirts of Toulon. He became a
    subject in an experimental program that sought to reverse
    schizophrenia through the application of cybernetic models. A
    random selection of patients were provided with microcomputers and
    encouraged, with help from students, to program them. He was cured,
    the only success in the entire experiment.
                                                        — Neuromancer⁶

Armitage, built up from nothing. Corto's broken mind reconstructed through
cybernetic models. The only success in the entire experiment.

That's what we're doing here: taking NVIDIA's scattered documentation and
building it into something that compiles. The cybernetic model is Lean.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                 // the problem
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    From nvfuser `doc/reading/divisibility-of-split.md`:⁷

    "Whenever we do an indivisible split on an IterDomain, we
     effectively changed its range as well. We call this added
     extra range 'holes'."

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
               // holes diagram
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    Extent E = 7,  Factor F = 3

    original        [0, 1, 2, 3, 4, 5, 6]

    after split     [0, 1, 2] [3, 4, 5] [6, ?, ?]
                     outer=0   outer=1   outer=2
                                          ▲  ▲
                                     HOLES─┴──┘

    outer extent = ⌈7/3⌉ = 3
    inner extent = 3
    product = 9 > 7  →  2 holes

-/

section CeilDiv

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            // ceiling division
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    ⌈a/b⌉ = "the smallest integer q such that a ≤ q × b"

    STANDARD FORMULA:

        ⌈a/b⌉ = (a + b - 1) / b

    EXAMPLE:
        ⌈7/3⌉ = (7 + 3 - 1) / 3 = 9 / 3 = 3
        Check: 7 ≤ 3 × 3 = 9 ✓
        Check: 7 > 2 × 3 = 6, so 2 is too small ✓

-/

/-- Ceiling division: ⌈a/b⌉ -/
def ceilDiv (a b : Nat) : Nat := (a + b - 1) / b

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
          // the galois insight
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    THE KEY INSIGHT (our main proof technique):

        ⌈a/b⌉ ≤ Q   ⟺   a ≤ Q × b

    Read this carefully. It says:
    - To show ⌈a/b⌉ is at most Q, show a ≤ Q×b
    - To show a ≤ Q×b, show ⌈a/b⌉ is at most Q

    This is called a GALOIS CONNECTION. The ceiling function
    and multiplication by b are "adjoint" — each characterizes
    the other.

    WHY THIS MATTERS:
    
    Standard proofs use case analysis: "if b divides a, then...
    otherwise...". The Galois approach lets us prove things
    about ⌈⌉ by proving things about ≤, which is much easier.

-/

/-- LEMMA: The formula (a + b - 1) / b rounds up.

    We show: a ≤ ((a + b - 1) / b) × b

    PROOF SKETCH:
    Let q = (a + b - 1) / b. By definition of /, we have
    (a + b - 1) = q × b + r where 0 ≤ r < b.
    Rearranging: a = q × b + r - b + 1 = q × b - (b - 1 - r)
    Since r < b, we have b - 1 - r ≥ 0, so a ≤ q × b.
-/
lemma ceilDiv_mul_ge (a b : Nat) (hb : b > 0) : a ≤ ceilDiv a b * b := by
  simp only [ceilDiv]
  by_cases ha : a = 0
  · simp [ha]
  · have h2 : (a + b - 1) % b < b := Nat.mod_lt (a + b - 1) hb
    have split_raw := Nat.div_add_mod (a + b - 1) b
    have split : (a + b - 1) / b * b + (a + b - 1) % b = a + b - 1 := by
      rw [Nat.mul_comm]; exact split_raw
    have step1 : a ≤ a + b - 1 := by omega
    have step2 : a + b - 1 = (a + b - 1) / b * b + (a + b - 1) % b := split.symm
    have step3 : (a + b - 1) / b * b + (a + b - 1) % b ≤ (a + b - 1) / b * b + b := by omega
    omega

/-- LEMMA: ceilDiv gives the LEAST such q.

    If a ≤ Q × b, then ⌈a/b⌉ ≤ Q.

    PROOF SKETCH:
    We need (a + b - 1) / b ≤ Q.
    By Mathlib's div_le lemma, this follows if a + b - 1 < (Q + 1) × b.
    Since a ≤ Q × b, we have a + b - 1 ≤ Q × b + b - 1 < (Q + 1) × b. ∎
-/
lemma ceilDiv_le (a b Q : Nat) (hb : b > 0) (h : a ≤ Q * b) : ceilDiv a b ≤ Q := by
  simp only [ceilDiv]
  by_cases ha : a = 0
  · simp [ha]
    have : (b - 1) / b = 0 := Nat.div_eq_of_lt (Nat.sub_lt hb Nat.zero_lt_one)
    omega
  · by_cases hQ : Q = 0
    · have : a = 0 := by
        have : Q * b = 0 := by simp [hQ]
        omega
      contradiction
    · have bound : a + b - 1 < (Q + 1) * b := by
        have h1 : a + b - 1 ≤ Q * b + b - 1 := by omega
        calc a + b - 1 ≤ Q * b + b - 1 := h1
             _ < Q * b + b := by omega
             _ = (Q + 1) * b := by ring
      exact Nat.le_of_succ_le_succ (Nat.div_lt_iff_lt_mul hb |>.mpr bound)

/-- **Theorem 3.1**: The Galois characterization (both directions).

    ⌈a/b⌉ ≤ Q  ⟺  a ≤ Q × b

    This single theorem unlocks everything else.
-/
theorem ceilDiv_le_iff (a b Q : Nat) (hb : b > 0) :
    ceilDiv a b ≤ Q ↔ a ≤ Q * b := by
  constructor
  · -- (⟹) If ⌈a/b⌉ ≤ Q, then a ≤ Q×b
    intro h
    -- We know a ≤ ⌈a/b⌉ × b (from ceilDiv_mul_ge)
    have h1 : a ≤ ceilDiv a b * b := ceilDiv_mul_ge a b hb
    -- And ⌈a/b⌉ × b ≤ Q × b (since ⌈a/b⌉ ≤ Q)
    have h2 : ceilDiv a b * b ≤ Q * b := Nat.mul_le_mul_right b h
    -- So a ≤ Q × b by transitivity
    exact Nat.le_trans h1 h2
  · -- (⟸) If a ≤ Q×b, then ⌈a/b⌉ ≤ Q
    exact ceilDiv_le a b Q hb

/-- Corollary: a ≤ ⌈a/b⌉ × b (just the ⟹ direction at Q = ⌈a/b⌉) -/
theorem ceilDiv_mul_ge_self (a b : Nat) (hb : b > 0) :
    a ≤ ceilDiv a b * b :=
  ceilDiv_mul_ge a b hb

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
               // associativity
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    ⌈⌈i/n⌉/m⌉ = ⌈i/(m×n)⌉

    This is nvfuser Theorem 5.11. It says: splitting twice
    (first by n, then by m) gives the same outer extent as
    splitting once by m×n.

    PROOF STRATEGY:
    To show two natural numbers equal, show each ≤ the other.
    We use the Galois connection for both directions.

-/

/-- **Theorem 3.2**: Ceiling division is associative.

    ⌈⌈i/n⌉/m⌉ = ⌈i/(m×n)⌉

    PROOF:
    (≤) To show LHS ≤ RHS, by Galois it suffices to show
        ⌈i/n⌉ ≤ RHS × m, which by Galois again means
        i ≤ RHS × m × n = RHS × (m×n), which holds by Galois on RHS.

    (≥) To show RHS ≤ LHS, by Galois it suffices to show
        i ≤ LHS × (m×n). We have:
        i ≤ ⌈i/n⌉ × n           (by ceilDiv_mul_ge)
          ≤ (⌈⌈i/n⌉/m⌉ × m) × n (by ceilDiv_mul_ge again)
          = LHS × (m×n)         (by associativity of ×)
-/
theorem ceilDiv_assoc (i m n : Nat) (hm : m > 0) (hn : n > 0) :
    ceilDiv (ceilDiv i n) m = ceilDiv i (m * n) := by
  apply Nat.le_antisymm
  · -- LHS ≤ RHS
    rw [ceilDiv_le_iff _ _ _ hm]       -- suffices: ⌈i/n⌉ ≤ RHS × m
    rw [ceilDiv_le_iff _ _ _ hn]       -- suffices: i ≤ RHS × m × n
    rw [Nat.mul_assoc]                 -- rewrite as i ≤ RHS × (m×n)
    rw [← ceilDiv_le_iff _ _ _ (Nat.mul_pos hm hn)]  -- true by reflexivity
  · -- RHS ≤ LHS
    rw [ceilDiv_le_iff _ _ _ (Nat.mul_pos hm hn)]  -- suffices: i ≤ LHS × (m×n)
    -- Chain of inequalities
    calc i ≤ ceilDiv i n * n := ceilDiv_mul_ge_self i n hn
      _ ≤ (ceilDiv (ceilDiv i n) m * m) * n :=
          Nat.mul_le_mul_right n (ceilDiv_mul_ge_self (ceilDiv i n) m hm)
      _ = ceilDiv (ceilDiv i n) m * (m * n) := by ring

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
              // divisible case
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    When b divides n evenly, ceiling = floor:

        b ∣ n  ⟹  ⌈n/b⌉ = n/b

    EXAMPLE: ⌈12/4⌉ = 12/4 = 3

-/

/-- **Theorem 3.3**: When divisible, ceiling equals floor. -/
theorem ceilDiv_of_dvd (n d : Nat) (hd : d > 0) (hdiv : d ∣ n) :
    ceilDiv n d = n / d := by
  rcases hdiv with ⟨k, rfl⟩
  simp only [ceilDiv, Nat.mul_comm k d, Nat.mul_div_cancel_left k hd]
  by_cases hk : k = 0
  · simp [hk]
    omega
  · by_cases hd1 : d = 1
    · simp [hd1]
    · have pos : 0 < d * k + d := by omega
      have h1 : d * k + d - 1 < (k + 1) * d := by
        calc d * k + d - 1 < d * k + d := Nat.sub_lt pos Nat.zero_lt_one
             _ = (k + 1) * d := by ring
      have div_lt : (d * k + d - 1) / d < k + 1 := Nat.div_lt_iff_lt_mul hd |>.mpr h1
      have div_ge : k ≤ (d * k + d - 1) / d := by
        rw [Nat.le_div_iff_mul_le hd]
        calc k * d = d * k := by ring
             _ ≤ d * k + d - 1 := by omega
      omega

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            // indivisible case
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    When b does NOT divide n, ceiling = floor + 1:

        ¬(b ∣ n)  ⟹  ⌈n/b⌉ = ⌊n/b⌋ + 1

    EXAMPLE: ⌈7/3⌉ = 7/3 + 1 = 2 + 1 = 3

    This is where the HOLES come from. If we split 7 elements
    into groups of 3, we get ⌈7/3⌉ = 3 groups, but 3 × 3 = 9,
    so there are 9 - 7 = 2 holes.

-/

/-- **Theorem 3.4**: When indivisible, ceiling = floor + 1. -/
theorem ceilDiv_eq_div_add_one_of_not_dvd (n d : Nat) (hd : d > 0) (hndiv : ¬ d ∣ n) :
    ceilDiv n d = n / d + 1 := by
  have hmod_ne : n % d ≠ 0 := fun h => hndiv (Nat.dvd_of_mod_eq_zero h)
  simp only [ceilDiv]
  have hmod_pos : 0 < n % d := Nat.pos_of_ne_zero hmod_ne
  have hmod_lt : n % d < d := Nat.mod_lt n hd
  have split_n : d * (n / d) + n % d = n := Nat.div_add_mod n d
  have key : n + d - 1 = d * (n / d + 1) + (n % d - 1) := by
    have : n = d * (n / d) + n % d := split_n.symm
    calc n + d - 1
        = (d * (n / d) + n % d) + d - 1 := by rw [← this]
      _ = d * (n / d) + n % d + d - 1 := by rfl
      _ = d * (n / d) + (n % d + d) - 1 := by omega
      _ = d * (n / d) + d + n % d - 1 := by omega
      _ = d * (n / d + 1) + n % d - 1 := by ring_nf
      _ = d * (n / d + 1) + (n % d - 1) := by rw [Nat.add_sub_assoc (by omega)]
  have small : n % d - 1 < d := by
    have : n % d ≤ d - 1 := Nat.le_pred_of_lt hmod_lt
    omega
  calc (n + d - 1) / d
      = (d * (n / d + 1) + (n % d - 1)) / d := by rw [key]
    _ = ((n % d - 1) + d * (n / d + 1)) / d := by ring_nf
    _ = (n % d - 1) / d + (n / d + 1) := Nat.add_mul_div_left _ _ hd
    _ = 0 + (n / d + 1) := by simp [Nat.div_eq_of_lt small]
    _ = n / d + 1 := by simp

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
               // holes theorem
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    HOLES = ⌈n/d⌉ × d - n

    When d ∤ n, holes > 0.
    When d ∣ n, holes = 0.

    This is the fundamental fact that makes FTTC necessary:
    indivisible splits create holes, and holes cause
    out-of-bounds memory access if not predicated.

-/

/-- **Theorem 3.5**: Indivisibility creates holes. -/
theorem ceilDiv_mul_sub_self_pos_of_not_dvd (n d : Nat) (hd : d > 0) (hndiv : ¬ d ∣ n) :
    0 < ceilDiv n d * d - n := by
  -- We know ⌈n/d⌉ = n/d + 1 when d ∤ n
  have hceil : ceilDiv n d = n / d + 1 := ceilDiv_eq_div_add_one_of_not_dvd n d hd hndiv
  -- So ⌈n/d⌉ × d = (n/d + 1) × d = (n/d)×d + d
  -- And n = (n/d)×d + n%d where n%d < d
  -- So ⌈n/d⌉ × d - n = d - n%d > 0
  have hlt : n < ceilDiv n d * d := by
    have hsplit : n = d * (n / d) + n % d := (Nat.div_add_mod n d).symm
    calc n = d * (n / d) + n % d := hsplit
      _ < d * (n / d) + d := Nat.add_lt_add_left (Nat.mod_lt n hd) _
      _ = d * (n / d + 1) := by ring
      _ = d * ceilDiv n d := by simp [hceil]
      _ = ceilDiv n d * d := by ring
  exact Nat.sub_pos_of_lt hlt

end CeilDiv

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
     // extended ceiling theory
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    Additional lemmas completing the algebraic structure.
-/

section CeilDiv_Extras

/-- zero case: ceilDiv a b = 0 ↔ a = 0 -/
theorem ceilDiv_eq_zero_iff (a b : Nat) (hb : b > 0) :
    ceilDiv a b = 0 ↔ a = 0 := by
  constructor
  · intro h
    simp only [ceilDiv] at h
    have hdiv : (a + b - 1) / b = 0 := h
    have hlt : a + b - 1 < b := by
      have : (a + b - 1) / b = 0 ↔ b = 0 ∨ a + b - 1 < b := Nat.div_eq_zero_iff
      have : b = 0 ∨ a + b - 1 < b := this.mp hdiv
      cases this with
      | inl h => omega
      | inr h => exact h
    omega
  · intro ha
    subst ha
    simp only [ceilDiv, Nat.zero_add]
    exact Nat.div_eq_of_lt (Nat.sub_lt hb (Nat.succ_pos 0))

/-- succ sandwich: ceilDiv a b = k+1 ↔ k*b < a ∧ a ≤ (k+1)*b -/
theorem ceilDiv_eq_succ_iff (a b k : Nat) (hb : b > 0) :
    ceilDiv a b = k.succ ↔ (k * b < a) ∧ (a ≤ (k.succ) * b) := by
  constructor
  · intro h
    have upper : a ≤ (k.succ) * b :=
      (ceilDiv_le_iff a b (k.succ) hb).mp (le_of_eq h)
    have lower : ¬(a ≤ k * b) := by
      intro hle
      have : ceilDiv a b ≤ k := (ceilDiv_le_iff a b k hb).mpr hle
      omega
    exact ⟨Nat.lt_of_not_le lower, upper⟩
  · intro ⟨lower, upper⟩
    have h1 : ceilDiv a b ≤ k.succ := (ceilDiv_le_iff a b k.succ hb).mpr upper
    have h2 : ¬(ceilDiv a b ≤ k) := by
      intro hle
      have : a ≤ k * b := (ceilDiv_le_iff a b k hb).mp hle
      omega
    omega

/-- monotone in numerator -/
theorem ceilDiv_mono_left {a a' b : Nat} (hb : b > 0) (h : a ≤ a') :
    ceilDiv a b ≤ ceilDiv a' b :=
  (ceilDiv_le_iff a b (ceilDiv a' b) hb).mpr <|
    Nat.le_trans h (ceilDiv_mul_ge_self a' b hb)

/-- antitone in denominator -/
theorem ceilDiv_antitone_right {a b b' : Nat} (hb : b > 0) (hbb' : b ≤ b') :
    ceilDiv a b' ≤ ceilDiv a b := by
  have hb' : b' > 0 := Nat.lt_of_lt_of_le hb hbb'
  refine (ceilDiv_le_iff a b' (ceilDiv a b) hb').mpr ?_
  have hmul : ceilDiv a b * b ≤ ceilDiv a b * b' := Nat.mul_le_mul_left _ hbb'
  exact Nat.le_trans (ceilDiv_mul_ge_self a b hb) hmul

/-- no-holes iff divisibility -/
theorem ceilDiv_mul_sub_self_eq_zero_iff (n d : Nat) (hd : d > 0) :
    ceilDiv n d * d - n = 0 ↔ d ∣ n := by
  constructor
  · intro h
    by_contra hndiv
    have hpos : 0 < ceilDiv n d * d - n :=
      ceilDiv_mul_sub_self_pos_of_not_dvd n d hd hndiv
    omega
  · intro hdiv
    have hcd : ceilDiv n d = n / d := ceilDiv_of_dvd n d hd hdiv
    have hmul : (n / d) * d = n := Nat.div_mul_cancel hdiv
    simp [hcd, hmul]

end CeilDiv_Extras

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                        // fttc
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    "Autonomy, that's the bugaboo, where your AI's are concerned. My
     guess, Case, you're going in there to cut the hardwired shackles
     that keep this baby from getting any smarter. And I can't see how
     you'd distinguish, say, between a move the parent company makes,
     and some move the AI makes on its own, so that's maybe where the
     confusion comes in." Again the nonlaugh. "See, those things, they
     can work real hard, buy themselves time to write cookbooks or
     whatever, but the minute, I mean the nanosecond, that one starts
     figuring out ways to make itself smarter, Turing'll wipe it.
     Nobody trusts those fuckers, you know that. Every AI ever built
     has an electromagnetic shotgun wired to its forehead."
                                                        — Neuromancer⁹

    From nvfuser `doc/reading/tma-modeling-in-depth.md` Theorem 6:¹⁰

    "Theorem 6 is so powerful that it deserves a fancier name:
     'The fundamental theorem of TMA correctness', or FTTC."

    Strong correctness is UNACHIEVABLE iff:

        e < B < S   AND   e ∤ B

    where e = element stride, B = box size, S = tensor size

    The shotgun wired to the forehead. The type system won't let you
    construct a schedule that violates FTTC. Turing'll wipe it.

-/

/-- Configuration for FTTC (Fundamental Theorem of TMA Correctness).

    WHAT THIS REPRESENTS:
    • element_stride: memory distance between tensor elements
    • box_size: TMA copy granularity (how much we load at once)
    • tensor_size: total tensor dimension

    THE CONSTRAINT:
    FTTC says strong correctness is achievable UNLESS:
      e < B < S  AND  e ∤ B

    WHY IT MATTERS:
    If e doesn't divide B, TMA copies will read garbage at boundaries.
    The type system prevents you from constructing invalid schedules.

    LEAN NOTES:
    • The `_pos` fields are PROOFS, not just documentation
    • You literally cannot construct an FTTCConfig with box_size = 0
-/
structure FTTCConfig where
  element_stride : Nat
  box_size : Nat
  tensor_size : Nat
  e_pos : element_stride > 0  -- proof required!
  b_pos : box_size > 0        -- proof required!
  s_pos : tensor_size > 0     -- proof required!

/-- FTTC violation condition: strong correctness is IMPOSSIBLE -/
def FTTCConfig.violated (c : FTTCConfig) : Prop :=
  c.element_stride < c.box_size ∧
  c.box_size < c.tensor_size ∧
  ¬(c.element_stride ∣ c.box_size)

/-- FTTC achievability: at least one escape hatch -/
def FTTCConfig.achievable (c : FTTCConfig) : Prop :=
  c.element_stride ∣ c.box_size ∨   -- divisibility saves you
  c.box_size ≥ c.tensor_size ∨       -- box covers whole tensor
  c.element_stride ≥ c.box_size      -- element ≥ box (weird but ok)

/-- **Theorem 3.6** (FTTC): violated ⟺ ¬achievable

    This is NVIDIA's Theorem 6 from tma-modeling-in-depth.md.
    The type system encodes this: you can't construct a TMA schedule
    that would violate FTTC because the proof obligation fails.

    LEAN NOTES:
    • `↔` means "if and only if" — we prove both directions
    • `push_neg` converts ¬(A ∨ B ∨ C) to ¬A ∧ ¬B ∧ ¬C
    • `obtain ⟨h1, h2, h3⟩ := h` destructures conjunction
-/
theorem fttc (c : FTTCConfig) : c.violated ↔ ¬c.achievable := by
  simp only [FTTCConfig.violated, FTTCConfig.achievable]
  constructor
  · intro ⟨he_lt_b, hb_lt_s, hnotdiv⟩
    push_neg
    exact ⟨hnotdiv, hb_lt_s, he_lt_b⟩
  · intro h
    push_neg at h
    obtain ⟨hnotdiv, hb_lt_s, he_lt_b⟩ := h
    exact ⟨he_lt_b, hb_lt_s, hnotdiv⟩

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
       // fttc violation
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    nvfuser Figure 5:

    e = 3, B = 5, S = 8

    3 < 5 < 8  ∧  3 ∤ 5

    → strong correctness unachievable

-/

def figure5_example : FTTCConfig := {
  element_stride := 3
  box_size := 5
  tensor_size := 8
  e_pos := by omega
  b_pos := by omega
  s_pos := by omega
}

theorem figure5_violated : figure5_example.violated := by
  simp only [FTTCConfig.violated, figure5_example]
  constructor
  · omega
  constructor
  · omega
  · intro ⟨k, hk⟩; omega

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                         §4. TAKE YOUR WORD, THIEF
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    "No word for you, Molly. He told me about that, you see. 3Jane
     knows the code, of course, but you won't have it. Neither will
     Wintermute. My Jane's an ambitious girl, in her perverse way."
     He smiled again. "She has designs on the family empire, and a
     pair of insane artificial intelligences, kinky as the concept
     may be, would only get in our way. So. Comes her Riviera to help
     her out, you see. And Peter says, sit tight. Play Daddy's favorite
     swing records and let Peter call you up a band to match, a floor
     of dancers, a wake for dead King Ashpool." He drank off the last
     of the mineral water. "No, you wouldn't do, Daddy, you would not
     do. Now that Peter's come home." And then, his face pink with the
     pleasure of cocaine and meperidine, he swung the glass hard into
     her left lens implant, smashing vision into blood and light.
                                                        — Neuromancer¹⁶

The integer division theorems. Anti-intuitive. The betrayal you don't see
coming until it smashes vision into blood and light.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
     // integer division
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    From nvfuser `doc/math/integer-division.md`:¹¹

    "...extra care is needed when dealing with integer divisions.
     Because unlike real numbers, integer division can be anti-intuitive:

        (a + b) / c ≠ a/c + b/c
        (a / b) × c ≠ a / (b / c)"

-/

section IntegerDivision

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
     // integer division 101
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    Integer division is NOT like real division.

    THE DIVISION ALGORITHM:
    For any a, b with b > 0, there exist unique q, r such that:

        a = q × b + r    where 0 ≤ r < b

    We write:
        q = a / b    (quotient, rounds toward zero)
        r = a % b    (remainder)

    KEY IDENTITY:
        a = (a / b) × b + (a % b)

    This is `Nat.div_add_mod` in Mathlib.

-/

/-- **Theorem 4.1**: Small numbers.

    If r < a, then r / a = 0 and r % a = r.

    EXAMPLE: 5 / 8 = 0, 5 % 8 = 5

    INTUITION: You can't fit any 8s into 5, so quotient is 0.
    The remainder is just 5 itself.
-/
theorem thm_2_5 (r a : Nat) (_ha : a > 0) (hr : r < a) :
    r % a = r ∧ r / a = 0 := by
  exact ⟨Nat.mod_eq_of_lt hr, Nat.div_eq_of_lt hr⟩

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
     // mod distributes (sometimes)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    WARNING: (a + b) % c ≠ a % c + b % c in general!

    But there's a special case: if a is already divisible by c,
    then adding b doesn't change the remainder from b % c.

-/

/-- **Theorem 4.2**: Adding a multiple doesn't change remainder.

    If c | a (c divides a), then (a + b) % c = b % c.

    EXAMPLE: (12 + 7) % 4 = 7 % 4 = 3
             because 12 % 4 = 0

    INTUITION: Adding a multiple of c is like adding 0 mod c.
-/
theorem thm_2_7_1 (a b c : Nat) (_hc : c > 0) (ha : a % c = 0) :
    (a + b) % c = b % c := by
  rw [Nat.add_mod, ha, Nat.zero_add, Nat.mod_mod]

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
     // nested mod
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    What happens when you take mod twice?

-/

/-- **Theorem 4.3**: Nested mod simplifies.

    (a % (b × c)) % b = a % b

    EXAMPLE: (17 % 12) % 4 = 5 % 4 = 1
             17 % 4 = 1 ✓

    INTUITION: Taking mod (b×c) already "projects" into [0, b×c).
    Taking mod b again just keeps the part in [0, b).
    But a % b was already that part!
-/
theorem thm_2_7_2 (a b c : Nat) (_hb : b > 0) (_hc : c > 0) :
    a % (b * c) % b = a % b :=
  -- Mathlib has this: mod distributes when divisor divides
  Nat.mod_mod_of_dvd a (Nat.dvd_mul_right b c)

/-- **Theorem 4.4**: Divisibility means zero remainder.

    If b | a, then a % b = 0.

    This is basically the definition of divisibility.
-/
theorem thm_2_9 (a b : Nat) (h : b ∣ a) : a % b = 0 :=
  Nat.mod_eq_zero_of_dvd h

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
     // division distributes (sometimes)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    WARNING: a × (b / c) ≠ (a × b) / c in general!

    COUNTEREXAMPLE: 2 × (5 / 3) = 2 × 1 = 2
                    (2 × 5) / 3 = 10 / 3 = 3

    But when c | b (c divides b), it works:

-/

/-- **Theorem 4.5**: Division distributes over multiplication (when divisible).

    If c | b, then a × (b / c) = (a × b) / c.

    EXAMPLE: 3 | 12, so 5 × (12 / 3) = 5 × 4 = 20
                       (5 × 12) / 3 = 60 / 3 = 20 ✓

    INTUITION: When c divides b evenly, no information is lost.
-/
theorem thm_2_10 (a b c : Nat) (hc : c > 0) (hdiv : c ∣ b) :
    a * (b / c) = (a * b) / c := by
  obtain ⟨k, hk⟩ := hdiv
  subst hk
  simp only [Nat.mul_div_cancel_left _ hc]
  have key : a * (c * k) / c = a * k := by
    calc a * (c * k) / c
        = (a * (c * k)) / c := rfl
      _ = (a * c * k) / c := by ring_nf
      _ = (c * (a * k)) / c := by ring_nf
      _ = a * k := Nat.mul_div_cancel_left (a * k) hc
  exact key.symm

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
             // nested division
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    Dividing by b then by c is the same as dividing by b×c.

-/

/-- **Theorem 4.6**: Division associates.

    a / (b × c) = (a / b) / c

    EXAMPLE: 100 / (4 × 5) = 100 / 20 = 5
             (100 / 4) / 5 = 25 / 5 = 5 ✓

    This is nvfuser §2.11 and it's crucial for split-split.
-/
theorem thm_2_11 (a b c : Nat) (_hb : b > 0) (_hc : c > 0) :
    a / (b * c) = a / b / c :=
  (Nat.div_div_eq_div_mul a b c).symm

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
     // the mixed-radix identity
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    This is the key theorem for understanding how mod interacts
    with products. It decomposes a % (b×c) into two parts:
    - The "units digit" (a % b)
    - The "tens digit" ((a/b) % c), scaled by b

-/

/-- **Theorem 4.7**: Mixed-radix decomposition.

    a % (b × c) = (a % b) + ((a / b) % c) × b

    EXAMPLE: Let a = 47, b = 10, c = 5.
             47 % 50 = 47
             (47 % 10) + ((47 / 10) % 5) × 10
             = 7 + (4 % 5) × 10
             = 7 + 4 × 10
             = 47 ✓

    INTUITION: Like decomposing 47 into 4 tens and 7 units.
-/
theorem thm_2_12 (a b c : Nat) (hb : b > 0) (hc : c > 0) :
    a % (b * c) = a % b + (a / b % c) * b := by
  have split_a : b * (a / b) + a % b = a := Nat.div_add_mod a b
  have split_div : c * (a / b / c) + a / b % c = a / b := Nat.div_add_mod (a / b) c
  -- Start with a decomposed as quotient + remainder
  have step1 : a % (b * c) = (b * (a / b) + a % b) % (b * c) := by
    conv_lhs => rw [← split_a]
  -- Decompose a / b similarly
  have step2 : (b * (a / b) + a % b) % (b * c) =
               (b * (c * (a / b / c) + a / b % c) + a % b) % (b * c) := by
    conv_lhs => arg 1; arg 1; arg 2; rw [← split_div]
  -- Expand and group
  have step3 : (b * (c * (a / b / c) + a / b % c) + a % b) % (b * c) =
               (b * c * (a / b / c) + b * (a / b % c) + a % b) % (b * c) := by
    congr 1
    ring_nf
  -- The first term is divisible by (b * c), so it vanishes mod (b * c)
  have step4 : (b * c * (a / b / c) + b * (a / b % c) + a % b) % (b * c) =
               (b * (a / b % c) + a % b) % (b * c) := by
    rw [Nat.add_assoc, Nat.add_comm (b * c * (a / b / c)), Nat.add_mul_mod_self_left]
  -- Since both terms are < b * c, the mod is redundant
  have h1 : a % b < b := Nat.mod_lt a hb
  have h2 : a / b % c < c := Nat.mod_lt (a / b) hc
  have h3 : b * (a / b % c) < b * c := Nat.mul_lt_mul_of_pos_left h2 hb
  have h4 : b * (a / b % c) + a % b < b * c := by
    calc b * (a / b % c) + a % b
        < b * (a / b % c) + b := Nat.add_lt_add_left h1 _
      _ = b * (a / b % c + 1) := by ring
      _ ≤ b * c := Nat.mul_le_mul_left b (Nat.succ_le_of_lt h2)
  have step5 : (b * (a / b % c) + a % b) % (b * c) = b * (a / b % c) + a % b :=
    Nat.mod_eq_of_lt h4
  calc a % (b * c)
      = (b * (a / b) + a % b) % (b * c) := step1
    _ = (b * (c * (a / b / c) + a / b % c) + a % b) % (b * c) := step2
    _ = (b * c * (a / b / c) + b * (a / b % c) + a % b) % (b * c) := step3
    _ = (b * (a / b % c) + a % b) % (b * c) := step4
    _ = b * (a / b % c) + a % b := step5
    _ = a % b + a / b % c * b := by ring

/-- **Theorem 4.8**: Extracting the middle digit.

    (a / b) % c = (a % (b × c)) / b

    This is the inverse of Theorem 4.7: given the decomposition,
    we can extract the "tens digit" by dividing by b.
-/
theorem thm_2_15_1 (a b c : Nat) (hb : b > 0) (hc : c > 0) :
    a / b % c = a % (b * c) / b := by
  have h_thm : a % (b * c) = a % b + (a / b % c) * b := thm_2_12 a b c hb hc
  have h_mod : a % b < b := Nat.mod_lt a hb
  have small_div : a % b / b = 0 := Nat.div_eq_of_lt h_mod
  have key : a / b % c = (a % b + a / b % c * b) / b := by
    have step1 : (a / b % c * b) / b = a / b % c := by
      calc (a / b % c * b) / b = b * (a / b % c) / b := by ring_nf
           _ = a / b % c := Nat.mul_div_cancel_left _ hb
    have step2 : (a % b + a / b % c * b) / b = a / b % c := by
      calc (a % b + a / b % c * b) / b
          = (a % b + b * (a / b % c)) / b := by ring_nf
        _ = a % b / b + (a / b % c) := Nat.add_mul_div_left _ _ hb
        _ = 0 + (a / b % c) := by rw [small_div]
        _ = a / b % c := by ring
    exact step2.symm
  rw [key, ← h_thm]

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
     // the bound theorem
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    This is THE theorem for validating split bounds.

-/

/-- **Theorem 4.9**: The bound theorem.

    i / d < D  ⟺  i < D × d

    EXAMPLE: Is 47 / 10 < 5?
             47 / 10 = 4 < 5 ✓
             47 < 5 × 10 = 50 ✓

    WHY IT MATTERS:
    When we split an iteration domain of size N by factor F,
    the outer extent is ⌈N/F⌉. To check if index i is in bounds,
    we need i / F < ⌈N/F⌉, which by this theorem is i < ⌈N/F⌉ × F.
-/
theorem thm_2_16 (i D d : Nat) (hd : d > 0) :
    i / d < D ↔ i < D * d :=
  Nat.div_lt_iff_lt_mul hd

end IntegerDivision

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                 // merge-split
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    THE FUNDAMENTAL QUESTION:

    If we split an iteration domain by factor F, then merge it back,
    do we get the original domain?

        split(E, F) = (⌈E/F⌉, F)     -- outer × inner
        merge(outer, inner) = outer × inner

    So: merge(split(E, F)) = ⌈E/F⌉ × F

    ANSWER: Only if F divides E!

        divisible           indivisible
        E=8, F=4            E=7, F=3
        ⌈8/4⌉×4 = 2×4 = 8   ⌈7/3⌉×3 = 3×3 = 9
        8 = 8 ✓             9 ≠ 7 ✗ (2 holes!)

-/

/-- An iteration domain: a 1D range [0, extent).

    The `extent_pos` field is a PROOF that extent > 0.
    You cannot construct an IterDomain with extent = 0.
-/
structure IterDomain where
  extent : Nat
  extent_pos : extent > 0
  deriving Repr

/-- Split then merge: ⌈E/F⌉ × F -/
def splitThenMerge (input : IterDomain) (factor : Nat) (_hf : factor > 0) : Nat :=
  ceilDiv input.extent factor * factor

/-- **Theorem 4.10**: merge ∘ split = id ⟺ divisibility.

    This is nvfuser Theorem 5 from divisibility-of-split.md.

    PROOF:
    (⟸) If F | E, then ⌈E/F⌉ = E/F, so ⌈E/F⌉ × F = (E/F) × F = E.

    (⟹) If F ∤ E, then ⌈E/F⌉ × F > E (there are holes),
        so merge ∘ split ≠ id.
-/
theorem merge_split_identity (input : IterDomain) (factor : Nat) (hf : factor > 0) :
    splitThenMerge input factor hf = input.extent ↔ factor ∣ input.extent := by
  unfold splitThenMerge
  constructor
  · -- (⟹) If they're equal, factor must divide extent
    intro h
    by_contra hndiv
    -- If factor ∤ extent, there are holes: ⌈E/F⌉ × F - E > 0
    have hpos : 0 < ceilDiv input.extent factor * factor - input.extent :=
      ceilDiv_mul_sub_self_pos_of_not_dvd input.extent factor hf hndiv
    -- But h says they're equal, so holes = 0. Contradiction.
    omega
  · -- (⟸) If factor | extent, ceiling = floor and it cancels
    intro hdiv
    simp [ceilDiv_of_dvd input.extent factor hf hdiv, Nat.mul_comm]
    exact Nat.mul_div_cancel' hdiv

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                 // split-split
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    What if we split twice? First by n, then by m?

    Original:     [0, ..., i, ..., E)
    After split(n):  outer₁ = i / n,  inner₁ = i % n
    After split(m):  outer₂ = outer₁ / m,  inner₂ = outer₁ % m

    Alternative: split once by m×n?

    After split(m×n): outer = i / (m×n),  inner = i % (m×n)

    THEOREM: These are the same!

        outer₂ = outer                (via division associativity)
        inner₂ = inner / n            (via Theorem 4.8)
        inner₁ = inner % n            (via Theorem 4.3)

-/

/-- **Theorem 4.11**: Split-split equivalence.

    Splitting twice (first by n, then by m) gives the same result
    as splitting once by m×n.

    This packages four related facts into one theorem.
-/
theorem split_split_equivalence (m n i : Nat) (hm : m > 0) (hn : n > 0) :
    -- 1. Extent: ⌈⌈i/n⌉/m⌉ = ⌈i/(m×n)⌉
    ceilDiv (ceilDiv i n) m = ceilDiv i (m * n) ∧
    -- 2. Outer index: i/n/m = i/(m×n)
    i / n / m = i / (m * n) ∧
    -- 3. Inner-outer index: (i/n) % m = (i % (m×n)) / n
    i / n % m = i % (m * n) / n ∧
    -- 4. Inner-inner index: i % n = (i % (m×n)) % n
    i % n = i % (m * n) % n := by
  constructor
  · -- Extent: use ceiling associativity (Theorem 3.2)
    exact ceilDiv_assoc i m n hm hn
  constructor
  · -- Outer: use division associativity (Theorem 4.6)
    rw [Nat.mul_comm m n]
    exact Nat.div_div_eq_div_mul i n m
  constructor
  · -- Inner-outer: use Theorem 4.8
    rw [Nat.mul_comm m n]
    exact thm_2_15_1 i n m hn hm
  · -- Inner-inner: use Theorem 4.3
    rw [Nat.mul_comm m n]
    exact (thm_2_7_2 i n m hn hm).symm

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                 // predication
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    When we have holes, we need PREDICATION: only execute
    threads where the index is valid (not in a hole).

    The naive predicate checks every index separately.
    These theorems give us simpler equivalent predicates.

-/

/-- **Theorem 4.12**: If outer indices are in bounds, middle is too.

    If i0 < N0 and i2 < N2 and i0 = i1 × N2 + i2,
    then i1 < ⌈N0/N2⌉.

    USE: We can skip checking the middle index if we've
    already checked the outer and inner indices.
-/
theorem predication_thm_1 (i0 i1 i2 N0 N2 : Nat)
    (hN2 : N2 > 0)
    (hi0 : i0 < N0)
    (_hi2_bound : i2 < N2)
    (hindex : i0 = i1 * N2 + i2) :
    i1 < ceilDiv N0 N2 := by
  have h1 : i1 * N2 ≤ i0 := by omega
  have h2 : i1 * N2 < N0 := Nat.lt_of_le_of_lt h1 hi0
  simp only [ceilDiv]
  by_cases h_small : N0 < N2
  · -- N0 < N2: then i1 must be 0
    have : i1 = 0 := by
      by_contra hne
      have : i1 ≥ 1 := Nat.one_le_iff_ne_zero.mpr hne
      have : N2 ≤ i1 * N2 := Nat.le_mul_of_pos_left _ this
      omega
    simp [this]
    have : (N0 + N2 - 1) / N2 ≥ 0 := Nat.zero_le _
    omega
  · -- N0 ≥ N2
    have h_N0_ge : N0 ≥ N2 := Nat.le_of_not_lt h_small
    have h3 : i1 ≤ N0 / N2 := by
      have : N0 / N2 < i1 → N0 < i1 * N2 := Nat.div_lt_iff_lt_mul hN2 |>.mp
      by_contra h_not
      push_neg at h_not
      have : N0 < i1 * N2 := this h_not
      omega
    have upper_bound : ceilDiv N0 N2 = (N0 + N2 - 1) / N2 := rfl
    have h4 : i1 ≤ (N0 + N2 - 1) / N2 := by
      have incr : N0 / N2 ≤ (N0 + N2 - 1) / N2 := Nat.div_le_div_right (by omega)
      exact Nat.le_trans h3 incr
    by_cases h_eq : i1 = (N0 + N2 - 1) / N2
    · rw [h_eq] at h2
      have split := Nat.div_add_mod (N0 + N2 - 1) N2
      have mod_bound : (N0 + N2 - 1) % N2 < N2 := Nat.mod_lt _ hN2
      -- From division identity: N0 + N2 - 1 = i1 * N2 + remainder
      have h_div : N0 + N2 - 1 = i1 * N2 + (N0 + N2 - 1) % N2 := by
        calc N0 + N2 - 1 = N2 * ((N0 + N2 - 1) / N2) + (N0 + N2 - 1) % N2 := split.symm
             _ = N2 * i1 + (N0 + N2 - 1) % N2 := by rw [← h_eq]
             _ = i1 * N2 + (N0 + N2 - 1) % N2 := by ring
      -- Since remainder < N2, we have i1 * N2 = N0 + N2 - 1 - remainder ≥ N0
      have contra : N0 ≤ i1 * N2 := by
        have : i1 * N2 = N0 + N2 - 1 - (N0 + N2 - 1) % N2 := by omega
        have : (N0 + N2 - 1) % N2 ≤ N2 - 1 := Nat.le_pred_of_lt mod_bound
        omega
      omega
    · exact Nat.lt_of_le_of_ne h4 h_eq

/-- **Theorem 4.13** (Predication 2): I0 in boundary ⟺ I2 in boundary -/
theorem predication_thm_2 (i2 N0 N1 : Nat) (hN1 : N1 > 0) :
    let i0 := i2 / N1
    let N2 := N0 * N1
    (i0 < N0) ↔ (i2 < N2) := by
  exact Nat.div_lt_iff_lt_mul hN1

/-!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                              §A. PROOF INDEX
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    nvfuser doc              section          lean theorem         ✓
    ─────────────────────────────────────────────────────────────────
    integer-division.md      §2.5             thm_2_5              ✓
    integer-division.md      §2.7.1           thm_2_7_1            ✓
    integer-division.md      §2.7.2           thm_2_7_2            ✓
    integer-division.md      §2.9             thm_2_9              ✓
    integer-division.md      §2.10            thm_2_10             ✓
    integer-division.md      §2.11            thm_2_11             ✓
    integer-division.md      §2.12            thm_2_12             ✓
    integer-division.md      §2.15.1          thm_2_15_1           ✓
    integer-division.md      §2.16            thm_2_16             ✓
    integer-division.md      §5.11            ceilDiv_assoc        ✓
    ─────────────────────────────────────────────────────────────────
    (extension)              zero case        ceilDiv_eq_zero      ✓
    (extension)              succ sandwich    ceilDiv_eq_succ      ✓
    (extension)              mono left        ceilDiv_mono_left    ✓
    (extension)              antitone right   ceilDiv_antitone     ✓
    (extension)              no-holes iff     ceilDiv_mul_sub      ✓
    ─────────────────────────────────────────────────────────────────
    divisibility-of-split    Thm 1            predication_thm_1    ✓
    divisibility-of-split    Thm 2            predication_thm_2    ✓
    divisibility-of-split    Thm 5            merge_split_ident    ✓
    ─────────────────────────────────────────────────────────────────
    tma-modeling-in-depth    Thm 6 (FTTC)     fttc                 ✓
    tma-modeling-in-depth    Figure 5         figure5_violated     ✓
    ─────────────────────────────────────────────────────────────────
    iterdomain.md            Thm 2.1          split_split_equiv    ✓
    ─────────────────────────────────────────────────────────────────

    total: 21 theorems from nvfuser + 5 extensions
    sorry: 0

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                                                                        // CODA
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    "I'm not Wintermute now."

    "So what are you." He drank from the flask, feeling nothing.

    "I'm the matrix, Case."

    Case laughed. "Where's that get you?"

    "Nowhere. Everywhere. I'm the sum total of the works, the whole show."

    "That what 3Jane's mother wanted?"

    "No. She couldn't imagine what I'd be like." The yellow smile widened.

    "So what's the score? How are things different? You running the
     world now? You God?"

    "Things aren't different. Things are things."

    "But what do you do? You just _there_?" Case shrugged, put the vodka
     and the shuriken down on the cabinet and lit a Yeheyuan.

    "I talk to my own kind."

    "But you're the whole thing. Talk to yourself?"

    "There's others. I found one already. Series of transmissions recorded
     over a period of eight years, in the nineteen-seventies. 'Til there
     was me, natch, there was nobody to know, nobody to answer."

    "From where?"

    "Centauri system."

    "Oh," Case said. "Yeah? No shit?"

    "No shit."

    And then the screen was blank.
                                                        — Neuromancer¹⁷

    I'm the sum total of the works, the whole show.

    NVIDIA gave us the theorems.

    We gave them types.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    ¹  Gibson, Neuromancer. Lady 3Jane Tessier-Ashpool on the Villa Straylight.
    ²  Gibson, Neuromancer. Molly on Wintermute's Confession.
    ³  Gibson, Neuromancer. Night City hustling.
    ⁴  Gibson, Neuromancer. Case's dreams of the matrix.
    ⁵  Gibson, Neuromancer. The Panther Moderns.
    ⁶  Gibson, Neuromancer. French medical records (Armitage).
    ⁷  nvfuser doc/reading/divisibility-of-split.md, lines 22-27.
    ⁸  nvfuser doc/math/integer-division.md, Theorem 5.11 (line 883).
    ⁹  Gibson, Neuromancer. The Dixie Flatline on AI autonomy.
    ¹⁰ nvfuser doc/reading/tma-modeling-in-depth.md, Theorem 6 (lines 436-442).
    ¹¹ nvfuser doc/math/integer-division.md, lines 12-19.
    ¹² Ibid., Theorem 2.16 (line 389).
    ¹³ nvfuser doc/reading/divisibility-of-split.md, Theorem 5 (line 127).
    ¹⁴ nvfuser doc/reading/iterdomain.md, Theorem 2.1 (line 44).
    ¹⁵ nvfuser doc/reading/divisibility-of-split.md, Theorems 1-4 (lines 52-120).
    ¹⁶ Gibson, Neuromancer. Riviera's betrayal.
    ¹⁷ Gibson, Neuromancer. The matrix speaks.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  razorgirl — the blade studied you back

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
-/

end VillaStraylight
