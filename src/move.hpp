#ifndef MOVE_HPP
#define MOVE_HPP

#include "common.hpp"
#include "square.hpp"
#include "piece.hpp"
#include "position.hpp"

// Move は Bonanza と同じ形式にする。
// xxxxxxxx xxxxxxxx xxxxxxxx x1111111  移動先
// xxxxxxxx xxxxxxxx xx111111 1xxxxxxx  移動元。駒打ちの際には、PieceType + SquareNum - 1
// xxxxxxxx xxxxxxxx x1xxxxxx xxxxxxxx  1 なら成り
// xxxxxxxx xxxx1111 xxxxxxxx xxxxxxxx  移動する駒の PieceType 駒打ちの際には使用しない。
// xxxxxxxx 1111xxxx xxxxxxxx xxxxxxxx  取られた駒の PieceType

// todo: piece to move と captured piece は指し手としてパックする必要あるの？
//       from, to , promo だけだったら、16bit で済む。
// class 化せずに、 u32 の typedef にした方が見易いかも知れない。
class Move {
public:
	Move() {}
	explicit Move(const u32 u) : value_(u) {}
	Move& operator = (const Move& m) { value_ = m.value_; return *this; }
	Move& operator = (const volatile Move& m) { value_ = m.value_; return *this; }
	// volatile Move& 型の *this を返すとなぜか警告が出るので、const Move& 型の m を返すことにする。
	const Move& operator = (const Move& m) volatile { value_ = m.value_; return m; }
	Move(const Move& m) { value_ = m.value_; }
	Move(const volatile Move& m) { value_ = m.value_; }

	// 移動先
	Square to() const {
		return static_cast<Square>((value() >> 0) & 0x7f);
	}
	// 移動元
	Square from() const {
		return static_cast<Square>((value() >> 7) & 0x7f);
	}
	// 移動元、移動先
	u32 fromAndTo() const {
		return (value() >> 0) & 0x3fff;
	}
	// 16bit に変換(transposition table に格納する為)
	u16 fromAndTo16() const {
		return static_cast<u16>(value());
	}
	// 成り、移動元、移動先
	u32 proFromAndTo() const {
		return (value() >> 0) & 0x7fff;
	}
	// 取った駒の種類
	PieceType cap() const {
		return static_cast<PieceType>((value() >> 20) & 0xf);
	}
	// 成るかどうか
	u32 isPromotion() const {
		return value() & PromoteFlag;
	}
	// 移動する駒の種類
	PieceType pieceTypeFrom() const {
		return static_cast<PieceType>((value() >> 16) & 0xf);
	}
	// 移動した後の駒の種類
	PieceType pieceTypeTo() const {
		if(isDrop()) {
			return pieceTypeDropped();
		}
		return pieceTypeTo(pieceTypeFrom());
	}
	// 移動前の PieceType を引数に取り、移動後の PieceType を返す。
	// 高速化の為、ptFrom が確定しているときに使用する。
	PieceType pieceTypeTo(const PieceType ptFrom) const {
		// これらは同じ意味。
#if 1
		return (ptFrom + static_cast<PieceType>((value() & PromoteFlag) >> 11));
#else
		return (isPromotion()) ? ptFrom + PTPromote : ptFrom;
#endif
	}
	bool isDrop() const {
		return this->from() >= 81;
	}
	bool isCapture() const {
		// 0xf00000 は 取られる駒のマスク
		return (value() & 0xf00000) ? true : false;
	}
	// todo: これは Position クラスに無くて大丈夫？
	bool isCaptureOrPromotion() const {
		// 0xf04000 は 取られる駒と成のマスク
		return (value() & 0xf04000) ? true : false;
	}
	// 打つ駒の種類
	PieceType pieceTypeDropped() const {
		return static_cast<PieceType>(this->from() - SquareNum + 1);
	}
	PieceType pieceTypeFromOrDropped() const {
		return (isDrop() ? pieceTypeDropped() : pieceTypeFrom());
	}
	HandPiece handPieceDropped() const {
		assert(this->isDrop());
		return pieceTypeToHandPiece(pieceTypeDropped());
	}
	// 値が入っているか。
	bool isNone() const {
		return (value() == MoveNone);
	}
	// メンバ変数 value_ の取得
	u32 value() const {
		return value_;
	}
	Move operator |= (const Move rhs) {
		this->value_ |= rhs.value();
		return *this;
	}
	Move operator | (const Move rhs) const {
		Move temp(*this);
		temp |= rhs;
		return temp;
	}
	bool operator == (const Move rhs) const {
		return this->value() == rhs.value();
	}
	bool operator != (const Move rhs) const {
		return !(*this == rhs);
	}
	std::string promoteFlagToStringUSI() const {
		return (this->isPromotion()) ? "+" : "";
	}
	std::string toUSI() const;
	std::string toCSA() const;

	static Move moveNone() {
		return Move(MoveNone);
	}
	static Move moveNull() {
		return Move(MoveNull);
	}

	static const u32 PromoteFlag = 1 << 14;
	static const u32 MoveNone    = 0;
	static const u32 MoveNull    = 129;

private:
	u32 value_;
};

// 成り flag
inline Move promoteFlag() {
	return static_cast<Move>(Move::PromoteFlag);
}
inline Move moveNone() {
	return static_cast<Move>(Move::MoveNone);
}

// 移動先から指し手に変換
inline Move to2Move(const Square to) {
	return static_cast<Move>(to << 0);
}

// 移動元から指し手に変換
inline Move from2Move(const Square from) {
	return static_cast<Move>(from << 7);
}

// 駒打ちの駒の種類から移動元に変換
// todo: PieceType を HandPiece に変更
inline Square drop2From(const PieceType pt) {
	return static_cast<Square>(SquareNum - 1 + pt);
}

// 駒打ち(移動元)から指し手に変換
inline Move drop2Move(const PieceType pt) {
	return static_cast<Move>(drop2From(pt) << 7);
}

// 移動する駒の種類から指し手に変換
inline Move pieceType2Move(const PieceType pt) {
	return static_cast<Move>(pt << 16);
}

// 移動元から駒打ちの駒の種類に変換
inline PieceType from2Drop(const Square from) {
	return static_cast<PieceType>(from - SquareNum + 1);
}

// 取った駒の種類から指し手に変換
inline Move capturedPieceType2Move(const PieceType captured) {
	return static_cast<Move>(captured << 20);
}
// 移動先と、Position から 取った駒の種類を判別し、指し手に変換
// 駒を取らないときは、0 (MoveNone) を返す。
inline Move capturedPieceType2Move(const Square to, const Position& pos) {
	const PieceType captured = pieceToPieceType( pos.piece(to) );
	return capturedPieceType2Move(captured);
}

// 移動元、移動先から指し手に変換
inline Move makeMove(const Square from, const Square to) {
	return from2Move(from) | to2Move(to);
}

// 移動元、移動先、移動する駒の種類から指し手に変換
inline Move makeMove(const PieceType pt, const Square from, const Square to) {
	return pieceType2Move(pt) | makeMove(from, to);
}

// 取った駒を判別する必要がある。
// この関数は駒を取らないときにも使える。
inline Move makeCaptureMove(const PieceType pt, const Square from, const Square to, const Position& pos) {
	return capturedPieceType2Move(to, pos) | makeMove(pt, from, to);
}

// makeMove() かつ 成り
inline Move makePromoteMove(const PieceType pt, const Square from, const Square to) {
	return makeMove(pt, from, to) | promoteFlag();
}

// makeCaptureMove() かつ 成り
inline Move makeCapturePromoteMove(const PieceType pt, const Square from, const Square to, const Position& pos) {
	return makeCaptureMove(pt, from, to, pos) | promoteFlag();
}

// 駒打ちの makeMove()
// todo: PieceType を HandPiece に変更
inline Move makeDropMove(const PieceType pt, const Square to) {
	return makeMove(drop2From(pt), to);
}

struct MoveStack {
	Move move;
	int score;
};

// 汎用的な insertionSort() を使用するために必要。
inline bool operator < (const MoveStack& f, const MoveStack& s) {
	return f.score < s.score;
}

// 汎用的な insertion sort. 要素数が少ない時、高速にソートできる。
// 降順(大きいものが先頭付近に集まる)
// *(first - 1) に 番兵(sentinel) として MAX 値が入っていると仮定して高速化してある。
// T には ポインタかイテレータを使用出来る。
// todo: プロファイルはしていないので本当に速いコードが書けているか自信なし。
// todo: MoveStack で使う場合は、手を生成する度に先頭の一つ前を番兵にしないといけない。
//       RootMove で使用する際にも番兵のセットが出来ていないので、
//       まだ番兵使用版のinsertionSort() は使用しないでおく。
template<typename T> inline void insertionSort(T first, T last) {
#ifdef USE_SENTINEL
	assert(std::max_element(first - 1, last) == first - 1); // 番兵が最大値となることを確認
#endif
	if(first != last) {
		for(T curr = first + 1; curr != last; ++curr) {
			if(*(curr - 1) < *curr) {
				const auto tmp = *curr;
				do {
					*curr = *(curr - 1);
					--curr;
				} while(
#if !defined USE_SENTINEL
					curr != first &&
#endif
					*(curr - 1) < tmp); // curr != first をチェックすることで番兵無しでも動作する。
				*curr = tmp; // todo: std::move() で良いはず。
			}
		}
	}
}

// 最も score の高い moveStack のポインタを返す。
// MoveStack の数が多いとかなり時間がかかるので、
// 駒打ちを含むときに使用してはならない。
inline MoveStack* pickBest(MoveStack* currMove, MoveStack* lastMove) {
	std::swap(*currMove, *std::max_element(currMove, lastMove));
	return currMove;
}

inline Move move16toMove(const Move move, const Position& pos) {
	if(move.isNone()) {
		return Move::moveNone();
	}
	if(move.isDrop()) {
		return move;
	}
	const Square from = move.from();
	const PieceType ptFrom = pieceToPieceType(pos.piece(from));
	return move | pieceType2Move(ptFrom) | capturedPieceType2Move(move.to(), pos);
}

inline Move move16toMove(const u16 move16, const Position& pos) {
	return move16toMove(Move(move16), pos);
}

#endif // #ifndef MOVE_HPP
