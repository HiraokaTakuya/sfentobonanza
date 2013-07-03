#include "common.hpp"
#include "init.hpp"
#include "mt64bit.hpp"
#include "evaluate.hpp"
//#include "book.hpp"
//#include "search.hpp"

namespace {
	namespace Bonanza {
		// pc_on_sqで使うためのテーブル
		// 評価関数用のテーブルの開始,終了オフセット。
		// f_XXX : first , e_XXX : end
		// マジックナンバーをそのまま書くべきではない。
		// 持ち駒の部分は、最大持ち駒数＋１（ 持ち駒無しの場合があるので ）
		enum { f_hand_pawn   =    0, // 0
			   e_hand_pawn   =   19, // ↑ + 18 + 1
			   f_hand_lance  =   38, // ↑ + 18 + 1
			   e_hand_lance  =   43, // ↑ +  4 + 1
			   f_hand_knight =   48, // ↑ +  4 + 1
			   e_hand_knight =   53, // ↑ +  4 + 1
			   f_hand_silver =   58, // ↑ +  4 + 1
			   e_hand_silver =   63, // ↑ +  4 + 1
			   f_hand_gold   =   68, // ↑ +  4 + 1
			   e_hand_gold   =   73, // ↑ +  4 + 1
			   f_hand_bishop =   78, // ↑ +  4 + 1
			   e_hand_bishop =   81, // ↑ +  2 + 1
			   f_hand_rook   =   84, // ↑ +  2 + 1
			   e_hand_rook   =   87, // ↑ +  2 + 1
			   fe_hand_end   =   90, // ↑ +  2 + 1 , 手駒の終端

			   f_pawn        =   81, // = ↑ - 9 (歩は1段目には存在しないのでそれを除外してある)
			   e_pawn        =  162, // = ↑+9*9 (ここ、8*9までしか使用していない)
			   f_lance       =  225, // = ↑+7*9 (香も1段目には存在しないのでそれを除外してある)
			   e_lance       =  306, // = ↑+9*9 (ここ、8*9までしか使用していない)
			   f_knight      =  360, // = ↑+9*6 (桂は、1,2段目に存在しないのでそれを除外)
			   e_knight      =  441, // = ↑+9*9 (ここも、7*9までしか使用していない)
			   f_silver      =  504, // = ↑+9*6
			   e_silver      =  585, // = ↑+9*9
			   f_gold        =  666, // = ↑+9*9
			   e_gold        =  747, // 以下、+9*9ずつ増える。
			   f_bishop      =  828,
			   e_bishop      =  909,
			   f_horse       =  990,
			   e_horse       = 1071,
			   f_rook        = 1152,
			   e_rook        = 1233,
			   f_dragon      = 1314,
			   e_dragon      = 1395,
			   fe_end        = 1476,

			   // 持ち駒の部分は、最大持ち駒数＋１（ 持ち駒無しの場合があるので ）
			   kkp_hand_pawn   =   0,
			   kkp_hand_lance  =  19,
			   kkp_hand_knight =  24,
			   kkp_hand_silver =  29,
			   kkp_hand_gold   =  34,
			   kkp_hand_bishop =  39,
			   kkp_hand_rook   =  42,
			   kkp_hand_end    =  45,
			   // 盤上の駒の部分は、先手の歩香は１段目には存在し得ない、
			   // 先手の桂は１段目、２段目には存在しない
			   // ので、歩、香、桂馬の部分は + 9*9 とはなっていない
			   kkp_pawn        =  36,
			   kkp_lance       = 108,
			   kkp_knight      = 171,
			   kkp_silver      = 252,
			   kkp_gold        = 333,
			   kkp_bishop      = 414,
			   kkp_horse       = 495,
			   kkp_rook        = 576,
			   kkp_dragon      = 657,
			   kkp_end         = 738 };

		const int pos_n  = fe_end * ( fe_end + 1 ) / 2;

		enum Square {
			A9 = 0,
			A8 = 9,
			A7 = 18,
			I3 = 62,
			I2 = 71,
			I1 = 80,
			// 他は省略
			SquareNum = 81
		};
		OverloadEnumOperators(Square);
	}

	// Square のそれぞれの位置の enum の値を、
	// Apery 方式と Bonanza 方式で変換。
	// ちなみに Apery は ponanza 方式を採用している。
	Bonanza::Square squareAperyToBonanza(const Square sq) {
		return static_cast<Bonanza::Square>(8 - makeFile(sq)) + static_cast<Bonanza::Square>(makeRank(sq) * 9);
	}

	Square squareBonanzaToApery(const Bonanza::Square sqBona) {
		const Rank bonaRank = static_cast<Rank>(sqBona / 9);
		const File bonaFile = static_cast<File>(8 - (sqBona % 9));

		return makeSquare(bonaFile, bonaRank);
	}

	// square のマスにおける、障害物を調べる必要がある場所を調べて Bitboard で返す。
	Bitboard rookBlockMaskOneByOne(const Square square) {
		const Rank rank = makeRank(square);
		const File file = makeFile(square);

		Bitboard result = allZeroBB();

		for(Rank r = rank + 1; r < Rank1; ++r) {
			const Square sq = makeSquare(file, r);
			result.setBit(sq);
		}
		for(Rank r = rank - 1; Rank9 < r; --r) {
			const Square sq = makeSquare(file, r);
			result.setBit(sq);
		}

		for(File f = file + 1; f < FileA; ++f) {
			const Square sq = makeSquare(f, rank);
			result.setBit(sq);
		}
		for(File f = file - 1; FileI < f; --f) {
			const Square sq = makeSquare(f, rank);
			result.setBit(sq);
		}

		return result;
	}

	// square のマスにおける、障害物を調べる必要がある場所を調べて Bitboard で返す。
	Bitboard bishopBlockMaskOneByOne(const Square square) {
		const Rank rank = makeRank(square);
		const File file = makeFile(square);

		Bitboard result = allZeroBB();

		{
			File f = file + 1;
			for(Rank r = rank + 1; r < Rank1 && f < FileA; ++r, ++f) {
				const Square sq = makeSquare(f, r);
				result.setBit(sq);
			}
		}
		{
			File f = file - 1;
			for(Rank r = rank + 1; r < Rank1 && FileI < f; ++r, --f) {
				const Square sq = makeSquare(f, r);
				result.setBit(sq);
			}
		}
		{
			File f = file + 1;
			for(Rank r = rank - 1; Rank9 < r && f < FileA; --r, ++f) {
				const Square sq = makeSquare(f, r);
				result.setBit(sq);
			}
		}
		{
			File f = file - 1;
			for(Rank r = rank - 1; Rank9 < r && FileI < f; --r, --f) {
				const Square sq = makeSquare(f, r);
				result.setBit(sq);
			}
		}

		return result;
	}

	// square のマスにおける、障害物を調べる必要がある場所を調べて Bitboard で返す。
	// lance の前方だけを調べれば良さそうだけど、Rank8 ~ Rank2 の状態をそのまま index に使いたいので、
	// 縦方向全て(端を除く)の occupied を全て調べる。
	Bitboard lanceBlockMaskOneByOne(const Square square) {
		const File file = makeFile(square);

		Bitboard result = allZeroBB();

		for(Rank r = Rank8; r < Rank1; ++r) {
			const Square sq = makeSquare(file, r);
			result.setBit(sq);
		}

		return result;
	}

	// rook の利きの範囲を調べて bitboard で返す。端まであり。
	// occupied  障害物があるマスが 1 の bitboard
	Bitboard rookAttackOneByOne(const Square pos, const Bitboard& occupied) {
		const Rank rank = makeRank(pos);
		const File file = makeFile(pos);

		Bitboard result = allZeroBB();

		for(Rank r = rank + 1; r <= 8; ++r) {
			const Square sq = makeSquare(file, r);
			result.setBit(sq);
			if(occupied.isSet(sq)) {
				break;
			}
		}
		for(Rank r = rank - 1; 0 <= r; --r) {
			const Square sq = makeSquare(file, r);
			result.setBit(sq);
			if(occupied.isSet(sq)) {
				break;
			}
		}

		for(File f = file + 1; f <= 8; ++f) {
			const Square sq = makeSquare(f, rank);
			result.setBit(sq);
			if(occupied.isSet(sq)) {
				break;
			}
		}
		for(File f = file - 1; 0 <= f; --f) {
			const Square sq = makeSquare(f, rank);
			result.setBit(sq);
			if(occupied.isSet(sq)) {
				break;
			}
		}

		return result;
	}

	// bishop の利きの範囲を調べて bitboard で返す。端まであり。
	// occupied  障害物があるマスが 1 の bitboard
	Bitboard bishopAttackOneByOne(const Square square, const Bitboard& occupied) {
		const Rank rank = makeRank(square);
		const File file = makeFile(square);

		Bitboard result = allZeroBB();

		{
			File f = file + 1;
			for(Rank r = rank + 1; r <= Rank1 && f <= FileA; ++r, ++f) {
				const Square sq = makeSquare(f, r);
				result.setBit(sq);
				if(occupied.isSet(sq)) {
					break;
				}
			}
		}
		{
			File f = file - 1;
			for(Rank r = rank + 1; r <= Rank1 && FileI <= f; ++r, --f) {
				const Square sq = makeSquare(f, r);
				result.setBit(sq);
				if(occupied.isSet(sq)) {
					break;
				}
			}
		}
		{
			File f = file + 1;
			for(Rank r = rank - 1; Rank9 <= r && f <= FileA; --r, ++f) {
				const Square sq = makeSquare(f, r);
				result.setBit(sq);
				if(occupied.isSet(sq)) {
					break;
				}
			}
		}
		{
			File f = file - 1;
			for(Rank r = rank - 1; Rank9 <= r && FileI <= f; --r, --f) {
				const Square sq = makeSquare(f, r);
				result.setBit(sq);
				if(occupied.isSet(sq)) {
					break;
				}
			}
		}

		return result;
	}

	// lance の利きの範囲を調べて bitboard で返す。端まであり。
	// occupied  障害物があるマスが 1 の bitboard
	Bitboard lanceAttackOneByOne(const Color c, const Square square, const Bitboard& occupied) {
		const Rank rank = makeRank(square);
		const File file = makeFile(square);

		Bitboard result = allZeroBB();

		if(c == Black) {
			for(Rank r = rank - 1; Rank9 <= r; --r) {
				const Square sq = makeSquare(file, r);
				result.setBit(sq);
				if(occupied.isSet(sq)) {
					break;
				}
			}
		}
		else {
			for(Rank r = rank + 1; r <= Rank1; ++r) {
				const Square sq = makeSquare(file, r);
				result.setBit(sq);
				if(occupied.isSet(sq)) {
					break;
				}
			}
		}

		return result;
	}

	// index, bits の情報を元にして、occupied の 1 のbit を いくつか 0 にする。
	// index の値を, occupied の 1のbit の位置に変換する。
	// index   (0, 1<<bits] の範囲のindex
	// bits    bit size
	// blockMask   利きのあるマスが 1 のbitboard
	// result  occupied
	Bitboard indexToOccupied(const int index, const int bits, const Bitboard& blockMask) {
		Bitboard tmpBlockMask = blockMask;
		Bitboard result = allZeroBB();
		for(int i = 0; i < bits; ++i) {
			const Square sq = tmpBlockMask.firstOneFromI9();
			if(index & (1 << i)) {
				result.setBit(sq);
			}
		}
		return result;
	}

	void initAttacks(Bitboard attacks[], int attackIndex[], Bitboard blockMask[],
					 const int shift[], const u64 magic[], const bool isBishop)
	{
		int index = 0;
		for(Square sq = I9; sq < SquareNum; ++sq) {
			blockMask[sq] = (isBishop ? bishopBlockMaskOneByOne(sq) : rookBlockMaskOneByOne(sq));
			attackIndex[sq] = index;

			Bitboard occupied[1 << 14];
			const int num1s = (isBishop ? BishopBlockBits[sq] : RookBlockBits[sq]);
			for(int i = 0; i < (1 << num1s); ++i) {
				occupied[i] = indexToOccupied(i, num1s, blockMask[sq]);
				attacks[index + occupiedToIndex(occupied[i], magic[sq], shift[sq])] =
					(isBishop ? bishopAttackOneByOne(sq, occupied[i]) : rookAttackOneByOne(sq, occupied[i]));
			}
			index += 1 << (64 - shift[sq]);
		}
	}

	// LanceBlockMask, LanceAttack の値を設定する。
	void initLanceAttacks() {
		for(Color c = Black; c < ColorNum; ++c) {
			for(Square sq = I9; sq < SquareNum; ++sq) {
				const Bitboard lanceBlockMask = lanceBlockMaskOneByOne(sq);
				const int num1s = lanceBlockMask.popCount();
				for(int i = 0; i < (1 << num1s); ++i) {
					Bitboard occupied = indexToOccupied(i, num1s, lanceBlockMask);
					LanceAttack[c][sq][i] = lanceAttackOneByOne(c, sq, occupied);
				}
			}
		}
	}

	void setStepAttacks(Bitboard& bb, const Square from, const SquareDelta delta) {
		const Square to = from + delta;
		// 近接駒の Rank の変化は桂馬の 2 が最大。
		// 2 よりも変化をしているとき、Bitboard の上下の境界を飛び越えている。
		if(isInSquare(to) && abs(makeRank(to) - makeRank(from)) <= 2) {
			bb.setBit(to);
		}
	}

	void initKingAttacks() {
		for(Square sq = I9; sq < SquareNum; ++sq) {
			KingAttack[sq] = allZeroBB();
			setStepAttacks(KingAttack[sq], sq, DeltaN);
			setStepAttacks(KingAttack[sq], sq, DeltaNE);
			setStepAttacks(KingAttack[sq], sq, DeltaNW);
			setStepAttacks(KingAttack[sq], sq, DeltaE);
			setStepAttacks(KingAttack[sq], sq, DeltaW);
			setStepAttacks(KingAttack[sq], sq, DeltaS);
			setStepAttacks(KingAttack[sq], sq, DeltaSE);
			setStepAttacks(KingAttack[sq], sq, DeltaSW);
		}
	}

	void initGoldAttacks() {
		// 先手用の利き
		for(Square sq = I9; sq < SquareNum; ++sq) {
			GoldAttack[Black][sq] = allZeroBB();
			setStepAttacks(GoldAttack[Black][sq], sq, DeltaN);
			setStepAttacks(GoldAttack[Black][sq], sq, DeltaNE);
			setStepAttacks(GoldAttack[Black][sq], sq, DeltaNW);
			setStepAttacks(GoldAttack[Black][sq], sq, DeltaE);
			setStepAttacks(GoldAttack[Black][sq], sq, DeltaW);
			setStepAttacks(GoldAttack[Black][sq], sq, DeltaS);
		}
		// 後手用の利き
		for(Square sq = I9; sq < SquareNum; ++sq) {
			GoldAttack[White][sq] = allZeroBB();
			setStepAttacks(GoldAttack[White][sq], sq, DeltaS);
			setStepAttacks(GoldAttack[White][sq], sq, DeltaSE);
			setStepAttacks(GoldAttack[White][sq], sq, DeltaSW);
			setStepAttacks(GoldAttack[White][sq], sq, DeltaE);
			setStepAttacks(GoldAttack[White][sq], sq, DeltaW);
			setStepAttacks(GoldAttack[White][sq], sq, DeltaN);
		}
	}

	void initSilverAttacks() {
		// 先手用の利き
		for(Square sq = I9; sq < SquareNum; ++sq) {
			SilverAttack[Black][sq] = allZeroBB();
			setStepAttacks(SilverAttack[Black][sq], sq, DeltaN);
			setStepAttacks(SilverAttack[Black][sq], sq, DeltaNE);
			setStepAttacks(SilverAttack[Black][sq], sq, DeltaNW);
			setStepAttacks(SilverAttack[Black][sq], sq, DeltaSE);
			setStepAttacks(SilverAttack[Black][sq], sq, DeltaSW);
		}
		// 後手用の利き
		for(Square sq = I9; sq < SquareNum; ++sq) {
			SilverAttack[White][sq] = allZeroBB();
			setStepAttacks(SilverAttack[White][sq], sq, DeltaS);
			setStepAttacks(SilverAttack[White][sq], sq, DeltaSE);
			setStepAttacks(SilverAttack[White][sq], sq, DeltaSW);
			setStepAttacks(SilverAttack[White][sq], sq, DeltaNE);
			setStepAttacks(SilverAttack[White][sq], sq, DeltaNW);
		}
	}

	void initKnightAttacks() {
		// 先手用の利き
		for(Square sq = I9; sq < SquareNum; ++sq) {
			KnightAttack[Black][sq] = allZeroBB();
			setStepAttacks(KnightAttack[Black][sq], sq, DeltaN + DeltaNE);
			setStepAttacks(KnightAttack[Black][sq], sq, DeltaN + DeltaNW);
		}
		// 後手用の利き
		for(Square sq = I9; sq < SquareNum; ++sq) {
			KnightAttack[White][sq] = allZeroBB();
			setStepAttacks(KnightAttack[White][sq], sq, DeltaS + DeltaSE);
			setStepAttacks(KnightAttack[White][sq], sq, DeltaS + DeltaSW);
		}
	}

	void initPawnAttacks() {
		// 先手用の利き
		for(Square sq = I9; sq < SquareNum; ++sq) {
			PawnAttack[Black][sq] = allZeroBB();
			setStepAttacks(PawnAttack[Black][sq], sq, DeltaN);
		}
		// 後手用の利き
		for(Square sq = I9; sq < SquareNum; ++sq) {
			PawnAttack[White][sq] = allZeroBB();
			setStepAttacks(PawnAttack[White][sq], sq, DeltaS);
		}
	}

	void initBetweenBB() {
		for(Square sq1 = I9; sq1 < SquareNum; ++sq1) {
			for(Square sq2 = I9; sq2 < SquareNum; ++sq2) {
				BetweenBB[sq1][sq2] = allZeroBB();

				const Bitboard ba = bishopAttack(sq1, allZeroBB());
				const Bitboard ra = rookAttack(sq1, allZeroBB());
				const Bitboard qa = ba | ra;
				// sq1 と sq2 が 縦、横、斜めの位置にあるか
				if(qa.isSet(sq2)) {
					const Rank r1 = makeRank(sq1);
					const Rank r2 = makeRank(sq2);
					const File f1 = makeFile(sq1);
					const File f2 = makeFile(sq2);

					const SquareDelta rDelta = (r1 < r2 ? DeltaS : (r2 < r1 ? DeltaN : DeltaNothing));
					const SquareDelta fDelta = (f1 < f2 ? DeltaW : (f2 < f1 ? DeltaE : DeltaNothing));
					const SquareDelta sqDelta = rDelta + fDelta;
					// sq1, sq2 の間(sq1, sq2 は含まない) の位置のビットを立てる
					for(Square sq = sq1 + sqDelta; sq != sq2; sq += sqDelta) {
						BetweenBB[sq1][sq2].setBit(sq);
					}
				}
			}
		}
	}

	// 障害物が無いときの利きの Bitboard
	// RookAttack, BishopAttack, LanceAttack を設定してから、この関数を呼ぶこと。
	void initAttackToEdge() {
		for(Square sq = I9; sq < SquareNum; ++sq) {
			RookAttackToEdge[sq] = rookAttack(sq, allZeroBB());
			BishopAttackToEdge[sq] = bishopAttack(sq, allZeroBB());
			LanceAttackToEdge[Black][sq] = lanceAttack(Black, sq, allZeroBB());
			LanceAttackToEdge[White][sq] = lanceAttack(White, sq, allZeroBB());
			NotQueenAttackToEdgeOrKingAttack[sq] =
				((bishopAttack(sq, allZeroBB()) | rookAttack(sq, allZeroBB())) ^ allOneBB())
				| kingAttack(sq);
		}
	}

	void initSquareRelation() {
		for(Square sq1 = I9; sq1 < SquareNum; ++sq1) {
			for(Square sq2 = I9; sq2 < SquareNum; ++sq2) {
				SquareRelation[sq1][sq2] = DirecMisc;

				if(makeFile(sq1) == makeFile(sq2) && sq1 != sq2) {
					SquareRelation[sq1][sq2] = DirecFile;
				}

				if(makeRank(sq1) == makeRank(sq2) && sq1 != sq2) {
					SquareRelation[sq1][sq2] = DirecRank;
				}

				{
					// DeltaNE
					File f = makeFile(sq1) - 1;
					for(Rank r = makeRank(sq1) - 1; Rank9 <= r && FileI <= f; --r, --f) {
						if(makeSquare(f, r) == sq2) {
							SquareRelation[sq1][sq2] = DirecDiagNESW;
						}
					}
				}
				{
					// DeltaSW
					File f = makeFile(sq1) + 1;
					for(Rank r = makeRank(sq1) + 1; r <= Rank1 && f <= FileA; ++r, ++f) {
						if(makeSquare(f, r) == sq2) {
							SquareRelation[sq1][sq2] = DirecDiagNESW;
						}
					}
				}
				{
					// DeltaNW
					File f = makeFile(sq1) + 1;
					for(Rank r = makeRank(sq1) - 1; Rank9 <= r && f <= FileA; --r, ++f) {
						if(makeSquare(f, r) == sq2) {
							SquareRelation[sq1][sq2] = DirecDiagNWSE;
						}
					}
				}
				{
					// DeltaSE
					File f = makeFile(sq1) - 1;
					for(Rank r = makeRank(sq1) + 1; r <= Rank1 && FileI <= f; ++r, --f) {
						if(makeSquare(f, r) == sq2) {
							SquareRelation[sq1][sq2] = DirecDiagNWSE;
						}
					}
				}
			}
		}
	}

	void initCheckTable() {
		for(Color c = Black; c < ColorNum; ++c) {
			const Color opp = oppositeColor(c);
			for(Square sq = I9; sq < SquareNum; ++sq) {
				GoldCheckTable[c][sq] = allZeroBB();
				Bitboard checkBB = goldAttack(opp, sq);
				while(checkBB.isNot0()) {
					const Square checkSq = checkBB.firstOneFromI9();
					GoldCheckTable[c][sq] |= goldAttack(opp, checkSq);
				}
				GoldCheckTable[c][sq] &= ~(setMaskBB(sq) | goldAttack(opp, sq));
			}
		}

		for(Color c = Black; c < ColorNum; ++c) {
			const Color opp = oppositeColor(c);
			for(Square sq = I9; sq < SquareNum; ++sq) {
				SilverCheckTable[c][sq] = allZeroBB();

				Bitboard checkBB = silverAttack(opp, sq);
				while(checkBB.isNot0()) {
					const Square checkSq = checkBB.firstOneFromI9();
					SilverCheckTable[c][sq] |= silverAttack(opp, checkSq);
				}
				const Bitboard TRank789BB = (c == Black ? inFrontMask<Black, Rank6>() : inFrontMask<White, Rank4>());
				checkBB = goldAttack(opp, sq);
				while(checkBB.isNot0()) {
					const Square checkSq = checkBB.firstOneFromI9();
					// 移動元が敵陣である位置なら、金に成って王手出来る。
					SilverCheckTable[c][sq] |= (silverAttack(opp, checkSq) & TRank789BB);
				}

				const Bitboard TRank6BB = (c == Black ? rankMask<Rank6>() : rankMask<Rank4>());
				// 移動先が3段目で、4段目に移動したときも、成ることが出来る。
				checkBB = goldAttack(opp, sq) & TRank789BB;
				while(checkBB.isNot0()) {
					const Square checkSq = checkBB.firstOneFromI9();
					SilverCheckTable[c][sq] |= (silverAttack(opp, checkSq) & TRank6BB);
				}
				SilverCheckTable[c][sq] &= ~(setMaskBB(sq) | silverAttack(opp, sq));
			}
		}

		for(Color c = Black; c < ColorNum; ++c) {
			const Color opp = oppositeColor(c);
			for(Square sq = I9; sq < SquareNum; ++sq) {
				KnightCheckTable[c][sq] = allZeroBB();

				Bitboard checkBB = knightAttack(opp, sq);
				while(checkBB.isNot0()) {
					const Square checkSq = checkBB.firstOneFromI9();
					KnightCheckTable[c][sq] |= knightAttack(opp, checkSq);
				}
				const Bitboard TRank789BB = (c == Black ? inFrontMask<Black, Rank6>() : inFrontMask<White, Rank4>());
				checkBB = goldAttack(opp, sq) & TRank789BB;
				while(checkBB.isNot0()) {
					const Square checkSq = checkBB.firstOneFromI9();
					KnightCheckTable[c][sq] |= knightAttack(opp, checkSq);
				}
			}
		}

		for(Color c = Black; c < ColorNum; ++c) {
			const Color opp = oppositeColor(c);
			for(Square sq = I9; sq < SquareNum; ++sq) {
				LanceCheckTable[c][sq] = allZeroBB();

				LanceCheckTable[c][sq] |= lanceAttackToEdge(opp, sq);

				const Bitboard TRank789BB = (c == Black ? inFrontMask<Black, Rank6>() : inFrontMask<White, Rank4>());
				Bitboard checkBB = goldAttack(opp, sq) & TRank789BB;
				while(checkBB.isNot0()) {
					const Square checkSq = checkBB.firstOneFromI9();
					LanceCheckTable[c][sq] |= lanceAttackToEdge(opp, checkSq);
				}
				LanceCheckTable[c][sq] &= ~(setMaskBB(sq) | pawnAttack(opp, sq));
			}
		}
	}

	void kppBonanzaToApery(const std::vector<s16>& kpptmp, const Bonanza::Square sqb1,
						   const Bonanza::Square sqb2, const int fe, const int feB)
	{
		const Square sq1 = squareBonanzaToApery(sqb1);
		const Square sq2 = (fe <= Apery::e_hand_rook ? static_cast<Square>(sqb2) : squareBonanzaToApery(sqb2));

#define KPPTMP(k,i,j) ((j <= i) ?										\
					   kpptmp[(k)*Bonanza::pos_n+((i)*((i)+1)/2+(j))] : kpptmp[(k)*Bonanza::pos_n+((j)*((j)+1)/2+(i))]);

		for(Bonanza::Square sqb3 = Bonanza::A9; sqb3 < Bonanza::SquareNum; ++sqb3) {
			const Square sq3 = squareBonanzaToApery(sqb3);
			// ここから持ち駒
			if(static_cast<int>(sqb3) <= 18) {
				KPP[sq1][fe + sq2][Apery::f_hand_pawn   + sqb3] = KPPTMP(sqb1, feB + sqb2, Bonanza::f_hand_pawn   + sqb3);
				KPP[sq1][fe + sq2][Apery::e_hand_pawn   + sqb3] = KPPTMP(sqb1, feB + sqb2, Bonanza::e_hand_pawn   + sqb3);
			}
			if(static_cast<int>(sqb3) <= 4) {
				KPP[sq1][fe + sq2][Apery::f_hand_lance  + sqb3] = KPPTMP(sqb1, feB + sqb2, Bonanza::f_hand_lance  + sqb3);
				KPP[sq1][fe + sq2][Apery::e_hand_lance  + sqb3] = KPPTMP(sqb1, feB + sqb2, Bonanza::e_hand_lance  + sqb3);
				KPP[sq1][fe + sq2][Apery::f_hand_knight + sqb3] = KPPTMP(sqb1, feB + sqb2, Bonanza::f_hand_knight + sqb3);
				KPP[sq1][fe + sq2][Apery::e_hand_knight + sqb3] = KPPTMP(sqb1, feB + sqb2, Bonanza::e_hand_knight + sqb3);
				KPP[sq1][fe + sq2][Apery::f_hand_silver + sqb3] = KPPTMP(sqb1, feB + sqb2, Bonanza::f_hand_silver + sqb3);
				KPP[sq1][fe + sq2][Apery::e_hand_silver + sqb3] = KPPTMP(sqb1, feB + sqb2, Bonanza::e_hand_silver + sqb3);
				KPP[sq1][fe + sq2][Apery::f_hand_gold   + sqb3] = KPPTMP(sqb1, feB + sqb2, Bonanza::f_hand_gold   + sqb3);
				KPP[sq1][fe + sq2][Apery::e_hand_gold   + sqb3] = KPPTMP(sqb1, feB + sqb2, Bonanza::e_hand_gold   + sqb3);
			}
			if(static_cast<int>(sqb3) <= 2) {
				KPP[sq1][fe + sq2][Apery::f_hand_bishop + sqb3] = KPPTMP(sqb1, feB + sqb2, Bonanza::f_hand_bishop + sqb3);
				KPP[sq1][fe + sq2][Apery::e_hand_bishop + sqb3] = KPPTMP(sqb1, feB + sqb2, Bonanza::e_hand_bishop + sqb3);
				KPP[sq1][fe + sq2][Apery::f_hand_rook   + sqb3] = KPPTMP(sqb1, feB + sqb2, Bonanza::f_hand_rook   + sqb3);
				KPP[sq1][fe + sq2][Apery::e_hand_rook   + sqb3] = KPPTMP(sqb1, feB + sqb2, Bonanza::e_hand_rook   + sqb3);
			}

			// ここから盤上の駒
			if(Bonanza::A8 <= sqb3) {
				KPP[sq1][fe + sq2][Apery::f_pawn   + sq3] = KPPTMP(sqb1, feB + sqb2, Bonanza::f_pawn   + sqb3);
				KPP[sq1][fe + sq2][Apery::f_lance  + sq3] = KPPTMP(sqb1, feB + sqb2, Bonanza::f_lance  + sqb3);
			}
			if(sqb3 <= Bonanza::I2) {
				KPP[sq1][fe + sq2][Apery::e_pawn   + sq3] = KPPTMP(sqb1, feB + sqb2, Bonanza::e_pawn   + sqb3);
				KPP[sq1][fe + sq2][Apery::e_lance  + sq3] = KPPTMP(sqb1, feB + sqb2, Bonanza::e_lance  + sqb3);
			}
			if(Bonanza::A7 <= sqb3) {
				KPP[sq1][fe + sq2][Apery::f_knight + sq3] = KPPTMP(sqb1, feB + sqb2, Bonanza::f_knight + sqb3);
			}
			if(sqb3 <= Bonanza::I3) {
				KPP[sq1][fe + sq2][Apery::e_knight + sq3] = KPPTMP(sqb1, feB + sqb2, Bonanza::e_knight + sqb3);
			}
			KPP[sq1][fe + sq2][Apery::f_silver + sq3] = KPPTMP(sqb1, feB + sqb2, Bonanza::f_silver + sqb3);
			KPP[sq1][fe + sq2][Apery::e_silver + sq3] = KPPTMP(sqb1, feB + sqb2, Bonanza::e_silver + sqb3);
			KPP[sq1][fe + sq2][Apery::f_gold   + sq3] = KPPTMP(sqb1, feB + sqb2, Bonanza::f_gold   + sqb3);
			KPP[sq1][fe + sq2][Apery::e_gold   + sq3] = KPPTMP(sqb1, feB + sqb2, Bonanza::e_gold   + sqb3);
			KPP[sq1][fe + sq2][Apery::f_bishop + sq3] = KPPTMP(sqb1, feB + sqb2, Bonanza::f_bishop + sqb3);
			KPP[sq1][fe + sq2][Apery::e_bishop + sq3] = KPPTMP(sqb1, feB + sqb2, Bonanza::e_bishop + sqb3);
			KPP[sq1][fe + sq2][Apery::f_horse  + sq3] = KPPTMP(sqb1, feB + sqb2, Bonanza::f_horse  + sqb3);
			KPP[sq1][fe + sq2][Apery::e_horse  + sq3] = KPPTMP(sqb1, feB + sqb2, Bonanza::e_horse  + sqb3);
			KPP[sq1][fe + sq2][Apery::f_rook   + sq3] = KPPTMP(sqb1, feB + sqb2, Bonanza::f_rook   + sqb3);
			KPP[sq1][fe + sq2][Apery::e_rook   + sq3] = KPPTMP(sqb1, feB + sqb2, Bonanza::e_rook   + sqb3);
			KPP[sq1][fe + sq2][Apery::f_dragon + sq3] = KPPTMP(sqb1, feB + sqb2, Bonanza::f_dragon + sqb3);
			KPP[sq1][fe + sq2][Apery::e_dragon + sq3] = KPPTMP(sqb1, feB + sqb2, Bonanza::e_dragon + sqb3);
		}
#undef KPPTMP
	}

#if 0
	void initFV() {
		std::ifstream ifs("../../bonanza_v6.0/winbin/fv.bin", std::ios::binary);

		std::vector<s16> kpptmp(Bonanza::SquareNum * Bonanza::pos_n);
		std::vector<s16> kkptmp(Bonanza::SquareNum * Bonanza::SquareNum * Bonanza::kkp_end);

		ifs.read(reinterpret_cast<char*>(&kpptmp[0]),
				 sizeof(s16) * static_cast<int>(Bonanza::SquareNum * Bonanza::pos_n));
		ifs.read(reinterpret_cast<char*>(&kkptmp[0])  ,
				 sizeof(s16) * static_cast<int>(Bonanza::SquareNum * Bonanza::SquareNum * Bonanza::kkp_end));

		for(Bonanza::Square sqb1 = Bonanza::A9; sqb1 < Bonanza::SquareNum; ++sqb1) {
			for(Bonanza::Square sqb2 = Bonanza::A9; sqb2 < Bonanza::SquareNum; ++sqb2) {
				// ここから持ち駒
				// sqb2 は持ち駒の枚数を表す。
				if(static_cast<int>(sqb2) <= 18) {
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::f_hand_pawn  , Bonanza::f_hand_pawn  );
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::e_hand_pawn  , Bonanza::e_hand_pawn  );
				}
				if(static_cast<int>(sqb2) <= 4) {
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::f_hand_lance , Bonanza::f_hand_lance );
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::e_hand_lance , Bonanza::e_hand_lance );
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::f_hand_knight, Bonanza::f_hand_knight);
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::e_hand_knight, Bonanza::e_hand_knight);
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::f_hand_silver, Bonanza::f_hand_silver);
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::e_hand_silver, Bonanza::e_hand_silver);
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::f_hand_gold  , Bonanza::f_hand_gold  );
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::e_hand_gold  , Bonanza::e_hand_gold  );
				}
				if(static_cast<int>(sqb2) <= 2) {
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::f_hand_bishop, Bonanza::f_hand_bishop);
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::e_hand_bishop, Bonanza::e_hand_bishop);
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::f_hand_rook  , Bonanza::f_hand_rook  );
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::e_hand_rook  , Bonanza::e_hand_rook  );
				}

				// ここから盤上の駒
				// sqb2 は Bonanza 方式での盤上の位置を表す
				if(Bonanza::A8 <= sqb2) {
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::f_pawn  , Bonanza::f_pawn  );
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::f_lance , Bonanza::f_lance );
				}
				if(sqb2 <= Bonanza::I2) {
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::e_pawn  , Bonanza::e_pawn  );
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::e_lance , Bonanza::e_lance );
				}
				if(Bonanza::A7 <= sqb2) {
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::f_knight, Bonanza::f_knight);
				}
				if(sqb2 <= Bonanza::I3) {
					kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::e_knight, Bonanza::e_knight);
				}
				kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::f_silver, Bonanza::f_silver);
				kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::e_silver, Bonanza::e_silver);
				kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::f_gold  , Bonanza::f_gold  );
				kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::e_gold  , Bonanza::e_gold  );
				kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::f_bishop, Bonanza::f_bishop);
				kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::e_bishop, Bonanza::e_bishop);
				kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::f_horse , Bonanza::f_horse );
				kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::e_horse , Bonanza::e_horse );
				kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::f_rook  , Bonanza::f_rook  );
				kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::e_rook  , Bonanza::e_rook  );
				kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::f_dragon, Bonanza::f_dragon);
				kppBonanzaToApery(kpptmp, sqb1, sqb2, Apery::e_dragon, Bonanza::e_dragon);
			}
		}

		// kkp の Square 変換
		for(Bonanza::Square sqb1 = Bonanza::A9; sqb1 < Bonanza::SquareNum; ++sqb1) {
			const Square sq1 = squareBonanzaToApery(sqb1);
			for(Bonanza::Square sqb2 = Bonanza::A9; sqb2 < Bonanza::SquareNum; ++sqb2) {
				const Square sq2 = squareBonanzaToApery(sqb2);
				for(Bonanza::Square sqb3 = Bonanza::A9; sqb3 < Bonanza::SquareNum; ++sqb3) {
					const Square sq3 = squareBonanzaToApery(sqb3);

					// ここから持ち駒
					// sqb3 は持ち駒の枚数を表す。
					if(static_cast<int>(sqb3) <= 18) {
						KKP[sq1][sq2][Apery::kkp_hand_pawn   + sqb3] = kkptmp[(sqb1 * Bonanza::SquareNum + sqb2) * Bonanza::kkp_end + (Bonanza::kkp_hand_pawn   + sqb3)];
					}
					if(static_cast<int>(sqb3) <= 4) {
						KKP[sq1][sq2][Apery::kkp_hand_lance  + sqb3] = kkptmp[(sqb1 * Bonanza::SquareNum + sqb2) * Bonanza::kkp_end + (Bonanza::kkp_hand_lance  + sqb3)];
						KKP[sq1][sq2][Apery::kkp_hand_knight + sqb3] = kkptmp[(sqb1 * Bonanza::SquareNum + sqb2) * Bonanza::kkp_end + (Bonanza::kkp_hand_knight + sqb3)];
						KKP[sq1][sq2][Apery::kkp_hand_silver + sqb3] = kkptmp[(sqb1 * Bonanza::SquareNum + sqb2) * Bonanza::kkp_end + (Bonanza::kkp_hand_silver + sqb3)];
						KKP[sq1][sq2][Apery::kkp_hand_gold   + sqb3] = kkptmp[(sqb1 * Bonanza::SquareNum + sqb2) * Bonanza::kkp_end + (Bonanza::kkp_hand_gold   + sqb3)];
					}
					if(static_cast<int>(sqb3) <= 2) {
						KKP[sq1][sq2][Apery::kkp_hand_bishop + sqb3] = kkptmp[(sqb1 * Bonanza::SquareNum + sqb2) * Bonanza::kkp_end + (Bonanza::kkp_hand_bishop + sqb3)];
						KKP[sq1][sq2][Apery::kkp_hand_rook   + sqb3] = kkptmp[(sqb1 * Bonanza::SquareNum + sqb2) * Bonanza::kkp_end + (Bonanza::kkp_hand_rook   + sqb3)];
					}

					// ここから盤上の駒
					// sqb3 は Bonanza 方式での盤上の位置を表す
					if(Bonanza::A8 <= sqb3) {
						KKP[sq1][sq2][Apery::kkp_pawn   + sq3] = kkptmp[(sqb1 * Bonanza::SquareNum + sqb2) * Bonanza::kkp_end + (Bonanza::kkp_pawn   + sqb3)];
						KKP[sq1][sq2][Apery::kkp_lance  + sq3] = kkptmp[(sqb1 * Bonanza::SquareNum + sqb2) * Bonanza::kkp_end + (Bonanza::kkp_lance  + sqb3)];
					}
					if(Bonanza::A7 <= sqb3) {
						KKP[sq1][sq2][Apery::kkp_knight + sq3] = kkptmp[(sqb1 * Bonanza::SquareNum + sqb2) * Bonanza::kkp_end + (Bonanza::kkp_knight + sqb3)];
					}
					KKP[sq1][sq2][Apery::kkp_silver + sq3] = kkptmp[(sqb1 * Bonanza::SquareNum + sqb2) * Bonanza::kkp_end + (Bonanza::kkp_silver + sqb3)];
					KKP[sq1][sq2][Apery::kkp_gold   + sq3] = kkptmp[(sqb1 * Bonanza::SquareNum + sqb2) * Bonanza::kkp_end + (Bonanza::kkp_gold   + sqb3)];
					KKP[sq1][sq2][Apery::kkp_bishop + sq3] = kkptmp[(sqb1 * Bonanza::SquareNum + sqb2) * Bonanza::kkp_end + (Bonanza::kkp_bishop + sqb3)];
					KKP[sq1][sq2][Apery::kkp_horse  + sq3] = kkptmp[(sqb1 * Bonanza::SquareNum + sqb2) * Bonanza::kkp_end + (Bonanza::kkp_horse  + sqb3)];
					KKP[sq1][sq2][Apery::kkp_rook   + sq3] = kkptmp[(sqb1 * Bonanza::SquareNum + sqb2) * Bonanza::kkp_end + (Bonanza::kkp_rook   + sqb3)];
					KKP[sq1][sq2][Apery::kkp_dragon + sq3] = kkptmp[(sqb1 * Bonanza::SquareNum + sqb2) * Bonanza::kkp_end + (Bonanza::kkp_dragon + sqb3)];
				}
			}
		}
	}
#endif
}
void initTable() {
	initAttacks(RookAttack, RookAttackIndex, RookBlockMask, RookShiftBits, RookMagic, false);
	initAttacks(BishopAttack, BishopAttackIndex, BishopBlockMask, BishopShiftBits, BishopMagic, true);
	initKingAttacks();
	initGoldAttacks();
	initSilverAttacks();
	initKnightAttacks();
	initPawnAttacks();
	initLanceAttacks();
	initBetweenBB();
	initAttackToEdge();
	initSquareRelation();
	initCheckTable();

//	initFV();

//	Book::init();
//	initSearchTable();
}

#if defined FIND_MAGIC
// square の位置の rook, bishop それぞれのMagic Bitboard に使用するマジックナンバーを見つける。
// isBishop  : true なら bishop, false なら rook のマジックナンバーを見つける。
u64 findMagic(const Square square, const bool isBishop) {
	Bitboard occupied[1<<14];
	Bitboard attack[1<<14];
	Bitboard attackUsed[1<<14];
	Bitboard mask = (isBishop ? bishopBlockMaskOneByOne(square) : rookBlockMaskOneByOne(square));
	int num1s = (isBishop ? BishopBlockBits[square] : RookBlockBits[square]);

	// n bit の全ての数字 (利きのあるマスの全ての 0 or 1 の組み合わせ)
	for(int i = 0; i < (1 << num1s); ++i) {
		occupied[i] = indexToOccupied(i, num1s, mask);
		attack[i] = (isBishop ?
					 bishopAttackOneByOne(square, occupied[i]) : rookAttackOneByOne(square, occupied[i]));
	}

	for(u64 k = 0; k < UINT64_C(100000000); ++k) {
		const u64 magic = g_mt64bit.randomFewBits();
		bool fail = false;

		// これは無くても良いけど、少しマジックナンバーが見つかるのが早くなるのだと思う。
		// よく分からないまま、chess programming wiki の実装例に従った。
		if(count1s((mask.merge() * magic) & UINT64_C(0xfff0000000000000)) < 6) {
			continue;
		}

		for(int i = 0; i < (1<<14); ++i) {
			attackUsed[i] = allZeroBB();
		}

		for(int i = 0; !fail && i < (1 << num1s); ++i) {
			const int shiftBits = (isBishop ? BishopShiftBits[square] : RookShiftBits[square]);
			const u64 index = occupiedToIndex(occupied[i], magic, shiftBits);
			if(attackUsed[index] == allZeroBB()) {
				attackUsed[index] = attack[i];
			}
			else if(attackUsed[index] != attack[i]) {
				fail = true;
			}
		}
		if(!fail) {
			return magic;
		}
	}

	std::cout << "/***Failed***/\t";
	return 0;
}
#endif // #if defined FIND_MAGIC
