#ifndef EVALUATE_HPP
#define EVALUATE_HPP

#include "overloadEnumOperators.hpp"
#include "common.hpp"
#include "square.hpp"
#include "piece.hpp"
#include "pieceScore.hpp"

// 評価関数用テーブル

// KPP: King-Piece-Piece 玉と2駒の関係を評価するためのテーブル
// Bonanza の三角テーブルを矩形テーブルに直したもの。
extern s16 KPP[SquareNum][Apery::fe_end][Apery::fe_end];
// KKP: King-King-Piece 2玉と駒の関係を評価するためのテーブル
extern s16 KKP[SquareNum][SquareNum][Apery::kkp_end];

inline Score kpp(const Square sq, const int i) {
	return static_cast<Score>(KPP[sq][i][i]);
}
inline Score kpp(const Square sq, const int i, const int j) {
	return static_cast<Score>(KPP[sq][i][j]);
}

inline Score kkp(const Square sq1, const Square sq2, const int i) {
	return static_cast<Score>(KKP[sq1][sq2][i]);
}

class Position;
struct SearchStack;

Score evaluateBody(const Position& pos, SearchStack* ss);

// todo: サイズは改善の余地あり。
const size_t EvaluateTableSize = 0x40000;
//const size_t EvaluateTableSize = 0x40000000;

// 64bit 変数1つなのは理由があって、
// データを取得している最中に他のスレッドから書き換えられることが無くなるから。
// lockless hash と呼ばれる。
// 128bit とかだったら、64bitずつ2回データを取得しないといけないので、
// key と score が対応しない可能性がある。
// transposition table は正にその問題を抱えているが、
// 静的評価値のように差分評価をする訳ではないので、問題になることは少ない。
// 64bitに収まらない場合や、transposition table なども安全に扱いたいなら、
// lockするか、チェックサムを持たせるか、key を複数の変数に分けて保持するなどの方法がある。
// 31- 0 keyhigh32
// 63-32 score
struct EvaluateHashEntry {
	u32 key() const     { return static_cast<u32>(word); }
	Score score() const { return static_cast<Score>(static_cast<s64>(word) >> 32); }
	void save(const Key k, const Score s) {
		word = static_cast<u64>(k >> 32) | static_cast<u64>(static_cast<s64>(s) << 32);
	}
	u64 word;
};

struct EvaluateHashTable : HashTable<EvaluateHashEntry, EvaluateTableSize> {};

Score evaluateUnUseDiff(const Position& pos);
Score evaluate(const Position& pos, SearchStack* ss);

#endif // #ifndef EVALUATE_HPP
