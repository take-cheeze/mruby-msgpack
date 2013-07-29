assert('MsgPack.dump') {
  MsgPack.dump({ :compact => true, :schema => 0 }) == "\x82\xA7compact\xC3\xA6schema\x00"
}

assert('MsgPack.load') {
  MsgPack.load("\x82\xA7compact\xC3\xA6schema\x00") == { 'compact' => true, 'schema' => 0 }
}
