#ifndef HAND_HPP
#define HAND_HPP

#include "common.hpp"
#include "piece.hpp"

// 手駒
// 手駒の状態 (32bit に pack する)
// xxxxxxxx xxxxxxxx xxxxxxxx xxx11111  Pawn
// xxxxxxxx xxxxxxxx xxxxxxxx 111xxxxx  Lance
// xxxxxxxx xxxxxxxx xxxxx111 xxxxxxxx  Knight
// xxxxxxxx xxxxxxxx xx111xxx xxxxxxxx  Silver
// xxxxxxxx xxxxxxx1 11xxxxxx xxxxxxxx  Gold
// xxxxxxxx xxxxx11x xxxxxxxx xxxxxxxx  Bishop
// xxxxxxxx xxx11xxx xxxxxxxx xxxxxxxx  Rook
class Hand {
public:
	Hand() {}
	explicit Hand(u32 v) : value_(v) {}
	u32 value() const {
		return value_;
	}
	template<HandPiece HP> u32 numOf() const {
		return (HP == HPawn   ? ((value() & HPawnMask  ) >> HPawnShiftBits  ) :
				HP == HLance  ? ((value() & HLanceMask ) >> HLanceShiftBits ) :
				HP == HKnight ? ((value() & HKnightMask) >> HKnightShiftBits) :
				HP == HSilver ? ((value() & HSilverMask) >> HSilverShiftBits) :
				HP == HGold   ? ((value() & HGoldMask  ) >> HGoldShiftBits  ) :
				HP == HBishop ? ((value() & HBishopMask) >> HBishopShiftBits) :
				/*HP == HRook   ?*/ ((value() & HRookMask  ) >> HRookShiftBits  ));
	}
	u32 numOf(const HandPiece handPiece) const {
		return (value() & HandPieceMask[handPiece]) >> HandPieceShiftBits[handPiece];
	}
	// 2つの Hand 型変数の、同じ種類の駒の数を比較する必要があるため、
	// bool じゃなくて、u32 型でそのまま返す。
	template<HandPiece HP> u32 exists() const {
		return (HP == HPawn   ? (value() & HPawnMask  ) :
				HP == HLance  ? (value() & HLanceMask ) :
				HP == HKnight ? (value() & HKnightMask) :
				HP == HSilver ? (value() & HSilverMask) :
				HP == HGold   ? (value() & HGoldMask  ) :
				HP == HBishop ? (value() & HBishopMask) :
				/*HP == HRook   ?*/ (value() & HRookMask  ));
	}
	u32 exists(const HandPiece handPiece) const {
		return value() & HandPieceMask[handPiece];
	}
	u32 exceptPawnExists() const {
		return value() & HandPieceExceptPawnMask;
	}
	// num が int だけどまあ良いか。
	void orEqual(const int num, const HandPiece handPiece) {
		value_ |= num << HandPieceShiftBits[handPiece];
	}
	void plusOne(const HandPiece handPiece) {
		value_ += HandPieceOne[handPiece];
	}
	void minusOne(const HandPiece handPiece) {
		value_ -= HandPieceOne[handPiece];
	}
	bool operator == (const Hand rhs) const {
		return this->value() == rhs.value();
	}
	bool operator != (const Hand rhs) const {
		return this->value() != rhs.value();
	}
	// 手駒の優劣判定
	// 手駒が ref と同じか、勝っていれば true
	// 勝っている状態とは、全ての種類の手駒が、ref 以上の枚数があることを言う。
	bool isEqualOrSuperior(const Hand ref) const {
#if 0
		if(ref.exists<HKnight>() <= this->exists<HKnight>()
		   && ref.exists<HSilver>() <= this->exists<HSilver>()
		   && ref.exists<HGold  >() <= this->exists<HGold  >()
		   && ref.exists<HBishop>() <= this->exists<HBishop>()
		   && ref.exists<HRook  >() <= this->exists<HRook  >()
		{
			// 全ての駒が ref 以上の枚数なので、true
			return true;
		}
#else
		// こちらは、同じ意味でより高速

		// 飛角金銀桂香歩 と並んでいるので、飛金桂歩 と 角銀香 に分けて引き算をすることで、
		// ref の方がどれか一つでも多くの枚数の駒を持っていれば、*Borrow の位置のビットが立つ。
		const u32 result1 = ((this->value() & PKGRMask) - (ref.value() & PKGRMask)) & PKGRBorrow;
		const u32 result2 = ((this->value() & LSBMask ) - (ref.value() & LSBMask )) & LSBBorrow;
		return ((result1 | result2) == 0 ? true : false);
#endif
	}

	static const int BitSize = 21;

private:
	static const int HPawnShiftBits   =  0;
	static const int HLanceShiftBits  =  5;
	static const int HKnightShiftBits =  8;
	static const int HSilverShiftBits = 11;
	static const int HGoldShiftBits   = 14;
	static const int HBishopShiftBits = 17;
	static const int HRookShiftBits   = 19;
	static const u32 HPawnMask   = 0x1f << HPawnShiftBits;
	static const u32 HLanceMask  = 0x7  << HLanceShiftBits;
	static const u32 HKnightMask = 0x7  << HKnightShiftBits;
	static const u32 HSilverMask = 0x7  << HSilverShiftBits;
	static const u32 HGoldMask   = 0x7  << HGoldShiftBits;
	static const u32 HBishopMask = 0x3  << HBishopShiftBits;
	static const u32 HRookMask   = 0x3  << HRookShiftBits;
	static const u32 HandPieceExceptPawnMask =
		HLanceMask  | HKnightMask |
		HSilverMask | HGoldMask   |
		HBishopMask | HRookMask;
	static const int HandPieceShiftBits[HandPieceNum];
	static const u32 HandPieceMask[HandPieceNum];
	// 特定の種類の持ち駒を 1 つ増やしたり減らしたりするときに使用するテーブル
	static const u32 HandPieceOne[HandPieceNum];
	// 歩、桂、金、飛のマスク
	static const u32 PKGRMask = 0x19c71f;
	// 香、銀、角のマスク
	static const u32 LSBMask = 0x638e0;
	// 歩、桂、金、飛 それぞれの1ビット上のビット
	static const u32 PKGRBorrow = 0x220820;
	// 香、銀、角 それぞれの1ビット上のビット
	static const u32 LSBBorrow = 0x84100;

	u32 value_;
};

#endif // #ifndef HAND_HPP
