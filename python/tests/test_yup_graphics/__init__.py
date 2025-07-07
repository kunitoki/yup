import pytest

from .. import common

import yup

if not hasattr(yup, "Graphics"):
    pytest.skip(allow_module_level=True)
