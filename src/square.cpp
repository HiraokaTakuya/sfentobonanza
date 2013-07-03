#include "square.hpp"

Direction SquareRelation[SquareNum][SquareNum];

// for debug
void printSquare(const Square s) {
	// 駒打ちの際、移動元の位置は SquareNum を超える。
	if(s < SquareNum) {
		const Rank rank = makeRank(s);
		const File file = makeFile(s);

		std::cout << file + 1 << rank + 1;
	}
	else {
		std::cout << "00";
	}
}
