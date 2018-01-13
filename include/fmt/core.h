// Formatting library for C++ - the core API
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_CORE_H_
#define FMT_CORE_H_

#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>

#ifdef __has_feature
# define FMT_HAS_FEATURE(x) __has_feature(x)
#else
# define FMT_HAS_FEATURE(x) 0
#endif

#ifdef __GNUC__
# define FMT_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#endif

#ifdef _MSC_VER
# define FMT_MSC_VER _MSC_VER
#else
# define FMT_MSC_VER 0
#endif

// Check if exceptions are disabled.
#if defined(__GNUC__) && !defined(__EXCEPTIONS)
# define FMT_EXCEPTIONS 0
#elif FMT_MSC_VER && !_HAS_EXCEPTIONS
# define FMT_EXCEPTIONS 0
#endif
#ifndef FMT_EXCEPTIONS
# define FMT_EXCEPTIONS 1
#endif

// Define FMT_USE_NOEXCEPT to make fmt use noexcept (C++11 feature).
#ifndef FMT_USE_NOEXCEPT
# define FMT_USE_NOEXCEPT 0
#endif

#ifndef FMT_NOEXCEPT
# if FMT_EXCEPTIONS
#  if FMT_USE_NOEXCEPT || FMT_HAS_FEATURE(cxx_noexcept) || \
      FMT_GCC_VERSION >= 408 || FMT_MSC_VER >= 1900
#   define FMT_NOEXCEPT noexcept
#  else
#   define FMT_NOEXCEPT throw()
#  endif
# else
#  define FMT_NOEXCEPT
# endif
#endif

#if !defined(FMT_HEADER_ONLY) && defined(_WIN32)
# ifdef FMT_EXPORT
#  define FMT_API __declspec(dllexport)
# elif defined(FMT_SHARED)
#  define FMT_API __declspec(dllimport)
# endif
#endif
#ifndef FMT_API
# define FMT_API
#endif

#ifndef FMT_ASSERT
# define FMT_ASSERT(condition, message) assert((condition) && message)
#endif

#define FMT_DELETED = delete

// A macro to disallow the copy construction and assignment.
#define FMT_DISALLOW_COPY_AND_ASSIGN(Type) \
    Type(const Type &) FMT_DELETED; \
    void operator=(const Type &) FMT_DELETED

namespace fmt {

/**
  \rst
  An implementation of ``std::basic_string_view`` for pre-C++17. It provides a
  subset of the API.
  \endrst
 */
template <typename Char>
class basic_string_view {
 private:
  const Char *data_;
  size_t size_;

 public:
  using char_type = Char;
  using iterator = const Char *;

  constexpr basic_string_view() FMT_NOEXCEPT : data_(0), size_(0) {}

  /** Constructs a string reference object from a C string and a size. */
  constexpr basic_string_view(const Char *s, size_t size) FMT_NOEXCEPT
    : data_(s), size_(size) {}

  /**
    \rst
    Constructs a string reference object from a C string computing
    the size with ``std::char_traits<Char>::length``.
    \endrst
   */
  basic_string_view(const Char *s)
    : data_(s), size_(std::char_traits<Char>::length(s)) {}

  /**
    \rst
    Constructs a string reference from an ``std::string`` object.
    \endrst
   */
  constexpr basic_string_view(const std::basic_string<Char> &s) FMT_NOEXCEPT
  : data_(s.c_str()), size_(s.size()) {}

  /**
    \rst
    Converts a string reference to an ``std::string`` object.
    \endrst
   */
  std::basic_string<Char> to_string() const {
    return std::basic_string<Char>(data_, size_);
  }

  /** Returns a pointer to the string data. */
  const Char *data() const { return data_; }

  /** Returns the string size. */
  constexpr size_t size() const { return size_; }

  constexpr iterator begin() const { return data_; }
  constexpr iterator end() const { return data_ + size_; }

  constexpr void remove_prefix(size_t n) {
    data_ += n;
    size_ -= n;
  }

  // Lexicographically compare this string reference to other.
  int compare(basic_string_view other) const {
    size_t size = size_ < other.size_ ? size_ : other.size_;
    int result = std::char_traits<Char>::compare(data_, other.data_, size);
    if (result == 0)
      result = size_ == other.size_ ? 0 : (size_ < other.size_ ? -1 : 1);
    return result;
  }

  friend bool operator==(basic_string_view lhs, basic_string_view rhs) {
    return lhs.compare(rhs) == 0;
  }
  friend bool operator!=(basic_string_view lhs, basic_string_view rhs) {
    return lhs.compare(rhs) != 0;
  }
  friend bool operator<(basic_string_view lhs, basic_string_view rhs) {
    return lhs.compare(rhs) < 0;
  }
  friend bool operator<=(basic_string_view lhs, basic_string_view rhs) {
    return lhs.compare(rhs) <= 0;
  }
  friend bool operator>(basic_string_view lhs, basic_string_view rhs) {
    return lhs.compare(rhs) > 0;
  }
  friend bool operator>=(basic_string_view lhs, basic_string_view rhs) {
    return lhs.compare(rhs) >= 0;
  }
};

using string_view = basic_string_view<char>;
using wstring_view = basic_string_view<wchar_t>;

/** A contiguous memory buffer with an optional growing ability. */
template <typename T>
class basic_buffer {
 private:
  FMT_DISALLOW_COPY_AND_ASSIGN(basic_buffer);

  T *ptr_;
  std::size_t size_;
  std::size_t capacity_;

 protected:
  basic_buffer() FMT_NOEXCEPT : ptr_(0), size_(0), capacity_(0) {}

  /** Sets the buffer data and capacity. */
  void set(T *data, std::size_t capacity) FMT_NOEXCEPT {
    ptr_ = data;
    capacity_ = capacity;
  }

  /**
    \rst
    Increases the buffer capacity to hold at least *capacity* elements.
    \endrst
   */
  virtual void grow(std::size_t capacity) = 0;

 public:
  using value_type = T;

  virtual ~basic_buffer() {}

  T *begin() FMT_NOEXCEPT { return ptr_; }
  T *end() FMT_NOEXCEPT { return ptr_ + size_; }

  /** Returns the size of this buffer. */
  std::size_t size() const FMT_NOEXCEPT { return size_; }

  /** Returns the capacity of this buffer. */
  std::size_t capacity() const FMT_NOEXCEPT { return capacity_; }

  /** Returns a pointer to the buffer data. */
  T *data() FMT_NOEXCEPT { return ptr_; }

  /** Returns a pointer to the buffer data. */
  const T *data() const FMT_NOEXCEPT { return ptr_; }

  /**
    Resizes the buffer. If T is a POD type new elements may not be initialized.
   */
  void resize(std::size_t new_size) {
    reserve(new_size);
    size_ = new_size;
  }

  /**
    \rst
    Reserves space to store at least *capacity* elements.
    \endrst
   */
  void reserve(std::size_t capacity) {
    if (capacity > capacity_)
      grow(capacity);
  }

  void push_back(const T &value) {
    reserve(size_ + 1);
    ptr_[size_++] = value;
  }

  /** Appends data to the end of the buffer. */
  template <typename U>
  void append(const U *begin, const U *end);

  T &operator[](std::size_t index) { return ptr_[index]; }
  const T &operator[](std::size_t index) const { return ptr_[index]; }
};

using buffer = basic_buffer<char>;
using wbuffer = basic_buffer<wchar_t>;

template <typename Context>
class basic_arg;

template <typename Context>
class basic_format_args;

// A formatter for objects of type T.
template <typename T, typename Char = char, typename Enable = void>
struct formatter;

namespace internal {

// A helper function to suppress bogus "conditional expression is constant"
// warnings.
template <typename T>
inline T const_check(T value) { return value; }

struct error_handler {
  constexpr error_handler() {}
  constexpr error_handler(const error_handler &) {}

  // This function is intentionally not constexpr to give a compile-time error.
  void on_error(const char *message);
};

// Formatting of wide characters and strings into a narrow output is disallowed:
//   fmt::format("{}", L"test"); // error
// To fix this, use a wide format string:
//   fmt::format(L"{}", L"test");
template <typename Char>
inline void require_wchar() {
  static_assert(
      std::is_same<wchar_t, Char>::value,
      "formatting of wide characters into a narrow output is disallowed");
}

using yes = char[1];
using no = char[2];

yes &convert(unsigned long long);
no &convert(...);

template<typename T, bool ENABLE_CONVERSION>
struct convert_to_int_impl {
  enum { value = ENABLE_CONVERSION };
};

template<typename T, bool ENABLE_CONVERSION>
struct convert_to_int_impl2 {
  enum { value = false };
};

template<typename T>
struct convert_to_int_impl2<T, true> {
  enum {
    // Don't convert arithmetic types.
    value = convert_to_int_impl<T, !std::is_arithmetic<T>::value>::value
  };
};

template<typename T>
struct convert_to_int {
  enum {
    enable_conversion = sizeof(convert(std::declval<T>())) == sizeof(yes)
  };
  enum { value = convert_to_int_impl2<T, enable_conversion>::value };
};

#define FMT_DISABLE_CONVERSION_TO_INT(Type) \
  template <> \
  struct convert_to_int<Type> { enum { value = 0 }; }

// Silence warnings about convering float to int.
FMT_DISABLE_CONVERSION_TO_INT(float);
FMT_DISABLE_CONVERSION_TO_INT(double);
FMT_DISABLE_CONVERSION_TO_INT(long double);

template <typename Char>
struct named_arg_base;

template <typename T, typename Char>
struct named_arg;

template <typename T>
struct is_named_arg : std::false_type {};

template <typename T, typename Char>
struct is_named_arg<named_arg<T, Char>> : std::true_type {};

enum type {
  NONE, NAMED_ARG,
  // Integer types should go first,
  INT, UINT, LONG_LONG, ULONG_LONG, BOOL, CHAR, LAST_INTEGER_TYPE = CHAR,
  // followed by floating-point types.
  DOUBLE, LONG_DOUBLE, LAST_NUMERIC_TYPE = LONG_DOUBLE,
  CSTRING, STRING, POINTER, CUSTOM
};

constexpr bool is_integral(type t) {
  FMT_ASSERT(t != internal::NAMED_ARG, "invalid argument type");
  return t > internal::NONE && t <= internal::LAST_INTEGER_TYPE;
}

constexpr bool is_arithmetic(type t) {
  FMT_ASSERT(t != internal::NAMED_ARG, "invalid argument type");
  return t > internal::NONE && t <= internal::LAST_NUMERIC_TYPE;
}

template <typename T>
constexpr type get_type() {
  return std::is_reference<T>::value || std::is_array<T>::value ?
        get_type<typename std::decay<T>::type>() :
        (is_named_arg<T>::value ?
           NAMED_ARG : (convert_to_int<T>::value ? INT : CUSTOM));
}

template <> constexpr type get_type<bool>() { return BOOL; }
template <> constexpr type get_type<short>() { return INT; }
template <> constexpr type get_type<unsigned short>() { return UINT; }
template <> constexpr type get_type<int>() { return INT; }
template <> constexpr type get_type<unsigned>() { return UINT; }
template <> constexpr type get_type<long>() {
  return sizeof(long) == sizeof(int) ? INT : LONG_LONG;
}
template <> constexpr type get_type<unsigned long>() {
  return sizeof(unsigned long) == sizeof(unsigned) ? UINT : ULONG_LONG;
}
template <> constexpr type get_type<long long>() { return LONG_LONG; }
template <> constexpr type get_type<unsigned long long>() { return ULONG_LONG; }
template <> constexpr type get_type<float>() { return DOUBLE; }
template <> constexpr type get_type<double>() { return DOUBLE; }
template <> constexpr type get_type<long double>() { return LONG_DOUBLE; }
template <> constexpr type get_type<signed char>() { return INT; }
template <> constexpr type get_type<unsigned char>() { return UINT; }
template <> constexpr type get_type<char>() { return CHAR; }

#if !defined(_MSC_VER) || defined(_NATIVE_WCHAR_T_DEFINED)
template <> constexpr type get_type<wchar_t>() { return CHAR; }
#endif

template <> constexpr type get_type<char *>() { return CSTRING; }
template <> constexpr type get_type<const char *>() { return CSTRING; }
template <> constexpr type get_type<signed char *>() { return CSTRING; }
template <> constexpr type get_type<const signed char *>() { return CSTRING; }
template <> constexpr type get_type<unsigned char *>() { return CSTRING; }
template <> constexpr type get_type<const unsigned char *>() { return CSTRING; }
template <> constexpr type get_type<std::string>() { return STRING; }
template <> constexpr type get_type<string_view>() { return STRING; }
template <> constexpr type get_type<wchar_t *>() { return CSTRING; }
template <> constexpr type get_type<const wchar_t *>() { return CSTRING; }
template <> constexpr type get_type<std::wstring>() { return STRING; }
template <> constexpr type get_type<wstring_view>() { return STRING; }
template <> constexpr type get_type<void *>() { return POINTER; }
template <> constexpr type get_type<const void *>() { return POINTER; }
template <> constexpr type get_type<std::nullptr_t>() { return POINTER; }

template <typename Arg, typename... Args>
constexpr uint64_t get_types() {
  return get_type<Arg>() | (get_types<Args...>() << 4);
}

template <>
constexpr uint64_t get_types<void>() { return 0; }

template <typename Context, typename T>
constexpr basic_arg<Context> make_arg(const T &value);

template <typename Char>
struct string_value {
  const Char *value;
  std::size_t size;
};

template <typename Context>
struct custom_value {
  const void *value;
  void (*format)(const void *arg, Context &ctx);
};

// A formatting argument value.
template <typename Context>
class value {
 public:
  using char_type = typename Context::char_type;

  union {
    int int_value;
    unsigned uint_value;
    long long long_long_value;
    unsigned long long ulong_long_value;
    double double_value;
    long double long_double_value;
    const void *pointer;
    string_value<char_type> string;
    string_value<signed char> sstring;
    string_value<unsigned char> ustring;
    custom_value<Context> custom;
  };

  constexpr value() : int_value(0) {}
  value(bool val) { set<BOOL>(int_value, val); }
  value(short val) { set<INT>(int_value, val); }
  value(unsigned short val) { set<UINT>(uint_value, val); }
  constexpr value(int val) : int_value(val) {}
  value(unsigned val) { set<UINT>(uint_value, val); }

  value(long val) {
    // To minimize the number of types we need to deal with, long is
    // translated either to int or to long long depending on its size.
    if (const_check(sizeof(val) == sizeof(int)))
      int_value = static_cast<int>(val);
    else
      long_long_value = val;
  }

  value(unsigned long val) {
    if (const_check(sizeof(val) == sizeof(unsigned)))
      uint_value = static_cast<unsigned>(val);
    else
      ulong_long_value = val;
  }

  value(long long val) { set<LONG_LONG>(long_long_value, val); }
  value(unsigned long long val) { set<ULONG_LONG>(ulong_long_value, val); }
  value(float val) { set<DOUBLE>(double_value, val); }
  value(double val) { set<DOUBLE>(double_value, val); }
  value(long double val) { set<LONG_DOUBLE>(long_double_value, val); }
  value(signed char val) { set<INT>(int_value, val); }
  value(unsigned char val) { set<UINT>(uint_value, val); }
  value(char val) { set<CHAR>(int_value, val); }

#if !defined(_MSC_VER) || defined(_NATIVE_WCHAR_T_DEFINED)
  value(wchar_t value) {
    require_wchar<char_type>();
    set<CHAR>(int_value, value);
  }
#endif

  // Formatting of wide strings into a narrow buffer and multibyte strings
  // into a wide buffer is disallowed (https://github.com/fmtlib/fmt/pull/606).
  value(char_type *s) { set<CSTRING>(string.value, s); }
  value(const char_type *s) { set<CSTRING>(string.value, s); }
  value(signed char *s) { set_cstring(sstring.value, s); }
  value(const signed char *s) { set_cstring(sstring.value, s); }
  value(unsigned char *s) { set_cstring(ustring.value, s); }
  value(const unsigned char *s) { set_cstring(ustring.value, s); }
  value(basic_string_view<char_type> s) { set_string(s); }
  value(const std::basic_string<char_type> &s) { set_string(s); }

  template <typename T>
  value(T *p) { set_pointer(p); }

  // Match function pointers explicitly. If we do not then the
  // forwarding T * constructor below will end up trying to call
  // itself when a function pointer is passed, which leads to a
  // confusing error message.
  template <typename T, typename ...Args>
  value(T (*p)(Args...)) {
    // This static_assert must use something that is dependent on T,
    // otherwise it will always fire, so let's use the same check as
    // above - it will never match.
    static_assert(std::is_same<T, void>::value, "formatting of function pointers is disallowed");
  }

  template <typename T>
  value(const T *p) { set_pointer(p); }

  value(std::nullptr_t) { pointer = nullptr; }

  template <typename T>
  value(const T &value,
        typename std::enable_if<convert_to_int<T>::value, int>::type = 0) {
    static_assert(get_type<T>() == INT, "invalid type");
    int_value = value;
  }

  template <typename T>
  value(const T &value,
        typename std::enable_if<!convert_to_int<T>::value, int>::type = 0) {
    static_assert(get_type<T>() == CUSTOM, "invalid type");
    custom.value = &value;
    custom.format = &format_custom_arg<T>;
  }

  template <typename T>
  value(const named_arg<T, char_type> &na) {
    static_assert(get_type<const named_arg<T, char_type> &>() == NAMED_ARG,
                  "invalid type");
    basic_arg<Context> arg = make_arg<Context>(na.value);
    std::memcpy(na.data, &arg, sizeof(arg));
    pointer = &na;
  }

  const named_arg_base<char_type> &as_named_arg() {
    return *static_cast<const named_arg_base<char_type>*>(pointer);
  }

 private:
  template <type TYPE, typename T, typename U>
  constexpr void set(T &field, const U &value) {
    static_assert(get_type<U>() == TYPE, "invalid type");
    field = value;
  }

  template <typename T>
  void set_string(const T &value) {
    static_assert(get_type<T>() == STRING, "invalid type");
    string.value = value.data();
    string.size = value.size();
  }

  template <typename T, typename U>
  constexpr void set_cstring(T &field, const U *str) {
    static_assert(std::is_same<char, char_type>::value,
                  "incompatible string types");
    set<CSTRING>(field, str);
  }

  // Formatting of arbitrary pointers is disallowed. If you want to output a
  // pointer cast it to "void *" or "const void *". In particular, this forbids
  // formatting of "[const] volatile char *" which is printed as bool by
  // iostreams.
  template <typename T>
  void set_pointer(T *p) {
    using type = typename std::remove_const<T>::type;
    static_assert(std::is_same<type, void>::value,
                  "formatting of non-void pointers is disallowed");
    set<POINTER>(pointer, p);
  }

  // Formats an argument of a custom type, such as a user-defined class.
  template <typename T>
  static void format_custom_arg(const void *arg, Context &ctx) {
    // Get the formatter type through the context to allow different contexts
    // have different extension points, e.g. `formatter<T>` for `format` and
    // `printf_formatter<T>` for `printf`.
    typename Context::template formatter_type<T> f;
    auto &&parse_ctx = ctx.parse_context();
    parse_ctx.advance_to(f.parse(parse_ctx));
    f.format(*static_cast<const T*>(arg), ctx);
  }
};

// Maximum number of arguments with packed types.
enum { MAX_PACKED_ARGS = 15 };

template <typename Context>
class arg_map;
}

// A formatting argument. It is a trivially copyable/constructible type to
// allow storage in basic_memory_buffer.
template <typename Context>
class basic_arg {
 private:
  internal::value<Context> value_;
  internal::type type_;

  template <typename ContextType, typename T>
  friend constexpr basic_arg<ContextType> internal::make_arg(const T &value);

  template <typename Visitor, typename Ctx>
  friend constexpr typename std::result_of<Visitor(int)>::type
    visit(Visitor &&vis, basic_arg<Ctx> arg);

  friend class basic_format_args<Context>;
  friend class internal::arg_map<Context>;

  using char_type = typename Context::char_type;

 public:
  class handle {
   public:
    explicit handle(internal::custom_value<Context> custom): custom_(custom) {}

    void format(Context &ctx) { custom_.format(custom_.value, ctx); }

   private:
    internal::custom_value<Context> custom_;
  };

  constexpr basic_arg() : type_(internal::NONE) {}

  explicit operator bool() const FMT_NOEXCEPT {
    return type_ != internal::NONE;
  }

  internal::type type() const { return type_; }

  bool is_integral() const { return internal::is_integral(type_); }
  bool is_arithmetic() const { return internal::is_arithmetic(type_); }
  bool is_pointer() const { return type_ == internal::POINTER; }
};

// Parsing context consisting of a format string range being parsed and an
// argument counter for automatic indexing.
template <typename Char, typename ErrorHandler = internal::error_handler>
class basic_parse_context : private ErrorHandler {
 private:
  basic_string_view<Char> format_str_;
  int next_arg_id_;

 protected:
  constexpr bool check_no_auto_index() {
    if (next_arg_id_ > 0) {
      on_error("cannot switch from automatic to manual argument indexing");
      return false;
    }
    next_arg_id_ = -1;
    return true;
  }

 public:
  using char_type = Char;
  using iterator = typename basic_string_view<Char>::iterator;

  explicit constexpr basic_parse_context(
      basic_string_view<Char> format_str, ErrorHandler eh = ErrorHandler())
    : ErrorHandler(eh), format_str_(format_str), next_arg_id_(0) {}

  // Returns an iterator to the beginning of the format string range being
  // parsed.
  constexpr iterator begin() const FMT_NOEXCEPT { return format_str_.begin(); }

  // Returns an iterator past the end of the format string range being parsed.
  constexpr iterator end() const FMT_NOEXCEPT { return format_str_.end(); }

  // Advances the begin iterator to ``it``.
  constexpr void advance_to(iterator it) {
    format_str_.remove_prefix(it - begin());
  }

  // Returns the next argument index.
  constexpr unsigned next_arg_id();

  constexpr void check_arg_id(unsigned) { check_no_auto_index(); }
  void check_arg_id(basic_string_view<Char>) {}

  constexpr void on_error(const char *message) {
    ErrorHandler::on_error(message);
  }

  constexpr ErrorHandler error_handler() const { return *this; }
};

using parse_context = basic_parse_context<char>;
using wparse_context = basic_parse_context<wchar_t>;

namespace internal {
template <typename Context, typename T>
constexpr basic_arg<Context> make_arg(const T &value) {
  basic_arg<Context> arg;
  arg.type_ = get_type<T>();
  arg.value_ = value;
  return arg;
}

template <bool IS_PACKED, typename Context, typename T>
inline typename std::enable_if<IS_PACKED, value<Context>>::type
    make_arg(const T &value) {
  return value;
}

template <bool IS_PACKED, typename Context, typename T>
inline typename std::enable_if<!IS_PACKED, basic_arg<Context>>::type
    make_arg(const T &value) {
  return make_arg<Context>(value);
}

template <typename Context>
class arg_map {
 private:
  FMT_DISALLOW_COPY_AND_ASSIGN(arg_map);

  using char_type = typename Context::char_type;

  struct entry {
    basic_string_view<char_type> name;
    basic_arg<Context> arg;
  };

  entry *map_ = nullptr;
  unsigned size_ = 0;

  void push_back(value<Context> val) {
    const internal::named_arg_base<char_type> &named = val.as_named_arg();
    map_[size_] = entry{named.name, named.template deserialize<Context>()};
    ++size_;
  }

 public:
  arg_map() {}
  void init(const basic_format_args<Context> &args);
  ~arg_map() { delete [] map_; }

  basic_arg<Context> find(basic_string_view<char_type> name) const {
    // The list is unsorted, so just return the first matching name.
    for (auto it = map_, end = map_ + size_; it != end; ++it) {
      if (it->name == name)
        return it->arg;
    }
    return basic_arg<Context>();
  }
};

template <typename Range, typename Context>
class context_base : public basic_parse_context<typename Range::value_type> {
 private:
  Range &range_;
  basic_format_args<Context> args_;

 protected:
  using char_type = typename Range::value_type;
  using format_arg = basic_arg<Context>;

  context_base(Range &range, basic_string_view<char_type> format_str,
               basic_format_args<Context> args)
  : basic_parse_context<char_type>(format_str), range_(range), args_(args) {}

  basic_format_args<Context> args() const { return args_; }

  // Returns the argument with specified index.
  format_arg do_get_arg(unsigned arg_id) {
    format_arg arg = args_[arg_id];
    if (!arg)
      this->on_error("argument index out of range");
    return arg;
  }

  // Checks if manual indexing is used and returns the argument with
  // specified index.
  format_arg get_arg(unsigned arg_id) {
    return this->check_no_auto_index() ?
      this->do_get_arg(arg_id) : format_arg();
  }

 public:
  basic_parse_context<char_type> &parse_context() { return *this; }
  Range &range() { return range_; }
};
}  // namespace internal

template <typename Range>
class basic_context :
  public internal::context_base<Range, basic_context<Range>> {
 public:
  /** The character type for the output. */
  using char_type = typename Range::value_type;

  template <typename T>
  using formatter_type = formatter<T, char_type>;

 private:
  internal::arg_map<basic_context> map_;

  FMT_DISALLOW_COPY_AND_ASSIGN(basic_context);

  using base = internal::context_base<Range, basic_context>;

  using format_arg = typename base::format_arg;
  using base::get_arg;

 public:
  /**
   \rst
   Constructs a ``basic_context`` object. References to the arguments are
   stored in the object so make sure they have appropriate lifetimes.
   \endrst
   */
  basic_context(Range &range, basic_string_view<char_type> format_str,
                basic_format_args<basic_context> args)
    : base(range, format_str, args) {}

  format_arg next_arg() { return this->do_get_arg(this->next_arg_id()); }
  format_arg get_arg(unsigned arg_id) { return this->do_get_arg(arg_id); }

  // Checks if manual indexing is used and returns the argument with
  // specified name.
  format_arg get_arg(basic_string_view<char_type> name);
};

using context = basic_context<buffer>;
using wcontext = basic_context<wbuffer>;

template <typename Context, typename ...Args>
class arg_store {
 private:
  static const size_t NUM_ARGS = sizeof...(Args);

  // Packed is a macro on MinGW so use IS_PACKED instead.
  static const bool IS_PACKED = NUM_ARGS < internal::MAX_PACKED_ARGS;

  using value_type = typename std::conditional<
    IS_PACKED, internal::value<Context>, basic_arg<Context>>::type;

  // If the arguments are not packed, add one more element to mark the end.
  value_type data_[NUM_ARGS + (IS_PACKED ? 0 : 1)];

 public:
  static const uint64_t TYPES = IS_PACKED ?
      internal::get_types<Args..., void>() : -static_cast<int64_t>(NUM_ARGS);

  arg_store(const Args &... args)
    : data_{internal::make_arg<IS_PACKED, Context>(args)...} {}

  const value_type *data() const { return data_; }
};

template <typename Context, typename ...Args>
inline arg_store<Context, Args...> make_args(const Args & ... args) {
  return arg_store<Context, Args...>(args...);
}

template <typename ...Args>
inline arg_store<context, Args...> make_args(const Args & ... args) {
  return arg_store<context, Args...>(args...);
}

/** Formatting arguments. */
template <typename Context>
class basic_format_args {
 public:
  using size_type = unsigned;
  using format_arg = basic_arg<Context> ;

 private:
  // To reduce compiled code size per formatting function call, types of first
  // MAX_PACKED_ARGS arguments are passed in the types_ field.
  uint64_t types_;
  union {
    // If the number of arguments is less than MAX_PACKED_ARGS, the argument
    // values are stored in values_, otherwise they are stored in args_.
    // This is done to reduce compiled code size as storing larger objects
    // may require more code (at least on x86-64) even if the same amount of
    // data is actually copied to stack. It saves ~10% on the bloat test.
    const internal::value<Context> *values_;
    const format_arg *args_;
  };

  typename internal::type type(unsigned index) const {
    unsigned shift = index * 4;
    uint64_t mask = 0xf;
    return static_cast<typename internal::type>(
      (types_ & (mask << shift)) >> shift);
  }

  friend class internal::arg_map<Context>;

  void set_data(const internal::value<Context> *values) { values_ = values; }
  void set_data(const format_arg *args) { args_ = args; }

  format_arg get(size_type index) const {
    int64_t signed_types = static_cast<int64_t>(types_);
    if (signed_types < 0) {
      uint64_t num_args = -signed_types;
      return index < num_args ? args_[index] : format_arg();
    }
    format_arg arg;
    if (index > internal::MAX_PACKED_ARGS)
      return arg;
    arg.type_ = type(index);
    if (arg.type_ == internal::NONE)
      return arg;
    internal::value<Context> &val = arg.value_;
    val = values_[index];
    return arg;
  }

 public:
  basic_format_args() : types_(0) {}

  template <typename... Args>
  basic_format_args(const arg_store<Context, Args...> &store)
  : types_(store.TYPES) {
    set_data(store.data());
  }

  /** Returns the argument at specified index. */
  format_arg operator[](size_type index) const {
    format_arg arg = get(index);
    return arg.type_ == internal::NAMED_ARG ?
          arg.value_.as_named_arg().template deserialize<Context>() : arg;
  }

  unsigned max_size() const {
    int64_t signed_types = static_cast<int64_t>(types_);
    return signed_types < 0 ? -signed_types : internal::MAX_PACKED_ARGS;
  }
};

using format_args = basic_format_args<context>;
using wformat_args = basic_format_args<wcontext>;

namespace internal {
template <typename Char>
struct named_arg_base {
  basic_string_view<Char> name;

  // Serialized value<context>.
  mutable char data[sizeof(basic_arg<context>)];

  template <typename Context>
  basic_arg<Context> deserialize() const {
    basic_arg<Context> arg;
    std::memcpy(&arg, data, sizeof(basic_arg<Context>));
    return arg;
  }
};

template <typename T, typename Char>
struct named_arg : named_arg_base<Char> {
  const T &value;

  named_arg(basic_string_view<Char> name, const T &val)
    : named_arg_base<Char>{name}, value(val) {}
};
}

/**
  \rst
  Returns a named argument for formatting functions.

  **Example**::

    print("Elapsed time: {s:.2f} seconds", arg("s", 1.23));
  \endrst
 */
template <typename T>
inline internal::named_arg<T, char> arg(string_view name, const T &arg) {
  return internal::named_arg<T, char>(name, arg);
}

template <typename T>
inline internal::named_arg<T, wchar_t> arg(wstring_view name, const T &arg) {
  return internal::named_arg<T, wchar_t>(name, arg);
}

// The following two functions are deleted intentionally to disable
// nested named arguments as in ``format("{}", arg("a", arg("b", 42)))``.
template <typename T>
void arg(string_view, internal::named_arg<T, char>) FMT_DELETED;
template <typename T>
void arg(wstring_view, internal::named_arg<T, wchar_t>) FMT_DELETED;

enum Color { BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };

FMT_API void vprint_colored(Color c, string_view format, format_args args);

/**
  Formats a string and prints it to stdout using ANSI escape sequences to
  specify color (experimental).
  Example:
    print_colored(fmt::RED, "Elapsed time: {0:.2f} seconds", 1.23);
 */
template <typename... Args>
inline void print_colored(Color c, string_view format_str,
                          const Args & ... args) {
  vprint_colored(c, format_str, make_args(args...));
}

void vformat_to(buffer &buf, string_view format_str, format_args args);
void vformat_to(wbuffer &buf, wstring_view format_str, wformat_args args);

template <typename... Args>
inline void format_to(buffer &buf, string_view format_str,
                      const Args & ... args) {
  vformat_to(buf, format_str, make_args(args...));
}

template <typename... Args>
inline void format_to(wbuffer &buf, wstring_view format_str,
                      const Args & ... args) {
  vformat_to(buf, format_str, make_args<wcontext>(args...));
}

std::string vformat(string_view format_str, format_args args);
std::wstring vformat(wstring_view format_str, wformat_args args);

/**
  \rst
  Formats arguments and returns the result as a string.

  **Example**::

    std::string message = format("The answer is {}", 42);
  \endrst
*/
template <typename... Args>
inline std::string format(string_view format_str, const Args & ... args) {
  return vformat(format_str, make_args(args...));
}
template <typename... Args>
inline std::wstring format(wstring_view format_str, const Args & ... args) {
  return vformat(format_str, make_args<wcontext>(args...));
}

FMT_API void vprint(std::FILE *f, string_view format_str, format_args args);

/**
  \rst
  Prints formatted data to the file *f*.

  **Example**::

    print(stderr, "Don't {}!", "panic");
  \endrst
 */
template <typename... Args>
inline void print(std::FILE *f, string_view format_str, const Args & ... args) {
  vprint(f, format_str, make_args(args...));
}

FMT_API void vprint(string_view format_str, format_args args);

/**
  \rst
  Prints formatted data to ``stdout``.

  **Example**::

    print("Elapsed time: {0:.2f} seconds", 1.23);
  \endrst
 */
template <typename... Args>
inline void print(string_view format_str, const Args & ... args) {
  vprint(format_str, make_args(args...));
}
}  // namespace fmt

#endif  // FMT_CORE_H_
