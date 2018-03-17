#include <mruby.h>
#include <mruby/array.h>
#include <mruby/hash.h>
#include <mruby/string.h>

#include <algorithm>
#include <limits>
#include <type_traits>

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace {

#define not_impl(M, msg, ...) \
  mrb_raisef(M, mrb_class_get(M, "NotImplementedError"), msg, __VA_ARGS__)

template<class T, class V>
bool check_min_max(V const v) {
  return std::numeric_limits<T>::min() <= v and v <= std::numeric_limits<T>::max();
}

template<class T, class V>
bool check_max(V const v) {
  return v <= std::numeric_limits<T>::max();
}

uint8_t read_byte(mrb_state* M, char const*& src, char const* end) {
  if(src >= end) mrb_raise(M, mrb_class_get(M, "RangeError"), "read_byte error");
  return *(src++);
}

mrb_value load(mrb_state* M, char const*& src, char const* end);

mrb_value read_map(mrb_state* M, char const*& src, char const* end, size_t const size) {
  mrb_value ret = mrb_hash_new_capa(M, size);
  for(size_t i = 0; i < size; ++i) {
    mrb_value key = load(M, src, end);
    mrb_value val = load(M, src, end);
    mrb_hash_set(M, ret, key, val);
  }
  return ret;
}

mrb_value read_raw(mrb_state* M, char const*& src, char const* end, size_t const size) {
  mrb_value ret = mrb_str_buf_new(M, size);
  mrb_str_resize(M, ret, size);
  for(size_t i = 0; i < size; ++i) { RSTRING_PTR(ret)[i] = read_byte(M, src, end); }
  return ret;
}

mrb_value read_array(mrb_state* M, char const*& src, char const* end, size_t const size) {
  mrb_value ret = mrb_ary_new_capa(M, size);
  for(size_t i = 0; i < size; ++i) { mrb_ary_push(M, ret, load(M, src, end)); }
  return ret;
}

template<class T>
T read_int(mrb_state* M, char const*& src, char const* end) {
  T ret = 0;
  for(size_t i = 0; i < sizeof(T); ++i) {
    ret |= static_cast<typename std::make_unsigned<T>::type>(read_byte(M, src, end)) << (8 * (sizeof(T) - i - 1));
  }
  return ret;
}

template<class T>
mrb_value float_fallback(mrb_state* M, T const val) {
  static_assert(std::numeric_limits<T>::is_integer, "input must be integer");
  return
      check_min_max<mrb_int>(val)? mrb_fixnum_value(val):
      check_min_max<mrb_float>(val)? mrb_float_value(M, val):
      (mrb_raise(M, mrb_class_get(M, "RangeError"), "integer out of range"),
       mrb_nil_value());
}

mrb_value load(mrb_state* M, char const*& src, char const* end) {
  uint8_t const tag = read_byte(M, src, end);

  switch(tag) {
    case 0xC0: return mrb_nil_value();
    case 0xC2: return mrb_false_value();
    case 0xC3: return mrb_true_value();

    case 0xCA: { // float
      uint32_t const val = read_int<uint32_t>(M, src, end);
      float converted;
      std::memcpy(&converted, &val, sizeof(float));
      return mrb_float_value(M, static_cast<mrb_float>(converted));
    }
    case 0xCB: { // double
      uint64_t const val = read_int<uint64_t>(M, src, end);
      double converted;
      std::memcpy(&converted, &val, sizeof(double));
      return mrb_float_value(M, static_cast<mrb_float>(converted));
    }

    // mrb_int is at least int16_t
    case 0xCC: return mrb_fixnum_value(read_int<uint8_t>(M, src, end));
    case 0xCD: return float_fallback(M, read_int<uint16_t>(M, src, end));
    case 0xCE: return float_fallback(M, read_int<uint32_t>(M, src, end));
    case 0xCF: return float_fallback(M, read_int<uint64_t>(M, src, end));
    case 0xD0: return mrb_fixnum_value(read_int<int8_t>(M, src, end));
    case 0xD1: return mrb_fixnum_value(read_int<int16_t>(M, src, end));
    case 0xD2: return float_fallback(M, read_int<int32_t>(M, src, end));
    case 0xD3: return float_fallback(M, read_int<int64_t>(M, src, end));

    case 0xDA: return read_raw(M, src, end, read_int<uint16_t>(M, src, end));
    case 0xDB: return read_raw(M, src, end, read_int<uint32_t>(M, src, end));
    case 0xDC: return read_array(M, src, end, read_int<uint16_t>(M, src, end));
    case 0xDD: return read_array(M, src, end, read_int<uint32_t>(M, src, end));
    case 0xDE: return read_map(M, src, end, read_int<uint16_t>(M, src, end));
    case 0xDF: return read_map(M, src, end, read_int<uint32_t>(M, src, end));

    default:
      return
          (/* 0 <= tag and */ tag <= 0x7f)? mrb_fixnum_value(tag):
          (0x80 <= tag and tag <= 0x8F)? read_map(M, src, end, tag & 0x0f):
          (0x90 <= tag and tag <= 0x9F)? read_array(M, src, end, tag & 0x0f):
          (0xA0 <= tag and tag <= 0xBf)? read_raw(M, src, end, tag & 0x1f):
          (0xE0 <= tag /* and tag <= 0xff */)? mrb_fixnum_value(~0xff | tag):
          (not_impl(M, "unsupported type tag: %S", mrb_fixnum_value(tag)), mrb_nil_value());
  }
}

template<uint8_t T>
mrb_value const& write_tag(mrb_state* M, mrb_value const& out) {
  char const buf[] = {char(T)};
  return mrb_str_buf_cat(M, out, buf, sizeof(buf)), out;
}

template<class T>
mrb_value const& write_int(mrb_state* M, mrb_value const& out, T v) {
  char buf[sizeof(T)];
  for(size_t i = 0; i < sizeof(T); ++i) {
    buf[i] = (v >> (8 * (sizeof(T) - i - 1))) & 0xff;
  }
  return mrb_str_buf_cat(M, out, buf, sizeof(buf)), out;
}

template<uint8_t tag, class T>
mrb_value const& write_int_with_tag(mrb_state* M, mrb_value const& out, T v) {
  return write_int<T>(M, write_tag<tag>(M, out), v);
}

template<uint8_t FixBase, uint8_t FixRange, uint8_t NonFixTag>
void write_tag_with_fix(mrb_state* M, mrb_value const& out, size_t const size) {
  (size < FixRange)
      ? write_int<uint8_t>(M, out, FixBase + size):
      check_min_max<uint16_t>(size)
      ? write_int<uint16_t>(M, write_int<uint8_t>(M, out, NonFixTag + 0), size):
      check_min_max<uint32_t>(size)
      ? write_int<uint32_t>(M, write_int<uint8_t>(M, out, NonFixTag + 1), size):
      (mrb_raisef(M, mrb_class_get(M, "RangeError"),
                  "size too big: %S (max: 0xffffffff)", mrb_float_value(M, size)), out);
}

#define PP_write_int(tag, type) \
  check_min_max<type>(num)? write_int_with_tag<tag, type>(M, out, num):
#define PP_write_uint(tag, type) \
  check_max<type>(num)? write_int_with_tag<tag, type>(M, out, num):
#define range_error                                                     \
  (mrb_raisef(M, mrb_class_get(M, "RangeError"),                        \
              "integer out of range: %S", mrb_float_value(M, num)), out)

template<class T>
mrb_value const& dump_integer(mrb_state* M, mrb_value const& out, T const num) {
  return
      (-32 <= num and num <= 0x7f)? write_int<uint8_t>(M, out, static_cast<mrb_int>(num) & 0xff):
      (num >= 0)? (
          PP_write_uint(0xCC, uint8_t)
          PP_write_uint(0xCD, uint16_t)
          PP_write_uint(0xCE, uint32_t)
          PP_write_uint(0xCF, uint64_t)
          range_error):
      PP_write_int(0xD0, int8_t)
      PP_write_int(0xD1, int16_t)
      PP_write_int(0xD2, int32_t)
      PP_write_int(0xD3, int64_t)
      range_error;
}

#undef range_error
#undef PP_write_uint
#undef PP_write_int

mrb_value const& dump(mrb_state* M, mrb_value const& out, mrb_value const& v) {
  if(mrb_nil_p(v)) { return write_tag<0xC0>(M, out); }

  switch(mrb_vtype(mrb_type(v))) {
    case MRB_TT_FALSE: return write_tag<0xC2>(M, out);
    case MRB_TT_TRUE : return write_tag<0xC3>(M, out);

    case MRB_TT_FIXNUM: return dump_integer(M, out, mrb_fixnum(v));

    case MRB_TT_FLOAT: {
      mrb_float num = mrb_float(v);
      char* ptr = reinterpret_cast<char*>(&num);
#ifndef MRB_ENDIAN_BIG
      for(size_t i = 0; i < sizeof(mrb_float) / 2; ++i) {
        std::swap(ptr[i], ptr[sizeof(mrb_float) - i - 1]);
      }
#endif
      write_tag<(sizeof(num) == 4)? 0xCA : 0xCB>(M, out);
      return mrb_str_buf_cat(M, out, ptr, sizeof(mrb_float)), out;
    }

    case MRB_TT_ARRAY:
      write_tag_with_fix<0x90, 0x10, 0xDC>(M, out, RARRAY_LEN(v));
      for(mrb_int i = 0; i < RARRAY_LEN(v); ++i) { dump(M, out, RARRAY_PTR(v)[i]); }
      return out;

    case MRB_TT_HASH: {
      mrb_value const keys = mrb_hash_keys(M, v);
      write_tag_with_fix<0x80, 0x10, 0xDE>(M, out, RARRAY_LEN(keys));
      for(mrb_int i = 0; i < RARRAY_LEN(keys); ++i) {
        dump(M, out, RARRAY_PTR(keys)[i]);
        dump(M, out, mrb_hash_get(M, v, RARRAY_PTR(keys)[i]));
      }
      return out;
    }

    case MRB_TT_SYMBOL: {
      mrb_int len;
      char const* str = mrb_sym2name_len(M, mrb_symbol(v), &len);
      write_tag_with_fix<0xA0, 0x20, 0xDA>(M, out, len);
      return mrb_str_buf_cat(M, out, str, len), out;
    }

    case MRB_TT_STRING:
      write_tag_with_fix<0xA0, 0x20, 0xDA>(M, out, RSTRING_LEN(v));
      mrb_str_concat(M, out, v);
      return out;

    default: {
      mrb_sym const func_sym = mrb_intern_lit(M, "to_msgpack");
      if(not mrb_obj_respond_to(M, mrb_obj_class(M, v), func_sym)) {
        return not_impl(M, "cannot dump object with class \"%S\"",
                        mrb_str_new_cstr(M, mrb_obj_classname(M, v))), out;
      } else {
        return mrb_str_concat(M, out, mrb_funcall_argv(M, v, func_sym, 0, NULL)), out;
      }
    }
  }
}

mrb_value msgpack_dump(mrb_state* M, mrb_value self) {
  mrb_value v;
  mrb_get_args(M, "o", &v);
  return dump(M, mrb_str_new(M, "", 0), v);
}

mrb_value msgpack_load(mrb_state* M, mrb_value self) {
  char const* str;
  mrb_int len;
  mrb_get_args(M, "s", &str, &len);
  return load(M, str, str + len);
}

}

extern "C"
void mrb_mruby_msgpack_gem_init(mrb_state* M) {
  RClass* const mod = mrb_define_module(M, "MsgPack");
  mrb_define_module_function(M, mod, "load", &msgpack_load, MRB_ARGS_REQ(1));
  mrb_define_module_function(M, mod, "dump", &msgpack_dump, MRB_ARGS_REQ(1));
}

extern "C"
void mrb_mruby_msgpack_gem_final(mrb_state*) {}
