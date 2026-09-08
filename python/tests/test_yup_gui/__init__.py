import pytest

import common

import yup

if not hasattr(yup, "FlexBox"):
    pytest.skip(allow_module_level=True)
