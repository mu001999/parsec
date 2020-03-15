#pragma once

#include <any>
#include <tuple>
#include <string>
#include <memory>
#include <variant>
#include <optional>
#include <exception>
#include <functional>
#include <type_traits>

namespace parsec {
template<typename T> struct IsTuple {
    static constexpr bool value = false;
};
template<typename ...Ts> struct IsTuple<std::tuple<Ts...>> {
    static constexpr bool value = true;
};
template<typename T> constexpr bool IsTuple_v = IsTuple<T>::value;

template<typename T1, typename T2> struct Concat;
template<typename T1, typename T2> struct Addition;
template<typename T1, typename T2> struct Product;

template<typename T1, typename T2> using Concat_t = typename Concat<T1, T2>::type;
template<typename T1, typename T2> using Addition_t = typename Addition<T1, T2>::type;
template<typename T1, typename T2> using Product_t = typename Product<T1, T2>::type;

template<typename T1, typename ...T2s> struct Concat<T1, std::variant<T2s...>> {
    using type = std::variant<T1, T2s...>;
};
template<typename ...T1s, typename ...T2s> struct Concat<std::variant<T1s...>, std::variant<T2s...>> {
    using type = std::variant<T1s..., T2s...>;
};

template<typename T1, typename T2> struct Addition {
    using type = std::conditional_t<std::is_same_v<T1, T2>, T1, std::variant<T1, T2>>;
};
template<typename T1, typename T2> struct Addition<std::variant<T1>, T2> {
    using type = std::conditional_t<std::is_same_v<T1, T2>, std::variant<T1>, std::variant<T1 ,T2>>;
};
template<typename T1, typename T2> struct Addition<T1, std::variant<T2>> {
    using type = std::conditional_t<std::is_same_v<T1, T2>, std::variant<T1>, std::variant<T1 ,T2>>;
};
template<typename T1, typename ...T1s, typename T2> struct Addition<std::variant<T1, T1s...>, T2> {
    using type = std::conditional_t<std::is_same_v<T1, T2>, std::variant<T1, T1s...>, Concat_t<T1, Addition_t<std::variant<T1s...>, T2>>>;
};
template<typename T1, typename T2, typename ...T2s> struct Addition<T1, std::variant<T2, T2s...>> {
    using type = std::conditional_t<std::is_same_v<T1, T2>, std::variant<T2, T2s...>, Concat_t<T2, Addition_t<std::variant<T2s...>, T1>>>;
};
template<typename T1, typename T2> struct Addition<std::variant<T1>, std::variant<T2>> {
    using type = std::conditional_t<std::is_same_v<T1, T2>, std::variant<T1>, std::variant<T1, T2>>;
};
template<typename T1, typename T2, typename ...T2s> struct Addition<std::variant<T1>, std::variant<T2, T2s...>> {
    using type = std::conditional_t<std::is_same_v<T1, T2>, std::variant<T2, T2s...>, Concat_t<T2, Addition_t<T1, std::variant<T2s...>>>>;
};
template<typename T1, typename ...T1s, typename ...T2s> struct Addition<std::variant<T1, T1s...>, std::variant<T2s...>> {
    using type = Concat_t<Addition_t<T1, std::variant<T2s...>>, Addition_t<std::variant<T1s...>, std::variant<T2s...>>>;
};

template<typename T1, typename T2> struct Product {
    using type = std::tuple<T1, T2>;
};
template<typename ...T1s, typename T2> struct Product<std::tuple<T1s...>, T2> {
    using type = std::tuple<T1s..., T2>;
};
template<typename T1, typename ...T2s> struct Product<T1, std::tuple<T2s...>> {
    using type = std::tuple<T1, T2s...>;
};
template<typename ...T1s, typename ...T2s> struct Product<std::tuple<T1s...>, std::tuple<T2s...>> {
    using type = std::tuple<T1s..., T2s...>;
};
template<typename T1, typename T2> struct Product<std::variant<T1>, T2> {
    using type = std::variant<std::tuple<T1, T2>>;
};
template<typename T1, typename T2> struct Product<T1, std::variant<T2>> {
    using type = std::variant<std::tuple<T1, T2>>;
};
template<typename T1, typename ...T1s, typename T2> struct Product<std::variant<T1, T1s...>, T2> {
    using type = Concat_t<Product_t<T1, T2>, Product_t<std::variant<T1s...>, T2>>;
};
template<typename T1, typename T2, typename ...T2s> struct Product<T1, std::variant<T2, T2s...>> {
    using type = Concat_t<Product_t<T1, T2>, Product_t<T1, std::variant<T2s...>>>;
};
template<typename T1, typename ...T2s> struct Product<std::variant<T1>, std::variant<T2s...>> {
    using type = Product_t<T1, std::variant<T2s...>>;
};
template<typename T1, typename ...T1s, typename ...T2s> struct Product<std::variant<T1, T1s...>, std::variant<T2s...>> {
    using type = Addition_t<Product_t<T1, std::variant<T2s...>>, Product_t<std::variant<T1s...>, std::variant<T2s...>>>;
};

class ParsecComponentBase {
  public:
    virtual ~ParsecComponentBase() = default;
};

template<typename Result>
class ParsecComponent;

template<typename Result>
class Parsec;

template<typename Result, typename Func1, typename Func2>
inline std::optional<Result> callback_template(Func1 exec, Func2 callback, const std::string &str, std::size_t &index) {
    auto anchor = index;
    auto result = exec(str, index);
    if (result) {
        auto v = result.value();
        if constexpr (IsTuple_v<decltype(v)>) {
            return std::apply(callback, v);
        } else {
            return callback(v);
        }
    } else {
        index = anchor;
        return {};
    }
}

template<typename Result, typename Func1, typename Func2>
inline std::optional<Result> alternate_template(Func1 lexec, Func2 rexec, const std::string &str, std::size_t &index) {
    auto anchor = index;
    auto lr = lexec(str, index);
    if (lr) {
        return lr;
    } else {
        index = anchor;
        auto rr = rexec(str, index);
        if (rr) {
            return rr;
        } else {
            index = anchor;
            return {};
        }
    }
}

template<typename Result, typename Func1, typename Func2>
inline std::optional<Result> connect_template(Func1 lexec, Func2 rexec, const std::string &str, std::size_t &index) {
    auto anchor = index;
    auto lr = lexec(str, index);
    if (lr) {
        anchor = index;
        auto rr = rexec(str, index);
        if (rr) {
            auto lv = lr.value();
            auto rv = rr.value();
            if constexpr (IsTuple_v<decltype(lv)> and IsTuple_v<decltype(rv)>) {
                return std::tuple_cat(lv, rv);
            } else if constexpr (IsTuple_v<decltype(lv)>) {
                return std::tuple_cat(lv, std::make_tuple(rv));
            } else if constexpr (IsTuple_v<decltype(rv)>) {
                return std::tuple_cat(std::make_tuple(lv), rv);
            } else {
                return std::make_tuple(lv, rv);
            };
        } else {
            index = anchor;
            return {};
        }
    } else {
        index = anchor;
        return {};
    }
}

// Result must be a single type, Tuple or Variant
template<typename Result>
class ParsecComponent : public ParsecComponentBase {
  public:
    using ResultType = std::optional<Result>;

    ParsecComponent() = default;
    ParsecComponent(ParsecComponent &&component)
      : exec_(std::move(component.exec_)) {}
    virtual ~ParsecComponent() = default;

    ResultType operator()(const std::string &str, std::size_t &index) const {
        return exec_(str, index);
    }

    template<typename Func>
    auto operator>>(Func &&callback) const {
        std::function cb = std::move(callback);
        using NewResult = typename decltype(cb)::result_type;
        ParsecComponent<NewResult> component;
        component.set_exec([exec = this->exec(), cb](const std::string &str, std::size_t &index) {
            return callback_template<NewResult>(exec, cb, str, index);
        });
        return component;
    }

    template<typename RhsResult,
        typename NewResult = Addition_t<Result, RhsResult>>
    ParsecComponent<NewResult> operator|(const ParsecComponent<RhsResult> &rhs) const {
        ParsecComponent<NewResult> component;
        component.set_exec([lexec = this->exec(), rexec = rhs.exec()](const std::string &str, std::size_t &index) {
            return alternate_template<NewResult>(lexec, rexec, str, index);
        });
        return component;
    }

    template<typename RhsResult,
        typename NewResult = Product_t<Result, RhsResult>>
    ParsecComponent<NewResult> operator+(const ParsecComponent<RhsResult> &rhs) const {
        ParsecComponent<NewResult> component;
        component.set_exec([lexec = this->exec(), rexec = rhs.exec()](const std::string &str, std::size_t &index) {
            return connect_template(lexec, rexec, str, index);
        });
        return component;
    }

    template<typename RhsResult>
    ParsecComponent<Addition_t<Result, RhsResult>> operator|(const Parsec<RhsResult> &rhs) const;

    template<typename RhsResult>
    ParsecComponent<Product_t<Result, RhsResult>> operator+(const Parsec<RhsResult> &rhs) const;

    void set_exec(std::function<ResultType(const std::string &, std::size_t &)> &&exec) {
        exec_ = std::move(exec);
    }

    std::function<ResultType(const std::string &, std::size_t &)> exec() const {
        return exec_;
    }

    friend ParsecComponent<char> operator""_T(char ch);

  private:
    std::function<ResultType(const std::string &, std::size_t &)> exec_;
};

template<typename Result>
class Parsec {
  public:
    Parsec() : component_(nullptr) {};

    std::shared_ptr<ParsecComponent<Result>> component() const {
        return std::static_pointer_cast<ParsecComponent<Result>>(component_);
    }

    Parsec &operator=(ParsecComponent<Result> &&component) {
        component_ = std::make_shared<ParsecComponent<Result>>(std::move(component));
        return *this;
    }

    Result operator()(const std::string &str) const {
        std::size_t index = 0;
        return operator()(str, index);
    }

    Result operator()(const std::string &str, std::size_t &index) const {
        if (component_) {
            auto r = component()->operator()(str, index);
            if (r) {
                return r.value();
            } else {
                throw std::runtime_error("parse error at " + std::to_string(index));
            }
        } else {
            throw std::runtime_error("not give definition");
        }
    }

    template<typename Func>
    auto operator>>(Func &&callback) {
        std::function cb = std::move(callback);
        using NewResult = typename decltype(cb)::result_type;
        ParsecComponent<NewResult> component;
        component.set_exec([this, cb](const std::string &str, std::size_t &index) {
            return callback_template<NewResult>(this->component()->exec(), cb, str, index);
        });
        return component;
    }

    template<typename RhsResult,
        typename NewResult = Addition_t<Result, RhsResult>>
    ParsecComponent<NewResult> operator|(const Parsec<RhsResult> &rhs) {
        ParsecComponent<NewResult> component;
        component.set_exec([this, &rhs](const std::string &str, std::size_t &index) {
            return alternate_template<NewResult>(this->component()->exec(), rhs.component()->exec(), str, index);
        });
        return component;
    }

    template<typename RhsResult,
        typename NewResult = Product_t<Result, RhsResult>>
    ParsecComponent<NewResult> operator+(const Parsec<RhsResult> &rhs) {
        ParsecComponent<NewResult> component;
        component.set_exec([this, &rhs](const std::string &str, std::size_t &index) {
            return connect_template<NewResult>(this->component()->exec(), rhs.component()->exec(), str, index);
        });
        return component;
    }

    template<typename RhsResult,
        typename NewResult = Addition_t<Result, RhsResult>>
    ParsecComponent<NewResult> operator|(const ParsecComponent<RhsResult> &rhs) {
        ParsecComponent<NewResult> component;
        component.set_exec([this, rexec = rhs.exec()](const std::string &str, std::size_t &index) {
            return alternate_template<NewResult>(this->component()->exec(), rexec, str, index);
        });
        return component;
    }

    template<typename RhsResult,
        typename NewResult = Product_t<Result, RhsResult>>
    ParsecComponent<NewResult> operator+(const ParsecComponent<RhsResult> &rhs) {
        ParsecComponent<NewResult> component;
        component.set_exec([this, rexec = rhs.exec()](const std::string &str, std::size_t &index) {
            return connect_template<NewResult>(this->component()->exec(), rexec, str, index);
        });
        return component;
    }

  private:
    std::shared_ptr<ParsecComponentBase> component_;
};

template<typename Result>
template<typename RhsResult>
inline ParsecComponent<Addition_t<Result, RhsResult>>
ParsecComponent<Result>::operator|(const Parsec<RhsResult> &rhs) const {
    using NewResult = Addition_t<Result, RhsResult>;
    ParsecComponent<NewResult> component;
    component.set_exec([lexec = this->exec(), &rhs](const std::string &str, std::size_t &index) {
        return alternate_template<NewResult>(lexec, rhs.component()->exec(), str, index);
    });
    return component;
}

template<typename Result>
template<typename RhsResult>
inline ParsecComponent<Product_t<Result, RhsResult>>
ParsecComponent<Result>::operator+(const Parsec<RhsResult> &rhs) const {
    using NewResult = Product_t<Result, RhsResult>;
    ParsecComponent<NewResult> component;
    component.set_exec([lexec = this->exec(), &rhs](const std::string &str, std::size_t &index) {
        return connect_template<NewResult>(lexec, rhs.component()->exec(), str, index);
    });
    return component;
}

inline ParsecComponent<char> operator""_T(char ch) {
    ParsecComponent<char> component;
    component.set_exec([ch](const std::string &str, std::size_t &length) -> std::optional<char> {
        if (str[length] == ch) {
            ++length;
            return ch;
        } else {
            return {};
        }
    });
    return component;
}
}
