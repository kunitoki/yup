import os
import subprocess
import sys

import pytest

import yup

"""
A Python class that derives from two bound YUP types (`class X(yup.Component, yup.Timer)`, the
direct translation of the JUCE idiom) used to corrupt memory on destruction: the instance is
torn down through pybind11's multiple-inheritance value_and_holder layout, and the dealloc walk
reaches `~PyComponent()` with a stale `this` and segfaults.

The pattern is exercised in a child interpreter on purpose - the failure mode is a signal, not an
exception, so running it in-process would take the whole suite down with it. Each test therefore
asserts on the child's exit status:

  * exit 0    - the pattern is fully supported
  * exit != 0 - it was rejected loudly (the class construction raised), which is safe
  * killed by a signal (negative returncode) - it corrupted memory, which is the bug

`test_two_bound_bases_never_corrupt_memory` is marked `xfail` because the vendored pybind11
(pybind11 3.0.1) still has the bug; it will flip to `xpassed` on its own once the bindings are
fixed, which is the signal to delete the marker.
"""


def run_in_child_interpreter(source: str) -> subprocess.CompletedProcess:
    """Runs source in a fresh interpreter, with the bound yup module importable."""
    yup_folder = os.path.dirname(os.path.abspath(__file__))
    preamble = f"import sys; sys.path.insert(0, {yup_folder!r})\n"

    return subprocess.run(
        [sys.executable, "-c", preamble + source],
        capture_output=True,
        text=True,
        timeout=60,
    )


def describe(result: subprocess.CompletedProcess) -> str:
    direction = (
        f"killed by signal {-result.returncode}" if result.returncode < 0 else f"exit {result.returncode}"
    )

    return f"{direction}\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}"


SINGLE_BOUND_BASE = """
import yup

class OneBase(yup.Component):
    pass

for _ in range(3):
    component = OneBase()
    del component

print("survived")
"""

TWO_BOUND_BASES = """
import yup

class TwoBases(yup.Component, yup.Timer):
    def timerCallback(self):
        pass

for _ in range(3):
    component = TwoBases()
    del component

print("survived")
"""


def test_single_bound_base_survives_destruction():
    result = run_in_child_interpreter(SINGLE_BOUND_BASE)

    assert result.returncode == 0, f"a single bound base must keep working\n{describe(result)}"
    assert "survived" in result.stdout


@pytest.mark.xfail(
    strict=False,
    reason="pybind11 3.0.1 (vendored) corrupts the teardown of a Python class with two bound bases",
)
def test_two_bound_bases_never_corrupt_memory():
    result = run_in_child_interpreter(TWO_BOUND_BASES)

    assert result.returncode >= 0, (
        f"the interpreter was killed by a signal instead of reporting an error\n{describe(result)}"
    )

    if result.returncode == 0:
        assert "survived" in result.stdout
    else:
        # A refusal is fine, as long as it names what it refused.
        assert "multiple" in (result.stdout + result.stderr).lower(), describe(result)
