#pragma once

#include <any>
#include <map>
#include <tuple>
#include <string>
#include <memory>
#include <variant>
#include <optional>
#include <typeinfo>
#include <exception>
#include <functional>
#include <type_traits>

namespace parsec {
#if __cplusplus > 201703L

template<typename T>
struct lambda_traits_base;
template<typename ClassType, typename R, typename ...Args>
struct lambda_traits_base<R(ClassType::*)(Args...) const>
{
    using result_type = R;
    using args_type = std::tuple<Args...>;
    template<size_t index>
    using arg_type_at = std::tuple_element_t<index, args_type>;
    static constexpr size_t arity = sizeof...(Args);
};

template<typename T, typename = std::void_t<>>
struct call_which
{
    using type = decltype(&T::operator());
};
template<typename T>
struct call_which<T, std::void_t<decltype(&T::template operator()<>)>>
{
    using type = decltype(&T::template operator()<>);
};
template<typename T>
using call_which_t = typename call_which<T>::type;

template<typename T, typename ...Args>
struct lambda_traits
  : lambda_traits_base<decltype(&T::template operator()<Args...>)> {};
template<typename T>
struct lambda_traits<T> : lambda_traits_base<call_which_t<T>> {};

#else
template<typename T>
struct lambda_traits_base;
template<typename ClassType, typename R, typename ...Args>
struct lambda_traits_base<R(ClassType::*)(Args...) const>
{
    using result_type = R;
    using args_type = std::tuple<Args...>;
    template<size_t index>
    using arg_type_at = std::tuple_element_t<index, args_type>;
    static constexpr size_t arity = sizeof...(Args);
};

template<typename T>
struct lambda_traits : lambda_traits_base<decltype(&T::operator())> {};

#endif

template<typename T>
struct is_tuple
{
    static constexpr bool value = false;
};
template<typename ...Ts>
struct is_tuple<std::tuple<Ts...>>
{
    static constexpr bool value = true;
};
template<typename T>
constexpr bool is_tuple_v = is_tuple<T>::value;

template<typename T1, typename T2>
struct concat;
template<typename T1, typename T2>
struct addition;
template<typename T1, typename T2>
struct product;

template<typename T1, typename T2>
using concat_t = typename concat<T1, T2>::type;
template<typename T1, typename T2>
using addition_t = typename addition<T1, T2>::type;
template<typename T1, typename T2>
using product_t = typename product<T1, T2>::type;

template<typename T1, typename ...T2s>
struct concat<T1, std::variant<T2s...>>
{
    using type = std::variant<T1, T2s...>;
};
template<typename ...T1s, typename ...T2s>
struct concat<std::variant<T1s...>, std::variant<T2s...>>
{
    using type = std::variant<T1s..., T2s...>;
};

template<typename T1, typename T2>
struct addition
{
    using type = std::conditional_t<std::is_same_v<T1, T2>,
        T1, std::variant<T1, T2>>;
};
template<typename T1, typename T2>
struct addition<std::variant<T1>, T2>
{
    using type = std::conditional_t<std::is_same_v<T1, T2>,
        std::variant<T1>, std::variant<T1 ,T2>>;
};
template<typename T1, typename T2>
struct addition<T1, std::variant<T2>>
{
    using type = std::conditional_t<std::is_same_v<T1, T2>,
        std::variant<T1>, std::variant<T1 ,T2>>;
};
template<typename T1, typename ...T1s, typename T2>
struct addition<std::variant<T1, T1s...>, T2>
{
    using type = std::conditional_t<std::is_same_v<T1, T2>,
        std::variant<T1, T1s...>,
        concat_t<T1, addition_t<std::variant<T1s...>, T2>>>;
};
template<typename T1, typename T2, typename ...T2s>
struct addition<T1, std::variant<T2, T2s...>>
{
    using type = std::conditional_t<std::is_same_v<T1, T2>,
        std::variant<T2, T2s...>,
        concat_t<T2, addition_t<std::variant<T2s...>, T1>>>;
};
template<typename T1, typename T2>
struct addition<std::variant<T1>, std::variant<T2>>
{
    using type = std::conditional_t<std::is_same_v<T1, T2>,
        std::variant<T1>, std::variant<T1, T2>>;
};
template<typename T1, typename T2, typename ...T2s>
struct addition<std::variant<T1>, std::variant<T2, T2s...>>
{
    using type = std::conditional_t<std::is_same_v<T1, T2>,
        std::variant<T2, T2s...>,
        concat_t<T2, addition_t<T1, std::variant<T2s...>>>>;
};
template<typename T1, typename ...T1s, typename ...T2s>
struct addition<std::variant<T1, T1s...>, std::variant<T2s...>>
{
    using type = concat_t<addition_t<T1, std::variant<T2s...>>,
        addition_t<std::variant<T1s...>, std::variant<T2s...>>>;
};

template<typename T1, typename T2>
struct product
{
    using type = std::tuple<T1, T2>;
};
template<typename ...T1s, typename T2>
struct product<std::tuple<T1s...>, T2>
{
    using type = std::tuple<T1s..., T2>;
};
template<typename T1, typename ...T2s>
struct product<T1, std::tuple<T2s...>>
{
    using type = std::tuple<T1, T2s...>;
};
template<typename ...T1s, typename ...T2s>
struct product<std::tuple<T1s...>, std::tuple<T2s...>>
{
    using type = std::tuple<T1s..., T2s...>;
};

struct Placeholder
{
    Placeholder() = default;
    template<typename T>
    Placeholder(const T &&) {}
};

template<typename Result>
class ParsecComponent;

template<typename Result>
class Parsec;

inline
std::map<std::tuple<void *, const std::string *, size_t>,
    std::pair<void *, size_t>>
theMemory;

template<typename Func>
inline auto
get_result(const Func &func,
    const std::string &str, std::size_t &index)
{
    auto key = std::make_tuple<void *>((void *)&func, &str, index);
    if (theMemory.count(key))
    {
        auto pir = move(theMemory[key]);
        theMemory.erase(key);

        index = pir.second;

        auto ptr = reinterpret_cast<decltype(func(str, index)) *>(pir.first);
        auto value = std::move(*ptr);
        delete ptr;
        return value;
    }
    else
    {
        return func(str, index);
    }
}

template<typename Func, typename T>
inline void
put_in_mem(const Func &func,
    const std::string &str, size_t index,
    T &&value, size_t target)
{
    theMemory[std::make_tuple<void *>((void *)&func, &str, index)]
        = std::make_pair<void *>(new T(std::move(value)), target);
}

template<typename Result, typename Func1, typename Func2>
inline std::optional<Result>
callback_template(const Func1 &exec, const Func2 &callback,
    const std::string &str, std::size_t &index)
{
    auto anchor = index;
    auto result = get_result(exec, str, index);
    if (result)
    {
        auto v = std::move(result).value();
        if constexpr (is_tuple_v<decltype(v)>)
        {
            return std::apply(callback, std::move(v));
        }
        else
        {
            return callback(std::move(v));
        }
    }
    else
    {
        index = anchor;
        return {};
    }
}

template<typename Result, typename Func1, typename Func2>
inline std::optional<Result>
alternate_template(const Func1 &lexec, const Func2 &rexec,
    const std::string &str, std::size_t &index)
{
    auto anchor = index;
    auto lr = get_result(lexec, str, index);
    if (lr)
    {
        return lr;
    }
    else
    {
        index = anchor;
        auto rr = get_result(rexec, str, index);
        if (rr)
        {
            return rr;
        }
        else
        {
            index = anchor;
            return {};
        }
    }
}

template<typename Result, typename Func1, typename Func2>
inline std::optional<Result>
connect_template(const Func1 &lexec, const Func2 &rexec,
    const std::string &str, std::size_t &index)
{
    auto anchor = index;
    auto lr = get_result(lexec, str, index);
    if (lr)
    {
        auto lrtarget = index;
        auto rr = get_result(rexec, str, index);
        if (rr)
        {
            auto lv = std::move(lr).value();
            auto rv = std::move(rr).value();
            if constexpr (is_tuple_v<decltype(lv)> and is_tuple_v<decltype(rv)>)
            {
                return std::tuple_cat(std::move(lv), std::move(rv));
            }
            else if constexpr (is_tuple_v<decltype(lv)>)
            {
                return std::tuple_cat(std::move(lv), std::make_tuple(std::move(rv)));
            }
            else if constexpr (is_tuple_v<decltype(rv)>)
            {
                return std::tuple_cat(std::make_tuple(std::move(lv)), std::move(rv));
            }
            else
            {
                return std::make_tuple(std::move(lv), std::move(rv));
            };
        }
        else
        {
            index = anchor;
            put_in_mem(lexec, str, anchor, move(lr), lrtarget);
            return {};
        }
    }
    else
    {
        index = anchor;
        return {};
    }
}

// Result must be a single type, Tuple or Variant
template<typename Result>
class ParsecComponent
{
  public:
    using OptResult = std::optional<Result>;
    using ExecFunc = std::function<OptResult(const std::string &, std::size_t &)>;

    ParsecComponent() = default;
    ParsecComponent(ParsecComponent &&component)
      : exec_(std::move(component.exec_)) {}
    ParsecComponent(ExecFunc exec)
      : exec_(std::move(exec)) {}

    OptResult
    operator()(const std::string &str, std::size_t &index) const
    {
        return exec_(str, index);
    }

    template<typename Func,
        typename NewResult = typename lambda_traits<Func>::result_type>
    ParsecComponent<NewResult>
    operator>>(Func &&callback) const
    {
        return ParsecComponent<NewResult>(
            [exec = exec_, cb = std::move(callback)]
            (const std::string &str, std::size_t &index)
            {
                return callback_template<NewResult>(exec, cb, str, index);
            });
    }

    template<typename RhsResult,
        typename NewResult = addition_t<Result, RhsResult>>
    ParsecComponent<NewResult>
    operator|(const ParsecComponent<RhsResult> &rhs) const
    {
        return ParsecComponent<NewResult>(
            [lexec = exec_, rexec = rhs.exec()]
            (const std::string &str, std::size_t &index)
            {
                return alternate_template<NewResult>(lexec, rexec, str, index);
            });
    }

    template<typename RhsResult,
        typename NewResult = product_t<Result, RhsResult>>
    ParsecComponent<NewResult>
    operator+(const ParsecComponent<RhsResult> &rhs) const
    {
        return ParsecComponent<NewResult>(
            [lexec = exec_, rexec = rhs.exec()]
            (const std::string &str, std::size_t &index)
            {
                return connect_template<NewResult>(lexec, rexec, str, index);
            });
    }

    template<typename RhsResult>
    ParsecComponent<addition_t<Result, RhsResult>>
    operator|(const Parsec<RhsResult> &rhs) const;

    template<typename RhsResult>
    ParsecComponent<product_t<Result, RhsResult>>
    operator+(const Parsec<RhsResult> &rhs) const;

    auto &exec() const
    {
        return exec_;
    }

  private:
    std::function<OptResult(const std::string &, std::size_t &)> exec_;
};

template<typename Result>
class Parsec final
{
  public:
    Parsec() : component_(nullptr) {};
    Parsec(ParsecComponent<Result> &&component)
      : component_(std::make_shared<ParsecComponent<Result>(std::move(component))>) {}

    auto component() const
    {
        return component_;
    }

    template<typename RecvResult>
    Parsec
    &operator=(ParsecComponent<RecvResult> &&component)
    {
        if constexpr (std::is_same_v<RecvResult, Result>)
        {
            component_ = std::make_shared<ParsecComponent<Result>>(std::move(component));
        }
        else
        {
            static_assert(std::is_convertible_v<RecvResult, Result>);
            component_ = std::make_shared<ParsecComponent<Result>>(
                [exec = component.exec()]
                (const std::string &str, std::size_t &index)
                    -> std::optional<Result>
                {
                    return exec(str, index);
                });
        }
        return *this;
    }

    template<typename RecvResult>
    Parsec
    &operator=(const Parsec<RecvResult> &recv)
    {
        static_assert(std::is_convertible_v<RecvResult, Result>);
        component_ = std::make_shared<ParsecComponent<Result>>(
            [&recv]
            (const std::string &str, std::size_t &index)
                -> std::optional<Result>
            {
                return recv.component()->operator()(str, index);
            });
        return *this;
    }

    Result
    operator()(const std::string &str) const
    {
        std::size_t index = 0;
        return operator()(str, index);
    }

    Result
    operator()(const std::string &str, std::size_t &index) const
    {
        if (component_)
        {
            auto r = component_->operator()(str, index);
            if (r)
            {
                return std::move(r).value();
            }
            else
            {
                throw std::runtime_error("parse error at " + std::to_string(index));
            }
        }
        else
        {
            throw std::runtime_error("not give definition");
        }
    }

    template<typename Func,
        typename NewResult = typename lambda_traits<Func>::result_type>
    ParsecComponent<NewResult>
    operator>>(Func &&callback)
    {
        return ParsecComponent<NewResult>(
            [this, cb = std::move(callback)]
            (const std::string &str, std::size_t &index)
            {
                return callback_template<NewResult>(
                    this->component()->exec(), cb, str, index);
            });
    }

    template<typename RhsResult,
        typename NewResult = addition_t<Result, RhsResult>>
    ParsecComponent<NewResult>
    operator|(const Parsec<RhsResult> &rhs)
    {
        return ParsecComponent<NewResult>(
            [this, &rhs]
            (const std::string &str, std::size_t &index)
            {
                return alternate_template<NewResult>(
                    this->component()->exec(),
                    rhs.component()->exec(), str, index);
            });
    }

    template<typename RhsResult,
        typename NewResult = product_t<Result, RhsResult>>
    ParsecComponent<NewResult>
    operator+(const Parsec<RhsResult> &rhs)
    {
        return ParsecComponent<NewResult>(
            [this, &rhs]
            (const std::string &str, std::size_t &index)
            {
                return connect_template<NewResult>(
                    this->component()->exec(),
                    rhs.component()->exec(), str, index);
            });
    }

    template<typename RhsResult,
        typename NewResult = addition_t<Result, RhsResult>>
    ParsecComponent<NewResult>
    operator|(const ParsecComponent<RhsResult> &rhs)
    {
        return ParsecComponent<NewResult>(
            [this, rexec = rhs.exec()]
            (const std::string &str, std::size_t &index)
            {
                return alternate_template<NewResult>(
                    this->component()->exec(), rexec, str, index);
            });
    }

    template<typename RhsResult,
        typename NewResult = product_t<Result, RhsResult>>
    ParsecComponent<NewResult>
    operator+(const ParsecComponent<RhsResult> &rhs)
    {
        return ParsecComponent<NewResult>(
            [this, rexec = rhs.exec()]
            (const std::string &str, std::size_t &index)
            {
                return connect_template<NewResult>(
                    this->component()->exec(), rexec, str, index);
            });
    }

  private:
    std::shared_ptr<ParsecComponent<Result>> component_;
};

template<typename Result>
template<typename RhsResult>
inline
ParsecComponent<addition_t<Result, RhsResult>>
ParsecComponent<Result>::operator|(const Parsec<RhsResult> &rhs) const
{
    using NewResult = addition_t<Result, RhsResult>;
    return ParsecComponent<NewResult>(
        [lexec = exec_, &rhs]
        (const std::string &str, std::size_t &index)
        {
            return alternate_template<NewResult>(lexec,
                rhs.component()->exec(), str, index);
        });
}

template<typename Result>
template<typename RhsResult>
inline
ParsecComponent<product_t<Result, RhsResult>>
ParsecComponent<Result>::operator+(const Parsec<RhsResult> &rhs) const
{
    using NewResult = product_t<Result, RhsResult>;
    return ParsecComponent<NewResult>(
        [lexec = exec_, &rhs]
        (const std::string &str, std::size_t &index)
        {
            return connect_template<NewResult>(lexec,
                rhs.component()->exec(), str, index);
        });
}

struct Token
{
    static
    ParsecComponent<char>
    by(const std::function<bool(char)> &cond)
    {
        return ParsecComponent<char>(
            [cond]
            (const std::string &str, std::size_t &index)
                -> std::optional<char>
            {
                if (index < str.size() && cond(str[index]))
                {
                    return str[index++];
                }
                else
                {
                    return {};
                }
            });
    }

    template<typename R>
    static
    ParsecComponent<R>
    epsilon()
    {
        return ParsecComponent<R>([]
            (const std::string &str, std::size_t &index)
                -> std::optional<R>
            {
                return R();
            });
    }

    template<typename Func,
        typename R = typename lambda_traits<Func>::result_type>
    static
    ParsecComponent<R>
    epsilon(Func &&func)
    {
        if constexpr (lambda_traits<Func>::arity == 0)
        {
            return ParsecComponent<R>(
                [f = std::move(func)]
                (const std::string &str, std::size_t &index)
                    -> std::optional<R>
                {
                    return f();
                });
        }
        else if constexpr (lambda_traits<Func>::arity == 2
            and std::is_same_v<lambda_traits<Func>::template arg_type_at<0>,
                const std::string &>
            and std::is_same_v<lambda_traits<Func>::template arg_type_at<1>,
                std::size_t &>)
        {
            return ParsecComponent<R>(
                [f = std::move(func)]
                (const std::string &str, std::size_t &index)
                    -> std::optional<R>
                {
                    return f(str, index);
                });
        }
        else
        {
            return epsilon();
        }
    }
};

inline
ParsecComponent<char>
operator""_T(char ch)
{
    return ParsecComponent<char>(
        [ch]
        (const std::string &str, std::size_t &index)
            -> std::optional<char>
        {
            if (index < str.size() && str[index] == ch)
            {
                return str[index++];
            }
            else
            {
                return {};
            }
        });
}

inline
ParsecComponent<std::string>
operator""_T(const char *str, std::size_t len)
{
    return ParsecComponent<std::string>(
        [pattern = std::string(str)]
        (const std::string &str, std::size_t &index)
            -> std::optional<std::string>
        {
            for (auto c : pattern)
            {
                if (index < str.size()
                    and str[index] == c)
                {
                    ++index;
                }
                else
                {
                    return {};
                }
            }
            return pattern;
        });
}
}
