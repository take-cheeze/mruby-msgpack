
def hex_string(s)
  if s.respond_to?(:bytes)
    s.bytes.map{|c| format('%x', c) }
  else
    s
  end
end

def assert_equal(expected, given)
  if expected != given
    raise "Expected #{hex_string(expected)}, got #{hex_string(given)}"
  end
end


def header(str)
  puts "\n#{str}:"
end

def should(msg)
  spaces = 50 - msg.size
  if spaces < 0
    raise "raise spaces count in tool.rb"
  end
  
  print "  Should #{msg}... #{' ' * spaces}"
  yield
  puts "OK"
end

