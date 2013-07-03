#ifndef POSITION_HPP
#define POSITION_HPP

#include "piece.hpp"
#include "common.hpp"
#include "hand.hpp"
#include "bitboard.hpp"
#include "pieceScore.hpp"
#include <stack>
#include <memory>

class Position;

enum RepetitionType {
	NotRepetition, RepetitionDraw, RepetitionWin, RepetitionLose
};

struct CheckInfo {
	explicit CheckInfo(const Position&);
	Bitboard dcBB; // discoverd check candidates bitboard
	Bitboard pinned;
	Bitboard checkBB[PieceTypeNum];
};

// minimul stateinfo
struct StateInfoMin {
	Score material; // stocfish の npMaterial は 先手、後手の点数を配列で持っているけど、
					// 特に分ける必要は無い気がする。
	int pliesFromNull;
	int continuousCheck[ColorNum]; // Stockfish には無い。
};

// StateInfoMin だけ memcpy でコピーすることああるので、
// 継承を使っている。
struct StateInfo : public StateInfoMin {
	Key key;
	// 手番側の玉へ check している駒の Bitboard
	Bitboard checkersBB;
	StateInfo* previous;
	// capturedPieceType は move.cap() で取得出来るので必要無い。
};

typedef std::unique_ptr<std::stack<StateInfo> > StateStackPtr;

class Move;
struct Thread;

struct SearchInfo;

class Position {
public:
	Position() {}
	Position(const Position& pos) { *this = pos; }
	Position(const std::string& sfen/*, Thread* th, SearchInfo* si*/) {
		set(sfen/*, th, si*/);
	}

	Position& operator = (const Position& pos);
	void set(const std::string& sfen/*, Thread* th, SearchInfo* si*/);

	Bitboard bbOf(const PieceType pt) const                                            { return byTypeBB_[pt]; }
	Bitboard bbOf(const Color c) const                                                 { return byColorBB_[c]; }
	Bitboard bbOf(const PieceType pt, const Color c) const                             { return bbOf(pt) & bbOf(c); }
	Bitboard bbOf(const PieceType pt1, const PieceType pt2) const                      { return bbOf(pt1) | bbOf(pt2); }
	Bitboard bbOf(const PieceType pt1, const PieceType pt2, const Color c) const       { return bbOf(pt1, pt2) & bbOf(c); }
	Bitboard bbOf(const PieceType pt1, const PieceType pt2, const PieceType pt3) const { return bbOf(pt1, pt2) | bbOf(pt3); }
	Bitboard bbOf(const PieceType pt1, const PieceType pt2, const PieceType pt3, const PieceType pt4) const {
		return bbOf(pt1, pt2, pt3) | bbOf(pt4);
	}
	Bitboard bbOf(const PieceType pt1, const PieceType pt2, const PieceType pt3,
				  const PieceType pt4, const PieceType pt5) const
	{
		return bbOf(pt1, pt2, pt3, pt4) | bbOf(pt5);
	}
	Bitboard occupiedBB() const { return bbOf(Occupied); }
	// emptyBB() よりもわずかに速いはず。
	// emptyBB() とは異なり、全く使用しない位置(0 から数えて、right の 63bit目、left の 18 ~ 63bit目)
	// の bit が 1 になっても構わないとき、こちらを使う。
	Bitboard nOccupiedBB() const          { return ~occupiedBB(); }
	Bitboard emptyBB() const              { return occupiedBB() ^ allOneBB(); }
	// 金、成り金 の Bitboard
	Bitboard goldsBB() const              { return bbOf(Gold, ProPawn, ProLance, ProKnight, ProSilver); }
	Bitboard goldsBB(const Color c) const { return goldsBB() & bbOf(c); }

	Piece piece(const Square sq) const    { return piece_[sq]; }

	// hand
	Hand hand(const Color c) const { return hand_[c]; }

	// turn() 側が pin されている Bitboard を返す。
	// checkersBB が更新されている必要がある。
	Bitboard pinnedBB() const { return hiddenCheckers<true, true>(); }
	// 間の駒を動かすことで、turn() 側が空き王手が出来る駒のBitboardを返す。
	// checkersBB が更新されている必要はない。
	// BetweenIsUs == true  : 間の駒が自駒。
	// BetweenIsUs == false : 間の駒が敵駒。
	template<bool BetweenIsUs = true> Bitboard discoveredCheckBB() const { return hiddenCheckers<false, BetweenIsUs>(); }

	// to が Empty かつ、to と同じ筋に us の歩がないなら true
	// (駒のあるところへの歩打ち、二歩の判定)
	bool toIsEmptyAndNoPawns(const Color us, const Square to) const {
		// todo: 速い方を採用すること。
#if 1
		const int part = Bitboard::part(to);
		return !((occupiedBB().p(part) & setMaskBB(to).p(part)) | (bbOf(Pawn, us).p(part) & fileMask(makeFile(to)).p(part)));
#else
		return piece(to) == Empty && noPawns(us, makeFile(to));
#endif
	}
	// toFile と同じ筋に us の歩がないなら true
	bool noPawns(const Color us, const File toFile) const { return !bbOf(Pawn, us).andIsNot0(fileMask(toFile)); }
	bool isPawnDropCheckMate(const Color us, const Square sq) const;
	// Pinされているfromの駒がtoに移動出来なければtrueを返す。
	bool isPinnedIllegal(const Square from, const Square to, const Square ksq, const Bitboard& pinned) const {
		return pinned.isNot0() && pinned.isSet(from) && !isAligned<true>(from, to, ksq);
	}
	// 空き王手かどうか。
	bool isDiscoveredCheck(const Square from, const Square to, const Square ksq, const Bitboard& dcBB) const {
		// 偶然、isPinnedIllegal() と同じ実装になるので、使わせてもらう。
		return isPinnedIllegal(from, to, ksq, dcBB);
	}

	Bitboard checkersBB() const     { return st_->checkersBB; }
	Bitboard prevCheckersBB() const { return st_->previous->checkersBB; }
	// 王手が掛かっているか。
	bool inCheck() const            { return checkersBB().isNot0(); }
	// 離れた位置から王手されているか。ただし、桂馬は除く。
	bool inCheckFromAfar() const    { return checkersBB().andIsNot0(~notQueenAttackToEdgeOrKingAttack(kingSquare(turn()))); }

	Score material() const { return st_->material; }

#if defined USE_PIECELIST
	// pieceList
	const Square* pieceList(const Color c, const PieceType pt) const { return pieceList_[c][pt]; }
#endif
	FORCE_INLINE Square kingSquare(const Color c) const {
#if defined USE_PIECELIST
		// pieceList_ がある場合は以下の実装
		return pieceList_[c][King][0];
#else
		// pieceList_ がない場合は以下の実装
		return bbOf(King, c).constFirstOneFromI9();
#endif
	}

	bool moveGivesCheck(const Move m) const;
	bool moveGivesCheck(const Move move, const CheckInfo& ci) const;

	// attacks
	Bitboard attacksTo(const Square sq, const Bitboard& occupied) const;
	Bitboard attacksTo(const Color c, const Square sq) const;
	Bitboard attacksTo(const Color c, const Square sq, const Bitboard& occupied) const;
	Bitboard attacksToExceptKing(const Color c, const Square sq) const;
	bool attacksToIsNot0(const Color c, const Square sq) const { return attacksTo(c, sq).isNot0(); }
	bool attacksToIsNot0(const Color c, const Square sq, const Bitboard& occupied) const {
		return attacksTo(c, sq, occupied).isNot0();
	}
	// 移動王手が味方の利きに支えられているか。false なら相手玉で取れば詰まない。
	bool unDropCheckIsSupported(const Color c, const Square sq) const { return attacksTo(c, sq).isNot0(); }
	// 利きの生成

	// 任意の occupied に対する利きを生成する。
	template<PieceType PT> Bitboard attacksFrom(const Color c, const Square sq, const Bitboard& occupied) const;
	// 任意の occupied に対する利きを生成する。
	template<PieceType PT> Bitboard attacksFrom(const Square sq, const Bitboard& occupied) const {
		STATIC_ASSERT(PT == Bishop || PT == Rook || PT == Horse || PT == Dragon);
		// Color は何でも良い。
		return attacksFrom<PT>(ColorNum, sq, occupied);
	}

	template<PieceType PT> Bitboard attacksFrom(const Color c, const Square sq) const { return goldAttack(c, sq); }
	template<PieceType PT> Bitboard attacksFrom(const Square sq) const {
		STATIC_ASSERT(PT == Bishop || PT == Rook || PT == King || PT == Horse || PT == Dragon);
		// Color は何でも良い。
		return attacksFrom<PT>(ColorNum, sq);
	}
	Bitboard attacksFrom(const PieceType pt, const Color c, const Square sq) const;
	Bitboard attacksFrom(const PieceType pt, const Color c, const Square sq, const Bitboard& occupied) const;

	// 次の手番
	Color turn() const { return turn_; }

	// pseudoLegal とは
	// ・玉が相手駒の利きがある場所に移動する
	// ・pin の駒を移動させる
	// ・連続王手の千日手の手を指す
	// これらの反則手を含めた手の事と定義する。
	// よって、打ち歩詰めや二歩の手は pseudoLegal では無い。
	template<bool MUSTNOTDROP, bool FROMMUSTNOTBEKING>
	bool pseudoLegalMoveIsLegal(const Move move, const Bitboard& pinned) const;
	bool pseudoLegalMoveIsEvasion(const Move move, const Bitboard& pinned) const;
	// CHECK_PAWN_DROP : 二歩と打ち歩詰めも調べるなら true. killer の時くらいしか true にしなくて良いと思う。
	template<bool CHECK_PAWN_DROP> bool moveIsPseudoLegal(const Move move) const;
#if !defined NDEBUG
	bool moveIsLegal(const Move move) const;
	bool moveIsLegal(const Move move, const Bitboard& pinned) const;
#endif

	void doMove(const Move move, StateInfo& newSt);
	void doMove(const Move move, StateInfo& newSt, const CheckInfo& ci, const bool moveIsCheck);
	void undoMove(const Move move);
	template<bool DO> void doNullMove(StateInfo& backUpSt);

	template<bool MUSTDROP> Score see(const Move move, const int asymmThreshold = 0) const;
	Score seeSign(const Move move) const;

	template<Color US> Move mateMoveIn1Ply();
	Move mateMoveIn1Ply();

	Ply gamePly() const         { return gamePly_; }

	Key getKey() const          { return st_->key; }
	Key getExclusionKey() const { return st_->key ^ zobExclusion_; }
	Key getKeyExcludeTurn() const {
		STATIC_ASSERT(zobTurn_ == 1);
		return getKey() >> 1;
	}
	void print() const;

	u64 nodesSearched() const          { return nodes_; }
	void setNodesSearched(const u64 n) { nodes_ = n; }
	RepetitionType isDraw() const;

	Thread* thisThread() const { return thisThread_; }

	// sfen で最初に手数を設定しているので、それに続く moves の数を加算する。
	// todo: 時間管理に影響するのでこれで良いか後で考える。
	void addStartPosPly(const Ply tesu) { gamePly_ += tesu; }

#if !defined NDEBUG
	// for debug
	bool isOK() const;
#endif

	static void initZobrist();

	static Score pieceScore(const Piece pc)            { return PieceScore[pc]; }
	// Piece を index としても、 PieceType を index としても、
	// 同じ値が取得出来るようにしているので、PieceType => Piece への変換は必要ない。
	static Score pieceScore(const PieceType pt)        { return PieceScore[pt]; }
	static Score capturePieceScore(const Piece pc)     { return CapturePieceScore[pc]; }
	// Piece を index としても、 PieceType を index としても、
	// 同じ値が取得出来るようにしているので、PieceType => Piece への変換は必要ない。
	static Score capturePieceScore(const PieceType pt) { return CapturePieceScore[pt]; }
	static Score promotePieceScore(const PieceType pt) {
		assert(pt < Gold);
		return PromotePieceScore[pt];
	}

private:
	void clear();
	void setPiece(const Piece piece, const Square sq) {
		const Color c = pieceToColor(piece);
		const PieceType pt = pieceToPieceType(piece);

		piece_[sq] = piece;

		byTypeBB_[pt].setBit(sq);
		byColorBB_[c].setBit(sq);
		byTypeBB_[Occupied].setBit(sq);
	}
	void setHand(const HandPiece hp, const Color c, const int num) { hand_[c].orEqual(num, hp); }
	void setHand(const Piece piece, const int num) {
		const Color c = pieceToColor(piece);
		const HandPiece hp = pieceToHandPiece(piece);
		hand_[c].orEqual(num, hp);
	}

	// 手番側の玉へ check している駒を全て探して checkersBB_ にセットする。
	// 最後の手が何か覚えておけば、attacksTo() を使用しなくても良いはずで、処理が軽くなる。
	void findCheckers() { st_->checkersBB = attacksTo(oppositeColor(turn()), kingSquare(turn())); }

	Score computeMaterial() const;

	void xorBBs(const PieceType pt, const Square sq, const Color c);
	// turn() 側が
	// pin されて(して)いる駒の Bitboard を返す。
	// BetweenIsUs == true  : 間の駒が自駒。
	// BetweenIsUs == false : 間の駒が敵駒。
	template<bool FindPinned, bool BetweenIsUs> Bitboard hiddenCheckers() const {
		Bitboard result = allZeroBB();
		const Color us = turn();
		const Color them = oppositeColor(us);
		// pin する遠隔駒
		// まずは自駒か敵駒かで大雑把に判別
		Bitboard pinners = bbOf(FindPinned ? them : us);

		const Square ksq = kingSquare(FindPinned ? us : them);

		// 障害物が無ければ玉に到達出来る駒のBitboardだけ残す。
		pinners &= (bbOf(Lance) & lanceAttackToEdge((FindPinned ? us : them), ksq)) |
			(bbOf(Rook, Dragon) & rookAttackToEdge(ksq)) | (bbOf(Bishop, Horse) & bishopAttackToEdge(ksq));

		while(pinners.isNot0()) {
			const Square sq = pinners.firstOneFromI9();
			// pin する遠隔駒と玉の間にある駒の位置の Bitboard
			const Bitboard between = betweenBB(sq, ksq) & occupiedBB();

			// pin する遠隔駒と玉の間にある駒が1つで、かつ、引数の色のとき、その駒は(を) pin されて(して)いる。
			if(between.isNot0()
			   && between.isOneBit()
			   && between.andIsNot0(bbOf(BetweenIsUs ? us : them)))
			{
				result |= between;
			}
		}

		return result;
	}

	Key computeKey() const;

	void printHand(const Color c) const;

	static Key zobrist(const PieceType pt, const Square sq, const Color c) { return zobrist_[pt][sq][c]; }
	static Key zobTurn()                                                   { return zobTurn_; }
	static Key zobHand(const HandPiece hp, const Color c)                  { return zobHand_[hp][c]; }

	// byTypeBB は敵、味方の駒を区別しない。
	// byColorBB は駒の種類を区別しない。
	Bitboard byTypeBB_[PieceTypeNum];
	Bitboard byColorBB_[ColorNum];

	// 各マスの状態
	Piece piece_[SquareNum];

	// 手駒
	Hand hand_[ColorNum];
	Color turn_;

	StateInfo startState_;
	StateInfo* st_;
	// 時間管理に使用する。
	Ply gamePly_;
	Thread* thisThread_;
	u64 nodes_;

#if defined USE_PIECELIST
	// TODO: pieceList_, pieceIndex を使用するかは要検討
	// TODO: 小駒の成り駒などをまとめるべきか？ bonanza を参考にすること
	//       とりあえず小駒の成り駒は別々に保持することにする。
	Square pieceList_[ColorNum][PieceTypeNum][18]; // [Color][PieceType][駒の最大数]
	// 駒番号
	// pieceList_ のそれぞれの要素に番号をつける。
	int pieceIndex_[SquareNum];
#endif

	static Key zobrist_[PieceTypeNum][SquareNum][ColorNum];
	static const Key zobTurn_ = 1;
	static Key zobHand_[HandPieceNum][ColorNum];
	static Key zobExclusion_; // todo: これが必要か、要検討

//	// Stockfish にはない。
//public:
//	SearchInfo* searchInfo_;
};

template<> inline Bitboard Position::attacksFrom<Lance >(const Color c, const Square sq, const Bitboard& occupied) const { return lanceAttack(c, sq, occupied); }
template<> inline Bitboard Position::attacksFrom<Bishop>(const Color c, const Square sq, const Bitboard& occupied) const { return bishopAttack(sq, occupied); }
template<> inline Bitboard Position::attacksFrom<Rook  >(const Color c, const Square sq, const Bitboard& occupied) const { return rookAttack(sq, occupied); }
template<> inline Bitboard Position::attacksFrom<Horse >(const Color c, const Square sq, const Bitboard& occupied) const { return horseAttack(sq, occupied); }
template<> inline Bitboard Position::attacksFrom<Dragon>(const Color c, const Square sq, const Bitboard& occupied) const { return dragonAttack(sq, occupied); }

template<> inline Bitboard Position::attacksFrom<Pawn  >(const Color c, const Square sq) const { return pawnAttack(c, sq); }
template<> inline Bitboard Position::attacksFrom<Lance >(const Color c, const Square sq) const { return lanceAttack(c, sq, occupiedBB()); }
template<> inline Bitboard Position::attacksFrom<Knight>(const Color c, const Square sq) const { return knightAttack(c, sq); }
template<> inline Bitboard Position::attacksFrom<Silver>(const Color c, const Square sq) const { return silverAttack(c, sq); }
template<> inline Bitboard Position::attacksFrom<Bishop>(const Color c, const Square sq) const { return bishopAttack(sq, occupiedBB()); }
template<> inline Bitboard Position::attacksFrom<Rook  >(const Color c, const Square sq) const { return rookAttack(sq, occupiedBB()); }
template<> inline Bitboard Position::attacksFrom<King  >(const Color c, const Square sq) const { return kingAttack(sq); }
template<> inline Bitboard Position::attacksFrom<Horse >(const Color c, const Square sq) const { return horseAttack(sq, occupiedBB()); }
template<> inline Bitboard Position::attacksFrom<Dragon>(const Color c, const Square sq) const { return dragonAttack(sq, occupiedBB()); }

// position sfen R8/2K1S1SSk/4B4/9/9/9/9/9/1L1L1L3 b PLNSGBR17p3n3g 1
// の局面が最大合法手局面で 593 手。番兵の分、+ 1 しておく。
const int MaxLegalMoves = 593 + 1;

class CharToPieceUSI : public std::map<char, Piece> {
public:
	CharToPieceUSI() {
		(*this)['P'] = BPawn;
		(*this)['p'] = WPawn;
		(*this)['L'] = BLance;
		(*this)['l'] = WLance;
		(*this)['N'] = BKnight;
		(*this)['n'] = WKnight;
		(*this)['S'] = BSilver;
		(*this)['s'] = WSilver;
		(*this)['B'] = BBishop;
		(*this)['b'] = WBishop;
		(*this)['R'] = BRook;
		(*this)['r'] = WRook;
		(*this)['G'] = BGold;
		(*this)['g'] = WGold;
		(*this)['K'] = BKing;
		(*this)['k'] = WKing;
	}
	Piece value(char c) const {
		return this->find(c)->second;
	}
	bool isLegalChar(char c) const {
		return (this->find(c) != this->end());
	}
};
extern const CharToPieceUSI g_charToPieceUSI;

#endif // #ifndef POSITION_HPP
