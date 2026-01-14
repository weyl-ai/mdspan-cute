// examples/dequant_gemm_1cta.cpp
//
// Double-Quantized GEMM: 1CTA Blackwell Style
//
// W4A8 with per-group scales, swizzled smem, f32 accumulation.
// Single CTA per SM - the sm_120 persistent kernel pattern.

#include <cute/swizzle.hpp>
#include <cute/layout.hpp>
#include <cute/tensor.hpp>
#include <mdspan_cute.h>

#include <array>
#include <cstdint>
#include <print>
#include <vector>

using namespace cute;

// ═══════════════════════════════════════════════════════════════════════════
// Tile shapes - 1CTA means we own the whole SM
// ═══════════════════════════════════════════════════════════════════════════

constexpr int CTA_M = 128;
constexpr int CTA_N = 128;
constexpr int CTA_K = 64;

// Quantization: weights are int4, activations are int8
// Scales are fp16, one per GROUP_SIZE elements
constexpr int GROUP_SIZE = 64;
constexpr int STAGES = 2;

// ═══════════════════════════════════════════════════════════════════════════
// Swizzled layouts for bank-conflict-free access
// ═══════════════════════════════════════════════════════════════════════════

// A matrix: [CTA_M, CTA_K] int8 activations
// 64 columns × 1 byte = 64 bytes per row, use Swizzle<2,3,3>
using SmemLayoutA = decltype(
    composition(Swizzle<2, 3, 3>{},
                make_layout(make_shape(Int<CTA_M>{}, Int<CTA_K>{}),
                           make_stride(Int<CTA_K>{}, Int<1>{}))));

// B matrix: [CTA_K, CTA_N] int4 weights (packed pairs)
// 128 columns / 2 = 64 bytes per row
using SmemLayoutB = decltype(
    composition(Swizzle<2, 3, 3>{},
                make_layout(make_shape(Int<CTA_K>{}, Int<CTA_N / 2>{}),
                           make_stride(Int<CTA_N / 2>{}, Int<1>{}))));

// Scale tiles: [CTA_M, num_groups] and [CTA_K, num_groups] fp16
// Small enough that swizzling doesn't matter much
constexpr int K_GROUPS = CTA_K / GROUP_SIZE;

using SmemLayoutScaleA = decltype(
    make_layout(make_shape(Int<CTA_M>{}, Int<K_GROUPS>{}),
                make_stride(Int<K_GROUPS>{}, Int<1>{})));

using SmemLayoutScaleB = decltype(
    make_layout(make_shape(Int<CTA_N>{}, Int<K_GROUPS>{}),
                make_stride(Int<K_GROUPS>{}, Int<1>{})));

// ═══════════════════════════════════════════════════════════════════════════
// mdspan-cute bridge types
// ═══════════════════════════════════════════════════════════════════════════

template <typename T, typename CuteLayout>
struct TileView {
    using layout_policy = mdspan_cute::layout_cute<CuteLayout>;
    using extents_type = std::extents<int, 
        size<0>(CuteLayout{}), 
        size<1>(CuteLayout{})>;
    using mapping_type = typename layout_policy::template mapping<extents_type>;
    using mdspan_type = std::mdspan<T, extents_type, layout_policy>;
    
    static constexpr int Rows = size<0>(CuteLayout{});
    static constexpr int Cols = size<1>(CuteLayout{});
};

using ATile = TileView<int8_t, SmemLayoutA>;
using BTile = TileView<uint8_t, SmemLayoutB>;  // packed int4 pairs
using ScaleATile = TileView<uint16_t, SmemLayoutScaleA>;  // fp16 as bits
using ScaleBTile = TileView<uint16_t, SmemLayoutScaleB>;

// ═══════════════════════════════════════════════════════════════════════════
// Shared memory - what one CTA owns
// ═══════════════════════════════════════════════════════════════════════════

struct alignas(128) SharedStorage {
    // Double-buffered operands
    std::array<std::array<std::array<int8_t, CTA_K>, CTA_M>, STAGES> A;
    std::array<std::array<std::array<uint8_t, CTA_N / 2>, CTA_K>, STAGES> B;
    
    // Double-buffered scales
    std::array<std::array<std::array<uint16_t, K_GROUPS>, CTA_M>, STAGES> scale_A;
    std::array<std::array<std::array<uint16_t, K_GROUPS>, CTA_N>, STAGES> scale_B;
};

// ═══════════════════════════════════════════════════════════════════════════
// Host-side simulation of the indexing patterns
// This validates the swizzle math without needing a GPU
// ═══════════════════════════════════════════════════════════════════════════

void simulate_mainloop() {
    std::println("═══════════════════════════════════════════════════════════════");
    std::println("  Double-Quantized GEMM Tile Access Patterns");
    std::println("  1CTA style: {}×{}×{}, {} stages", CTA_M, CTA_N, CTA_K, STAGES);
    std::println("═══════════════════════════════════════════════════════════════");
    std::println("");
    
    // Allocate "shared memory"
    SharedStorage smem{};
    
    // Initialize with recognizable patterns
    for (int s = 0; s < STAGES; ++s) {
        for (int m = 0; m < CTA_M; ++m) {
            for (int k = 0; k < CTA_K; ++k) {
                smem.A[s][m][k] = static_cast<int8_t>((m + k + s * 100) % 127);
            }
        }
        for (int k = 0; k < CTA_K; ++k) {
            for (int n = 0; n < CTA_N / 2; ++n) {
                // Pack two int4s: low nibble = n%16, high nibble = k%16
                smem.B[s][k][n] = static_cast<uint8_t>((k % 16) << 4 | (n % 16));
            }
        }
        for (int m = 0; m < CTA_M; ++m) {
            for (int g = 0; g < K_GROUPS; ++g) {
                // Scale pattern: encodes position
                smem.scale_A[s][m][g] = static_cast<uint16_t>(m * 10 + g + s * 1000);
            }
        }
        for (int n = 0; n < CTA_N; ++n) {
            for (int g = 0; g < K_GROUPS; ++g) {
                smem.scale_B[s][n][g] = static_cast<uint16_t>(n * 10 + g + s * 1000);
            }
        }
    }
    
    // ─────────────────────────────────────────────────────────────────────────
    // Create swizzled mdspan views - THE POINT OF THIS WHOLE EXERCISE
    // ─────────────────────────────────────────────────────────────────────────
    
    auto make_A_view = [&](int stage) {
        return ATile::mdspan_type(
            &smem.A[stage][0][0],
            ATile::mapping_type(SmemLayoutA{}));
    };
    
    auto make_B_view = [&](int stage) {
        return BTile::mdspan_type(
            &smem.B[stage][0][0],
            BTile::mapping_type(SmemLayoutB{}));
    };
    
    auto make_scale_A_view = [&](int stage) {
        return ScaleATile::mdspan_type(
            &smem.scale_A[stage][0][0],
            ScaleATile::mapping_type(SmemLayoutScaleA{}));
    };
    
    auto make_scale_B_view = [&](int stage) {
        return ScaleBTile::mdspan_type(
            &smem.scale_B[stage][0][0],
            ScaleBTile::mapping_type(SmemLayoutScaleB{}));
    };
    
    // ─────────────────────────────────────────────────────────────────────────
    // Simulate one mainloop iteration
    // ─────────────────────────────────────────────────────────────────────────
    
    constexpr int stage = 0;
    
    auto A = make_A_view(stage);
    auto B = make_B_view(stage);
    auto sA = make_scale_A_view(stage);
    auto sB = make_scale_B_view(stage);
    
    std::println("Tile dimensions:");
    std::println("  A:       [{}, {}] int8  (activations)", ATile::Rows, ATile::Cols);
    std::println("  B:       [{}, {}] uint8 (packed int4 weights)", BTile::Rows, BTile::Cols);
    std::println("  scale_A: [{}, {}] fp16", ScaleATile::Rows, ScaleATile::Cols);
    std::println("  scale_B: [{}, {}] fp16", ScaleBTile::Rows, ScaleBTile::Cols);
    std::println("");
    
    // ─────────────────────────────────────────────────────────────────────────
    // Show swizzle patterns for A tile
    // ─────────────────────────────────────────────────────────────────────────
    
    std::println("A tile swizzle pattern (first 4 rows, 8 cols):");
    std::println("  Swizzle<2,3,3> on 64-byte rows");
    std::println("");
    
    SmemLayoutA layout_A{};
    for (int m = 0; m < 4; ++m) {
        std::print("  row {:2d}: ", m);

        for (int k = 0; k < 8; ++k) {
            int logical = m * CTA_K + k;
            int physical = layout_A(m, k);
            int bank = physical % 32;
            std::print("[{:3d}→{:3d}:b{:2d}] ", logical, physical, bank);
        }

        std::println("");
    }

    std::println("");
    
    // ─────────────────────────────────────────────────────────────────────────
    // The actual access pattern a WGMMA would use
    // ─────────────────────────────────────────────────────────────────────────
    
    std::println("Simulated WGMMA access (8×8 tile at [64,32]):");
    std::println("");
    
    constexpr int base_m = 64;
    constexpr int base_k = 32;
    constexpr int base_n = 48;
    
    float accum = 0.0f;
    
    for (int dm = 0; dm < 8; ++dm) {
        for (int dk = 0; dk < 8; ++dk) {
            int m = base_m + dm;
            int k = base_k + dk;
            int n = base_n + dk;  // for outer product
            
            // ═══════════════════════════════════════════════════════════════
            // THIS IS IT: clean syntax, swizzled addressing
            // ═══════════════════════════════════════════════════════════════
            
            int8_t a_val = A[m, k];
            uint8_t b_packed = B[k, n / 2];
            
            // Unpack int4
            int b_lo = b_packed & 0xF;
            int b_hi = b_packed >> 4;
            int b_val = (n % 2 == 0) ? b_lo : b_hi;
            
            // Get scales for this group
            int group = k / GROUP_SIZE;
            uint16_t sa_bits = sA[m, group];
            uint16_t sb_bits = sB[n, group];
            
            // In real code: __half2float. Here just use the bits as proxy.
            float scale_a = static_cast<float>(sa_bits) / 1000.0f;
            float scale_b = static_cast<float>(sb_bits) / 1000.0f;
            
            // Double dequantize and accumulate
            float dequant = scale_a * scale_b * static_cast<float>(a_val) * static_cast<float>(b_val);
            accum += dequant;
        }
    }
    
    std::println("  A[{}, {}] through A[{}, {}]", base_m, base_k, base_m + 7, base_k + 7);
    std::println("  B[{}, {}] through B[{}, {}]", base_k, base_n / 2, base_k + 7, (base_n + 7) / 2);
    std::println("  Accumulated (proxy): {:.4f}", accum);
    std::println("");
    
    // ─────────────────────────────────────────────────────────────────────────
    // Verify swizzle correctness: read back what we wrote
    // ─────────────────────────────────────────────────────────────────────────
    
    std::println("Verification - values round-trip through swizzle:");
    
    bool all_correct = true;
    for (int m = 0; m < CTA_M; ++m) {
        for (int k = 0; k < CTA_K; ++k) {
            int8_t expected = static_cast<int8_t>((m + k + stage * 100) % 127);
            int8_t actual = A[m, k];
            if (expected != actual) {
                std::println("  MISMATCH at [{}, {}]: expected {}, got {}", 
                           m, k, expected, actual);
                all_correct = false;
            }
        }
    }
    
    if (all_correct) {
        std::println("  ✓ All {}×{} = {} A tile elements correct", 
                   CTA_M, CTA_K, CTA_M * CTA_K);
    }
    
    all_correct = true;
    for (int k = 0; k < CTA_K; ++k) {
        for (int n = 0; n < CTA_N / 2; ++n) {
            uint8_t expected = static_cast<uint8_t>((k % 16) << 4 | (n % 16));
            uint8_t actual = B[k, n];
            if (expected != actual) {
                std::println("  MISMATCH at [{}, {}]: expected {}, got {}", 
                           k, n, expected, actual);
                all_correct = false;
            }
        }
    }
    
    if (all_correct) {
        std::println("  ✓ All {}×{} = {} B tile elements correct", 
                   CTA_K, CTA_N / 2, CTA_K * CTA_N / 2);
    }
    
    std::println("");
    std::println("═══════════════════════════════════════════════════════════════");
    std::println("  A[m, k] = value;  // Swizzle<2,3,3> applied transparently");
    std::println("  B[k, n] = packed; // Different shape, same syntax");
    std::println("  scale[i, group] = s; // Layout algebra composes");
    std::println("═══════════════════════════════════════════════════════════════");
}

// ═══════════════════════════════════════════════════════════════════════════
// Bank conflict analysis
// ═══════════════════════════════════════════════════════════════════════════

void analyze_bank_conflicts() {
    std::println("");
    std::println("═══════════════════════════════════════════════════════════════");
    std::println("  Bank Conflict Analysis");
    std::println("═══════════════════════════════════════════════════════════════");
    std::println("");
    
    SmemLayoutA layout_A{};
    SmemLayoutB layout_B{};
    
    // Simulate 32 threads accessing column 0 (worst case without swizzle)
    std::println("Column 0 access pattern (32 consecutive rows):");
    std::println("Without swizzle: all 32 threads hit bank 0 → 32-way conflict");
    std::println("With Swizzle<2,3,3>:");
    std::println("");
    
    std::array<int, 32> bank_histogram{};
    
    for (int m = 0; m < 32; ++m) {
        int physical = layout_A(m, 0);
        int bank = physical % 32;
        bank_histogram[bank]++;
        std::print("  thread {:2d} → row {:2d}, col 0 → offset {:4d} → bank {:2d}\n",
                 m, m, physical, bank);
    }
    
    std::println("");
    std::println("Bank histogram:");
    int max_conflict = 0;
    for (int b = 0; b < 32; ++b) {
        if (bank_histogram[b] > 0) {
            std::println("  bank {:2d}: {} accesses", b, bank_histogram[b]);
            max_conflict = std::max(max_conflict, bank_histogram[b]);
        }
    }
    std::println("");
    std::println("Max conflicts per bank: {} (ideal: 1, without swizzle: 32)", max_conflict);
}

int main() {
    simulate_mainloop();
    analyze_bank_conflicts();
    return 0;
}
