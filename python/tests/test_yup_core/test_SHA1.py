import yup

#==================================================================================================

def test_sha1_of_empty_input():
    assert yup.SHA1(b"").toHexString() == "da39a3ee5e6b4b0d3255bfef95601890afd80709"

#==================================================================================================

def test_sha1_of_abc():
    assert yup.SHA1(b"abc").toHexString() == "a9993e364706816aba3e25717850c26c9cd0d89d"

#==================================================================================================

def test_sha1_of_text_matches_bytes():
    text = "The quick brown fox jumps over the lazy dog"

    assert yup.SHA1(text).toHexString() == "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12"
    assert yup.SHA1(text).toHexString() == yup.SHA1(text.encode("utf8")).toHexString()

#==================================================================================================

def test_sha1_raw_data_is_twenty_bytes():
    raw = yup.SHA1(b"abc").getRawData()

    assert isinstance(raw, bytes)
    assert len(raw) == 20
    assert raw.hex() == "a9993e364706816aba3e25717850c26c9cd0d89d"

#==================================================================================================

def test_sha1_from_memory_block():
    block = yup.MemoryBlock(b"abc")
    assert yup.SHA1(block).toHexString() == yup.SHA1(b"abc").toHexString()

#==================================================================================================

def test_sha1_default_constructor_is_not_the_hash_of_nothing():
    # The default constructor fills the hash with zeros, which is deliberately not
    # the same as hashing an empty block of data.
    default_hash = yup.SHA1()

    assert default_hash.toHexString() != yup.SHA1(b"").toHexString()
    assert default_hash.toHexString() == "00" * 20

#==================================================================================================

def test_sha1_equality_and_inequality():
    assert yup.SHA1(b"abc") == yup.SHA1(b"abc")
    assert yup.SHA1(b"abc") != yup.SHA1(b"abd")
    assert yup.SHA1(b"abc") == yup.SHA1("abc")

#==================================================================================================

def test_sha1_of_file(tmp_path):
    payload = b"The quick brown fox jumps over the lazy dog"

    path = tmp_path / "sha1-input.txt"
    path.write_bytes(payload)

    assert yup.SHA1(yup.File(str(path))).toHexString() == yup.SHA1(payload).toHexString()
