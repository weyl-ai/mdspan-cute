// examples/swizzled_tile.cpp
//
// The Polyhedral Wizards at Play
//
// This example demonstrates mdspan-cute: C++23 bracket syntax
// with CUTLASS cute's swizzled shared memory layouts.
//
// Two decades of work, one line of code:
//   tile[row, col] = value;

#include <cute/swizzle.hpp>
#include <mdspan_cute.h>
#include <print>
#include <vector>

using namespace cute;

int main() {
  std::println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  std::println("  The Polyhedral Wizards at Play");
  std::println("  mdspan + cute: two decades of work, one line of code");
  std::println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  std::println("");

  // ═══════════════════════════════════════════════════════════════════════
  // Create a swizzled layout for bank-conflict-free shared memory access
  // ═══════════════════════════════════════════════════════════════════════

  constexpr int rows = 32;
  constexpr int cols = 32;

  // Swizzle<3,3,3> is optimal for 128-byte rows (32 floats)
  // This is cute's polyhedral layout algebra at work
  auto base_layout = make_layout(make_shape(Int<rows>{}, Int<cols>{}),
                                 make_stride(Int<cols>{}, Int<1>{}));

  auto swizzled_layout = composition(Swizzle<3, 3, 3>{}, base_layout);
  using swizzled_layout_type = decltype(swizzled_layout);

  // Allocate storage (would be __shared__ in a real kernel)
  std::vector<float> storage(rows * cols);

  // ═══════════════════════════════════════════════════════════════════════
  // Create mdspan with cute's swizzled layout
  // This is the bridge: mdspan syntax, cute addressing
  // ═══════════════════════════════════════════════════════════════════════

  using layout_policy = mdspan_cute::layout_cute<swizzled_layout_type>;
  using extents_type = std::extents<int, rows, cols>;
  using mapping_type = typename layout_policy::mapping<extents_type>;

  std::mdspan<float, extents_type, layout_policy> tile(
      storage.data(), mapping_type(swizzled_layout));

  // ═══════════════════════════════════════════════════════════════════════
  // Two decades of work, one line of code
  // ═══════════════════════════════════════════════════════════════════════

  // Write with C++23 bracket syntax - swizzle applied transparently
  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < cols; ++col) {
      tile[row, col] = static_cast<float>(row * 100 + col);
    }
  }

  // Read back - same clean syntax
  std::println("Sample values from swizzled tile:");
  std::println("  tile[0, 0]   = {}", tile[0, 0]);
  std::println("  tile[0, 1]   = {}", tile[0, 1]);
  std::println("  tile[1, 0]   = {}", tile[1, 0]);
  std::println("  tile[16, 16] = {}", tile[16, 16]);
  std::println("  tile[31, 31] = {}", tile[31, 31]);

  // ═══════════════════════════════════════════════════════════════════════
  // Show the swizzle pattern - why this matters
  // ═══════════════════════════════════════════════════════════════════════

  std::println("");
  std::println("Swizzle pattern (logical → physical offset):");
  std::println("Without swizzle, column 0 would cause 32-way bank conflicts.");
  std::println("With Swizzle<3,3,3>, each row XORs addresses differently:");
  std::println("");

  for (int row = 0; row < 4; ++row) {
    std::print("  row {}: ", row);
    for (int col = 0; col < 8; ++col) {
      int logical = row * cols + col;
      int physical = swizzled_layout(row, col);
      int bank = physical % 32;
      std::print("[{}→{}:b{}] ", logical, physical, bank);
    }
    std::println("");
  }

  std::println("");
  std::println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  std::println("  tile[row, col] = value;");
  std::println("  // Feautrier's polyhedra, Kerr's swizzles, Lelbach's syntax");
  std::println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

  return 0;
}
