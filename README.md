# mruby-msgpack
msgpack mrbgem written in c++

## Install
add following line to build_config.rb
```ruby
	conf.gem 'path/to/mruby-msgpack'
```

## Usage
Use MsgPack.load to deserialize string to ruby object.
Use MsgPack.dump to serialize ruby object to string.

If you want to have a custom seralizer, define to_msgpack method that doesn't take argument
and returns string serialized with msgpack.

## TODO
* add tests for each type
* hash key sorting option in dumping
* option to use symbol in hash key
* UTF-8 checking
