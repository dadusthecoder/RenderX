#include <type_traits>

namespace Rx {
	template <typename Enum>
	struct EnableBitMaskOperators {
		static constexpr bool enable = false;
	};

	template <typename Enum>
	using EnableIfBitmask =
		std::enable_if_t<EnableBitMaskOperators<Enum>::enable, Enum>;

	template <typename Enum>
	using Underlying = std::underlying_type_t<Enum>;

	// OR
	template <typename Enum>
	inline constexpr EnableIfBitmask<Enum>
	operator|(Enum a, Enum b) {
		return static_cast<Enum>(
			static_cast<Underlying<Enum>>(a) |
			static_cast<Underlying<Enum>>(b));
	}

	// AND
	template <typename Enum>
	inline constexpr EnableIfBitmask<Enum>
	operator&(Enum a, Enum b) {
		return static_cast<Enum>(
			static_cast<Underlying<Enum>>(a) &
			static_cast<Underlying<Enum>>(b));
	}

	// XOR
	template <typename Enum>
	inline constexpr EnableIfBitmask<Enum>
	operator^(Enum a, Enum b) {
		return static_cast<Enum>(
			static_cast<Underlying<Enum>>(a) ^
			static_cast<Underlying<Enum>>(b));
	}

	// NOT
	template <typename Enum>
	inline constexpr EnableIfBitmask<Enum>
	operator~(Enum a) {
		return static_cast<Enum>(
			~static_cast<Underlying<Enum>>(a));
	}

	// |=
	template <typename Enum>
	inline constexpr EnableIfBitmask<Enum>&
	operator|=(Enum& a, Enum b) {
		a = a | b;
		return a;
	}

	// &=
	template <typename Enum>
	inline constexpr EnableIfBitmask<Enum>&
	operator&=(Enum& a, Enum b) {
		a = a & b;
		return a;
	}

	// ^=
	template <typename Enum>
	inline constexpr EnableIfBitmask<Enum>&
	operator^=(Enum& a, Enum b) {
		a = a ^ b;
		return a;
	}

	template <typename Enum>
	inline constexpr std::enable_if_t<
		EnableBitMaskOperators<Enum>::enable, bool>
	Any(Enum value) {
		return static_cast<Underlying<Enum>>(value) != 0;
	}

	template <typename Enum>
	inline constexpr std::enable_if_t<
		EnableBitMaskOperators<Enum>::enable, bool>
	Has(Enum mask, Enum flag) {
		return (static_cast<Underlying<Enum>>(mask) &
				   static_cast<Underlying<Enum>>(flag)) != 0;
	}

	template <typename Enum>
	inline constexpr std::enable_if_t<
		EnableBitMaskOperators<Enum>::enable, void>
	Set(Enum& mask, Enum flag) {
		mask |= flag;
	}

	template <typename Enum>
	inline constexpr std::enable_if_t<
		EnableBitMaskOperators<Enum>::enable, void>
	Clear(Enum& mask, Enum flag) {
		mask &= ~flag;
	}

#define ENABLE_BITMASK_OPERATORS(_enum)            \
	template <>                                    \
	struct EnableBitMaskOperators<_enum> { \
		static constexpr bool enable = true;       \
	};

} // namespace  RenderX
