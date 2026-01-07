// types.cppm
export module simulation_engine:types;

import std;

export namespace sim {
    /*----------------------------------*/
    /*      Strong Type Utilities       */
    /*----------------------------------*/
    template <class WrappedType, class Tag>
    struct Strong {
    public:
        static_assert(std::is_arithmetic_v<WrappedType>, "Strong<T,Tag> requires arithmetic T");

        constexpr Strong() = default;

        explicit constexpr Strong(WrappedType value) : value_(value) {}
        [[nodiscard]] constexpr WrappedType value() const noexcept { return value_; }
        explicit constexpr operator WrappedType() const noexcept { return value_; }

        // C++17 compatible comparison operators
        friend constexpr bool operator==(Strong a, Strong b) noexcept { return a.value_ == b.value_; }
        friend constexpr bool operator!=(Strong a, Strong b) noexcept { return a.value_ != b.value_; }
        friend constexpr bool operator<(Strong a, Strong b) noexcept { return a.value_ < b.value_; }
        friend constexpr bool operator<=(Strong a, Strong b) noexcept { return a.value_ <= b.value_; }
        friend constexpr bool operator>(Strong a, Strong b) noexcept { return a.value_ > b.value_; }
        friend constexpr bool operator>=(Strong a, Strong b) noexcept { return a.value_ >= b.value_; }

        // Unary operators
        friend constexpr Strong operator+(Strong a) noexcept { return Strong{+a.value_}; }
        friend constexpr Strong operator-(Strong a) noexcept { return Strong{-a.value_}; }
        friend constexpr Strong operator~(Strong a) noexcept { return Strong{~a.value_}; }

        // Increment/Decrement operators
        constexpr Strong& operator++() noexcept {
            ++value_;
            return *this;
        }
        constexpr Strong operator++(int) noexcept {
            Strong tmp{*this};
            ++value_;
            return tmp;
        }
        constexpr Strong& operator--() noexcept {
            --value_;
            return *this;
        }
        constexpr Strong operator--(int) noexcept {
            Strong tmp{*this};
            --value_;
            return tmp;
        }

        // Arithmetic operators
        [[nodiscard]] friend constexpr Strong operator+(Strong a, Strong b) noexcept {
            return Strong{a.value_ + b.value_};
        }
        [[nodiscard]] friend constexpr Strong operator-(Strong a, Strong b) noexcept {
            return Strong{a.value_ - b.value_};
        }
        [[nodiscard]] friend constexpr Strong operator*(Strong a, Strong b) noexcept {
            return Strong{a.value_ * b.value_};
        }
        [[nodiscard]] friend constexpr Strong operator/(Strong a, Strong b) noexcept {
            return Strong{a.value_ / b.value_};
        }

        // Modulo operator
        template <typename U = WrappedType>
        [[nodiscard]] friend constexpr std::enable_if_t<std::is_integral_v<U>, Strong> operator%(
            Strong a,
            Strong b) noexcept {
            return Strong{a.value_ % b.value_};
        }

        // Bitwise operators
        template <typename U = WrappedType>
        [[nodiscard]] friend constexpr std::enable_if_t<std::is_integral_v<U>, Strong> operator&(
            Strong a,
            Strong b) noexcept {
            return Strong{a.value_ & b.value_};
        }

        template <typename U = WrappedType>
        [[nodiscard]] friend constexpr std::enable_if_t<std::is_integral_v<U>, Strong> operator|(
            Strong a,
            Strong b) noexcept {
            return Strong{a.value_ | b.value_};
        }

        template <typename U = WrappedType>
        [[nodiscard]] friend constexpr std::enable_if_t<std::is_integral_v<U>, Strong> operator^(
            Strong a,
            Strong b) noexcept {
            return Strong{a.value_ ^ b.value_};
        }

        template <typename U = WrappedType>
        [[nodiscard]] friend constexpr std::enable_if_t<std::is_integral_v<U>, Strong> operator<<(
            Strong a,
            Strong b) noexcept {
            return Strong{a.value_ << b.value_};
        }

        template <typename U = WrappedType>
        [[nodiscard]] friend constexpr std::enable_if_t<std::is_integral_v<U>, Strong> operator>>(
            Strong a,
            Strong b) noexcept {
            return Strong{a.value_ >> b.value_};
        }

        // Compound assignment operators
        constexpr Strong& operator+=(Strong b) noexcept {
            value_ += b.value_;
            return *this;
        }
        constexpr Strong& operator-=(Strong b) noexcept {
            value_ -= b.value_;
            return *this;
        }
        constexpr Strong& operator*=(Strong b) noexcept {
            value_ *= b.value_;
            return *this;
        }
        constexpr Strong& operator/=(Strong b) noexcept {
            value_ /= b.value_;
            return *this;
        }

        // Modulo assignment
        template <typename U = WrappedType>
        constexpr std::enable_if_t<std::is_integral_v<U>, Strong&> operator%=(Strong b) noexcept {
            value_ %= b.value_;
            return *this;
        }

        // Bitwise assignment operators
        template <typename U = WrappedType>
        constexpr std::enable_if_t<std::is_integral_v<U>, Strong&> operator&=(Strong b) noexcept {
            value_ &= b.value_;
            return *this;
        }

        template <typename U = WrappedType>
        constexpr std::enable_if_t<std::is_integral_v<U>, Strong&> operator|=(Strong b) noexcept {
            value_ |= b.value_;
            return *this;
        }

        template <typename U = WrappedType>
        constexpr std::enable_if_t<std::is_integral_v<U>, Strong&> operator^=(Strong b) noexcept {
            value_ ^= b.value_;
            return *this;
        }

        template <typename U = WrappedType>
        constexpr std::enable_if_t<std::is_integral_v<U>, Strong&> operator<<=(Strong b) noexcept {
            value_ <<= b.value_;
            return *this;
        }

        template <typename U = WrappedType>
        constexpr std::enable_if_t<std::is_integral_v<U>, Strong&> operator>>=(Strong b) noexcept {
            value_ >>= b.value_;
            return *this;
        }

        // Mixed arithmetic with underlying type
        [[nodiscard]] friend constexpr Strong operator+(Strong a, WrappedType b) noexcept {
            return Strong{a.value_ + b};
        }
        [[nodiscard]] friend constexpr Strong operator+(WrappedType a, Strong b) noexcept {
            return Strong{a + b.value_};
        }
        [[nodiscard]] friend constexpr Strong operator-(Strong a, WrappedType b) noexcept {
            return Strong{a.value_ - b};
        }
        [[nodiscard]] friend constexpr Strong operator-(WrappedType a, Strong b) noexcept {
            return Strong{a - b.value_};
        }
        [[nodiscard]] friend constexpr Strong operator*(Strong a, WrappedType b) noexcept {
            return Strong{a.value_ * b};
        }
        [[nodiscard]] friend constexpr Strong operator*(WrappedType a, Strong b) noexcept {
            return Strong{a * b.value_};
        }
        [[nodiscard]] friend constexpr Strong operator/(Strong a, WrappedType b) noexcept {
            return Strong{a.value_ / b};
        }
        [[nodiscard]] friend constexpr Strong operator/(WrappedType a, Strong b) noexcept {
            return Strong{a / b.value_};
        }

        // Mixed compound assignment with underlying type
        constexpr Strong& operator+=(WrappedType b) noexcept {
            value_ += b;
            return *this;
        }
        constexpr Strong& operator-=(WrappedType b) noexcept {
            value_ -= b;
            return *this;
        }
        constexpr Strong& operator*=(WrappedType b) noexcept {
            value_ *= b;
            return *this;
        }
        constexpr Strong& operator/=(WrappedType b) noexcept {
            value_ /= b;
            return *this;
        }

        // Mixed comparison with underlying type
        friend constexpr bool operator==(Strong a, WrappedType b) noexcept { return a.value_ == b; }
        friend constexpr bool operator==(WrappedType a, Strong b) noexcept { return a == b.value_; }
        friend constexpr bool operator!=(Strong a, WrappedType b) noexcept { return a.value_ != b; }
        friend constexpr bool operator!=(WrappedType a, Strong b) noexcept { return a != b.value_; }
        friend constexpr bool operator<(Strong a, WrappedType b) noexcept { return a.value_ < b; }
        friend constexpr bool operator<(WrappedType a, Strong b) noexcept { return a < b.value_; }
        friend constexpr bool operator<=(Strong a, WrappedType b) noexcept { return a.value_ <= b; }
        friend constexpr bool operator<=(WrappedType a, Strong b) noexcept { return a <= b.value_; }
        friend constexpr bool operator>(Strong a, WrappedType b) noexcept { return a.value_ > b; }
        friend constexpr bool operator>(WrappedType a, Strong b) noexcept { return a > b.value_; }
        friend constexpr bool operator>=(Strong a, WrappedType b) noexcept { return a.value_ >= b; }
        friend constexpr bool operator>=(WrappedType a, Strong b) noexcept { return a >= b.value_; }

        friend std::ostream& operator<<(std::ostream& os, const Strong& x) { return os << x.value_; }

    private:
        WrappedType value_{};
    };

    // Zero-overhead property we care about
    struct DemoTag1 {};
    struct DemoTag2 {};
    static_assert(std::is_trivially_copyable_v<Strong<std::uint32_t, DemoTag1>>);
    static_assert(std::is_trivially_copyable_v<Strong<std::uint32_t, DemoTag2>>);

    /*------------------------------------*/
    /*      Tags & type definitions       */
    /*------------------------------------*/
    struct OrderIdTag {};
    struct VenueIdTag {};
    struct QuantityTag {};
    struct PriceTag {};
    struct TimeStampTag {};
    struct SizeTag {};
    struct DepthTag {};
    struct TicksTag {};

    using OrderId = Strong<std::uint64_t, OrderIdTag>;
    using VenueId = Strong<std::uint8_t, VenueIdTag>;
    using Quantity = Strong<std::uint32_t, QuantityTag>;  // Number of shares to buy/sell
    using Price = Strong<std::uint64_t, PriceTag>;        // Price in ticks (dollars * 1e9)
    using TimeStamp = Strong<std::uint64_t, TimeStampTag>;
    using Size = Strong<std::uint32_t, SizeTag>;   // size at a price level
    using Depth = Strong<std::uint8_t, DepthTag>;  // order book depth (levels)
    using Ticks = Strong<std::int64_t, TicksTag>;  // Price/value in ticks (signed for P&L calculations)
    using Percentage = std::int64_t;               // Percentage values

    // Default depth for order books
    inline constexpr Depth kDefaultDepth{10};

    /*--------------------------------------------------*/
    /*      Sentinels (reserved "invalid" values)       */
    /*--------------------------------------------------*/
    inline constexpr OrderId InvalidOrderId{std::numeric_limits<std::uint64_t>::max()};
    inline constexpr VenueId InvalidVenueId{std::numeric_limits<std::uint8_t>::max()};
    inline constexpr Quantity InvalidQuantity{std::numeric_limits<std::uint32_t>::max()};
    inline constexpr Price InvalidPrice{std::numeric_limits<std::uint64_t>::max()};
    inline constexpr TimeStamp InvalidTimeStamp{std::numeric_limits<std::uint64_t>::max()};
    inline constexpr Size InvalidSize{std::numeric_limits<std::uint32_t>::max()};
    inline constexpr Depth InvalidDepth{std::numeric_limits<std::uint8_t>::max()};
    inline constexpr Ticks InvalidTicks{std::numeric_limits<std::int64_t>::max()};

    /*---------------------------*/
    /*      Validation helpers   */
    /*---------------------------*/
    [[nodiscard]] inline constexpr bool is_valid(OrderId x) { return x != InvalidOrderId; }
    [[nodiscard]] inline constexpr bool is_valid(VenueId x) { return x != InvalidVenueId; }
    [[nodiscard]] inline constexpr bool is_valid(Quantity x) { return x != InvalidQuantity; }
    [[nodiscard]] inline constexpr bool is_valid(Price x) { return x != InvalidPrice; }
    [[nodiscard]] inline constexpr bool is_valid(TimeStamp x) { return x != InvalidTimeStamp; }
    [[nodiscard]] inline constexpr bool is_valid(Size x) { return x != InvalidSize; }
    [[nodiscard]] inline constexpr bool is_valid(Depth x) { return x != InvalidDepth; }
    [[nodiscard]] inline constexpr bool is_valid(Ticks x) { return x != InvalidTicks; }

    /*-----------------*/
    /*      Enums      */
    /*-----------------*/
    enum class Exchange : std::uint8_t { NASDAQ = 0, NYSE = 1 };
    enum class OrderInstruction : std::uint8_t { Buy = 0, Sell = 1 };
    enum class OrderType : std::uint8_t {
        Limit = 0,
        Market = 1,
        TrailingStop = 2,  // Not implemented
        StopMarket = 3,
        StopLimit = 4,
    };
    enum class TimeInForce : std::uint8_t { Day = 0, IOC = 1, FOK = 2, GTC = 3 };
    enum class ExecutionCondition : std::uint8_t { NotApplicable = 0, AON = 1, MinQty = 2 };

    // Mixed operators between Quantity and Ticks returning Ticks
    [[nodiscard]] inline constexpr Ticks operator*(Quantity quantity, Ticks ticks) noexcept {
        return Ticks{static_cast<std::int64_t>(quantity.value()) * ticks.value()};
    }

    [[nodiscard]] inline constexpr Ticks operator*(Ticks ticks, Quantity quantity) noexcept {
        return Ticks{ticks.value() * static_cast<std::int64_t>(quantity.value())};
    }

    [[nodiscard]] inline constexpr Ticks operator/(Ticks ticks, Quantity quantity) noexcept {
        return Ticks{ticks.value() / static_cast<std::int64_t>(quantity.value())};
    }

}  // namespace sim

/*-------------------------------------------------*/
/*      Hash support (for unordered_map/set)       */
/*-------------------------------------------------*/
namespace std {
    template <class T, class Tag>
    struct hash<sim::Strong<T, Tag>> {
        static_assert(std::is_invocable_r_v<size_t, std::hash<T>, T>,
            "Underlying type must be hashable");
        size_t operator()(const sim::Strong<T, Tag>& x) const
            noexcept(noexcept(std::hash<T>{}(x.value()))) {
            return std::hash<T>{}(x.value());
        }
    };
}  // namespace std


