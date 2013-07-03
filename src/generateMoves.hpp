#ifndef GENERATEMOVES_HPP
#define GENERATEMOVES_HPP

#include "common.hpp"
#include "piece.hpp"
#include "move.hpp"

// 指し手の種類
enum MoveType {
	Capture,					// 駒を取る手。    歩, 飛, 角 の不成は含まない。
	NonCapture,					// 駒を取らない手。 歩, 飛, 角 の不成は含まない。
	Drop,						// 駒打ち 二歩、打ち歩詰めは含まない
	CapturePlusPro,				// Capture + (歩, 香, 桂, 飛, 角 の駒を取らない成る手)
	NonCaptureMinusPro,			// NonCapture - (歩, 香, 桂, 飛, 角 の駒を取らない成る手)
	Recapture,					// 特定の位置への取り返しの手
	Evasion,					// 王手回避
	NonEvasion,					// 王手が掛かっていないときの合法手 (玉の移動による自殺手、pinされている駒の移動による自殺手は回避しない。)
	Legal,						// 王手が掛かっていれば Evasion, そうでないなら NonEvasion を生成し、
								// 玉の自殺手と pin されてる駒の移動による自殺手を排除。(連続王手の千日手は排除しない。)
	LegalAll,					// Legal + 王手が掛かっているときは、歩、飛、角の不成、香の2段目の不成を生成
	MoveTypeNone
};

// MoveType の全ての指し手を生成
template<MoveType MT>
MoveStack* generateMoves(MoveStack* moveStackList, const Position& pos);
template<MoveType MT>
MoveStack* generateMoves(MoveStack* moveStackList, const Position& pos, const Square to);

template<MoveType MT>
class MoveList {
public:
	explicit MoveList(const Position& pos) : curr_(moveStackList_), last_(generateMoves<MT>(moveStackList_, pos)) {}
	void operator ++ () { ++curr_; }
	bool end() const { return (curr_ == last_); }
	Move move() const { return curr_->move; }
	size_t size() const { return static_cast<size_t>(last_ - moveStackList_); }
	bool contains(const Move move) const {
		for(const MoveStack* it(moveStackList_); it != last_; ++it) {
			if(it->move == move) {
				return true;
			}
		}
		return false;
	}

private:
	MoveStack moveStackList_[MaxLegalMoves];
	MoveStack* curr_;
	MoveStack* last_;
};

enum PromoteMode {
	Promote,					// 必ず成る
	NonPromote,					// 必ず不成
	PromoteModeNone
};

// MoveType によって指し手生成関数を使い分ける。
// 最適化で switch が削除されることを期待している。
// Drop, Check, Evasion, の場合は別で指し手生成を行う。
template<MoveType MT, PromoteMode PM>
inline Move selectedMakeMove(const PieceType pt, const Square from, const Square to, const Position& pos) {
	STATIC_ASSERT(PM == Promote || PM == NonPromote);
	assert(!((pt == Gold || pt == King || MT == Drop) && PM == Promote));
	Move move;
	switch(MT) {
	case Capture:
	case CapturePlusPro:
	case NonEvasion:
	case Evasion:            move = makeCaptureMove(pt, from, to, pos); break;
	case NonCapture:
	case NonCaptureMinusPro: move = makeMove(pt, from, to);             break;
	default:                 UNREACHABLE;
	}
	if(PM == Promote) {
		move |= promoteFlag();
	}
	return move;
}

template<MoveType MT>
inline Move makePromoteMove(const PieceType pt, const Square from, const Square to, const Position& pos) {
	return selectedMakeMove<MT, Promote>(pt, from, to, pos);
}

template<MoveType MT>
inline Move makeNonPromoteMove(const PieceType pt, const Square from, const Square to, const Position& pos) {
	return selectedMakeMove<MT, NonPromote>(pt, from, to, pos);
}

#endif // #ifndef GENERATEMOVES_HPP
