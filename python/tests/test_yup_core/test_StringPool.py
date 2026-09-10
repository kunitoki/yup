import yup

#==================================================================================================

def test_pooled_string_compares_equal_to_the_original():
    pool = yup.StringPool()

    assert pool.getPooledString("hello") == "hello"

#==================================================================================================

def test_pooled_strings_are_stable():
    pool = yup.StringPool()

    first = pool.getPooledString("repeated")
    second = pool.getPooledString("repeated")

    assert first == second

#==================================================================================================

def test_pool_handles_each_overload():
    pool = yup.StringPool()

    assert pool.getPooledString("from a string") == "from a string"
    assert pool.getPooledString(b"from a c string".decode("ascii")) == "from a c string"

#==================================================================================================

def test_garbage_collect_keeps_pooled_strings_addressable():
    pool = yup.StringPool()

    pooled = pool.getPooledString("still here")
    pool.garbageCollect()

    assert pooled == "still here"
    assert pool.getPooledString("still here") == "still here"

#==================================================================================================

def test_global_pool_is_shared():
    pool = yup.StringPool.getGlobalPool()

    assert pool.getPooledString("global") == "global"
    assert yup.StringPool.getGlobalPool().getPooledString("global") == "global"
