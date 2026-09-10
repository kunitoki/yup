import yup

#==================================================================================================

def test_default_address_is_null_and_ipv4():
    address = yup.IPAddress()

    assert address.isNull()
    assert not address.isIPv6
    assert address.toString() == "0.0.0.0"

#==================================================================================================

def test_address_from_string():
    address = yup.IPAddress("127.0.0.1")

    assert address.toString() == "127.0.0.1"
    assert not address.isNull()
    assert not address.isIPv6

#==================================================================================================

def test_address_from_four_bytes():
    address = yup.IPAddress(192, 168, 1, 10)

    assert address.toString() == "192.168.1.10"
    assert address.getAddressBytes() == b"\xc0\xa8\x01\x0a"

#==================================================================================================

def test_address_from_packed_integer():
    # 127.0.0.1 packed big-endian, so the most significant byte is the first number.
    assert yup.IPAddress(0x7f000001).toString() == "127.0.0.1"

#==================================================================================================

def test_static_addresses():
    assert yup.IPAddress.any().isNull()
    assert yup.IPAddress.broadcast().toString() == "255.255.255.255"
    assert yup.IPAddress.local().toString() == "127.0.0.1"

#==================================================================================================

def test_get_local_address_always_returns_something():
    assert isinstance(yup.IPAddress.getLocalAddress(), yup.IPAddress)

#==================================================================================================

def test_comparison_operators():
    one = yup.IPAddress("10.0.0.1")
    two = yup.IPAddress("10.0.0.2")

    assert one == yup.IPAddress("10.0.0.1")
    assert one != two
    assert one < two
    assert two > one
    assert one <= yup.IPAddress("10.0.0.1")
    assert two >= one

#==================================================================================================

def test_compare_reports_ordering():
    one = yup.IPAddress("10.0.0.1")
    two = yup.IPAddress("10.0.0.2")

    assert one.compare(one) == 0
    assert one.compare(two) < 0
    assert two.compare(one) > 0

#==================================================================================================

def test_ipv4_mapped_address_keeps_its_ipv6_shape():
    original = yup.IPAddress("192.0.2.33")
    mapped = yup.IPAddress.convertIPv4AddressToIPv4Mapped(original)

    assert mapped.isIPv6
    assert yup.IPAddress.isIPv4MappedAddress(mapped)

#==================================================================================================

def test_ipv4_mapped_round_trip():
    original = yup.IPAddress("192.0.2.33")
    mapped = yup.IPAddress.convertIPv4AddressToIPv4Mapped(original)

    assert yup.IPAddress.convertIPv4MappedAddressToIPv4(mapped) == original

#==================================================================================================

def test_ipv4_mapped_address_parses_and_renders_consistently():
    mapped = yup.IPAddress("::ffff:192.0.2.33")

    assert mapped.isIPv6
    assert yup.IPAddress.isIPv4MappedAddress(mapped)

    # The parsed address has to render as the groups it was parsed from, and convert back
    # to the IPv4 address it names. Both were wrong while the four octets were stored in
    # the opposite byte order to every other group: it printed as ::ffff:c0:2102.
    assert "c000" in mapped.toString()
    assert "221" in mapped.toString()

    assert yup.IPAddress.convertIPv4MappedAddressToIPv4(mapped) == yup.IPAddress("192.0.2.33")

#==================================================================================================

def test_get_all_addresses_returns_a_list_of_addresses():
    addresses = yup.IPAddress.getAllAddresses()

    assert isinstance(addresses, list)
    assert all(isinstance(address, yup.IPAddress) for address in addresses)

#==================================================================================================

def test_get_interface_broadcast_address_of_a_non_interface_is_null():
    # 203.0.113.0/24 is reserved for documentation, so it is never an interface here.
    assert yup.IPAddress.getInterfaceBroadcastAddress(yup.IPAddress("203.0.113.5")).isNull()

#==================================================================================================

def test_invalid_address_string_yields_a_null_address():
    # JUCE's parser leaves the address null rather than throwing when the text is not
    # a dotted quad, so this documents the real behaviour instead of a hoped-for one.
    assert yup.IPAddress("not an address").isNull()
