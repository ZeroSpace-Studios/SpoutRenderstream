#pragma once

#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <stdint.h>

#define ARGPARSE_FAIL(msg) ::argparse::detail::NotifyError(msg)

#define ARGPARSE_FAIL_IF(condition, msg) \
  do {                                   \
    if (condition) {                     \
      ARGPARSE_FAIL(msg);                \
    }                                    \
  } while (0)

#define ARGPARSE_ASSERT(condition) \
  ARGPARSE_FAIL_IF(!(condition), "Argparse internal assumptions failed")

namespace argparse {

namespace util {

inline std::string JoinStrings(const std::vector<std::string>& parts,
                               const std::string delimiter) {
  if (parts.empty()) {
    return "";
  }
  std::string result;
  for (size_t i = 0; i < parts.size() - 1; i++) {
    result += parts[i] + delimiter;
  }
  return result + parts.back();
}

}  // namespace util

constexpr char kDefaultHelpString[] = "No help, sorry";

class ArgparseError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

template <typename Type>
class TypeTraits;

namespace detail {

inline void NotifyError(const std::string& msg) {
  throw ::argparse::ArgparseError(msg);
}

template <typename Type>
class TraitsProvider {
public:
#define ARGPARSE_FIND_FUNCTION(constant, checker)            \
  template <typename T, typename = void>                     \
  struct constant##IsTrue : std::false_type {};              \
  template <typename T>                                      \
  struct constant##IsTrue<T, std::void_t<decltype(checker)>> \
      : std::true_type {};                                   \
  static constexpr bool constant = constant##IsTrue<Type>::value

  ARGPARSE_FIND_FUNCTION(kOperatorEqualExists,
                         std::declval<T>() == std::declval<T>());
  ARGPARSE_FIND_FUNCTION(kTraitsEqualExists, &TypeTraits<T>::Equal);

  ARGPARSE_FIND_FUNCTION(kTraitsFromStringExists, &TypeTraits<T>::FromString);
  ARGPARSE_FIND_FUNCTION(kOperatorRightShiftExists,
                         std::declval<std::istream&>() >> std::declval<T&>());

  ARGPARSE_FIND_FUNCTION(kTraitsToStringExists, &TypeTraits<T>::ToString);
  ARGPARSE_FIND_FUNCTION(kOperatorLeftShiftExists, std::declval<std::ostream&>()
                                                       << std::declval<T&>());

#undef ARGPARSE_FIND_FUNCTION

  static constexpr bool kEqualComparable =
      kTraitsEqualExists || kOperatorEqualExists;

  static constexpr bool kFromStringCastable =
      kTraitsFromStringExists || kOperatorRightShiftExists;

  static constexpr bool kToStringCastable =
      kTraitsToStringExists || kOperatorLeftShiftExists;

  static bool Equal(const Type& a, const Type& b) {
    static_assert(kEqualComparable,
                  "No suitable method for values comparison found: neither "
                  "default operator== nor TypeTraits::Equal is defined");
    if constexpr (kTraitsEqualExists) {
      return TypeTraits<Type>::Equal(a, b);
    } else {
      return a == b;
    }
  }

  static Type FromString(const std::string& str) {
    static_assert(kFromStringCastable,
                  "No suitable method to cast a string to value found: neither "
                  "default operator>> nor TypeTraits::FromString is defined");
    if constexpr (kTraitsFromStringExists) {
      return TypeTraits<Type>::FromString(str);
    } else {
      std::istringstream stream(str);
      Type value;
      stream >> value;
      return value;
    }
  }

  static std::string ToString(const Type& value) {
    if constexpr (kTraitsToStringExists) {
      return TypeTraits<Type>::ToString(value);
    }

    if constexpr (kOperatorLeftShiftExists) {
      std::ostringstream stream;
      stream << value;
      return stream.str();
    }

    // should never get here
    ARGPARSE_ASSERT(false);
    return {};
  }
};

inline std::tuple<std::string, std::optional<std::string>> SplitLongArg(
    const std::string& arg) {
  auto pos = arg.find("=");
  if (pos == std::string::npos) {
    return {arg, std::nullopt};
  }
  return {arg.substr(0, pos), arg.substr(pos + 1)};
}

inline std::string EscapeValue(std::string value) {
  if (!value.empty() && value[0] == '\\') {
    value.erase(0, 1);
  }

  return value;
}

template <typename T>
bool Equal(const T& a, const T& b) {
  if constexpr (TraitsProvider<T>::kEqualComparable) {
    return TraitsProvider<T>::Equal(a, b);
  }

  // should never get here
  ARGPARSE_ASSERT(false);
  return false;
}

template <typename Type>
bool IsValidValue(const Type& value, const std::vector<Type>& options) {
  return std::any_of(options.begin(), options.end(),
                     [&value](const Type& option) {
                       return Equal(value, option);
                     });
}

}  // namespace detail

template <>
class TypeTraits<std::string> {
public:
  static std::string FromString(const std::string& str) {
    return str;
  }

  static std::string ToString(const std::string& str) {
    return str;
  }
};

template <>
class TypeTraits<bool> {
public:
  static bool FromString(const std::string& str) {
    if (str == "false") {
      return false;
    }
    if (str == "true") {
      return true;
    }
    ARGPARSE_FAIL("Failed to cast `" + str + "` to bool");
    return false;
  }

  static std::string ToString(const bool& value) {
    return value ? "true" : "false";
  }
};

template <>
class TypeTraits<long long int> {
public:
  static long long int FromString(const std::string& str) {
    char* endptr;
    long long int value = std::strtoll(str.c_str(), &endptr, 10);
    ARGPARSE_FAIL_IF(endptr != str.c_str() + str.length(),
                     "Failed to cast `" + str + "` to integer");
    return value;
  }
  static std::string ToString(const long long int& value) {
    return std::to_string(value);
  }
};

template <>
class TypeTraits<unsigned long long int> {
public:
  static unsigned long long int FromString(const std::string& str) {
    char* endptr;
    long long int value = std::strtoull(str.c_str(), &endptr, 10);
    ARGPARSE_FAIL_IF(endptr != str.c_str() + str.length(),
                     "Failed to cast `" + str + "` to unsigned integer");
    return value;
  }
  static std::string ToString(const unsigned long long int& value) {
    return std::to_string(value);
  }
};

template <>
class TypeTraits<long double> {
public:
  static long double FromString(const std::string& str) {
    char* endptr;
    long double value = std::strtold(str.c_str(), &endptr);
    ARGPARSE_FAIL_IF(endptr != str.c_str() + str.length(),
                     "Failed to cast `" + str + "` to floating point number");
    return value;
  }
  static std::string ToString(const long double& value) {
    return std::to_string(value);
  }
};

#define ARGPARSE_DEFINE_TRAITS(Type, BaseType)                         \
  template <>                                                          \
  class TypeTraits<Type> {                                             \
  public:                                                              \
    static Type FromString(const std::string& str) {                   \
      return static_cast<Type>(TypeTraits<BaseType>::FromString(str)); \
    }                                                                  \
    static std::string ToString(const Type& value) {                   \
      return TypeTraits<BaseType>::ToString(value);                    \
    }                                                                  \
  };

ARGPARSE_DEFINE_TRAITS(long int, long long int)
ARGPARSE_DEFINE_TRAITS(int, long long int)
ARGPARSE_DEFINE_TRAITS(short int, long long int)
ARGPARSE_DEFINE_TRAITS(unsigned long int, unsigned long long int)
ARGPARSE_DEFINE_TRAITS(unsigned int, unsigned long long int)
ARGPARSE_DEFINE_TRAITS(unsigned short int, unsigned long long int)
ARGPARSE_DEFINE_TRAITS(double, long double)
ARGPARSE_DEFINE_TRAITS(float, long double)

#undef ARGPARSE_DEFINE_TRAITS

struct OptionInfo {
  std::string fullname;
  char shortname;
  std::string help;
  bool required;
  std::optional<std::string> default_value;
  std::optional<std::string> options;
};

class ArgHolderBase {
public:
  ArgHolderBase(std::string fullname, char shortname, std::string help)
      : fullname_(std::move(fullname))
      , shortname_(shortname)
      , help_(std::move(help))
      , required_(false) {}

  virtual ~ArgHolderBase() = default;

  virtual bool HasValue() const = 0;
  virtual bool RequiresValue() const = 0;

  virtual void ProcessFlag() = 0;
  virtual void ProcessValue(const std::string& value_str) = 0;

  virtual const std::string& fullname() const {
    return fullname_;
  }
  char shortname() const {
    return shortname_;
  }
  const std::string& help() const {
    return help_;
  }
  void set_required(bool required) {
    ARGPARSE_FAIL_IF(HasValue(),
                     "Argument with a default value can't be required");
    required_ = required;
  }
  bool required() const {
    return required_;
  }

  virtual OptionInfo RichOptionInfo() const {
    return OptionInfo{
        /*.fullname = */this->fullname(),
        /*.shortname = */this->shortname(),
        /*.help = */this->help(),
        /*.required = */this->required(),
        /*.default_value = */std::nullopt,
        /*.options = */std::nullopt,
    };
  }

private:
  std::string fullname_;
  char shortname_;
  std::string help_;
  bool required_;
};

class FlagHolder : public ArgHolderBase {
public:
  FlagHolder(std::string fullname, char shortname, std::string help)
      : ArgHolderBase(fullname, shortname, help)
      , value_(0) {}

  virtual bool HasValue() const override {
    return true;
  }

  virtual bool RequiresValue() const override {
    return false;
  }

  virtual void ProcessFlag() override {
    value_++;
  }

  virtual void ProcessValue(const std::string& value_str) override {
    (void)value_str;
    ARGPARSE_FAIL("Flags don't accept values (`" + this->fullname() + "`)");
  }

  size_t value() const {
    return value_;
  }

private:
  size_t value_;
};

template <typename Type>
class ValueHolderBase : public ArgHolderBase {
public:
  using ArgHolderBase::ArgHolderBase;

  virtual bool RequiresValue() const final {
    return true;
  }

  virtual void ProcessFlag() override final {
    ARGPARSE_FAIL("Argument requires a value (`" + this->fullname() + "`)");
  }

  virtual void ProcessValue(const std::string& value_str) override final {
    Type value = detail::TraitsProvider<Type>::FromString(value_str);
    ARGPARSE_FAIL_IF(options_ && !detail::IsValidValue(value, *options_),
                     "Provided argument string casts to an illegal value (`" +
                         this->fullname() + "`)");

    StoreValue(std::move(value));
  }

  virtual void StoreValue(Type value) = 0;

  void set_options(std::vector<Type> options) {
    static_assert(detail::TraitsProvider<Type>::kEqualComparable,
                  "No equality method available to compare values");
    static_assert(detail::TraitsProvider<Type>::kToStringCastable,
                  "Type of argument is not castable to string");
    ARGPARSE_FAIL_IF(options.empty(), "Set of options can't be empty (`" +
                                          this->fullname() + "`)");
    options_ = std::move(options);
  }

  virtual OptionInfo RichOptionInfo() const override {
    OptionInfo info = ArgHolderBase::RichOptionInfo();

    if (options_.has_value()) {
      std::vector<std::string> option_strings;
      for (const auto& option : *options_) {
        option_strings.push_back(
            detail::TraitsProvider<Type>::ToString(option));
      }
      info.options = util::JoinStrings(option_strings, ", ");
    }
    return info;
  }

private:
  std::optional<std::vector<Type>> options_;
};

template <typename Type>
class ValueHolder : public ValueHolderBase<Type> {
public:
  using ValueHolderBase<Type>::ValueHolderBase;

  virtual bool HasValue() const override {
    return value_.has_value() || default_value_.has_value();
  }

  virtual void StoreValue(Type value) override {
    ARGPARSE_FAIL_IF(value_.has_value(), "Argument accepts only one value (`" +
                                             this->fullname() + "`)");

    value_ = std::move(value);
  }

  void set_default(Type value) {
    static_assert(detail::TraitsProvider<Type>::kToStringCastable,
                  "Type of argument is not castable to string");
    ARGPARSE_FAIL_IF(this->required(),
                     "Required argument can't have a default value");
    default_value_ = std::move(value);
  }

  const Type& value() const {
    ARGPARSE_FAIL_IF(
        !this->HasValue(),
        "Trying to obtain a value from an argument that wasn't set (`" +
            this->fullname() + "`)");
    if (!value_.has_value()) {
      return *default_value_;
    }
    return *value_;
  }

  virtual OptionInfo RichOptionInfo() const override {
    OptionInfo info = ValueHolderBase<Type>::RichOptionInfo();
    if (default_value_) {
      info.default_value =
          detail::TraitsProvider<Type>::ToString(*default_value_);
    }

    return info;
  }

private:
  std::optional<Type> value_;
  std::optional<Type> default_value_;
};

template <typename Type>
class MultiValueHolder : public ValueHolderBase<Type> {
public:
  using ValueHolderBase<Type>::ValueHolderBase;

  virtual bool HasValue() const override {
    return !values_.empty() || !default_values_.empty();
  }

  virtual void StoreValue(Type value) override {
    values_.push_back(std::move(value));
  }

  const std::vector<Type>& values() const {
    if (values_.empty()) {
      return default_values_;
    }

    return values_;
  }

  void set_default(std::vector<Type> value) {
    static_assert(detail::TraitsProvider<Type>::kToStringCastable,
                  "Type of argument is not castable to string");
    ARGPARSE_FAIL_IF(this->required(),
                     "Required argument can't have a default value");
    default_values_ = std::move(value);
  }

  virtual OptionInfo RichOptionInfo() const override {
    OptionInfo info = ValueHolderBase<Type>::RichOptionInfo();
    if (!default_values_.empty()) {
      info.default_value = "";
      for (const auto& value : default_values_) {
        *info.default_value +=
            detail::TraitsProvider<Type>::ToString(value) + ", ";
      }
      info.default_value->pop_back();
      info.default_value->pop_back();
    }

    return info;
  }

private:
  std::vector<Type> values_;
  std::vector<Type> default_values_;
};

class FlagHolderWrapper {
public:
  FlagHolderWrapper(FlagHolder* ptr)
      : ptr_(ptr) {}

  size_t operator*() const {
    return ptr_->value();
  }

private:
  FlagHolder* ptr_;
};

template <typename Type>
class ArgHolderWrapper {
public:
  ArgHolderWrapper(ValueHolder<Type>* ptr)
      : ptr_(ptr) {}

  ArgHolderWrapper& Required() {
    ptr_->set_required(true);
    return *this;
  }

  ArgHolderWrapper& Default(Type value) {
    ptr_->set_default(std::move(value));
    return *this;
  }

  ArgHolderWrapper& Options(std::vector<Type> options) {
    ptr_->set_options(std::move(options));
    return *this;
  }

  explicit operator bool() const {
    return ptr_->HasValue();
  }

  const Type& operator*() const {
    return ptr_->value();
  }

  const Type* operator->() const {
    return &ptr_->value();
  }

private:
  ValueHolder<Type>* ptr_;
};

template <typename Type>
class MultiArgHolderWrapper {
public:
  MultiArgHolderWrapper(MultiValueHolder<Type>* ptr)
      : ptr_(ptr) {}

  MultiArgHolderWrapper& Required() {
    ptr_->set_required(true);
    return *this;
  }

  MultiArgHolderWrapper& Default(std::vector<Type> values) {
    ptr_->set_default(std::move(values));
    return *this;
  }

  MultiArgHolderWrapper& Options(std::vector<Type> options) {
    ptr_->set_options(std::move(options));
    return *this;
  }

  explicit operator bool() const {
    return !ptr_->values().empty();
  }

  const std::vector<Type>& operator*() const {
    return ptr_->values();
  }

  const std::vector<Type>* operator->() const {
    return &ptr_->values();
  }

private:
  MultiValueHolder<Type>* ptr_;
};

class Holders {
public:
  FlagHolderWrapper AddFlag(const std::string& fullname, char shortname,
                            const std::string& help = kDefaultHelpString) {
    CheckOptionEntry(fullname, shortname);
    UpdateShortLongMapping(fullname, shortname);
    auto holder = std::make_unique<FlagHolder>(fullname, shortname, help);
    FlagHolderWrapper wrapper(holder.get());
    holders_[fullname] = std::move(holder);
    return wrapper;
  }

  template <typename Type>
  ArgHolderWrapper<Type> AddArg(const std::string& fullname, char shortname,
                                const std::string& help = kDefaultHelpString) {
    CheckOptionEntry(fullname, shortname);
    UpdateShortLongMapping(fullname, shortname);
    auto holder =
        std::make_unique<ValueHolder<Type>>(fullname, shortname, help);
    ArgHolderWrapper<Type> wrapper(holder.get());
    holders_[fullname] = std::move(holder);
    return wrapper;
  }

  template <typename Type>
  MultiArgHolderWrapper<Type> AddMultiArg(
      const std::string& fullname, char shortname,
      const std::string& help = kDefaultHelpString) {
    CheckOptionEntry(fullname, shortname);
    UpdateShortLongMapping(fullname, shortname);
    auto holder =
        std::make_unique<MultiValueHolder<Type>>(fullname, shortname, help);
    MultiArgHolderWrapper<Type> wrapper(holder.get());
    holders_[fullname] = std::move(holder);
    return wrapper;
  }

  ArgHolderBase* GetHolderByFullName(const std::string& fullname) {
    auto it = holders_.find(fullname);
    if (it == holders_.end()) {
      return nullptr;
    }

    return it->second.get();
  }

  ArgHolderBase* GetHolderByShortName(char shortname) {
    auto im = short_long_mapping_.find(shortname);
    if (im == short_long_mapping_.end()) {
      return nullptr;
    }

    auto it = holders_.find(im->second);
    if (it == holders_.end()) {
      return nullptr;
    }

    return it->second.get();
  }

  void CheckOptionEntry(const std::string& fullname, char shortname) {
    ARGPARSE_FAIL_IF(fullname == "help", "`help` is a predefined option");
    ARGPARSE_FAIL_IF(holders_.count(fullname),
                     "Argument is already defined (`" + fullname + "`)");
    ARGPARSE_FAIL_IF(
        short_long_mapping_.count(shortname),
        std::string("Argument with shortname is already defined (`") +
            shortname + "`)");
  }

  void PerformPostParseCheck() const {
    for (auto& [name, arg] : holders_) {
      ARGPARSE_FAIL_IF(arg->required() && !arg->HasValue(),
                       "No value provided for option `" + name + "`");
    }
  }

  size_t Size() const {
    return holders_.size();
  }

  std::vector<OptionInfo> OptionInfos() const {
    std::vector<OptionInfo> result;
    for (const auto& [name, holder] : holders_) {
      (void)name;
      result.push_back(holder->RichOptionInfo());
    }

    return result;
  }

private:
  void UpdateShortLongMapping(const std::string& fullname, char shortname) {
    ARGPARSE_ASSERT(!short_long_mapping_.count(shortname));
    if (shortname != '\0') {
      short_long_mapping_[shortname] = fullname;
    }
  }

  std::unordered_map<std::string, std::unique_ptr<ArgHolderBase>> holders_;
  std::unordered_map<char, std::string> short_long_mapping_;
};

inline Holders* GlobalHolders() {
  static Holders* holders = new Holders;
  return holders;
}

inline std::string PositionalArgumentName(size_t position) {
  return "__positional_argument__" + std::to_string(position);
}

inline FlagHolderWrapper AddGlobalFlag(
    const std::string& fullname, char shortname,
    const std::string& help = kDefaultHelpString) {
  return GlobalHolders()->AddFlag(fullname, shortname, help);
}

inline FlagHolderWrapper AddGlobalFlag(
    const std::string& fullname, const std::string& help = kDefaultHelpString) {
  return AddGlobalFlag(fullname, '\0', help);
}

template <typename Type>
ArgHolderWrapper<Type> AddGlobalArg(
    const std::string& fullname, char shortname,
    const std::string& help = kDefaultHelpString) {
  return GlobalHolders()->AddArg<Type>(fullname, shortname, help);
}

template <typename Type>
ArgHolderWrapper<Type> AddGlobalArg(
    const std::string& fullname, const std::string& help = kDefaultHelpString) {
  return AddGlobalArg<Type>(fullname, '\0', help);
}

template <typename Type>
MultiArgHolderWrapper<Type> AddGlobalMultiArg(
    const std::string& fullname, char shortname,
    const std::string& help = kDefaultHelpString) {
  return GlobalHolders()->AddMultiArg<Type>(fullname, shortname, help);
}

template <typename Type>
MultiArgHolderWrapper<Type> AddGlobalMultiArg(
    const std::string& fullname, const std::string& help = kDefaultHelpString) {
  return AddGlobalMultiArg<Type>(fullname, '\0', help);
}

class Parser {
public:
  Parser()
      : holders_()
      , positionals_()
      , free_args_(std::nullopt)
      , parse_global_args_(true)
      , usage_string_(std::nullopt)
      , exit_code_(std::nullopt) {}

  FlagHolderWrapper AddFlag(const std::string& fullname, char shortname,
                            const std::string& help = kDefaultHelpString) {
    if (parse_global_args_) {
      GlobalHolders()->CheckOptionEntry(fullname, shortname);
    }
    return holders_.AddFlag(fullname, shortname, help);
  }

  FlagHolderWrapper AddFlag(const std::string& fullname,
                            const std::string& help = kDefaultHelpString) {
    return AddFlag(fullname, '\0', help);
  }

  template <typename Type>
  ArgHolderWrapper<Type> AddArg(const std::string& fullname, char shortname,
                                const std::string& help = kDefaultHelpString) {
    if (parse_global_args_) {
      GlobalHolders()->CheckOptionEntry(fullname, shortname);
    }
    return holders_.AddArg<Type>(fullname, shortname, help);
  }

  template <typename Type>
  ArgHolderWrapper<Type> AddArg(const std::string& fullname,
                                const std::string& help = kDefaultHelpString) {
    return AddArg<Type>(fullname, '\0', help);
  }

  template <typename Type>
  MultiArgHolderWrapper<Type> AddMultiArg(
      const std::string& fullname, char shortname,
      const std::string& help = kDefaultHelpString) {
    if (parse_global_args_) {
      GlobalHolders()->CheckOptionEntry(fullname, shortname);
    }
    return holders_.AddMultiArg<Type>(fullname, shortname, help);
  }

  template <typename Type>
  MultiArgHolderWrapper<Type> AddMultiArg(
      const std::string& fullname,
      const std::string& help = kDefaultHelpString) {
    return AddMultiArg<Type>(fullname, '\0', help);
  }

  template <typename Type>
  ArgHolderWrapper<Type> AddPositionalArg() {
    return positionals_.AddArg<Type>(
        PositionalArgumentName(positionals_.Size()), '\0');
  }

  template <typename... Types>
  std::tuple<ArgHolderWrapper<Types>...> AddPositionalArgs() {
    return std::tuple<ArgHolderWrapper<Types>...>{AddPositionalArg<Types>()...};
  }

  void EnableFreeArgs() {
    free_args_ = std::vector<std::string>();
  }

  void IgnoreGlobalFlags() {
    parse_global_args_ = false;
  }

  void ExitOnFailure(int exit_code,
                     std::optional<std::string> usage_string = std::nullopt) {
    exit_code_ = exit_code;
    usage_string_ = std::move(usage_string);
  }

  void ParseArgs(const std::vector<std::string>& args) {
    try {
      DoParseArgs(args);
    } catch (const ArgparseError& error) {
      if (!exit_code_) {
        throw;
      }
      if (usage_string_) {
        std::cerr << *usage_string_;
      } else {
        std::cerr << "Failed to parse arguments. Error message: "
                  << error.what() << "\n\n";
        std::cerr << DefaultUsageString(args[0]) << "\n";
      }
      exit(*exit_code_);
    }
  }

  void ParseArgs(int argc, char* argv[]) {
    std::vector<std::string> args;
    for (int i = 0; i < argc; i++) {
      args.push_back(argv[i]);
    }

    ParseArgs(args);
  }

  const std::vector<std::string>& FreeArgs() const {
    return *free_args_;
  }

private:
  void DoParseArgs(const std::vector<std::string>& args) {
    size_t positional_arg_count = 0;
    for (size_t i = 1, step; i < args.size(); i += step) {
      if (args[i].length() > 2 && args[i].substr(0, 2) == "--") {
        step = ParseLongArg(args, i);
        ARGPARSE_ASSERT(step > 0);
        continue;
      }

      if (args[i].length() > 1 && args[i][0] == '-') {
        step = ParseShortArgs(args, i);
        ARGPARSE_ASSERT(step > 0);
        continue;
      }

      if (positional_arg_count < positionals_.Size()) {
        ArgHolderBase* holder = GetPositionalArgById(positional_arg_count++);
        ARGPARSE_ASSERT(holder != nullptr);
        holder->ProcessValue(detail::EscapeValue(args[i]));
        step = 1;
        continue;
      }

      ARGPARSE_FAIL_IF(!free_args_, "Free arguments are not enabled");

      free_args_->push_back(detail::EscapeValue(args[i]));

      step = 1;
    }

    PerformPostParseCheck();
  }

  void PerformPostParseCheck() const {
    if (parse_global_args_) {
      GlobalHolders()->PerformPostParseCheck();
    }

    holders_.PerformPostParseCheck();

    positionals_.PerformPostParseCheck();
  }

  size_t ParseLongArg(const std::vector<std::string>& args, size_t offset) {
    ARGPARSE_ASSERT(args[offset].length() > 2 &&
                    args[offset].substr(0, 2) == "--");

    auto [name, value] = detail::SplitLongArg(args[offset].substr(2));
    ArgHolderBase* holder = GetHolderByFullName(name);
    ARGPARSE_FAIL_IF(holder == nullptr, "Unknown long option (`" + name + "`)");

    if (value) {
      ARGPARSE_FAIL_IF(!holder->RequiresValue(),
                       "Long option doesn't require a value (`" + name + "`)");

      holder->ProcessValue(*value);
      return 1;
    }

    if (!holder->RequiresValue()) {
      holder->ProcessFlag();
      return 1;
    }

    ARGPARSE_FAIL_IF(offset + 1 >= args.size(),
                     "No value provided for a long option (`" + name + "`)");

    holder->ProcessValue(args[offset + 1]);
    return 2;
  }

  size_t ParseShortArgs(const std::vector<std::string>& args, size_t offset) {
    ARGPARSE_ASSERT(args[offset].length() > 1 && args[offset][0] == '-');

    const std::string& arg = args[offset];

    for (size_t i = 1; i < arg.length(); i++) {
      char ch = arg[i];
      ArgHolderBase* holder = GetHolderByShortName(ch);
      ARGPARSE_FAIL_IF(holder == nullptr,
                       std::string("Unknown short option (`") + ch + "`)");

      if (holder->RequiresValue()) {
        if (i + 1 == arg.length()) {
          // last short option of a group requiring a value
          // (e.g. -euxo pipefail)
          ARGPARSE_FAIL_IF(
              offset + 1 >= args.size(),
              std::string("No value provided for a short option (`") + ch +
                  "`)");

          holder->ProcessValue(args[offset + 1]);
          return 2;
        }

        if (i == 1 && arg.length() > 2) {
          // option like -j5
          holder->ProcessValue(arg.substr(2));
          return 1;
        }

        ARGPARSE_FAIL(
            "Short option requiring an argument is not allowed in the middle "
            "of short options group");
      }

      holder->ProcessFlag();
    }

    return 1;
  }

  ArgHolderBase* GetPositionalArgById(size_t id) {
    return positionals_.GetHolderByFullName(PositionalArgumentName(id));
  }

  ArgHolderBase* GetHolderByFullName(const std::string& fullname) {
    if (parse_global_args_) {
      auto holder = GlobalHolders()->GetHolderByFullName(fullname);
      if (holder) {
        return holder;
      }
    }

    return holders_.GetHolderByFullName(fullname);
  }

  ArgHolderBase* GetHolderByShortName(char shortname) {
    if (parse_global_args_) {
      auto holder = GlobalHolders()->GetHolderByShortName(shortname);
      if (holder) {
        return holder;
      }
    }

    return holders_.GetHolderByShortName(shortname);
  }

  static std::string OptionsDescription(Holders& holders) {
    static const size_t kSecondColumnIndent = 24;
    std::string description;
    auto options = holders.OptionInfos();
    std::sort(options.begin(), options.end(),
              [](const auto& opt1, const auto& opt2) {
                return opt1.fullname < opt2.fullname;
              });
    for (auto& info : options) {
      std::string line = "  ";
      if (info.shortname != '\0') {
        line += std::string("-") + info.shortname + ", ";
      } else {
        line += "    ";
      }
      line += "--" + std::string(info.fullname.begin(), info.fullname.end()) +
              "        ";
      for (size_t i = line.length(); i < kSecondColumnIndent; i++) {
        line.push_back(' ');
      }
      line += info.help;
      if (info.required || info.default_value || info.options) {
        std::vector<std::string> parts;
        if (info.required) {
          parts.push_back("required");
        }
        if (info.default_value) {
          parts.push_back("default: " + *info.default_value);
        }
        if (info.options) {
          parts.push_back("options: " + *info.options);
        }
        line += " (" + util::JoinStrings(parts, ", ") + ")";
      }
      description += line + "\n";
    }

    return description;
  }

  std::string DefaultUsageString(const std::string& argv0) {
    std::string help_string = "Usage: " + argv0;
    if (positionals_.Size() > 0) {
      help_string += " POSITIONALS";
    }
    help_string += " OPTIONS";
    help_string += "\n";
    help_string += "\n";

    if (parse_global_args_) {
      help_string += "Global options:\n";
      help_string += OptionsDescription(*GlobalHolders());
      help_string += "\n";
    }

    help_string += "Options:\n";
    help_string += OptionsDescription(holders_);

    return help_string;
  }

  Holders holders_;
  Holders positionals_;
  std::optional<std::vector<std::string>> free_args_;
  bool parse_global_args_;
  std::optional<std::string> usage_string_;
  std::optional<int> exit_code_;
};

}  // namespace argparse

#define ARGPARSE_DECLARE_GLOBAL_FLAG(name) \
  extern ::argparse::FlagHolderWrapper name
#define ARGPARSE_DECLARE_GLOBAL_ARG(Type, name) \
  extern ::argparse::ArgHolderWrapper<Type> name
#define ARGPARSE_DECLARE_GLOBAL_MULTIARG(Type, name) \
  extern ::argparse::MultiArgHolderWrapper<Type> name
