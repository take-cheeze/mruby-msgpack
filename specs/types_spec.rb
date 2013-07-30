header 'Types encoding'

should("encode boolean (true) ")  { assert_equal "\xC3",                  MsgPack.dump(true) }
should("encode boolean (false) ") { assert_equal "\xC2",                  MsgPack.dump(false) }
should("encode nil")              { assert_equal "\xC0",                  MsgPack.dump(nil) }

should("encode positive fixnum")  { assert_equal "\x7F",                  MsgPack.dump(127) }
should("encode uint8")            { assert_equal "\xCC\xFF",              MsgPack.dump(2**8 - 1) }
should("encode uint16")           { assert_equal "\xCD\xFF\xFF",          MsgPack.dump(2**16 - 1) }
# should("encode uint32")           { assert_equal "\xCE\xFF\xFF\xFF\xFF",  MsgPack.dump(2**32 - 1) }

should("encode negative fixnum")  { assert_equal "\xE0",                  MsgPack.dump(-32) }
# should("encode int8")             { assert_equal "\xD0\x81",              MsgPack.dump(-2**7  + 1) }
# should("encode int16")            { assert_equal "\xD1\x80\x81",          MsgPack.dump(-2**15 + 1) }



header 'Type decoding'

should("decode boolean (true) ")  { assert_equal true,          MsgPack.load("\xC3") }
should("decode boolean (false) ") { assert_equal false,         MsgPack.load("\xC2") }
should("decode nil")              { assert_equal nil,           MsgPack.load("\xC0") }

should("decode positive fixnum")  { assert_equal 127,           MsgPack.load("\x7F") }
should("decode uint8")            { assert_equal 2**8 - 1,      MsgPack.load("\xCC\xFF") }
should("decode uint16")           { assert_equal 2**16 - 1,     MsgPack.load("\xCD\xFF\xFF") }
# should("decode uint32")           { assert_equal 2**32 - 1, MsgPack.load("\xCE\xFF\xFF\xFF\xFF") }

# should("decode negative fixnum")  { assert_equal -32,   MsgPack.load("\xE0") } # fails
# should("decode int8")             { assert_equal -2**7  + 1,    MsgPack.load("\xD0\x81") }

