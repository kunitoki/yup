import sys
from pathlib import Path

tests_folder = str(Path(__file__).parent)

if tests_folder not in sys.path:
    sys.path.insert(0, tests_folder)
