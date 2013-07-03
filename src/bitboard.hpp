#ifndef BITBOARD_HPP
#define BITBOARD_HPP

#include "common.hpp"
#include "square.hpp"
#include "color.hpp"

class Bitboard;
extern const Bitboard SetMaskBB[SquareNum];
extern const Bitboard ClearMaskBB[SquareNum];

class Bitboard {
public:
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
	Bitboard& operator = (const Bitboard& rhs) {
		_mm_store_si128(&this->m_, rhs.m_);
		return *this;
	}
	Bitboard(const Bitboard& bb) {
		_mm_store_si128(&this->m_, bb.m_);
	}
#endif
	Bitboard() {}
	Bitboard(const u64 v0, const u64 v1) {
		this->p_[0] = v0;
		this->p_[1] = v1;
	}
	u64 p(const int index) const {
		return p_[index];
	}
	void set(const int index, const u64 val) {
		p_[index] = val;
	}
	u64 merge() const {
		return this->p(0) | this->p(1);
	}
	bool isNot0() const {
#ifdef HAVE_SSE4
		return !(_mm_testz_si128(this->m_, _mm_set1_epi8(static_cast<char>(0xffu))));
#else
		return (this->merge() ? true : false);
#endif
	}
	// これはコードが見難くなるけど仕方ない。
	bool andIsNot0(const Bitboard& bb) const {
#ifdef HAVE_SSE4
		return !(_mm_testz_si128(this->m_, bb.m_));
#else
		return (*this & bb).isNot0();
#endif
	}
	Bitboard operator ~ () const {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		Bitboard tmp;
		_mm_store_si128(&tmp.m_, _mm_andnot_si128(this->m_, _mm_set1_epi8(static_cast<char>(0xffu))));
		return tmp;
#else
		Bitboard tmp;
		tmp.p_[0] = ~this->p(0);
		tmp.p_[1] = ~this->p(1);
		return tmp;
#endif
	}
	Bitboard operator &= (const Bitboard& rhs) {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		_mm_store_si128(&this->m_, _mm_and_si128(this->m_, rhs.m_));
		return *this;
#else
		this->p_[0] &= rhs.p(0);
		this->p_[1] &= rhs.p(1);
		return *this;
#endif
	}
	Bitboard operator |= (const Bitboard& rhs) {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		_mm_store_si128(&this->m_, _mm_or_si128(this->m_, rhs.m_));
		return *this;
#else
		this->p_[0] |= rhs.p(0);
		this->p_[1] |= rhs.p(1);
		return *this;
#endif
	}
	Bitboard operator ^= (const Bitboard& rhs) {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		_mm_store_si128(&this->m_, _mm_xor_si128(this->m_, rhs.m_));
		return *this;
#else
		this->p_[0] ^= rhs.p(0);
		this->p_[1] ^= rhs.p(1);
		return *this;
#endif
	}
	Bitboard operator <<= (const int i) {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		_mm_store_si128(&this->m_, _mm_slli_epi64(this->m_, i));
		return *this;
#else
		this->p_[0] <<= i;
		this->p_[1] <<= i;
		return *this;
#endif
	}
	Bitboard operator >>= (const int i) {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		_mm_store_si128(&this->m_, _mm_srli_epi64(this->m_, i));
		return *this;
#else
		this->p_[0] >>= i;
		this->p_[1] >>= i;
		return *this;
#endif
	}
	Bitboard operator & (const Bitboard& rhs) const {
		return Bitboard(*this) &= rhs;
	}
	Bitboard operator | (const Bitboard& rhs) const {
		return Bitboard(*this) |= rhs;
	}
	Bitboard operator ^ (const Bitboard& rhs) const {
		return Bitboard(*this) ^= rhs;
	}
	Bitboard operator << (const int i) const {
		return Bitboard(*this) <<= i;
	}
	Bitboard operator >> (const int i) const {
		return Bitboard(*this) >>= i;
	}
	bool operator == (const Bitboard& rhs) const {
#ifdef HAVE_SSE4
		return (_mm_testc_si128(_mm_cmpeq_epi8(this->m_, rhs.m_), _mm_set1_epi8(static_cast<char>(0xffu))) ? true : false);
#else
		return ((this->p(0) == rhs.p(0)) && (this->p(1) == rhs.p(1))) ? true : false;
#endif
	}
	bool operator != (const Bitboard& rhs) const {
		return !(*this == rhs);
	}
	// これはコードが見難くなるけど仕方ない。
	Bitboard notThisAnd(const Bitboard& bb) const {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		Bitboard temp;
		_mm_store_si128(&temp.m_, _mm_andnot_si128(this->m_, bb.m_));
		return temp;
#else
		return ~(*this) & bb;
#endif
	}
	bool isSet(const Square sq) const {
		assert(isInSquare(sq));
		return andIsNot0(SetMaskBB[sq]);
	}
	void setBit(const Square sq) {
		*this |= SetMaskBB[sq];
	}
	void clearBit(const Square sq) {
		*this &= ClearMaskBB[sq];
	}
	void xorBit(const Square sq) {
		(*this) ^= SetMaskBB[sq];
	}
	void xorBit(const Square sq1, const Square sq2) {
		(*this) ^= (SetMaskBB[sq1] | SetMaskBB[sq2]);
	}
	// Bitboard の right 側だけの要素を調べて、最初に 1 であるマスの index を返す。
	// そのマスを 0 にする。
	// Bitboard の right 側が 0 でないことを前提にしている。
	FORCE_INLINE Square firstOneRightFromI9() {
		const Square sq = static_cast<Square>(firstOneFromLSB(this->p(0)));
		// LSB 側の最初の 1 の bit を 0 にする
		this->p_[0] &= this->p(0) - 1;
		return sq;
	}
	// Bitboard の left 側だけの要素を調べて、最初に 1 であるマスの index を返す。
	// そのマスを 0 にする。
	// Bitboard の left 側が 0 でないことを前提にしている。
	FORCE_INLINE Square firstOneLeftFromB9() {
		const Square sq = static_cast<Square>(firstOneFromLSB(this->p(1)) + 63);
		// LSB 側の最初の 1 の bit を 0 にする
		this->p_[1] &= this->p(1) - 1;
		return sq;
	}
	// Bitboard を I9 から A1 まで調べて、最初に 1 であるマスの index を返す。
	// そのマスを 0 にする。
	// Bitboard が allZeroBB() でないことを前提にしている。
	// VC++ の _BitScanForward() は入力が 0 のときに 0 を返す仕様なので、
	// 最初に 0 でないか判定するのは少し損。
	FORCE_INLINE Square firstOneFromI9() {
		if(this->p(0)) {
			return firstOneRightFromI9();
		}
		return firstOneLeftFromB9();
	}
	// 返す位置を 0 にしないバージョン。
	FORCE_INLINE Square constFirstOneRightFromI9() const {
		return static_cast<Square>(firstOneFromLSB(this->p(0)));
	}
	FORCE_INLINE Square constFirstOneLeftFromB9() const {
		return static_cast<Square>(firstOneFromLSB(this->p(1)) + 63);
	}
	FORCE_INLINE Square constFirstOneFromI9() const {
		if(this->p(0)) {
			return constFirstOneRightFromI9();
		}
		return constFirstOneLeftFromB9();
	}
	// Bitboard の 1 の bit を数える。
	int popCount() const {
		int count = count1s(this->p(0));
		count += count1s(this->p(1));
		return count;
	}
	// bit が 1 つだけ立っているかどうかを判定する。
	bool isOneBit() const {
#if defined (HAVE_SSE42)
		return (this->popCount() == 1);
#else
		if(!this->isNot0()) {
			return false;
		}
		else if(this->p(0)) {
			return !((this->p(0) & (this->p(0) - 1)) | this->p(1));
		}
		return !(this->p(1) & (this->p(1) - 1));
#endif
	}

	// for debug
	void printBoard() const {
		std::cout << "   A  B  C  D  E  F  G  H  I\n";
		for(Rank r = Rank9; r < RankNum; ++r) {
			std::cout << (9 - r);
			for(File f = FileA; FileI <= f; --f) {
				std::cout << (this->isSet(makeSquare(f, r)) ? "  X" : "  .");
			}
			std::cout << "\n";
		}

		std::cout << std::endl;
	}

	void printTable(const int part) const {
		for(Rank r = Rank9; r < RankNum; ++r) {
			for(File f = FileC; FileI <= f; --f) {
				std::cout << (UINT64_C(1) & (this->p(part) >> makeSquare(f, r)));
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}

	// 指定した位置が Bitboard のどちらの u64 変数の要素か
	static int part(const Square sq) {
		return static_cast<int>(C1 < sq);
	}

private:
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
	union {
		u64 p_[2];
		__m128i m_;
	};
#else
	u64 p_[2];	// p_[0] : 先手から見て、1一から7九までを縦に並べたbit. 63bit使用. right と呼ぶ。
				// p_[1] : 先手から見て、8一から1九までを縦に並べたbit. 18bit使用. left  と呼ぶ。
#endif
};

inline Bitboard setMaskBB(const Square sq) {
	return SetMaskBB[sq];
}
inline Bitboard clearMaskBB(const Square sq) {
	return ClearMaskBB[sq];
}

// 実際に使用する部分の全て bit が立っている Bitboard
const Bitboard AllOneBB = Bitboard(UINT64_C(0x7fffffffffffffff), UINT64_C(0x000000000003ffff));
inline Bitboard allOneBB() {
	return AllOneBB;
}

const Bitboard AllZeroBB = Bitboard(0, 0);
inline Bitboard allZeroBB() {
	return AllZeroBB;
}

extern const int RookBlockBits[SquareNum];
extern const int BishopBlockBits[SquareNum];
extern const int RookShiftBits[SquareNum];
extern const int BishopShiftBits[SquareNum];
extern const u64 RookMagic[SquareNum];
extern const u64 BishopMagic[SquareNum];

// 指定した位置の属する file の bit を shift し、
// index を求める為に使用する。
const int Slide[SquareNum] = {
	 1,  1,  1,  1,  1,  1,  1,  1,  1,
	10, 10, 10, 10, 10, 10, 10, 10, 10,
	19, 19, 19, 19, 19, 19, 19, 19, 19,
	28, 28, 28, 28, 28, 28, 28, 28, 28,
	37, 37, 37, 37, 37, 37, 37, 37, 37,
	46, 46, 46, 46, 46, 46, 46, 46, 46,
	55, 55, 55, 55, 55, 55, 55, 55, 55,
	 1,  1,  1,  1,  1,  1,  1,  1,  1,
	10, 10, 10, 10, 10, 10, 10, 10, 10
};

const Bitboard FileIMask = Bitboard(UINT64_C(0x1ff) << (9 * 0), 0);
const Bitboard FileHMask = Bitboard(UINT64_C(0x1ff) << (9 * 1), 0);
const Bitboard FileGMask = Bitboard(UINT64_C(0x1ff) << (9 * 2), 0);
const Bitboard FileFMask = Bitboard(UINT64_C(0x1ff) << (9 * 3), 0);
const Bitboard FileEMask = Bitboard(UINT64_C(0x1ff) << (9 * 4), 0);
const Bitboard FileDMask = Bitboard(UINT64_C(0x1ff) << (9 * 5), 0);
const Bitboard FileCMask = Bitboard(UINT64_C(0x1ff) << (9 * 6), 0);
const Bitboard FileBMask = Bitboard(0, 0x1ff << (9 * 0));
const Bitboard FileAMask = Bitboard(0, 0x1ff << (9 * 1));

const Bitboard Rank9Mask = Bitboard(UINT64_C(0x40201008040201) << 0, 0x201 << 0);
const Bitboard Rank8Mask = Bitboard(UINT64_C(0x40201008040201) << 1, 0x201 << 1);
const Bitboard Rank7Mask = Bitboard(UINT64_C(0x40201008040201) << 2, 0x201 << 2);
const Bitboard Rank6Mask = Bitboard(UINT64_C(0x40201008040201) << 3, 0x201 << 3);
const Bitboard Rank5Mask = Bitboard(UINT64_C(0x40201008040201) << 4, 0x201 << 4);
const Bitboard Rank4Mask = Bitboard(UINT64_C(0x40201008040201) << 5, 0x201 << 5);
const Bitboard Rank3Mask = Bitboard(UINT64_C(0x40201008040201) << 6, 0x201 << 6);
const Bitboard Rank2Mask = Bitboard(UINT64_C(0x40201008040201) << 7, 0x201 << 7);
const Bitboard Rank1Mask = Bitboard(UINT64_C(0x40201008040201) << 8, 0x201 << 8);

extern const Bitboard FileMask[FileNum];
extern const Bitboard RankMask[RankNum];
extern const Bitboard InFrontMask[ColorNum][RankNum];

inline Bitboard fileMask(const File f) {
	return FileMask[f];
}
template<File F> inline Bitboard fileMask() {
	STATIC_ASSERT(FileI <= F && F <= FileA);
	return (F == FileI ? FileIMask
			: F == FileH ? FileHMask
			: F == FileG ? FileGMask
			: F == FileF ? FileFMask
			: F == FileE ? FileEMask
			: F == FileD ? FileDMask
			: F == FileC ? FileCMask
			: F == FileB ? FileBMask
			: /*F == FileA ?*/ FileAMask);
}

inline Bitboard rankMask(const Rank r) {
	return RankMask[r];
}
template<Rank R> inline Bitboard rankMask() {
	STATIC_ASSERT(Rank9 <= R && R <= Rank1);
	return (R == Rank9 ? Rank9Mask
			: R == Rank8 ? Rank8Mask
			: R == Rank7 ? Rank7Mask
			: R == Rank6 ? Rank6Mask
			: R == Rank5 ? Rank5Mask
			: R == Rank4 ? Rank4Mask
			: R == Rank3 ? Rank3Mask
			: R == Rank2 ? Rank2Mask
			: /*R == Rank1 ?*/ Rank1Mask);
}

// 直接テーブル引きすべきだと思う。
inline Bitboard squareFileMask(const Square sq) {
	const File f = makeFile(sq);
	return fileMask(f);
}

// 直接テーブル引きすべきだと思う。
inline Bitboard squareRankMask(const Square sq) {
	const Rank r = makeRank(sq);
	return rankMask(r);
}

const Bitboard InFrontOfRank9Black = allZeroBB();
const Bitboard InFrontOfRank8Black = rankMask<Rank9>();
const Bitboard InFrontOfRank7Black = InFrontOfRank8Black | rankMask<Rank8>();
const Bitboard InFrontOfRank6Black = InFrontOfRank7Black | rankMask<Rank7>();
const Bitboard InFrontOfRank5Black = InFrontOfRank6Black | rankMask<Rank6>();
const Bitboard InFrontOfRank4Black = InFrontOfRank5Black | rankMask<Rank5>();
const Bitboard InFrontOfRank3Black = InFrontOfRank4Black | rankMask<Rank4>();
const Bitboard InFrontOfRank2Black = InFrontOfRank3Black | rankMask<Rank3>();
const Bitboard InFrontOfRank1Black = InFrontOfRank2Black | rankMask<Rank2>();

const Bitboard InFrontOfRank1White = allZeroBB();
const Bitboard InFrontOfRank2White = rankMask<Rank1>();
const Bitboard InFrontOfRank3White = InFrontOfRank2White | rankMask<Rank2>();
const Bitboard InFrontOfRank4White = InFrontOfRank3White | rankMask<Rank3>();
const Bitboard InFrontOfRank5White = InFrontOfRank4White | rankMask<Rank4>();
const Bitboard InFrontOfRank6White = InFrontOfRank5White | rankMask<Rank5>();
const Bitboard InFrontOfRank7White = InFrontOfRank6White | rankMask<Rank6>();
const Bitboard InFrontOfRank8White = InFrontOfRank7White | rankMask<Rank7>();
const Bitboard InFrontOfRank9White = InFrontOfRank8White | rankMask<Rank8>();

inline Bitboard inFrontMask(const Color c, const Rank r) {
	return InFrontMask[c][r];
}
template<Color C, Rank R> inline Bitboard inFrontMask() {
	STATIC_ASSERT(C == Black || C == White);
	STATIC_ASSERT(Rank9 <= R && R <= Rank1);
	return (C == Black ? (R == Rank9 ? InFrontOfRank9Black
						  : R == Rank8 ? InFrontOfRank8Black
						  : R == Rank7 ? InFrontOfRank7Black
						  : R == Rank6 ? InFrontOfRank6Black
						  : R == Rank5 ? InFrontOfRank5Black
						  : R == Rank4 ? InFrontOfRank4Black
						  : R == Rank3 ? InFrontOfRank3Black
						  : R == Rank2 ? InFrontOfRank2Black
						  : /*R == Rank1 ?*/ InFrontOfRank1Black)
			: (R == Rank9 ? InFrontOfRank9White
			   : R == Rank8 ? InFrontOfRank8White
			   : R == Rank7 ? InFrontOfRank7White
			   : R == Rank6 ? InFrontOfRank6White
			   : R == Rank5 ? InFrontOfRank5White
			   : R == Rank4 ? InFrontOfRank4White
			   : R == Rank3 ? InFrontOfRank3White
			   : R == Rank2 ? InFrontOfRank2White
			   : /*R == Rank1 ?*/ InFrontOfRank1White));
}

// メモリ節約の為、1次元配列にして無駄が無いようにしている。
extern Bitboard RookAttack[512000];
extern int RookAttackIndex[SquareNum];
// メモリ節約の為、1次元配列にして無駄が無いようにしている。
extern Bitboard BishopAttack[20224];
extern int BishopAttackIndex[SquareNum];
extern Bitboard RookBlockMask[SquareNum];
extern Bitboard BishopBlockMask[SquareNum];
// メモリ節約をせず、無駄なメモリを持っている。
extern Bitboard LanceAttack[ColorNum][SquareNum][128];

extern Bitboard KingAttack[SquareNum];
extern Bitboard GoldAttack[ColorNum][SquareNum];
extern Bitboard SilverAttack[ColorNum][SquareNum];
extern Bitboard KnightAttack[ColorNum][SquareNum];
extern Bitboard PawnAttack[ColorNum][SquareNum];

extern Bitboard BetweenBB[SquareNum][SquareNum];

extern Bitboard RookAttackToEdge[SquareNum];
extern Bitboard BishopAttackToEdge[SquareNum];
extern Bitboard LanceAttackToEdge[ColorNum][SquareNum];
extern Bitboard NotQueenAttackToEdgeOrKingAttack[SquareNum];

extern Bitboard GoldCheckTable[ColorNum][SquareNum];
extern Bitboard SilverCheckTable[ColorNum][SquareNum];
extern Bitboard KnightCheckTable[ColorNum][SquareNum];
extern Bitboard LanceCheckTable[ColorNum][SquareNum];

// magic bitboard.
// magic number を使って block の模様から利きのテーブルへのインデックスを算出
inline u64 occupiedToIndex(const Bitboard& block, const u64 magic, const int shiftBits) {
	return (block.merge() * magic) >> shiftBits;
}

inline Bitboard rookAttack(const Square sq, const Bitboard& occupied) {
	const Bitboard block(occupied & RookBlockMask[sq]);
	return RookAttack[RookAttackIndex[sq] + occupiedToIndex(block, RookMagic[sq], RookShiftBits[sq])];
}
inline Bitboard bishopAttack(const Square sq, const Bitboard& occupied) {
	const Bitboard block(occupied & BishopBlockMask[sq]);
	return BishopAttack[BishopAttackIndex[sq] + occupiedToIndex(block, BishopMagic[sq], BishopShiftBits[sq])];
}
// todo: 香車の筋がどこにあるか先に分かっていれば、Bitboard の片方の変数だけを調べれば良くなる。
inline Bitboard lanceAttack(const Color c, const Square sq, const Bitboard& occupied) {
	const int part = Bitboard::part(sq);
	const int index = (occupied.p(part) >> Slide[sq]) & 127;
	return LanceAttack[c][sq][index];
}
inline Bitboard goldAttack(const Color c, const Square sq) {
	return GoldAttack[c][sq];
}
inline Bitboard silverAttack(const Color c, const Square sq) {
	return SilverAttack[c][sq];
}
inline Bitboard knightAttack(const Color c, const Square sq) {
	return KnightAttack[c][sq];
}
inline Bitboard pawnAttack(const Color c, const Square sq) {
	return PawnAttack[c][sq];
}

// Bitboard で直接利きを返す関数。
// 1段目には歩は存在しないので、1bit シフトで別の筋に行くことはない。
// ただし、from に歩以外の駒の Bitboard を入れると、別の筋のビットが立ってしまうことがあるので、
// 別の筋のビットが立たないか、立っても問題ないかを確認して使用すること。
template<Color US>
inline Bitboard pawnAttack(const Bitboard& from) {
	return (US == Black ? (from >> 1) : (from << 1));
}

inline Bitboard kingAttack(const Square sq) {
	return KingAttack[sq];
}

inline Bitboard dragonAttack(const Square sq, const Bitboard& occupied) {
	return rookAttack(sq, occupied) | kingAttack(sq);
}

inline Bitboard horseAttack(const Square sq, const Bitboard& occupied) {
	return bishopAttack(sq, occupied) | kingAttack(sq);
}

inline Bitboard queenAttack(const Square sq, const Bitboard& occupied) {
	return rookAttack(sq, occupied) | bishopAttack(sq, occupied);
}

// sq1, sq2 の間(sq1, sq2 は含まない)のビットが立った Bitboard
inline Bitboard betweenBB(const Square sq1, const Square sq2) {
	return BetweenBB[sq1][sq2];
}

inline Bitboard rookAttackToEdge(const Square sq) {
	return RookAttackToEdge[sq];
}

inline Bitboard bishopAttackToEdge(const Square sq) {
	return BishopAttackToEdge[sq];
}

inline Bitboard lanceAttackToEdge(const Color c, const Square sq) {
	return LanceAttackToEdge[c][sq];
}

inline Bitboard dragonAttackToEdge(const Square sq) {
	// todo: テーブル引きにする
	return rookAttackToEdge(sq) | kingAttack(sq);
}

inline Bitboard horseAttackToEdge(const Square sq) {
	// todo: テーブル引きにする
	return bishopAttackToEdge(sq) | kingAttack(sq);
}

inline Bitboard notQueenAttackToEdgeOrKingAttack(const Square sq) {
	return NotQueenAttackToEdgeOrKingAttack[sq];
}

inline Bitboard goldCheckTable(const Color c, const Square sq) {
	return GoldCheckTable[c][sq];
}
inline Bitboard silverCheckTable(const Color c, const Square sq) {
	return SilverCheckTable[c][sq];
}
inline Bitboard knightCheckTable(const Color c, const Square sq) {
	return KnightCheckTable[c][sq];
}
inline Bitboard lanceCheckTable(const Color c, const Square sq) {
	return LanceCheckTable[c][sq];
}

inline Bitboard rookStepAttacks(const Square sq) {
	// todo: テーブル引きにする
	return goldAttack(Black, sq) & goldAttack(White, sq);
}
inline Bitboard bishopStepAttacks(const Square sq) {
	// todo: テーブル引きにする
	return silverAttack(Black, sq) & silverAttack(White, sq);
}
// 前方3方向の位置のBitboard
inline Bitboard goldAndSilverAttacks(const Color c, const Square sq) {
	return goldAttack(c, sq) & silverAttack(c, sq);
}

inline Bitboard makeMoveBB(const Square from, const Square to) {
	return SetMaskBB[from] | SetMaskBB[to];
}

// Bitboard の全ての bit に対して同様の処理を行う際に使用するマクロ
// xxx に処理を書く。
// xxx には template 引数を 2 つ以上持つクラスや関数を書くことが出来ない。
// これは C 言語マクロの制約。
// 同じ処理のコードが 2 箇所で生成されるため、コードサイズが膨れ上がる。
// その為、あまり多用すべきでないかも知れない。
#define FOREACH_BB(bb, sq, xxx)					\
	{											\
		while(bb.p(0)) {						\
			sq = bb.firstOneRightFromI9();		\
			xxx;								\
		}										\
		while(bb.p(1)) {						\
			sq = bb.firstOneLeftFromB9();		\
			xxx;								\
		}										\
	}

template<typename T> FORCE_INLINE void foreachBB(Bitboard& bb, Square& sq, T t) {
	while(bb.p(0)) {
		sq = bb.firstOneRightFromI9();
		t(0);
	}
	while(bb.p(1)) {
		sq = bb.firstOneLeftFromB9();
		t(1);
	}
}

#endif // #ifndef BITBOARD_HPP
