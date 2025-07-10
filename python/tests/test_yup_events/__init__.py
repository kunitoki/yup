import pytest
import sys

from .. import common

import yup

if sys.platform != "darwin" or not hasattr(yup, "MessageManager") or not hasattr(yup, "YUPApplication"):
    pytest.skip(allow_module_level=True)
