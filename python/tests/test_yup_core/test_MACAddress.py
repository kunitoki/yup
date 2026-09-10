import pytest

import yup

#==================================================================================================

def test_default_address_is_null():
    address = yup.MACAddress()

    assert address.isNull()
    assert address.toInt64() == 0
    assert address.getBytes() == b"\x00\x00\x00\x00\x00\x00"

#==================================================================================================

def test_address_from_six_bytes():
    address = yup.MACAddress(b"\x11\x22\x33\x44\x55\x66")

    assert not address.isNull()
    assert address.getBytes() == b"\x11\x22\x33\x44\x55\x66"
    assert address.toString() == "11-22-33-44-55-66"

#==================================================================================================

def test_address_bytes_may_be_padded():
    # The constructor reads the first six bytes and ignores anything after them.
    address = yup.MACAddress(b"\x11\x22\x33\x44\x55\x66\x77\x88")

    assert address.getBytes() == b"\x11\x22\x33\x44\x55\x66"

#==================================================================================================

def test_to_string_uses_the_requested_separator():
    address = yup.MACAddress(b"\x11\x22\x33\x44\x55\x66")

    assert address.toString(":") == "11:22:33:44:55:66"
    assert address.toString("") == "112233445566"

#==================================================================================================

def test_to_int64_is_little_endian():
    # The first byte of the address lands in the least significant byte.
    address = yup.MACAddress(b"\x11\x22\x33\x44\x55\x66")

    assert address.toInt64() == 0x665544332211

#==================================================================================================

def test_comparison_operators():
    one = yup.MACAddress(b"\x11\x22\x33\x44\x55\x66")
    two = yup.MACAddress(b"\x11\x22\x33\x44\x55\x66")
    three = yup.MACAddress(b"\x11\x22\x33\x44\x55\x67")

    assert one == two
    assert one != three

#==================================================================================================

def test_short_buffer_is_rejected():
    with pytest.raises(ValueError):
        yup.MACAddress(b"\x01\x02")

#==================================================================================================

def test_get_all_addresses_returns_a_list_of_addresses():
    addresses = yup.MACAddress.getAllAddresses()

    assert isinstance(addresses, list)
    assert all(isinstance(address, yup.MACAddress) for address in addresses)
