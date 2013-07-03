#ifndef COLOR_HPP
#define COLOR_HPP

#include "overloadEnumOperators.hpp"

enum Color {
	Black, White, ColorNum
};
OverloadEnumOperators(Color);

inline Color oppositeColor(const Color c) {
	return static_cast<Color>(static_cast<int>(c) ^ 1);
}

// oppositeColor() が constexpr じゃないので、代わりに使用する。
#define OPPOSITECOLOR(c) static_cast<Color>(static_cast<int>(c) ^ 1)

#endif // #ifndef COLOR_HPP
