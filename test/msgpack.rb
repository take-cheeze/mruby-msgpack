assert('MsgPack.dump') {
  MsgPack.dump({ :compact => true, :schema => 0 }) == "\x82\xA7compact\xC3\xA6schema\x00"
}

assert('MsgPack.load') {
  MsgPack.load("\x82\xA7compact\xC3\xA6schema\x00") == { 'compact' => true, 'schema' => 0 }
}

{
  'constant values' => [
    ['true', true, "\xC3"],
    ['false', false, "\xC2"],
    ['nil', nil, "\xC0"]
  ],
  'numbers' => [
    ['zero', 0, "\x00"],
    ['127', 127, "\x7F"],
    ['128', 128, "\xCC\x80"],
    ['256', 256, "\xCD\x01\x00"],
    ['-1', -1, "\xFF"],
    ['-33', -33, "\xD0\xDF"],
    ['-129', -129, "\xD1\xFF\x7F"],
    ['small integers', 42, "*"],
    ['medium integers', 333, "\xCD\x01M"],
    ['large integers', 2**31 - 1, "\xCE\x7F\xFF\xFF\xFF"],
    # ['huge integers', 2**64 - 1, "\xCF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"],
    ['negative integers', -1, "\xFF"],
    ['1.0', 1.0, "\xcb\x3f\xf0\x00\x00\x00\x00\x00\x00"],
    ['small floats', 3.14, "\xCB@\t\x1E\xB8Q\xEB\x85\x1F"],
    ['big floats', 3.141592653589793 * 1_000_000_000_000_000_000, "\xCBC\xC5\xCC\x96\xEF\xD1\x19%"],
    ['negative floats', -2.1, "\xCB\xC0\x00\xCC\xCC\xCC\xCC\xCC\xCD"]
  ],
  'strings' => [
    ['tiny strings', 'hello world', "\xABhello world"],
    ['short strings', 'hello' * 5, "\xB9hellohellohellohellohello"],
    ['empty strings', '', "\xA0"]
  ],
  # 'binary strings' => [
  #   ['tiny strings', 'hello world'.b, "\xC4\vhello world"],
  #   ['short strings', 'hello'.b * 5, "\xC4\x19hellohellohellohellohello"],
  #   ['empty strings', ''.b, "\xC4\x00"]
  # ],
  'arrays' => [
    ['empty arrays', [], "\x90"],
    ['arrays with strings', ["hello", "world"], "\x92\xA5hello\xA5world"],
    ['arrays with mixed values', ["hello", "world", 42], "\x93\xA5hello\xA5world*"],
    ['arrays of arrays', [[[[1, 2], 3], 4]], "\x91\x92\x92\x92\x01\x02\x03\x04"],
    ['empty arrays', [], "\x90"]
  ],
  'hashes' => [
    ['empty hashes', {}, "\x80"],
    ['hashes', {'foo' => 'bar'}, "\x81\xA3foo\xA3bar"],
    ['hashes with mixed keys and values', {'foo' => 'bar', 3 => 'three', 'four' => 4, 'x' => ['y'], 'a' => 'b'}, "\x85\xA3foo\xA3bar\x03\xA5three\xA4four\x04\xA1x\x91\xA1y\xA1a\xA1b"],
    ['hashes of hashes', {{'x' => {'y' => 'z'}} => 's'}, "\x81\x81\xA1x\x81\xA1y\xA1z\xA1s"],
    ['hashes with nils', {'foo' => nil}, "\x81\xA3foo\xC0"]
  ]
}.each do |ctx, cases|
  cases.each do |desc, unpacked, packed|
    next if MsgPack::SIZEOF_FLOAT != 8 && packed[0] == "\xCB"
    assert("#{ctx}: MsgPack encode #{desc}") do
      assert_equal(packed, MsgPack.dump(unpacked))
    end
    assert("#{ctx}: MsgPack decode #{desc}") do
      assert_equal(unpacked, MsgPack.load(packed))
    end
  end
end
