// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright 2026 Weyl AI

#pragma once

// This header intentionally left minimal. It exists to ensure include order
// works with GCC 15 + CUDA when compiling CUTLASS/cute headers first in TU.
// Add any compiler- or CUDA-specific pragmas you need here.
#if defined(__CUDACC__) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

// (no-op shim)

#if defined(__CUDACC__) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
