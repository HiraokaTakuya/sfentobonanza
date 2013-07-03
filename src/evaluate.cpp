#include "evaluate.hpp"
#include "position.hpp"
//#include "search.hpp"
//#include "thread.hpp"

#if 0
s16 KPP[SquareNum][Apery::fe_end][Apery::fe_end];
s16 KKP[SquareNum][SquareNum][Apery::kkp_end];

namespace {
	EvaluateHashTable g_evalTable;

	// kpp用で使うための駒リストを列挙する関数。
	// 駒リストを列挙したついでにKKPを評価する。
	// 52は 王を除く盤上の駒の最大数 38 + 手駒の種類7×2 = 52
	// list0 は先手から見た駒位置, list1は後手の駒から見た(Invマクロを通した)駒位置
	int make_list(const Position& pos, Score& score, int list0[52], int list1[52]) {
		int nlist = 14;
		Square sq;
		Score tmpScore;
		const Square sq_bk0 = pos.kingSquare(Black);
		const Square sq_wk0 = pos.kingSquare(White);
		Square sq_bk1 = inverse(sq_wk0);
		Square sq_wk1 = inverse(sq_bk0);
		Bitboard bb;
		const Hand handB = pos.hand(Black);
		const Hand handW = pos.hand(White);

		tmpScore = static_cast<Score>(0);

		// 先手の手駒の歩の枚数と、
		// 先手玉(sq_bk0 = square black king = 先手の玉の盤上の座標)と
		// 後手玉(sq_wk0 = square white king = 後手の玉の盤上の座標)のKKPを評価
		tmpScore += kkp(sq_bk0, sq_wk0, Apery::kkp_hand_pawn   + handB.numOf<HPawn  >());
		tmpScore += kkp(sq_bk0, sq_wk0, Apery::kkp_hand_lance  + handB.numOf<HLance >());
		tmpScore += kkp(sq_bk0, sq_wk0, Apery::kkp_hand_knight + handB.numOf<HKnight>());
		tmpScore += kkp(sq_bk0, sq_wk0, Apery::kkp_hand_silver + handB.numOf<HSilver>());
		tmpScore += kkp(sq_bk0, sq_wk0, Apery::kkp_hand_gold   + handB.numOf<HGold  >());
		tmpScore += kkp(sq_bk0, sq_wk0, Apery::kkp_hand_bishop + handB.numOf<HBishop>());
		tmpScore += kkp(sq_bk0, sq_wk0, Apery::kkp_hand_rook   + handB.numOf<HRook  >());

		// 同様に後手の手駒のKKPを評価
		tmpScore -= kkp(sq_bk1, sq_wk1, Apery::kkp_hand_pawn   + handW.numOf<HPawn  >());
		tmpScore -= kkp(sq_bk1, sq_wk1, Apery::kkp_hand_lance  + handW.numOf<HLance >());
		tmpScore -= kkp(sq_bk1, sq_wk1, Apery::kkp_hand_knight + handW.numOf<HKnight>());
		tmpScore -= kkp(sq_bk1, sq_wk1, Apery::kkp_hand_silver + handW.numOf<HSilver>());
		tmpScore -= kkp(sq_bk1, sq_wk1, Apery::kkp_hand_gold   + handW.numOf<HGold  >());
		tmpScore -= kkp(sq_bk1, sq_wk1, Apery::kkp_hand_bishop + handW.numOf<HBishop>());
		tmpScore -= kkp(sq_bk1, sq_wk1, Apery::kkp_hand_rook   + handW.numOf<HRook  >());

		bb = pos.bbOf(Pawn, Black);
		FOREACH_BB(bb, sq, {
				list0[nlist] = Apery::f_pawn + sq;
				list1[nlist] = Apery::e_pawn + inverse(sq);
				tmpScore += kkp(sq_bk0, sq_wk0, Apery::kkp_pawn + sq);
				nlist    += 1;
			});

		bb = pos.bbOf(Pawn, White);
		FOREACH_BB(bb, sq, {
				list0[nlist] = Apery::e_pawn + sq;
				list1[nlist] = Apery::f_pawn + inverse(sq);
				tmpScore -= kkp(sq_bk1, sq_wk1, Apery::kkp_pawn + inverse(sq));
				nlist    += 1;
			});

		bb = pos.bbOf(Lance, Black);
		FOREACH_BB(bb, sq, {
				list0[nlist] = Apery::f_lance + sq;
				list1[nlist] = Apery::e_lance + inverse(sq);
				tmpScore += kkp(sq_bk0, sq_wk0, Apery::kkp_lance + sq);
				nlist    += 1;
			});

		bb = pos.bbOf(Lance, White);
		FOREACH_BB(bb, sq, {
				list0[nlist] = Apery::e_lance + sq;
				list1[nlist] = Apery::f_lance + inverse(sq);
				tmpScore -= kkp(sq_bk1, sq_wk1, Apery::kkp_lance + inverse(sq));
				nlist    += 1;
			});

		bb = pos.bbOf(Knight, Black);
		FOREACH_BB(bb, sq, {
				list0[nlist] = Apery::f_knight + sq;
				list1[nlist] = Apery::e_knight + inverse(sq);
				tmpScore += kkp(sq_bk0, sq_wk0, Apery::kkp_knight + sq);
				nlist    += 1;
			});

		bb = pos.bbOf(Knight, White);
		FOREACH_BB(bb, sq, {
				list0[nlist] = Apery::e_knight + sq;
				list1[nlist] = Apery::f_knight + inverse(sq);
				tmpScore -= kkp(sq_bk1, sq_wk1, Apery::kkp_knight + inverse(sq));
				nlist    += 1;
			});

		bb = pos.bbOf(Silver, Black);
		FOREACH_BB(bb, sq, {
				list0[nlist] = Apery::f_silver + sq;
				list1[nlist] = Apery::e_silver + inverse(sq);
				tmpScore += kkp(sq_bk0, sq_wk0, Apery::kkp_silver + sq);
				nlist    += 1;
			});

		bb = pos.bbOf(Silver, White);
		FOREACH_BB(bb, sq, {
				list0[nlist] = Apery::e_silver + sq;
				list1[nlist] = Apery::f_silver + inverse(sq);
				tmpScore -= kkp(sq_bk1, sq_wk1, Apery::kkp_silver + inverse(sq));
				nlist    += 1;
			});

		const Bitboard goldsBB = pos.goldsBB();
		bb = goldsBB & pos.bbOf(Black);
		FOREACH_BB(bb, sq, {
				list0[nlist] = Apery::f_gold + sq;
				list1[nlist] = Apery::e_gold + inverse(sq);
				tmpScore += kkp(sq_bk0, sq_wk0, Apery::kkp_gold + sq);
				nlist    += 1;
			});

		bb = goldsBB & pos.bbOf(White);
		FOREACH_BB(bb, sq, {
				list0[nlist] = Apery::e_gold + sq;
				list1[nlist] = Apery::f_gold + inverse(sq);
				tmpScore -= kkp(sq_bk1, sq_wk1, Apery::kkp_gold + inverse(sq));
				nlist    += 1;
			});

		bb = pos.bbOf(Bishop, Black);
		FOREACH_BB(bb, sq, {
				list0[nlist] = Apery::f_bishop + sq;
				list1[nlist] = Apery::e_bishop + inverse(sq);
				tmpScore += kkp(sq_bk0, sq_wk0, Apery::kkp_bishop + sq);
				nlist    += 1;
			});

		bb = pos.bbOf(Bishop, White);
		FOREACH_BB(bb, sq, {
				list0[nlist] = Apery::e_bishop + sq;
				list1[nlist] = Apery::f_bishop + inverse(sq);
				tmpScore -= kkp(sq_bk1, sq_wk1, Apery::kkp_bishop + inverse(sq));
				nlist    += 1;
			});

		bb = pos.bbOf(Horse, Black);
		FOREACH_BB(bb, sq, {
				list0[nlist] = Apery::f_horse + sq;
				list1[nlist] = Apery::e_horse + inverse(sq);
				tmpScore += kkp(sq_bk0, sq_wk0, Apery::kkp_horse + sq);
				nlist    += 1;
			});

		bb = pos.bbOf(Horse, White);
		FOREACH_BB(bb, sq, {
				list0[nlist] = Apery::e_horse + sq;
				list1[nlist] = Apery::f_horse + inverse(sq);
				tmpScore -= kkp(sq_bk1, sq_wk1, Apery::kkp_horse + inverse(sq));
				nlist    += 1;
			});

		bb = pos.bbOf(Rook, Black);
		FOREACH_BB(bb, sq, {
				list0[nlist] = Apery::f_rook + sq;
				list1[nlist] = Apery::e_rook + inverse(sq);
				tmpScore += kkp(sq_bk0, sq_wk0, Apery::kkp_rook + sq);
				nlist    += 1;
			});

		bb = pos.bbOf(Rook, White);
		FOREACH_BB(bb, sq, {
				list0[nlist] = Apery::e_rook + sq;
				list1[nlist] = Apery::f_rook + inverse(sq);
				tmpScore -= kkp(sq_bk1, sq_wk1, Apery::kkp_rook + inverse(sq));
				nlist    += 1;
			});

		bb = pos.bbOf(Dragon, Black);
		FOREACH_BB(bb, sq, {
				list0[nlist] = Apery::f_dragon + sq;
				list1[nlist] = Apery::e_dragon + inverse(sq);
				tmpScore += kkp(sq_bk0, sq_wk0, Apery::kkp_dragon + sq);
				nlist    += 1;
			});

		bb = pos.bbOf(Dragon, White);
		FOREACH_BB(bb, sq, {
				list0[nlist] = Apery::e_dragon + sq;
				list1[nlist] = Apery::f_dragon + inverse(sq);
				tmpScore -= kkp(sq_bk1, sq_wk1, Apery::kkp_dragon + inverse(sq));
				nlist    += 1;
			});

		score = tmpScore;
		return nlist;
	}
}

Score evaluateUnUseDiff(const Position& pos) {
	int list0[52];
	int list1[52];
	Score score;

	const Hand handB = pos.hand(Black);
	const Hand handW = pos.hand(White);

	list0[ 0] = Apery::f_hand_pawn   + handB.numOf<HPawn  >();
	list0[ 1] = Apery::e_hand_pawn   + handW.numOf<HPawn  >();
	list0[ 2] = Apery::f_hand_lance  + handB.numOf<HLance >();
	list0[ 3] = Apery::e_hand_lance  + handW.numOf<HLance >();
	list0[ 4] = Apery::f_hand_knight + handB.numOf<HKnight>();
	list0[ 5] = Apery::e_hand_knight + handW.numOf<HKnight>();
	list0[ 6] = Apery::f_hand_silver + handB.numOf<HSilver>();
	list0[ 7] = Apery::e_hand_silver + handW.numOf<HSilver>();
	list0[ 8] = Apery::f_hand_gold   + handB.numOf<HGold  >();
	list0[ 9] = Apery::e_hand_gold   + handW.numOf<HGold  >();
	list0[10] = Apery::f_hand_bishop + handB.numOf<HBishop>();
	list0[11] = Apery::e_hand_bishop + handW.numOf<HBishop>();
	list0[12] = Apery::f_hand_rook   + handB.numOf<HRook  >();
	list0[13] = Apery::e_hand_rook   + handW.numOf<HRook  >();

	list1[ 0] = Apery::f_hand_pawn   + handW.numOf<HPawn  >();
	list1[ 1] = Apery::e_hand_pawn   + handB.numOf<HPawn  >();
	list1[ 2] = Apery::f_hand_lance  + handW.numOf<HLance >();
	list1[ 3] = Apery::e_hand_lance  + handB.numOf<HLance >();
	list1[ 4] = Apery::f_hand_knight + handW.numOf<HKnight>();
	list1[ 5] = Apery::e_hand_knight + handB.numOf<HKnight>();
	list1[ 6] = Apery::f_hand_silver + handW.numOf<HSilver>();
	list1[ 7] = Apery::e_hand_silver + handB.numOf<HSilver>();
	list1[ 8] = Apery::f_hand_gold   + handW.numOf<HGold  >();
	list1[ 9] = Apery::e_hand_gold   + handB.numOf<HGold  >();
	list1[10] = Apery::f_hand_bishop + handW.numOf<HBishop>();
	list1[11] = Apery::e_hand_bishop + handB.numOf<HBishop>();
	list1[12] = Apery::f_hand_rook   + handW.numOf<HRook  >();
	list1[13] = Apery::e_hand_rook   + handB.numOf<HRook  >();

	const int nlist = make_list(pos, score, list0, list1);
	const Square sq_bk = pos.kingSquare(Black);
	const Square sq_wk = inverse(pos.kingSquare(White));

	for(int i = 0; i < nlist; ++i) {
		const int k0 = list0[i];
		const int k1 = list1[i];
		for(int j = 0; j <= i; ++j) {
			const int l0 = list0[j];
			const int l1 = list1[j];
			score += kpp(sq_bk, k0, l0);
			score -= kpp(sq_wk, k1, l1);
		}
	}

	score += pos.material() * Apery::FVScale;

	//score += inaniwaScore(pos);

	if(pos.turn() == White) {
		score = -score;
	}

	return score;
}
#endif
