import pytest

from .. import common

import yup

if yup.__embedded_interpreter__ or not hasattr(yup, "MessageManager") or not hasattr(yup, "YUPApplication"):
    pytest.skip(allow_module_level=True)
