#!/usr/bin/env python3
"""One-time generator for the SLEEF per-ISA rename headers that SLEEF's build
generates with mkrename (src/libm/mkrename.c) from the function table in
src/libm/funcproto.h.

The module compiles exactly one ISA, so this emits the canonical, documented
cross-ISA public names without an ISA suffix:
  scalar  : xsinf_u1 -> Sleef_sinf_u10        (matches committed src/libm/rename.h)
  SIMD SP : xsinf_u1 -> Sleef_sinf4_u10
  SIMD DP : xsin_u1  -> Sleef_sind2_u10
Only the SIMD sections are needed by the per-ISA headers (renamesse2.h,
renameadvsimd.h) that sleefsimdsp.c / sleefsimddp.c include under DORENAME.

Usage: sleef_rename_gen.py funcproto.h > header.h
"""

import re
import sys

suffix = ['', '_u1', '_u05', '_u35', '_u15', '_u3500']

def func_list(path):
    rows = []
    for line in open(path):
        m = re.match(r'\s*\{\s*"([^"]*)"\s*,\s*(-?\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}', line)
        if not m:
            continue
        name = m.group(1)
        if name == 'NULL':
            continue
        rows.append((name, int(m.group(2)), int(m.group(3))))
    return rows

def main():
    functions = func_list(sys.argv[1])

    print('//   Generated once from SLEEF src/libm/funcproto.h for')
    print('//   thirdparty/sleef_library - do not edit by hand.')
    print('//   Regeneration recipe: see thirdparty/sleef_library/README.md.')
    print('//   Canonical names for a single-ISA static build (no ISA suffix):')
    print('//   DP carries d2, SP carries f4 (Sleef_sind2_u10, Sleef_sinf4_u10).')
    print('#ifndef DORENAME')
    print('#error "SLEEF rename headers are only included from SLEEF sources compiled with DORENAME"')
    print('#endif')
    print()

    for name, ulp, suf in functions:
        isuf = suffix[suf]
        if ulp >= 0:
            print(f'#define x{name}{isuf} Sleef_{name}d2_u{ulp:02d}')
        else:
            print(f'#define x{name} Sleef_{name}d2')

    print()

    for name, ulp, suf in functions:
        isuf = suffix[suf]
        if ulp >= 0:
            print(f'#define x{name}f{isuf} Sleef_{name}f4_u{ulp:02d}')
        else:
            print(f'#define x{name}f Sleef_{name}f4')

if __name__ == '__main__':
    main()
