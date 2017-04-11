/*
 Formatting library for C++ - std::ostream support

 Copyright (c) 2012 - 2016, Victor Zverovich
 All rights reserved.

 For the license information refer to format.h.
 */

#ifndef FMT_OSTREAM_H_
#define FMT_OSTREAM_H_

#include "format.h"
#include <ostream>

namespace fmt {

namespace internal {

template <class Char>
class FormatBuf : public std::basic_streambuf<Char> {
 private:
  typedef typename std::basic_streambuf<Char>::int_type int_type;
  typedef typename std::basic_streambuf<Char>::traits_type traits_type;

  Buffer<Char> &buffer_;

 public:
  FormatBuf(Buffer<Char> &buffer) : buffer_(buffer) {}

 protected:
  // The put-area is actually always empty. This makes the implementation
  // simpler and has the advantage that the streambuf and the buffer are always
  // in sync and sputc never writes into uninitialized memory. The obvious
  // disadvantage is that each call to sputc always results in a (virtual) call
  // to overflow. There is no disadvantage here for sputn since this always
  // results in a call to xsputn.

  int_type overflow(int_type ch = traits_type::eof()) FMT_OVERRIDE {
    if (!traits_type::eq_int_type(ch, traits_type::eof()))
      buffer_.push_back(ch);
    return ch;
  }

  std::streamsize xsputn(const Char *s, std::streamsize count) FMT_OVERRIDE {
    buffer_.append(s, s + count);
    return count;
  }
};

Yes &convert(std::ostream &);

struct DummyStream : std::ostream {
  DummyStream();  // Suppress a bogus warning in MSVC.
  // Hide all operator<< overloads from std::ostream.
  void operator<<(Null<>);
};

No &operator<<(std::ostream &, int);

template<typename T>
struct ConvertToIntImpl<T, true> {
  // Convert to int only if T doesn't have an overloaded operator<<.
  enum {
    value = sizeof(convert(get<DummyStream>() << get<T>())) == sizeof(No)
  };
};

// Write the content of w to os.
void write(std::ostream &os, Writer &w);

#if FMT_HAS_DECLTYPE_INCOMPLETE_RETURN_TYPES
template<typename T>
class is_streamable {
  template<typename U>
  static auto test(int) -> decltype(std::declval<std::ostream &>() << std::declval<U>(), std::true_type());

  template<typename>
  static auto test(...) -> std::false_type;

public:
  static constexpr bool value = decltype(test<T>(0))::value;
};
#endif
}  // namespace internal

// Formats a value.
template <typename Char, typename ArgFormatter_, typename T>
void format_arg(BasicFormatter<Char, ArgFormatter_> &f,
                const Char *&format_str, const T &value) {
  internal::MemoryBuffer<Char, internal::INLINE_BUFFER_SIZE> buffer;

  internal::FormatBuf<Char> format_buf(buffer);
  std::basic_ostream<Char> output(&format_buf);
  output << value;

  BasicStringRef<Char> str(&buffer[0], buffer.size());
  typedef internal::MakeArg< BasicFormatter<Char> > MakeArg;
  format_str = f.format(format_str, MakeArg(str));
}

/**
  \rst
  Prints formatted data to the stream *os*.

  **Example**::

    print(cerr, "Don't {}!", "panic");
  \endrst
 */
FMT_API void print(std::ostream &os, CStringRef format_str, ArgList args);
FMT_VARIADIC(void, print, std::ostream &, CStringRef)

template <typename Char, typename T>
BasicWriter<Char> &operator<<(BasicWriter<Char> &writer, const T &value) {
#if FMT_HAS_DECLTYPE_INCOMPLETE_RETURN_TYPES
  FMT_STATIC_ASSERT(internal::is_streamable<T>::value, "T must be Streamable");
#endif

  internal::FormatBuf<Char> format_buf(writer.buffer());
  std::basic_ostream<Char> output(&format_buf);
  output << value;

  return writer;
}

}  // namespace fmt

#ifdef FMT_HEADER_ONLY
# include "ostream.cc"
#endif

#endif  // FMT_OSTREAM_H_
