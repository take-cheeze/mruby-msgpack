def dump_test(expected, given)
  assert_equal expected, MsgPack.dump(given)
end

def load_test(expected, given)
  assert_equal expected, MsgPack.load(given)
end

def dump_load_test(description, str, val)
  should('encode ' + description) { dump_test str, val }
  should('decode ' + description) { load_test val, str }
end

header 'basic types'

dump_load_test "boolean (true)", "\xC3", true
dump_load_test "boolean (false)", "\xC2", false
dump_load_test "nil", "\xC0", nil

dump_load_test "positive fixnum", "\x7F", 127
dump_load_test "negative fixnum", "\xE0", -32

dump_load_test "uint8", "\xCC\xFF", 2**8 - 1
dump_load_test "uint16", "\xCD\x7F\xFF", 2**15 - 1
dump_load_test "uint32", "\xCE\x00\x04\x00\x00", 2**18
dump_load_test "uint32 max", "\xCE\xFF\xFF\xFF\xFF", 2**32 - 1
dump_load_test "uint64", "\xCF\x00\x00\x00\x04\x00\x00\x00\x00", 2**34

dump_load_test "int8 min", "\xD0\x81", -2**7  + 1
dump_load_test "int16 min", "\xD1\x80\x01", -2**15 + 1
dump_load_test "int32 min", "\xD2\x80\x00\x00\x01", -2**31 + 1
