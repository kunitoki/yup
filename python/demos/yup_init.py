import os
import sys
import glob
import time
import traceback
from pathlib import Path
from functools import wraps


try:
	import yup

except ImportError:
    folder = (Path(__file__).parent.parent / "build")
    for ext in ["*.so", "*.pyd"]:
        path_to_search = folder / "**" / ext
        for f in glob.iglob(str(path_to_search), recursive=True):
            if os.path.isfile(f):
                sys.path.append(str(Path(f).parent))
                break

    import yup


def timeit(func):
    @wraps(func)
    def timeit_wrapper(*args, **kwargs):
        start_time = time.perf_counter()
        result = func(*args, **kwargs)
        total_time = time.perf_counter() - start_time
        print(f'Function {func.__name__} Took {total_time:.4f} seconds') # {args} {kwargs}
        return result

    return timeit_wrapper
