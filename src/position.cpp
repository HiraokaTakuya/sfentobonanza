#include "position.hpp"
#include "move.hpp"
#include "mt64bit.hpp"
#include "generateMoves.hpp"
//#include "tt.hpp"

Key Position::zobrist_[PieceTypeNum][SquareNum][ColorNum];
Key Position::zobHand_[HandPieceNum][ColorNum];
Key Position::zobExclusion_;

const CharToPieceUSI g_charToPieceUSI;

namespace {
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

	class PieceToCharCSA : public std::map<Piece, const char*> {
	public:
		PieceToCharCSA() {
			(*this)[BPawn     ] = "+FU";
			(*this)[WPawn     ] = "-FU";
			(*this)[BLance    ] = "+KY";
			(*this)[WLance    ] = "-KY";
			(*this)[BKnight   ] = "+KE";
			(*this)[WKnight   ] = "-KE";
			(*this)[BSilver   ] = "+GI";
			(*this)[WSilver   ] = "-GI";
			(*this)[BBishop   ] = "+KA";
			(*this)[WBishop   ] = "-KA";
			(*this)[BRook     ] = "+HI";
			(*this)[WRook     ] = "-HI";
			(*this)[BGold     ] = "+KI";
			(*this)[WGold     ] = "-KI";
			(*this)[BKing     ] = "+OU";
			(*this)[WKing     ] = "-OU";
			(*this)[BProPawn  ] = "+TO";
			(*this)[WProPawn  ] = "-TO";
			(*this)[BProLance ] = "+NY";
			(*this)[WProLance ] = "-NY";
			(*this)[BProKnight] = "+NK";
			(*this)[WProKnight] = "-NK";
			(*this)[BProSilver] = "+NG";
			(*this)[WProSilver] = "-NG";
			(*this)[BHorse    ] = "+UM";
			(*this)[WHorse    ] = "-UM";
			(*this)[BDragon   ] = "+RY";
			(*this)[WDragon   ] = "-RY";
			(*this)[Empty     ] = " * ";
		}
		const char* value(Piece p) const {
			return this->find(p)->second;
		}
	};

	const PieceToCharCSA PieceToCharCSA;
}

CheckInfo::CheckInfo(const Position& pos) {
	const Color them = oppositeColor(pos.turn());
	const Square ksq = pos.kingSquare(them);

	pinned = pos.pinnedBB();
	dcBB = pos.discoveredCheckBB();

	checkBB[Pawn     ] = pos.attacksFrom<Pawn  >(them, ksq);
	checkBB[Lance    ] = pos.attacksFrom<Lance >(them, ksq);
	checkBB[Knight   ] = pos.attacksFrom<Knight>(them, ksq);
	checkBB[Silver   ] = pos.attacksFrom<Silver>(them, ksq);
	checkBB[Bishop   ] = pos.attacksFrom<Bishop>(ksq);
	checkBB[Rook     ] = pos.attacksFrom<Rook  >(ksq);
	checkBB[Gold     ] = pos.attacksFrom<Gold  >(them, ksq);
	checkBB[King     ] = allZeroBB();
	checkBB[ProPawn  ] = checkBB[Gold];
	checkBB[ProLance ] = checkBB[Gold];
	checkBB[ProKnight] = checkBB[Gold];
	checkBB[ProSilver] = checkBB[Gold];
	checkBB[Horse    ] = pos.attacksFrom<Horse >(ksq);
	checkBB[Dragon   ] = pos.attacksFrom<Dragon>(ksq);
}

Bitboard Position::attacksFrom(const PieceType pt, const Color c, const Square sq) const {
	switch(pt) {
	case Occupied:  return allZeroBB(); // pieceToPieceType(Empty) の場合、allZeroBB() を返した方が都合が良い。
	case Pawn:      return attacksFrom<Pawn>(c, sq);
	case Lance:     return attacksFrom<Lance>(c, sq);
	case Knight:    return attacksFrom<Knight>(c, sq);
	case Silver:    return attacksFrom<Silver>(c, sq);
	case Bishop:    return attacksFrom<Bishop>(sq);
	case Rook:      return attacksFrom<Rook>(sq);
	case Gold:
	case ProPawn:
	case ProLance:
	case ProKnight:
	case ProSilver: return attacksFrom<Gold>(c, sq);
	case King:      return attacksFrom<King>(sq);
	case Horse:     return attacksFrom<Horse>(sq);
	case Dragon:    return attacksFrom<Dragon>(sq);
	default:        UNREACHABLE;
	}
}

Bitboard Position::attacksFrom(const PieceType pt, const Color c, const Square sq, const Bitboard& occupied) const {
	switch(pt) {
	case Occupied:  return allZeroBB();
	case Pawn:      return pawnAttack(c, sq);
	case Lance:     return lanceAttack(c, sq, occupied);
	case Knight:    return knightAttack(c, sq);
	case Silver:    return silverAttack(c, sq);
	case Bishop:    return bishopAttack(sq, occupied);
	case Rook:      return rookAttack(sq, occupied);
	case Gold:
	case ProPawn:
	case ProLance:
	case ProKnight:
	case ProSilver: return goldAttack(c, sq);
	case King:      return kingAttack(sq);
	case Horse:     return horseAttack(sq, occupied);
	case Dragon:    return dragonAttack(sq, occupied);
	default:        UNREACHABLE;
	}
}

// 実際に指し手が合法手かどうか判定
// 連続王手の千日手は排除しない。
// 確実に駒打ちではないときは、MUSTNOTDROP == true とする。
// 確実に玉の移動で無いときは、FROMMUSTNOTKING == true とする。英語として正しい？
// todo : 遠隔駒で王手されているとき、その駒の利きがある場所に逃げる手を検出出来ない場合があるので、
//        その可能性が無い場合のみ、使用すること。
template<bool MUSTNOTDROP, bool FROMMUSTNOTKING>
bool Position::pseudoLegalMoveIsLegal(const Move move, const Bitboard& pinned) const {
	// 駒打ちは、打ち歩詰めや二歩は指し手生成時や、killerをMovePicker::nextMove() 内で排除しているので、常に合法手
	// (連続王手の千日手は省いていないけれど。)
	if(!MUSTNOTDROP && move.isDrop()) {
		return true;
	}
	assert(!move.isDrop());

	const Color us = turn();
	const Square from = move.from();

	if(!FROMMUSTNOTKING && pieceToPieceType(piece(from)) == King) {
		const Color them = oppositeColor(us);
		// 玉の移動先に相手の駒の利きがあれば、合法手でないので、false
		return !attacksToIsNot0(them, move.to());
	}
	// 玉以外の駒の移動
	return !isPinnedIllegal(from, move.to(), kingSquare(us), pinned);
}

template bool Position::pseudoLegalMoveIsLegal<false, false>(const Move move, const Bitboard& pinned) const;
template bool Position::pseudoLegalMoveIsLegal<false, true >(const Move move, const Bitboard& pinned) const;
template bool Position::pseudoLegalMoveIsLegal<true,  false>(const Move move, const Bitboard& pinned) const;

namespace {
	// for debug
	inline bool FOO(const Position& pos, const Move move, const Bitboard& pinned) {
		return pos.pseudoLegalMoveIsLegal<false, false>(move, pinned);
	}
}

bool Position::pseudoLegalMoveIsEvasion(const Move move, const Bitboard& pinned) const {
	assert(isOK());

	// 玉の移動
	if(move.pieceTypeFrom() == King) {
		// 遠隔駒で王手されたとき、王手している遠隔駒の利きには移動しないように指し手を生成している。
		// その為、移動先に他の駒の利きが無いか調べるだけで良い。
		const bool canMove = !attacksToIsNot0(oppositeColor(turn()), move.to());
		assert(canMove == FOO(*this, move, pinned));
		return canMove;
	}

	// 玉の移動以外
	Bitboard target = checkersBB();
	const Square checkSq = target.firstOneFromI9();

	if(target.isNot0()) {
		// 両王手のとき、玉の移動以外の手は指せない。
		return false;
	}

	const Color us = turn();
	const Square to = move.to();
	// 移動、又は打った駒が、王手をさえぎるか、王手している駒を取る必要がある。
	target = betweenBB(checkSq, kingSquare(us)) | checkersBB();
	return target.isSet(to) && pseudoLegalMoveIsLegal<false, true>(move, pinned);
}

// CHECK_PAWN_DROP : 二歩と打ち歩詰めも調べるなら true。 killer の時くらいしか true にしなくて良いと思う。
//                   これが true のとき、駒打ちの場合のみ Legal であることが確定する。
template<bool CHECK_PAWN_DROP> bool Position::moveIsPseudoLegal(const Move move) const {
	const Color us = turn();
	const Color them = oppositeColor(us);
	const Square to = move.to();

	if(move.isDrop()) {
		const PieceType ptFrom = move.pieceTypeDropped();
		if(!hand(us).exists(pieceTypeToHandPiece(ptFrom)) || piece(to) != Empty) {
			return false;
		}

		if(inCheck()) {
			// 王手されているので、合駒でなければならない。
			Bitboard target = checkersBB();
			const Square checksq = target.firstOneFromI9();

			if(target.isNot0()) {
				// 両王手は合駒出来無い。
				return false;
			}

			target = betweenBB(checksq, kingSquare(us));
			if(!target.isSet(to)) {
				// 玉と、王手した駒との間に駒を打っていない。
				return false;
			}
		}

		if(CHECK_PAWN_DROP && ptFrom == Pawn) {
			if((bbOf(Pawn, us) & fileMask(makeFile(to))).isNot0()) {
				// 二歩
				return false;
			}
			const SquareDelta TDeltaN = (us == Black ? DeltaN : DeltaS);
			if(to + TDeltaN == kingSquare(them)
				&& isPawnDropCheckMate(us, to))
			{
				// 王手かつ打ち歩詰め
				return false;
			}
		}
	}
	else {
		const Square from = move.from();
		const PieceType ptFrom = move.pieceTypeFrom();
		if(piece(from) != colorAndPieceTypeToPiece(us, ptFrom)
		   || bbOf(us).isSet(to))
		{
			return false;
		}

		if(!attacksFrom(ptFrom, us, from).isSet(to)) {
			return false;
		}

		if(inCheck()) {
			if(ptFrom == King) {
				Bitboard occ = occupiedBB();
				occ.clearBit(from);
				if(attacksToIsNot0(them, to, occ)) {
					// 王手から逃げていない。
					return false;
				}
			}
			else {
				// 玉以外の駒を移動させたとき。
				Bitboard target = checkersBB();
				const Square checksq = target.firstOneFromI9();

				if(target.isNot0()) {
					// 両王手なので、玉が逃げない手は駄目
					return false;
				}

				target = betweenBB(checksq, kingSquare(us)) | checkersBB();
				if(!target.isSet(to)) {
					// 玉と、王手した駒との間に移動するか、王手した駒を取る以外は駄目。
					return false;
				}
			}
		}
	}

	return true;
}

template bool Position::moveIsPseudoLegal<true >(const Move move) const;
template bool Position::moveIsPseudoLegal<false>(const Move move) const;

#if !defined NDEBUG
// 過去(又は現在)に生成した指し手が現在の局面でも有効か判定。
// あまり速度が要求される場面で使ってはいけない。
bool Position::moveIsLegal(const Move move) const {
	return MoveList<Legal>(*this).contains(move);
}
// こっちの方が引数が move だけのものより速い。
bool Position::moveIsLegal(const Move move, const Bitboard& pinned) const {
	assert(isOK());
	assert(pinned == pinnedBB());

	const Color us = turn();
	const Color them = oppositeColor(us);
	const Square to = move.to();

	if(move.isDrop()) {
		const HandPiece hp = move.handPieceDropped();

		if(!hand(us).exists(hp)) {
			return false;
		}

		if(piece(to) != Empty) {
			return false;
		}

		if(hp == HPawn) {
			// 二歩、打ち歩詰めではないか。
			if(noPawns(us, makeFile(to))) {
				return false;
			}

			// 打ち歩詰めの判定
			const Square pawnDropCheckSquare =
				kingSquare(them) + (us == Black ? DeltaS : DeltaN); // isInSquare(pawnDropCheckSquare) == false かもしれないので注意。
			if(to == pawnDropCheckSquare) {
				// isInSquare(pawnDropCheckSquare) == true が確定。
				if(isPawnDropCheckMate(us, pawnDropCheckSquare)) {
					return false;
				}
			}
		}

		return true;
	}
	else {
		// 駒打ち以外
		const Square from = move.from();
		const PieceType pt = move.pieceTypeFrom();

		if(pieceToPieceType(piece(from)) != pt) {
			return false;
		}

		if(!attacksFrom(pt, us, from).isSet(to)) {
			return false;
		}

		return inCheck() ? pseudoLegalMoveIsEvasion(move, pinned) : pseudoLegalMoveIsLegal<true, false>(move, pinned);
	}
}
#endif

// 局面の更新
void Position::doMove(const Move move, StateInfo& newSt) {
	const CheckInfo ci(*this);
	doMove(move, newSt, ci, moveGivesCheck(move, ci));
}

// 局面の更新
void Position::doMove(const Move move, StateInfo& newSt, const CheckInfo& ci, const bool moveIsCheck) {
	assert(isOK());
	assert(!move.isNone());
	assert(&newSt != st_);

	Key key = getKey();
	key ^= zobTurn();

	memcpy(&newSt, st_, sizeof(StateInfoMin));
	newSt.previous = st_;
	st_ = &newSt;

	const Color us = turn();
	const Square to = move.to();
	const PieceType ptCaptured = move.cap();
	PieceType ptTo;
	if(move.isDrop()) {
		ptTo = move.pieceTypeDropped();
		const HandPiece hpTo = pieceTypeToHandPiece(ptTo);

		key -= zobHand(hpTo, us);
		key += zobrist(ptTo, to, us);

		//prefetch(g_tt.firstEntry(key));

		hand_[us].minusOne(hpTo);
		byTypeBB_[Occupied].xorBit(to);
		byTypeBB_[ptTo].xorBit(to);
		byColorBB_[us].xorBit(to);
		piece_[to] = colorAndPieceTypeToPiece(us, ptTo);

		if(moveIsCheck) {
			// Direct checks
			if(ci.checkBB[ptTo].isSet(to)) {
				st_->checkersBB = setMaskBB(to);
			}
			st_->continuousCheck[us] += 2;
		}
		else {
			st_->checkersBB = allZeroBB();
			st_->continuousCheck[us] = 0;
		}
	}
	else {
		const Square from = move.from();
		const PieceType ptFrom = move.pieceTypeFrom();
		ptTo = move.pieceTypeTo(ptFrom);

		byTypeBB_[ptFrom].xorBit(from);
		byTypeBB_[ptTo].xorBit(to);
		byTypeBB_[Occupied].xorBit(from);
		byColorBB_[us].xorBit(from, to);
		piece_[from] = Empty;
		piece_[to] = colorAndPieceTypeToPiece(us, ptTo);
		key -= zobrist(ptFrom, from, us);
		key += zobrist(ptTo, to, us);

		if(ptCaptured) {
			// 駒を取ったとき
			// todo: 後でここ高速化すること。ptCaptured で switch し、
			//       テーブルアクセスを減らすこと。
			const HandPiece hpCaptured = pieceTypeToHandPiece(ptCaptured);
			const Color them = oppositeColor(us);

			key -= zobrist(ptCaptured, to, them);
			key += zobHand(hpCaptured, us);

			//prefetch(g_tt.firstEntry(key));

			byTypeBB_[ptCaptured].xorBit(to);
			byColorBB_[them].xorBit(to);
			hand_[us].plusOne(hpCaptured);

			// todo: 今は適当にしておく。
			st_->material += (us == Black ?
							  capturePieceScore(ptCaptured)
							  : -capturePieceScore(ptCaptured));
		}
		else {
			// 駒を取らなかったとき
			byTypeBB_[Occupied].xorBit(to);

			//prefetch(g_tt.firstEntry(key));
		}

		// todo: これは成ったときだけにした方が速いのだろうか？
		if(move.isPromotion()) {
			st_->material += (us == Black ?
							  (pieceScore(ptTo) - pieceScore(ptFrom))
							  : -(pieceScore(ptTo) - pieceScore(ptFrom)));
		}

		if(moveIsCheck) {
			// Direct checks
			if(ci.checkBB[ptTo].isSet(to)) {
				st_->checkersBB = setMaskBB(to);
			}
			else {
				st_->checkersBB = allZeroBB();
			}

			// Discovery checks
			const Square ksq = kingSquare(oppositeColor(us));
			if(isDiscoveredCheck(from, to, ksq, ci.dcBB)) {
				switch(squareRelation(from, ksq)) {
				case DirecMisc: assert(false); break; // 最適化の為のダミー
				case DirecFile:
					st_->checkersBB |=
						((attacksFrom<Lance>(oppositeColor(us), ksq) & bbOf(Lance, Rook, Dragon))
						 | (attacksFrom<Lance>(us, ksq) & (bbOf(Rook, Dragon))))
						& bbOf(us);
					break;
				case DirecRank:
					st_->checkersBB |= attacksFrom<Rook>(ksq) & bbOf(Rook, Dragon, us);
					break;
				case DirecDiagNESW:
				case DirecDiagNWSE:
					st_->checkersBB |= attacksFrom<Bishop>(ksq) & bbOf(Bishop, Horse, us);
					break;
				default: UNREACHABLE;
				}
			}
			st_->continuousCheck[us] += 2;
		}
		else {
			st_->checkersBB = allZeroBB();
			st_->continuousCheck[us] = 0;
		}
	}

	st_->key = key;
	++st_->pliesFromNull;

	turn_ = oppositeColor(us);

	assert(isOK());
}

void Position::undoMove(const Move move) {
	assert(isOK());
	assert(!move.isNone());

	const Color them = turn();
	const Color us = oppositeColor(them);
	const Square to = move.to();
	turn_ = us;
	// ここで先に turn_ を戻したので、以下、move は us の指し手とする。
	if(move.isDrop()) {
		const PieceType ptTo = move.pieceTypeDropped();
		//byTypeBB_[Occupied].xorBit(to);
		byTypeBB_[ptTo].xorBit(to);
		byColorBB_[us].xorBit(to);
		piece_[to] = Empty;
		hand_[us].plusOne(pieceTypeToHandPiece(ptTo));
	}
	else {
		const Square from = move.from();
		const PieceType ptFrom = move.pieceTypeFrom();
		const PieceType ptTo = move.pieceTypeTo(ptFrom);
		const PieceType ptCaptured = move.cap(); // todo: st_->capturedType 使えば良い。

		if(ptCaptured) {
			// 駒を取ったとき
			// todo: 後でここ高速化すること。ptCaptured で switch し、
			//       テーブルアクセスを減らすこと。
			byTypeBB_[ptCaptured].xorBit(to);
			byColorBB_[them].xorBit(to);
			hand_[us].minusOne(pieceTypeToHandPiece(ptCaptured));
			piece_[to] = colorAndPieceTypeToPiece(them, ptCaptured);
		}
		else {
			//byTypeBB_[Occupied].xorBit(to);
			// 駒を取らないときは、colorAndPieceTypeToPiece(us, ptCaptured) は 0 または 16 になる。
			// 16 になると困るので、駒を取らないときは明示的に Empty にする。
			piece_[to] = Empty;
		}
		//byTypeBB_[Occupied].xorBit(from);
		byTypeBB_[ptFrom].xorBit(from);
		byTypeBB_[ptTo].xorBit(to);
		byColorBB_[us].xorBit(from, to);
		piece_[from] = colorAndPieceTypeToPiece(us, ptFrom);
	}
	// Occupied は to, from の位置のビットを操作するよりも、
	// Black と White の or を取る方が速いはず。
	byTypeBB_[Occupied] = bbOf(Black) | bbOf(White);

	// key などは StateInfo にまとめられているので、
	// previous のポインタを st_ に代入するだけで良い。
	st_ = st_->previous;

	assert(isOK());
}

// Stockfish とは異なる実装をしているが、自己対戦でこちらの方が強かった。
template<bool DO> void Position::doNullMove(StateInfo& backUpSt) {
	assert(!inCheck());

	StateInfo* src = (DO ? st_ : &backUpSt);
	StateInfo* dst = (DO ? &backUpSt : st_);

	dst->key           = src->key;
	dst->pliesFromNull = src->pliesFromNull;
	turn_ = oppositeColor(turn());

	if(DO) {
		st_->key ^= zobTurn();
		//prefetch(g_tt.firstEntry(st_->key));
		st_->pliesFromNull = 0;
		st_->continuousCheck[turn()] = 0;
	}

	assert(isOK());
}

template void Position::doNullMove<true >(StateInfo&);
template void Position::doNullMove<false>(StateInfo&);

// MUSTDROP: 確実に駒打ちの場合、true
//           駒打ちか、そうでないか分からない場合、false
template<bool MUSTDROP> Score Position::see(const Move move, const int asymmThreshold) const {
	const Square to = move.to();
	Square from;
	PieceType ptCaptured;
	Bitboard occ = occupiedBB();
	Bitboard attackers;
	Bitboard opponentAttackers;
	Color turn = this->turn();
	Score swapList[32];
	if(MUSTDROP || move.isDrop()) {
		attackers = attacksTo(to, occ);
		turn = oppositeColor(turn);
		opponentAttackers = attackers & bbOf(turn);
		if(!opponentAttackers.isNot0()) {
			return ScoreZero;
		}
		swapList[0] = ScoreZero;
		ptCaptured = move.pieceTypeDropped();
	}
	else {
		assert(!move.isDrop());
		from = move.from();
		ptCaptured = move.cap();

		occ.xorBit(from);
		attackers = attacksTo(to, occ);
		turn = oppositeColor(turn);
		opponentAttackers = attackers & bbOf(turn);
		if(!opponentAttackers.isNot0()) {
			if(move.isPromotion()) {
				const PieceType ptFrom = move.pieceTypeFrom();
				return capturePieceScore(ptCaptured) + promotePieceScore(ptFrom);
			}
			return capturePieceScore(ptCaptured);
		}
		swapList[0] = capturePieceScore(ptCaptured);
		ptCaptured = move.pieceTypeFrom();
		if(move.isPromotion()) {
			const PieceType ptFrom = move.pieceTypeFrom();
			swapList[0] += promotePieceScore(ptFrom);
			ptCaptured += PTPromote;
		}
	}

	int slIndex = 1;
	const Rank toRank = makeRank(to);
	do {
		const bool toIsOpponentField = canPromote(turn, toRank);
		if(opponentAttackers.andIsNot0(bbOf(Pawn))) {
			const PieceType pt = Pawn;
			const Bitboard bb = opponentAttackers & bbOf(pt);
			from = bb.constFirstOneFromI9();
			occ.xorBit(from);
			attackers |= (rookAttack(to, occ) & bbOf(Rook, Dragon));

			swapList[slIndex] = -swapList[slIndex - 1] + capturePieceScore(ptCaptured);
			ptCaptured = pt;
			if(toIsOpponentField || canPromote(turn, makeRank(from))) {
				swapList[slIndex] += promotePieceScore(pt);
				ptCaptured += PTPromote;
			}
		}
		else if(opponentAttackers.andIsNot0(bbOf(Lance))) {
			const PieceType pt = Lance;
			const Bitboard bb = opponentAttackers & bbOf(pt);
			from = bb.constFirstOneFromI9();
			occ.xorBit(from);
			attackers |= (rookAttack(to, occ) & bbOf(Rook, Dragon));

			swapList[slIndex] = -swapList[slIndex - 1] + capturePieceScore(ptCaptured);
			ptCaptured = pt;
			if(toIsOpponentField || canPromote(turn, makeRank(from))) {
				swapList[slIndex] += promotePieceScore(pt);
				ptCaptured += PTPromote;
			}
		}
		else if(opponentAttackers.andIsNot0(bbOf(Knight))) {
			const PieceType pt = Knight;
			const Bitboard bb = opponentAttackers & bbOf(pt);
			from = bb.constFirstOneFromI9();
			occ.xorBit(from);

			swapList[slIndex] = -swapList[slIndex - 1] + capturePieceScore(ptCaptured);
			ptCaptured = pt;
			if(toIsOpponentField || canPromote(turn, makeRank(from))) {
				swapList[slIndex] += promotePieceScore(pt);
				ptCaptured += PTPromote;
			}
		}
		else if(opponentAttackers.andIsNot0(bbOf(ProPawn))) {
			const PieceType pt = ProPawn;
			const Bitboard bb = opponentAttackers & bbOf(pt);
			from = bb.constFirstOneFromI9();
			occ.xorBit(from);
			attackers |= (lanceAttack(oppositeColor(turn), to, occ) & bbOf(Lance))
				| (rookAttack(to, occ) & bbOf(Rook, Dragon))
				| (bishopAttack(to, occ) & bbOf(Bishop, Horse));

			swapList[slIndex] = -swapList[slIndex - 1] + capturePieceScore(ptCaptured);
			ptCaptured = pt;
		}
		else if(opponentAttackers.andIsNot0(bbOf(ProLance))) {
			const PieceType pt = ProLance;
			const Bitboard bb = opponentAttackers & bbOf(pt);
			from = bb.constFirstOneFromI9();
			occ.xorBit(from);
			attackers |= (lanceAttack(oppositeColor(turn), to, occ) & bbOf(Lance))
				| (rookAttack(to, occ) & bbOf(Rook, Dragon))
				| (bishopAttack(to, occ) & bbOf(Bishop, Horse));

			swapList[slIndex] = -swapList[slIndex - 1] + capturePieceScore(ptCaptured);
			ptCaptured = pt;
		}
		else if(opponentAttackers.andIsNot0(bbOf(ProKnight))) {
			const PieceType pt = ProKnight;
			const Bitboard bb = opponentAttackers & bbOf(pt);
			from = bb.constFirstOneFromI9();
			occ.xorBit(from);
			attackers |= (lanceAttack(oppositeColor(turn), to, occ) & bbOf(Lance))
				| (rookAttack(to, occ) & bbOf(Rook, Dragon))
				| (bishopAttack(to, occ) & bbOf(Bishop, Horse));

			swapList[slIndex] = -swapList[slIndex - 1] + capturePieceScore(ptCaptured);
			ptCaptured = pt;
		}
		else if(opponentAttackers.andIsNot0(bbOf(Silver))) {
			const PieceType pt = Silver;
			const Bitboard bb = opponentAttackers & bbOf(pt);
			from = bb.constFirstOneFromI9();
			occ.xorBit(from);
			attackers |= ((rookAttack(to, occ) & bbOf(Rook, Dragon))
						  | (bishopAttack(to, occ) & bbOf(Bishop, Horse)));

			swapList[slIndex] = -swapList[slIndex - 1] + capturePieceScore(ptCaptured);
			if(toIsOpponentField || canPromote(turn, makeRank(from))) {
				swapList[slIndex] += promotePieceScore(pt);
			}
			ptCaptured = pt;
		}
		else if(opponentAttackers.andIsNot0(bbOf(ProSilver))) {
			const PieceType pt = ProSilver;
			const Bitboard bb = opponentAttackers & bbOf(pt);
			from = bb.constFirstOneFromI9();
			occ.xorBit(from);
			attackers |= (lanceAttack(oppositeColor(turn), to, occ) & bbOf(Lance))
				| (rookAttack(to, occ) & bbOf(Rook, Dragon))
				| (bishopAttack(to, occ) & bbOf(Bishop, Horse));

			swapList[slIndex] = -swapList[slIndex - 1] + capturePieceScore(ptCaptured);
			ptCaptured = pt;
		}
		else if(opponentAttackers.andIsNot0(bbOf(Gold))) {
			const PieceType pt = Gold;
			const Bitboard bb = opponentAttackers & bbOf(pt);
			from = bb.constFirstOneFromI9();
			occ.xorBit(from);
			attackers |= (lanceAttack(oppositeColor(turn), to, occ) & bbOf(Lance))
				| (rookAttack(to, occ) & bbOf(Rook, Dragon))
				| (bishopAttack(to, occ) & bbOf(Bishop, Horse));

			swapList[slIndex] = -swapList[slIndex - 1] + capturePieceScore(ptCaptured);
			ptCaptured = pt;
		}
		else if(opponentAttackers.andIsNot0(bbOf(Bishop))) {
			const PieceType pt = Bishop;
			const Bitboard bb = opponentAttackers & bbOf(pt);
			from = bb.constFirstOneFromI9();
			occ.xorBit(from);
			attackers |= (bishopAttack(to, occ) & bbOf(Bishop, Horse));

			swapList[slIndex] = -swapList[slIndex - 1] + capturePieceScore(ptCaptured);
			ptCaptured = pt;
			if(toIsOpponentField || canPromote(turn, makeRank(from))) {
				swapList[slIndex] += promotePieceScore(pt);
				ptCaptured += PTPromote;
			}
		}
		else if(opponentAttackers.andIsNot0(bbOf(Horse))) {
			const PieceType pt = Horse;
			const Bitboard bb = opponentAttackers & bbOf(pt);
			from = bb.constFirstOneFromI9();
			occ.xorBit(from);
			attackers |= (lanceAttack(oppositeColor(turn), to, occ) & bbOf(Lance))
				| (rookAttack(to, occ) & bbOf(Rook, Dragon))
				| (bishopAttack(to, occ) & bbOf(Bishop, Horse));

			swapList[slIndex] = -swapList[slIndex - 1] + capturePieceScore(ptCaptured);
			ptCaptured = pt;
		}
		else if(opponentAttackers.andIsNot0(bbOf(Rook))) {
			const PieceType pt = Rook;
			const Bitboard bb = opponentAttackers & bbOf(pt);
			from = bb.constFirstOneFromI9();
			occ.xorBit(from);
			attackers |= (lanceAttack(oppositeColor(turn), to, occ) & bbOf(Lance))
				| (rookAttack(to, occ) & bbOf(Rook, Dragon));

			swapList[slIndex] = -swapList[slIndex - 1] + capturePieceScore(ptCaptured);
			ptCaptured = pt;
			if(toIsOpponentField || canPromote(turn, makeRank(from))) {
				swapList[slIndex] += promotePieceScore(pt);
				ptCaptured += PTPromote;
			}
		}
		else if(opponentAttackers.andIsNot0(bbOf(Dragon))) {
			const PieceType pt = Dragon;
			const Bitboard bb = opponentAttackers & bbOf(pt);
			from = bb.constFirstOneFromI9();
			occ.xorBit(from);
			attackers |= (lanceAttack(oppositeColor(turn), to, occ) & bbOf(Lance))
				| (rookAttack(to, occ) & bbOf(Rook, Dragon))
				| (bishopAttack(to, occ) & bbOf(Bishop, Horse));

			swapList[slIndex] = -swapList[slIndex - 1] + capturePieceScore(ptCaptured);
			ptCaptured = pt;
		}
		else {
			ptCaptured = King;
		}

		attackers &= occ;
		++slIndex;
		turn = oppositeColor(turn);
		opponentAttackers = attackers & bbOf(turn);

		if(ptCaptured == King) {
			if(opponentAttackers.isNot0()) {
				swapList[slIndex] = CaptureKingScore;
				++slIndex;
			}
			break;
		}
	} while(opponentAttackers.isNot0());

	if(asymmThreshold) {
		for(int i = 0; i < slIndex; i += 2) {
			if(swapList[i] < asymmThreshold) {
				swapList[i] = -CaptureKingScore;
			}
		}
	}

	// nega max 的に駒の取り合いの点数を求める。
	while(--slIndex) {
		swapList[slIndex-1] = std::min(-swapList[slIndex], swapList[slIndex-1]);
	}
	return swapList[0];
}

template Score Position::see<true >(const Move move, const int asymmThreshold) const;
template Score Position::see<false>(const Move move, const int asymmThreshold) const;

Score Position::seeSign(const Move move) const {
	assert(isOK());
	if(move.isDrop()) {
		return see<true>(move);
	}
	const PieceType ptFrom = move.pieceTypeFrom();
	const Square to = move.to();
	if(pieceScore(ptFrom) <= pieceScore(piece(to))) {
		return static_cast<Score>(1);
	}
	return see<false>(move);
}

namespace {
	// them(相手) 側の玉が逃げられるか。
	// sq : 王手した相手の駒の位置。紐付きか、桂馬の位置とする。よって、玉は sq には行けない。
	// bb : sq の利きのある場所のBitboard。よって、玉は bb のビットが立っている場所には行けない。
	// sq と ksq の位置の Occupied Bitboard のみは、ここで更新して評価し、元に戻す。
	// (実際にはテンポラリのOccupied Bitboard を使うので、元には戻さない。)
	bool canKingEscape(const Position& pos, const Color us, const Square sq, const Bitboard& bb) {
		const Color them = oppositeColor(us);
		const Square ksq = pos.kingSquare(them);
		Bitboard kingMoveBB = bb.notThisAnd(pos.bbOf(them).notThisAnd(kingAttack(ksq)));
//		Bitboard kingMoveBB = kingAttack(ksq) & ~pos.bbOf(them) & ~bb;
		kingMoveBB.clearBit(sq); // sq には行けないので、クリアする。xorBit(sq)ではダメ。

		if(kingMoveBB.isNot0()) {
			Bitboard tempOccupied = pos.occupiedBB();
			tempOccupied.setBit(sq);
			tempOccupied.clearBit(ksq);
			do {
				const Square to = kingMoveBB.firstOneFromI9();
				// 玉の移動先に、us 側の利きが無ければ、true
				if(!pos.attacksToIsNot0(us, to, tempOccupied)) {
					return true;
				}
			} while(kingMoveBB.isNot0());
		}
		// 玉の移動先が無い。
		return false;
	}
	// them(相手) 側の玉以外の駒が sq にある us 側の駒を取れるか。
	// todo: 引数整理すること。dcBB は pos のメンバに出来ないか？ (駒を動かして王手したときに更新する必要があるけど。)
	bool canPieceCapture(const Position& pos, const Color them, const Square sq, const Bitboard& dcBB) {
		// 玉以外で打った駒を取れる相手側の駒の Bitboard
		Bitboard fromBB = pos.attacksToExceptKing(them, sq);

		if(fromBB.isNot0()) {
			const Square ksq = pos.kingSquare(them);
			do {
				const Square from = fromBB.firstOneFromI9();
				if(!pos.isDiscoveredCheck(from, sq, ksq, dcBB)) {
					// them 側から見て、pin されていない駒で、打たれた駒を取れるので、true
					return true;
				}
			} while(fromBB.isNot0());
		}
		// 玉以外の駒で、打った駒を取れない。
		return false;
	}
}

// us が sq へ歩を打ったとき、them の玉が詰むか。
// us が sq へ歩を打つのは王手であると仮定する。
// 打ち歩詰めのとき、true を返す。
// todo: 引数の template 化や inline 化は後で検討すること。
bool Position::isPawnDropCheckMate(const Color us, const Square sq) const {
	const Color them = oppositeColor(us);
	// 玉以外の駒で、打たれた歩が取れるなら、打ち歩詰めではない。
	if(canPieceCapture(*this, them, sq, discoveredCheckBB<false>())) {
		return false;
	}
	// todo: ここで玉の位置を求めるのは、上位で求めたものと2重になるので無駄。後で整理すること。
	const Square ksq = kingSquare(them);

	// 玉以外で打った歩を取れないとき、玉が歩を取るか、玉が逃げるか。

	// 利きを求める際に、occupied の歩を打った位置の bit を立てた Bitboard を使用する。
	// ここでは歩の Bitboard は更新する必要がない。
	// color の Bitboard も更新する必要がない。(相手玉が動くとき、こちらの打った歩で玉を取ることは無い為。)
	const Bitboard tempOccupied = occupiedBB() | setMaskBB(sq);
	Bitboard kingMoveBB = bbOf(them).notThisAnd(kingAttack(ksq));
//	Bitboard kingMoveBB = kingAttack(ksq) & ~bbOf(them);

	// 少なくとも歩を取る方向には玉が動けるはずなので、do while を使用。
	assert(kingMoveBB.isNot0());
	do {
		const Square to = kingMoveBB.firstOneFromI9();
		if(!attacksToIsNot0(us, to, tempOccupied)) {
			// 相手玉の移動先に自駒の利きがないなら、打ち歩詰めではない。
			return false;
		}
	} while(kingMoveBB.isNot0());

	return true;
}

inline void Position::xorBBs(const PieceType pt, const Square sq, const Color c) {
	// todo: 1 手詰め関数内で、Occupied 以外を xor する必要はあるの？要調査
	byTypeBB_[pt].xorBit(sq);
	byTypeBB_[Occupied].xorBit(sq);
	byColorBB_[c].xorBit(sq);
}

// 相手玉が1手詰みかどうかを判定。
// 1手詰みなら、詰みに至る指し手の一部の情報(from, to のみとか)を返す。
// 1手詰みでないなら、Move::moveNone() を返す。
// Bitboard の状態を途中で更新する為、const 関数ではない。(更新後、元に戻すが。)
// todo: Position クラスのメンバ関数である必要は無い気がする。
//       クラスに属さない関数で良いと思う。
template<Color US> Move Position::mateMoveIn1Ply() {
	// todo: 玉の 8 近傍の利きをメモ化して高速化出来ないか検討
	//       駒打ちのときなら出来そう。
	// todo: oppositeColor(US) が constexpr なら Them = oppositeColor(US) を使用すること。
	const Color Them = OPPOSITECOLOR(US);
	const Square ksq = kingSquare(Them);
	const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);

	assert(!attacksToIsNot0(Them, kingSquare(US)));

	// 駒打ちを調べる。
	const Bitboard dropTarget = nOccupiedBB(); // emptyBB() ではないので注意して使うこと。
	const Hand ourHand = hand(US);
	// 王手する前の状態の dcBB。
	// 間にある駒は相手側の駒。
	// 駒打ちのときは、打った後も、打たれる前の状態の dcBB を使用する。
	const Bitboard dcBB_betweenIsThem = discoveredCheckBB<false>();

	// 飛車打ち
	if(ourHand.exists<HRook>()) {
		// 合駒されるとややこしいので、3手詰み関数の中で調べる。
		// ここでは離れた位置から王手するのは考えない。
		Bitboard toBB = dropTarget & rookStepAttacks(ksq);
		while(toBB.isNot0()) {
			const Square to = toBB.firstOneFromI9();
			// 駒を打った場所に自駒の利きがあるか。(無ければ玉で取られて詰まない)
			if(attacksToIsNot0(US, to)) {
				// 玉が逃げられず、他の駒で取ることも出来ないか
				if(!canKingEscape(*this, US, to, rookAttackToEdge(to))
				   && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
				{
					return makeDropMove(Rook, to);
				}
			}
		}
	}
	// 香車打ち
	// 飛車で詰まなければ香車でも詰まないので、else if を使用。
	// 玉が 9(1) 段目にいれば香車で王手出来無いので、それも省く。
	else if(ourHand.exists<HLance>() && isInFrontOf<US, Rank1, Rank9>(makeRank(ksq))) {
		const Square to = ksq + TDeltaS;
		if(piece(to) == Empty && attacksToIsNot0(US, to)) {
			if(!canKingEscape(*this, US, to, lanceAttackToEdge(US, to))
			   && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
			{
				return makeDropMove(Lance, to);
			}
		}
	}

	// 角打ち
	if(ourHand.exists<HBishop>()) {
		Bitboard toBB = dropTarget & bishopStepAttacks(ksq);
		while(toBB.isNot0()) {
			const Square to = toBB.firstOneFromI9();
			if(attacksToIsNot0(US, to)) {
				if(!canKingEscape(*this, US, to, bishopAttackToEdge(to))
				   && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
				{
					return makeDropMove(Bishop, to);
				}
			}
		}
	}

	// 金打ち
	if(ourHand.exists<HGold>()) {
		Bitboard toBB;
		if(ourHand.exists<HRook>()) {
			// 飛車打ちを先に調べたので、尻金だけは省く。
			toBB = dropTarget & (goldAttack(Them, ksq) ^ pawnAttack(US, ksq));
		}
		else {
			toBB = dropTarget & goldAttack(Them, ksq);
		}
		while(toBB.isNot0()) {
			const Square to = toBB.firstOneFromI9();
			if(attacksToIsNot0(US, to)) {
				if(!canKingEscape(*this, US, to, goldAttack(US, to))
				   && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
				{
					return makeDropMove(Gold, to);
				}
			}
		}
	}

	if(ourHand.exists<HSilver>()) {
		Bitboard toBB;
		if(ourHand.exists<HGold>()) {
			// 金打ちを先に調べたので、斜め後ろから打つ場合だけを調べる。

			if(ourHand.exists<HBishop>()) {
				// 角打ちを先に調べたので、斜めからの王手も除外できる。銀打ちを調べる必要がない。
				goto silver_drop_end;
			}
			// 斜め後ろから打つ場合を調べる必要がある。
			toBB = dropTarget & (silverAttack(Them, ksq) & inFrontMask(US, makeRank(ksq)));
		}
		else {
			if(ourHand.exists<HBishop>()) {
				// 斜め後ろを除外。前方から打つ場合を調べる必要がある。
				toBB = dropTarget & (silverAttack(Them, ksq) & goldAttack(Them, ksq));
			}
			else {
				toBB = dropTarget & silverAttack(Them, ksq);
			}
		}
		while(toBB.isNot0()) {
			const Square to = toBB.firstOneFromI9();
			if(attacksToIsNot0(US, to)) {
				if(!canKingEscape(*this, US, to, silverAttack(US, to))
				   && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
				{
					return makeDropMove(Silver, to);
				}
			}
		}
	}
silver_drop_end:

	if(ourHand.exists<HKnight>()) {
		Bitboard toBB = dropTarget & knightAttack(Them, ksq);
		while(toBB.isNot0()) {
			const Square to = toBB.firstOneFromI9();
			// 桂馬は紐が付いている必要はない。
			// よって、このcanKingEscape() 内での to の位置に逃げられないようにする処理は必要ない。
			// todo: 関数を分けるなどして、桂馬用に最適化すること。
			if(!canKingEscape(*this, US, to, allZeroBB())
			   && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
			{
				return makeDropMove(Knight, to);
			}
		}
	}

	// 歩打ちで詰ますと反則なので、調べない。

	// 駒を移動する場合
	const Bitboard moveTarget = ~bbOf(US);
	const Bitboard pinned = pinnedBB();
	const Bitboard dcBB_betweenIsUs = discoveredCheckBB<true>();

	{
		// 竜による移動
		Bitboard fromBB = bbOf(Dragon, US);
		while(fromBB.isNot0()) {
			const Square from = fromBB.firstOneFromI9();
			// 遠隔王手は考えない。
			Bitboard toBB = moveTarget & attacksFrom<Dragon>(from) & kingAttack(ksq);
			if(toBB.isNot0()) {
				xorBBs(Dragon, from, US);
				// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
				const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
				// to の位置の Bitboard は canKingEscape の中で更新する。
				do {
					const Square to = toBB.firstOneFromI9();
					// 王手した駒の場所に自駒の利きがあるか。(無ければ玉で取られて詰まない)
					if(unDropCheckIsSupported(US, to)) {
						// 玉が逃げられない
						// かつ、(空き王手 または 他の駒で取れない)
						// かつ、王手した駒が pin されていない
						if(!canKingEscape(*this, US, to, ((squareRelation(to, ksq) & DirecCross) ?
														  //                    ここは竜の利きでも飛車の利きでも同じ。速い方を使う。
														  dragonAttackToEdge(to) : attacksFrom<Rook>(to)))
						   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
							   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
						   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
						{
							xorBBs(Dragon, from, US);
							return makeCaptureMove(Dragon, from, to, *this);
						}
					}
				} while(toBB.isNot0());
				xorBBs(Dragon, from, US);
			}
		}
	}

	{
		// 飛車による移動

		// Txxx は先手、後手の情報を吸収した変数。数字は先手に合わせている。
		const Rank TRank6 = (US == Black ? Rank6 : Rank4);
		const Bitboard TRank789BB = inFrontMask<US, TRank6>();
		Bitboard fromBB = bbOf(Rook, US);
		Bitboard fromOn789BB = fromBB & TRank789BB;
		// from が 789 段目
		if(fromOn789BB.isNot0()) {
			fromBB &= ~TRank789BB;
			do {
				const Square from = fromOn789BB.firstOneFromI9();
				Bitboard toBB = moveTarget & attacksFrom<Rook>(from) & kingAttack(ksq);
				if(toBB.isNot0()) {
					xorBBs(Rook, from, US);
					// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
					const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
					// to の位置の Bitboard は canKingEscape の中で更新する。
					do {
						const Square to = toBB.firstOneFromI9();
						if(unDropCheckIsSupported(US, to)) {
							if(!canKingEscape(*this, US, to, ((squareRelation(to, ksq) & DirecCross) ?
															  //                    ここは竜の利きでも飛車の利きでも同じ。速い方を使う。
															  dragonAttackToEdge(to) : attacksFrom<Rook>(to)))
							   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
								   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
							   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
							{
								xorBBs(Rook, from, US);
								return makeCapturePromoteMove(Rook, from, to, *this);
							}
						}
					} while(toBB.isNot0());
					xorBBs(Rook, from, US);
				}
			} while(fromOn789BB.isNot0());
		}

		// from が 1~6 段目
		while(fromBB.isNot0()) {
			const Square from = fromBB.firstOneFromI9();
			Bitboard toBB = moveTarget & attacksFrom<Rook>(from) & (rookStepAttacks(ksq)
																	| (kingAttack(ksq) & TRank789BB));
			if(toBB.isNot0()) {
				xorBBs(Rook, from, US);
				// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
				const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();

				Bitboard toOn789BB = toBB & TRank789BB;
				// 成り
				if(toOn789BB.isNot0()) {
					do {
						const Square to = toOn789BB.firstOneFromI9();
						if(unDropCheckIsSupported(US, to)) {
							if(!canKingEscape(*this, US, to, ((squareRelation(to, ksq) & DirecCross) ?
															  //                    ここは竜の利きでも飛車の利きでも同じ。速い方を使う。
															  dragonAttackToEdge(to) : attacksFrom<Rook>(to)))
							   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
								   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
							   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
							{
								xorBBs(Rook, from, US);
								return makeCapturePromoteMove(Rook, from, to, *this);
							}
						}
					} while(toOn789BB.isNot0());

					toBB &= ~TRank789BB;
				}
				// 不成
				while(toBB.isNot0()) {
					const Square to = toBB.firstOneFromI9();
					if(unDropCheckIsSupported(US, to)) {
						if(!canKingEscape(*this, US, to, rookAttackToEdge(to))
						   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
							   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
						   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
						{
							xorBBs(Rook, from, US);
							return makeCaptureMove(Rook, from, to, *this);
						}
					}
				}
				xorBBs(Rook, from, US);
			}
		}
	}

	{
		// 馬による移動
		Bitboard fromBB = bbOf(Horse, US);
		while(fromBB.isNot0()) {
			const Square from = fromBB.firstOneFromI9();
			// 遠隔王手は考えない。
			Bitboard toBB = moveTarget & attacksFrom<Horse>(from) & kingAttack(ksq);
			if(toBB.isNot0()) {
				xorBBs(Horse, from, US);
				// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
				const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
				// to の位置の Bitboard は canKingEscape の中で更新する。
				do {
					const Square to = toBB.firstOneFromI9();
					// 王手した駒の場所に自駒の利きがあるか。(無ければ玉で取られて詰まない)
					if(unDropCheckIsSupported(US, to)) {
						// 玉が逃げられない
						// かつ、(空き王手 または 他の駒で取れない)
						// かつ、動かした駒が pin されていない)
						if(!canKingEscape(*this, US, to, horseAttackToEdge(to)) // 竜の場合と違って、常に最大の利きを使用して良い。
						   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
							   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
						   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
						{
							xorBBs(Horse, from, US);
							return makeCaptureMove(Horse, from, to, *this);
						}
					}
				} while(toBB.isNot0());
				xorBBs(Horse, from, US);
			}
		}
	}

	{
		// 角による移動

		// Txxx は先手、後手の情報を吸収した変数。数字は先手に合わせている。
		const Rank TRank6 = (US == Black ? Rank6 : Rank4);
		const Bitboard TRank789BB = inFrontMask<US, TRank6>();
		Bitboard fromBB = bbOf(Bishop, US);
		Bitboard fromOn789BB = fromBB & TRank789BB;
		// from が 789 段目
		if(fromOn789BB.isNot0()) {
			fromBB &= ~TRank789BB;
			do {
				const Square from = fromOn789BB.firstOneFromI9();
				Bitboard toBB = moveTarget & attacksFrom<Bishop>(from) & kingAttack(ksq);
				if(toBB.isNot0()) {
					xorBBs(Bishop, from, US);
					// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
					const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
					// to の位置の Bitboard は canKingEscape の中で更新する。
					do {
						const Square to = toBB.firstOneFromI9();
						if(unDropCheckIsSupported(US, to)) {
							if(!canKingEscape(*this, US, to, horseAttackToEdge(to))
							   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
								   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
							   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
							{
								xorBBs(Bishop, from, US);
								return makeCapturePromoteMove(Bishop, from, to, *this);
							}
						}
					} while(toBB.isNot0());
					xorBBs(Bishop, from, US);
				}
			} while(fromOn789BB.isNot0());
		}

		// from が 1~6 段目
		while(fromBB.isNot0()) {
			const Square from = fromBB.firstOneFromI9();
			Bitboard toBB = moveTarget & attacksFrom<Bishop>(from) & (bishopStepAttacks(ksq)
																	  | (kingAttack(ksq) & TRank789BB));
			if(toBB.isNot0()) {
				xorBBs(Bishop, from, US);
				// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
				const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();

				Bitboard toOn789BB = toBB & TRank789BB;
				// 成り
				if(toOn789BB.isNot0()) {
					do {
						const Square to = toOn789BB.firstOneFromI9();
						if(unDropCheckIsSupported(US, to)) {
							if(!canKingEscape(*this, US, to, horseAttackToEdge(to))
							   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
								   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
							   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
							{
								xorBBs(Bishop, from, US);
								return makeCapturePromoteMove(Bishop, from, to, *this);
							}
						}
					} while(toOn789BB.isNot0());

					toBB &= ~TRank789BB;
				}
				// 不成
				while(toBB.isNot0()) {
					const Square to = toBB.firstOneFromI9();
					if(unDropCheckIsSupported(US, to)) {
						if(!canKingEscape(*this, US, to, bishopAttackToEdge(to))
						   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
							   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
						   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
						{
							xorBBs(Bishop, from, US);
							return makeCaptureMove(Bishop, from, to, *this);
						}
					}
				}
				xorBBs(Bishop, from, US);
			}
		}
	}

	{
		// 金、成り金による移動
		Bitboard fromBB = goldsBB(US) & goldCheckTable(US, ksq);
		while(fromBB.isNot0()) {
			const Square from = fromBB.firstOneFromI9();
			Bitboard toBB = moveTarget & attacksFrom<Gold>(US, from) & attacksFrom<Gold>(Them, ksq);
			if(toBB.isNot0()) {
				const PieceType pt = pieceToPieceType(piece(from));
				xorBBs(pt, from, US);
				// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
				const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
				// to の位置の Bitboard は canKingEscape の中で更新する。
				do {
					const Square to = toBB.firstOneFromI9();
					// 王手した駒の場所に自駒の利きがあるか。(無ければ玉で取られて詰まない)
					if(unDropCheckIsSupported(US, to)) {
						// 玉が逃げられない
						// かつ、(空き王手 または 他の駒で取れない)
						// かつ、動かした駒が pin されていない)
						if(!canKingEscape(*this, US, to, attacksFrom<Gold>(US, to))
						   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
							   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
						   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
						{
							xorBBs(pt, from, US);
							return makeCaptureMove(pt, from, to, *this);
						}
					}
				} while(toBB.isNot0());
				xorBBs(pt, from, US);
			}
		}
	}

	{
		// 銀による移動

		Bitboard fromBB = bbOf(Silver, US) & silverCheckTable(US, ksq);
		if(fromBB.isNot0()) {
			// Txxx は先手、後手の情報を吸収した変数。数字は先手に合わせている。
			const Rank TRank6 = (US == Black ? Rank6 : Rank4);
			const Bitboard TRank789BB = inFrontMask<US, TRank6>();
			const Bitboard TRank1_5BB = inFrontMask<Them, TRank6>();
			const Bitboard chkBB = attacksFrom<Silver>(Them, ksq);
			const Bitboard chkBB_promo = attacksFrom<Gold>(Them, ksq);

			Bitboard fromOn789BB = fromBB & TRank789BB;
			// from が敵陣
			if(fromOn789BB.isNot0()) {
				fromBB &= ~TRank789BB;
				do {
					const Square from = fromOn789BB.firstOneFromI9();
					Bitboard toBB = moveTarget & attacksFrom<Silver>(US, from);
					Bitboard toBB_promo = toBB;

					toBB &= chkBB;
					toBB_promo &= chkBB_promo;
					if((toBB_promo | toBB).isNot0()) {
						xorBBs(Silver, from, US);
						// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
						const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
						// to の位置の Bitboard は canKingEscape の中で更新する。
						while(toBB_promo.isNot0()) {
							const Square to = toBB_promo.firstOneFromI9();
							if(unDropCheckIsSupported(US, to)) {
								// 成り
								if(!canKingEscape(*this, US, to, attacksFrom<Gold>(US, to))
								   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
									   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
								   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
								{
									xorBBs(Silver, from, US);
									return makeCapturePromoteMove(Silver, from, to, *this);
								}
							}
						}

						// 玉の前方に移動する場合、成で詰まなかったら不成でも詰まないので、ここで省く。
						// sakurapyon の作者が言ってたので実装。
						toBB &= ~inFrontMask(Them, makeRank(ksq));
						while(toBB.isNot0()) {
							const Square to = toBB.firstOneFromI9();
							if(unDropCheckIsSupported(US, to)) {
								// 不成
								if(!canKingEscape(*this, US, to, attacksFrom<Silver>(US, to))
								   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
									   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
								   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
								{
									xorBBs(Silver, from, US);
									return makeCaptureMove(Silver, from, to, *this);
								}
							}
						}

						xorBBs(Silver, from, US);
					}
				} while(fromOn789BB.isNot0());
			}

			// from が 5~9段目 (必ず不成)
			Bitboard fromOn1_5BB = fromBB & TRank1_5BB;
			if(fromOn1_5BB.isNot0()) {
				fromBB &= ~TRank1_5BB;
				do {
					const Square from = fromOn1_5BB.firstOneFromI9();
					Bitboard toBB = moveTarget & attacksFrom<Silver>(US, from);
					toBB &= chkBB;

					if(toBB.isNot0()) {
						xorBBs(Silver, from, US);
						// 動いた後の dcBB, pinned: to の位置の occupied や checkers は関係ないので、ここで生成できる。
						const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
						// to の位置の Bitboard は canKingEscape の中で更新する。
						while(toBB.isNot0()) {
							const Square to = toBB.firstOneFromI9();
							if(unDropCheckIsSupported(US, to)) {
								// 成り
								if(!canKingEscape(*this, US, to, attacksFrom<Silver>(US, to))
								   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
									   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
								   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
								{
									xorBBs(Silver, from, US);
									return makeCaptureMove(Silver, from, to, *this);
								}
							}
						}

						xorBBs(Silver, from, US);
					}
				} while(fromOn1_5BB.isNot0());
			}

			// 残り 4 段目のみ
			// 前進するときは成れるが、後退するときは成れない。
			while(fromBB.isNot0()) {
				const Square from = fromBB.firstOneFromI9();
				Bitboard toBB = moveTarget & attacksFrom<Silver>(US, from);
				Bitboard toBB_promo = toBB & TRank789BB; // 3 段目にしか成れない。

				toBB &= chkBB;
				toBB_promo &= chkBB_promo;
				if((toBB_promo | toBB).isNot0()) {
					xorBBs(Silver, from, US);
					// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
					const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
					// to の位置の Bitboard は canKingEscape の中で更新する。
					while(toBB_promo.isNot0()) {
						const Square to = toBB_promo.firstOneFromI9();
						if(unDropCheckIsSupported(US, to)) {
							// 成り
							if(!canKingEscape(*this, US, to, attacksFrom<Gold>(US, to))
							   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
								   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
							   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
							{
								xorBBs(Silver, from, US);
								return makeCapturePromoteMove(Silver, from, to, *this);
							}
						}
					}

					// 玉の前方に移動する場合、成で詰まなかったら不成でも詰まないので、ここで省く。
					// sakurapyon の作者が言ってたので実装。
					toBB &= ~inFrontMask(Them, makeRank(ksq));
					while(toBB.isNot0()) {
						const Square to = toBB.firstOneFromI9();
						if(unDropCheckIsSupported(US, to)) {
							// 不成
							if(!canKingEscape(*this, US, to, attacksFrom<Silver>(US, to))
							   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
								   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
							   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
							{
								xorBBs(Silver, from, US);
								return makeCaptureMove(Silver, from, to, *this);
							}
						}
					}

					xorBBs(Silver, from, US);
				}
			}
		}
	}

	{
		// 桂による移動

		Bitboard fromBB = bbOf(Knight, US) & knightCheckTable(US, ksq);
		if(fromBB.isNot0()) {
			// Txxx は先手、後手の情報を吸収した変数。数字は先手に合わせている。
			const Rank TRank6 = (US == Black ? Rank6 : Rank4);
			const Bitboard TRank789BB = inFrontMask<US, TRank6>();

			const Bitboard chkBB_promo = attacksFrom<Gold>(Them, ksq) & TRank789BB;
			const Bitboard chkBB = attacksFrom<Knight>(Them, ksq);

			do {
				const Square from = fromBB.firstOneFromI9();
				Bitboard toBB = moveTarget & attacksFrom<Knight>(US, from);
				Bitboard toBB_promo = toBB & chkBB_promo;
				toBB &= chkBB;
				if((toBB_promo | toBB).isNot0()) {
					xorBBs(Knight, from, US);
					// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
					const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
					// to の位置の Bitboard は canKingEscape の中で更新する。
					while(toBB_promo.isNot0()) {
						const Square to = toBB_promo.firstOneFromI9();
						if(unDropCheckIsSupported(US, to)) {
							// 成り
							if(!canKingEscape(*this, US, to, attacksFrom<Gold>(US, to))
							   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
								   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
							   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
							{
								xorBBs(Knight, from, US);
								return makeCapturePromoteMove(Knight, from, to, *this);
							}
						}
					}

					while(toBB.isNot0()) {
						const Square to = toBB.firstOneFromI9();
						// 桂馬は紐が付いてなくて良いので、紐が付いているかは調べない。
						// 不成
						if(!canKingEscape(*this, US, to, allZeroBB())
						   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
							   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
						   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
						{
							xorBBs(Knight, from, US);
							return makeCaptureMove(Knight, from, to, *this);
						}
					}
					xorBBs(Knight, from, US);
				}
			} while(fromBB.isNot0());
		}
	}

	{
		// 香車による移動

		Bitboard fromBB = bbOf(Lance, US) & lanceCheckTable(US, ksq);
		if(fromBB.isNot0()) {
			// Txxx は先手、後手の情報を吸収した変数。数字は先手に合わせている。
			const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);
			const Rank TRank8 = (US == Black ? Rank8 : Rank2);
			const Rank TRank6 = (US == Black ? Rank6 : Rank4);
			const Bitboard TRank789BB = inFrontMask<US, TRank6>();
			const Bitboard chkBB_promo = attacksFrom<Gold>(Them, ksq) & TRank789BB;
			// 玉の前方1マスのみ。
			// 玉が 1 段目にいるときは、成のみで良いので省く。
			const Bitboard chkBB = attacksFrom<Pawn>(Them, ksq) & inFrontMask<Them, TRank8>();

			do {
				const Square from = fromBB.firstOneFromI9();
				Bitboard toBB = moveTarget & attacksFrom<Lance>(US, from);
				Bitboard toBB_promo = toBB & chkBB_promo;

				toBB &= chkBB;

				if((toBB_promo | toBB).isNot0()) {
					xorBBs(Lance, from, US);
					// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
					const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
					// to の位置の Bitboard は canKingEscape の中で更新する。

					while(toBB_promo.isNot0()) {
						const Square to = toBB_promo.firstOneFromI9();
						if(unDropCheckIsSupported(US, to)) {
							// 成り
							if(!canKingEscape(*this, US, to, attacksFrom<Gold>(US, to))
							   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
								   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
							   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
							{
								xorBBs(Lance, from, US);
								return makeCapturePromoteMove(Lance, from, to, *this);
							}
						}
					}

					if(toBB.isNot0()) {
						assert(toBB.isOneBit());
						// 不成で王手出来るのは、一つの場所だけなので、ループにする必要が無い。
						const Square to = ksq + TDeltaS;
						if(unDropCheckIsSupported(US, to)) {
							// 不成
							if(!canKingEscape(*this, US, to, lanceAttackToEdge(US, to))
							   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
								   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
							   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
							{
								xorBBs(Lance, from, US);
								return makeCaptureMove(Lance, from, to, *this);
							}
						}
					}
					xorBBs(Lance, from, US);
				}
			} while(fromBB.isNot0());
		}
	}

	{
		// 歩による移動
		// 成れる場合は必ずなる。
		// todo: ちょっと条件分岐多くて遅い気がする。このブロック内は要書き換え。

		const Rank krank = makeRank(ksq);
		// 歩が移動して王手になるのは、相手玉が1~7段目の時のみ。
		if(isInFrontOf<US, Rank2, Rank8>(krank)) {
			// Txxx は先手、後手の情報を吸収した変数。数字は先手に合わせている。
			const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);
			const SquareDelta TDeltaN = (US == Black ? DeltaN : DeltaS);

			Bitboard fromBB = bbOf(Pawn, US);
			// 玉が敵陣にいないと成で王手になることはない。
			if(isInFrontOf<US, Rank6, Rank4>(krank)) {
				const Rank TRank6 = (US == Black ? Rank6 : Rank4);
				const Bitboard TRank789BB = inFrontMask<US, TRank6>();
				// 成った時に王手になる位置
				const Bitboard toBB_promo = moveTarget & attacksFrom<Gold>(Them, ksq) & TRank789BB;
				Bitboard fromBB_promo = fromBB & pawnAttack<Them>(toBB_promo);
				while(fromBB_promo.isNot0()) {
					const Square from = fromBB_promo.firstOneFromI9();
					const Square to = from + TDeltaN;

					xorBBs(Pawn, from, US);
					// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
					const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
					// to の位置の Bitboard は canKingEscape の中で更新する。
					if(unDropCheckIsSupported(US, to)) {
						// 成り
						if(!canKingEscape(*this, US, to, attacksFrom<Gold>(US, to))
						   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
							   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
						   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
						{
							xorBBs(Pawn, from, US);
							return makeCapturePromoteMove(Pawn, from, to, *this);
						}
					}
					xorBBs(Pawn, from, US);
				}
			}

			// 不成
			// 玉が 8,9 段目にいることは無いので、from,to が隣の筋を指すことは無い。
			const Square to = ksq + TDeltaS;
			const Square from = to + TDeltaS;
			if(fromBB.isSet(from) && !bbOf(US).isSet(to)) {
				// 玉が 1, 2 段目にいるなら、成りで王手出来るので不成は調べない。
				if(isBehind<US, Rank8, Rank2>(krank)) {
					xorBBs(Pawn, from, US);
					// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
					const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
					// to の位置の Bitboard は canKingEscape の中で更新する。
					if(unDropCheckIsSupported(US, to)) {
						// 不成
						if(!canKingEscape(*this, US, to, allZeroBB())
						   && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
							   || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
						   && !isPinnedIllegal(from, to, kingSquare(US), pinned))
						{
							xorBBs(Pawn, from, US);
							return makeCaptureMove(Pawn, from, to, *this);
						}
					}
					xorBBs(Pawn, from, US);
				}
			}
		}
	}

	return Move::moveNone();
}

Move Position::mateMoveIn1Ply() {
	return (turn() == Black ? mateMoveIn1Ply<Black>() : mateMoveIn1Ply<White>());
}

void Position::initZobrist() {
	// zobTurn_ は 1 であり、その他は 1桁目を使わない。
	// zobTurn のみ xor で更新する為、他の桁に影響しないようにする為。
	// hash値の更新は普通は全て xor を使うが、持ち駒の更新の為に +, - を使用した方が都合が良い。
	for(PieceType pt = Occupied; pt < PieceTypeNum; ++pt) {
		for(Square sq = I9; sq < SquareNum; ++sq) {
			for(Color c = Black; c < ColorNum; ++c) {
				zobrist_[pt][sq][c] = g_mt64bit.random() & ~UINT64_C(1);
			}
		}
	}
	for(HandPiece hp = HPawn; hp < HandPieceNum; ++hp) {
		zobHand_[hp][Black] = g_mt64bit.random() & ~UINT64_C(1);
		zobHand_[hp][White] = g_mt64bit.random() & ~UINT64_C(1);
	}
	zobExclusion_ = g_mt64bit.random() & ~UINT64_C(1);
}

void Position::print() const {
	std::cout << "'  9  8  7  6  5  4  3  2  1" << std::endl;
	int i = 0;
	for(Rank r = Rank9; r < RankNum; ++r) {
		++i;
		std::cout << "P" << i;
		for(File f = FileA; FileI <= f; --f) {
			std::cout << PieceToCharCSA.value(piece(makeSquare(f, r)));
		}
		std::cout << std::endl;
	}
	printHand(Black);
	printHand(White);
	std::cout << (turn() == Black ? "+" : "-") << std::endl;
	std::cout << std::endl;
	std::cout << "key = " << getKey() << std::endl;
}

#if !defined NDEBUG
bool Position::isOK() const {
	static Key prevKey;
	const bool debugAll = true;

	const bool debugBitboards    = debugAll || false;
	const bool debugKingCount    = debugAll || false;
	const bool debugKingCapture  = debugAll || false;
	const bool debugCheckerCount = debugAll || false;
	const bool debugKey          = debugAll || false;
	const bool debugPiece        = debugAll || false;
	const bool debugMaterial     = debugAll || false;

	int failedStep = 0;
	if(debugBitboards) {
		if((bbOf(Black) & bbOf(White)).isNot0()) {
			goto incorrect_position;
		}
		if((bbOf(Black) | bbOf(White)) != occupiedBB()) {
			goto incorrect_position;
		}
		if((bbOf(Pawn     ) ^ bbOf(Lance    ) ^ bbOf(Knight) ^ bbOf(Silver ) ^ bbOf(Bishop  ) ^
			bbOf(Rook     ) ^ bbOf(Gold     ) ^ bbOf(King  ) ^ bbOf(ProPawn) ^ bbOf(ProLance) ^
			bbOf(ProKnight) ^ bbOf(ProSilver) ^ bbOf(Horse ) ^ bbOf(Dragon )) != occupiedBB())
		{
			goto incorrect_position;
		}
		for(PieceType pt1 = Pawn; pt1 < PieceTypeNum; ++pt1) {
			for(PieceType pt2 = pt1 + 1; pt2 < PieceTypeNum; ++pt2) {
				if((bbOf(pt1) & bbOf(pt2)).isNot0()) {
					goto incorrect_position;
				}
			}
		}
	}

	++failedStep;
	if(debugKingCount) {
		int kingCount[ColorNum] = {0, 0};
		if(bbOf(King).popCount() != 2) {
			goto incorrect_position;
		}
		if(!bbOf(King, Black).isOneBit()) {
			goto incorrect_position;
		}
		if(!bbOf(King, White).isOneBit()) {
			goto incorrect_position;
		}
		for(Square sq = I9; sq < SquareNum; ++sq) {
			if(piece(sq) == BKing) {
				++kingCount[Black];
			}
			if(piece(sq) == WKing) {
				++kingCount[White];
			}
		}
		if(kingCount[Black] != 1 || kingCount[White] != 1) {
			goto incorrect_position;
		}
	}

	++failedStep;
	if(debugKingCapture) {
		// 相手玉を取れないことを確認
		const Color us = turn();
		const Color them = oppositeColor(us);
		const Square ksq = kingSquare(them);
		if(attacksTo(us, ksq).isNot0()) {
			goto incorrect_position;
		}
	}

	++failedStep;
	if(debugCheckerCount) {
		if(2 < st_->checkersBB.popCount()) {
			goto incorrect_position;
		}
	}

	++failedStep;
	if(debugKey) {
		if(getKey() != computeKey()) {
			goto incorrect_position;
		}
	}

	++failedStep;
	if(debugPiece) {
		for(Square sq = I9; sq < SquareNum; ++sq) {
			const Piece pc = piece(sq);
			if(pc == Empty) {
				if(!emptyBB().isSet(sq)) {
					goto incorrect_position;
				}
			}
			else {
				if(!bbOf(pieceToPieceType(pc), pieceToColor(pc)).isSet(sq)) {
					goto incorrect_position;
				}
			}
		}
	}

	++failedStep;
	if(debugMaterial) {
		if(material() != computeMaterial()) {
			goto incorrect_position;
		}
	}

	prevKey = getKey();
	return true;

incorrect_position:
	std::cout << "Error! failedStep = " << failedStep << std::endl;
	std::cout << "prevKey = " << prevKey << std::endl;
	std::cout << "currKey = " << getKey() << std::endl;
	print();
	return false;
}
#endif

Key Position::computeKey() const {
	Key result = 0;
	for(Square sq = I9; sq < SquareNum; ++sq) {
		if(piece(sq) != Empty) {
			result += zobrist(pieceToPieceType(piece(sq)), sq, pieceToColor(piece(sq)));
		}
	}
	for(HandPiece hp = HPawn; hp < HandPieceNum; ++hp) {
		for(Color c = Black; c < ColorNum; ++c) {
			const int num = hand(c).numOf(hp);
			for(int i = 0; i < num; ++i) {
				result += zobHand(hp, c);
			}
		}
	}
	if(turn() == White) {
		result ^= zobTurn();
	}
	return result;
}

// todo: isRepetition() に名前変えた方が良さそう。
//       同一局面4回をきちんと数えていないけど問題ないか。
RepetitionType Position::isDraw() const {
	const int Start = 4;
	int i = Start;
	const int e = st_->pliesFromNull;

	// 4手掛けないと千日手には絶対にならない。
	if(i <= e) {
		// 現在の局面と、少なくとも 4 手戻らないと同じ局面にならない。
		// ここでまず 2 手戻る。
		StateInfo* stp = st_->previous->previous;

		do {
			// 更に 2 手戻る。
			stp = stp->previous->previous;
			if(stp->key == st_->key) {
				if(i <= st_->continuousCheck[turn()]) {
					return RepetitionLose;
				}
				else if(i <= st_->continuousCheck[oppositeColor(turn())]) {
					return RepetitionWin;
				}
#if defined BAN_BLACK_REPETITION
				return (turn() == Black ? RepetitionLose : RepetitionWin);
#elif defined BAN_WHITE_REPETITION
				return (turn() == White ? RepetitionLose : RepetitionWin);
#else
				return RepetitionDraw;
#endif
			}
			i += 2;
		} while(i <= e);
	}
	return NotRepetition;
}

namespace {
	void printHandPiece(const Position& pos, const HandPiece hp, const Color c, const std::string& str) {
		if(pos.hand(c).numOf(hp)) {
			const char* sign = (c == Black ? "+" : "-");
			std::cout << "P" << sign;
			for(u32 i = 0; i < pos.hand(c).numOf(hp); ++i) {
				std::cout << "00" << str;
			}
			std::cout << std::endl;
		}
	}
}
void Position::printHand(const Color c) const {
	printHandPiece(*this, HPawn  , c, "FU");
	printHandPiece(*this, HLance , c, "KY");
	printHandPiece(*this, HKnight, c, "KE");
	printHandPiece(*this, HSilver, c, "GI");
	printHandPiece(*this, HGold  , c, "KI");
	printHandPiece(*this, HBishop, c, "KA");
	printHandPiece(*this, HRook  , c, "HI");
}

Position& Position::operator = (const Position& pos) {
	memcpy(this, &pos, sizeof(Position));
	startState_ = *st_;
	st_ = &startState_;
	nodes_ = 0;

	assert(isOK());

	return *this;
}

void Position::set(const std::string& sfen/*, Thread* th, SearchInfo* si*/) {
	Piece promoteFlag = UnPromoted;
	std::istringstream ss(sfen);
	char token;
	Square sq = A9;

	clear();

	// 盤上の駒
	while(ss.get(token) && token != ' ') {
		if(isdigit(token)) {
			sq += DeltaE * (token - '0');
		}
		else if(token == '/') {
			sq += (DeltaW * 9) + DeltaS;
		}
		else if(token == '+') {
			promoteFlag = Promoted;
		}
		else if(g_charToPieceUSI.isLegalChar(token)) {
			if(isInSquare(sq)) {
				setPiece(g_charToPieceUSI.value(token) + promoteFlag, sq);
				promoteFlag = UnPromoted;
				sq += DeltaE;
			}
			else {
				goto INCORRECT;
			}
		}
		else {
			goto INCORRECT;
		}
	}

	// 手番
	while(ss.get(token) && token != ' ') {
		if(token == 'b') {
			turn_ = Black;
		}
		else if(token == 'w') {
			turn_ = White;
		}
		else {
			goto INCORRECT;
		}
	}

	// 持ち駒
	for(int digits = 0; ss.get(token) && token != ' '; ) {
		if(token == '-') {
			memset(hand_, 0, sizeof(hand_));
		}
		else if(isdigit(token)) {
			digits = digits * 10 + token - '0';
		}
		else if(g_charToPieceUSI.isLegalChar(token)) {
			// 持ち駒を32bit に pack する
			const Piece piece = g_charToPieceUSI.value(token);
			setHand(piece, (digits == 0 ? 1 : digits));

			digits = 0;
		}
		else {
			goto INCORRECT;
		}
	}

	// 次の手が何手目か
	ss >> gamePly_;
	gamePly_ = std::max(2 * (gamePly_ - 1), 0) + static_cast<int>(turn() == White);

	// 残り時間, hash key, (もし実装するなら)駒番号などをここで設定
	st_->key = computeKey();

	findCheckers();
	st_->material = computeMaterial();
//	thisThread_ = th;
//	searchInfo_ = si;

	return;
INCORRECT:
	std::cout << "incorrect SFEN string : " << sfen << std::endl;
}

bool Position::moveGivesCheck(const Move move) const {
	return moveGivesCheck(move, CheckInfo(*this));
}

// move が王手なら true
bool Position::moveGivesCheck(const Move move, const CheckInfo& ci) const {
	assert(isOK());
	assert(ci.dcBB == discoveredCheckBB());

	const Square to = move.to();
	if(move.isDrop()) {
		const PieceType ptTo = move.pieceTypeDropped();
		// Direct Check ?
		if(ci.checkBB[ptTo].isSet(to)) {
			return true;
		}
	}
	else {
		const Square from = move.from();
		const PieceType ptFrom = move.pieceTypeFrom();
		const PieceType ptTo = move.pieceTypeTo(ptFrom);
		assert(ptFrom == pieceToPieceType(piece(from)));
		// Direct Check ?
		if(ci.checkBB[ptTo].isSet(to)) {
			return true;
		}

		// Discovery Check ?
		if(isDiscoveredCheck(from, to, kingSquare(oppositeColor(turn())), ci.dcBB)) {
			return true;
		}
	}

	return false;
}

void Position::clear() {
	memset(this, 0, sizeof(Position));
	st_ = &startState_;

#if defined USE_PIECELIST
	for(Color c = Black; c < ColorNum; ++c) {
		for(PieceType pt = Occupied; pt < PieceTypeNum; ++pt) {
			for(int i = 0; i < 18; ++i) {
				pieceList_[c][pt][i] = SquareNum;
			}
		}
	}
#endif
}

// 先手、後手に関わらず、sq へ移動可能な Bitboard を返す。
Bitboard Position::attacksTo(const Square sq, const Bitboard& occupied) const {
	const Bitboard golds = goldsBB();
	return (((attacksFrom<Pawn  >(Black, sq) & bbOf(Pawn))
			 | (attacksFrom<Lance >(Black, sq, occupied) & bbOf(Lance))
			 | (attacksFrom<Knight>(Black, sq)           & bbOf(Knight))
			 | (attacksFrom<Silver>(Black, sq)           & bbOf(Silver))
			 | (attacksFrom<Gold  >(Black, sq)           & golds))
			& bbOf(White))
		| (((attacksFrom<Pawn  >(White, sq) & bbOf(Pawn))
			| (attacksFrom<Lance >(White, sq, occupied) & bbOf(Lance))
			| (attacksFrom<Knight>(White, sq)           & bbOf(Knight))
			| (attacksFrom<Silver>(White, sq)           & bbOf(Silver))
			| (attacksFrom<Gold  >(White, sq)           & golds))
		   & bbOf(Black))
		| (attacksFrom<Bishop>(sq, occupied) & bbOf(Bishop, Horse))
		| (attacksFrom<Rook  >(sq, occupied) & bbOf(Rook,   Dragon))
		| (attacksFrom<King  >(sq)           & (bbOf(King, Horse, Dragon)));
}

// sq へ移動可能な c 側の駒の Bitboard を返す。
Bitboard Position::attacksTo(const Color c, const Square sq) const {
	const Color opposite = oppositeColor(c);
	return ((attacksFrom<Pawn  >(opposite, sq) & bbOf(Pawn))
			| (attacksFrom<Lance >(opposite, sq) & bbOf(Lance))
			| (attacksFrom<Knight>(opposite, sq) & bbOf(Knight))
			| (attacksFrom<Silver>(opposite, sq) & bbOf(Silver))
			| (attacksFrom<Gold  >(opposite, sq) & goldsBB())
			| (attacksFrom<Bishop>(sq)           & bbOf(Bishop, Horse))
			| (attacksFrom<Rook  >(sq)           & bbOf(Rook  , Dragon))
			| (attacksFrom<King  >(sq)           & (bbOf(King, Horse, Dragon))))
		& bbOf(c);
}

// occupied を Position::occupiedBB() 以外のものを使用する場合に使用する。
Bitboard Position::attacksTo(const Color c, const Square sq, const Bitboard& occupied) const {
	const Color opposite = oppositeColor(c);
	return ((attacksFrom<Pawn  >(opposite, sq) & bbOf(Pawn))
			| (attacksFrom<Lance >(opposite, sq, occupied) & bbOf(Lance))
			| (attacksFrom<Knight>(opposite, sq) & bbOf(Knight))
			| (attacksFrom<Silver>(opposite, sq) & bbOf(Silver))
			| (attacksFrom<Gold  >(opposite, sq) & goldsBB())
			| (attacksFrom<Bishop>(sq, occupied) & bbOf(Bishop, Horse))
			| (attacksFrom<Rook  >(sq, occupied) & bbOf(Rook  , Dragon))
			| (attacksFrom<King  >(sq)           & (bbOf(King, Horse, Dragon))))
		& bbOf(c);
}

// 玉以外で sq へ移動可能な c 側の駒の Bitboard を返す。
Bitboard Position::attacksToExceptKing(const Color c, const Square sq) const {
	const Color opposite = oppositeColor(c);
	return ((attacksFrom<Pawn  >(opposite, sq) & bbOf(Pawn))
			| (attacksFrom<Lance >(opposite, sq) & bbOf(Lance))
			| (attacksFrom<Knight>(opposite, sq) & bbOf(Knight))
			| (attacksFrom<Silver>(opposite, sq) & bbOf(Silver))
			| (attacksFrom<Gold  >(opposite, sq) & goldsBB())
			| (attacksFrom<Bishop>(sq)           & bbOf(Bishop, Horse))
			| (attacksFrom<Rook  >(sq)           & bbOf(Rook  , Dragon))
			| (attacksFrom<King  >(sq)           & bbOf(Horse, Dragon)))
		& bbOf(c);
}

Score Position::computeMaterial() const {
	Score s;
	int num;
	num  = bbOf(Pawn     , Black).popCount() + hand(Black).numOf<HPawn  >();
	num -= bbOf(Pawn     , White).popCount() + hand(White).numOf<HPawn  >();
	s  = num * PawnScore;
	num  = bbOf(Lance    , Black).popCount() + hand(Black).numOf<HLance >();
	num -= bbOf(Lance    , White).popCount() + hand(White).numOf<HLance >();
	s += num * LanceScore;
	num  = bbOf(Knight   , Black).popCount() + hand(Black).numOf<HKnight>();
	num -= bbOf(Knight   , White).popCount() + hand(White).numOf<HKnight>();
	s += num * KnightScore;
	num  = bbOf(Silver   , Black).popCount() + hand(Black).numOf<HSilver>();
	num -= bbOf(Silver   , White).popCount() + hand(White).numOf<HSilver>();
	s += num * SilverScore;
	num  = bbOf(Bishop   , Black).popCount() + hand(Black).numOf<HBishop>();
	num -= bbOf(Bishop   , White).popCount() + hand(White).numOf<HBishop>();
	s += num * BishopScore;
	num  = bbOf(Rook     , Black).popCount() + hand(Black).numOf<HRook  >();
	num -= bbOf(Rook     , White).popCount() + hand(White).numOf<HRook  >();
	s += num * RookScore;
	num  = bbOf(Gold     , Black).popCount() + hand(Black).numOf<HGold  >();
	num -= bbOf(Gold     , White).popCount() + hand(White).numOf<HGold  >();
	s += num * GoldScore;
	num  = bbOf(ProPawn  , Black).popCount();
	num -= bbOf(ProPawn  , White).popCount();
	s += num * ProPawnScore;
	num  = bbOf(ProLance , Black).popCount();
	num -= bbOf(ProLance , White).popCount();
	s += num * ProLanceScore;
	num  = bbOf(ProKnight, Black).popCount();
	num -= bbOf(ProKnight, White).popCount();
	s += num * ProKnightScore;
	num  = bbOf(ProSilver, Black).popCount();
	num -= bbOf(ProSilver, White).popCount();
	s += num * ProSilverScore;
	num  = bbOf(Horse    , Black).popCount();
	num -= bbOf(Horse    , White).popCount();
	s += num * HorseScore;
	num  = bbOf(Dragon   , Black).popCount();
	num -= bbOf(Dragon   , White).popCount();
	s += num * DragonScore;
	return s;
}
