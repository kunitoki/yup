import pytest

import yup

if not hasattr(yup, "YuiDocument"):
    pytest.skip(allow_module_level=True)
