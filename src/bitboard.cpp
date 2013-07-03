#include "common.hpp"
#include "bitboard.hpp"

const Bitboard SetMaskBB[SquareNum] = {
	Bitboard(UINT64_C(1) <<  0,                 0),  // 0 , I9
	Bitboard(UINT64_C(1) <<  1,                 0),  // 1 , I8
	Bitboard(UINT64_C(1) <<  2,                 0),  // 2 , I7
	Bitboard(UINT64_C(1) <<  3,                 0),  // 3 , I6
	Bitboard(UINT64_C(1) <<  4,                 0),  // 4 , I5
	Bitboard(UINT64_C(1) <<  5,                 0),  // 5 , I4
	Bitboard(UINT64_C(1) <<  6,                 0),  // 6 , I3
	Bitboard(UINT64_C(1) <<  7,                 0),  // 7 , I2
	Bitboard(UINT64_C(1) <<  8,                 0),  // 8 , I1
	Bitboard(UINT64_C(1) <<  9,                 0),  // 9 , H9
	Bitboard(UINT64_C(1) << 10,                 0),  // 10, H8
	Bitboard(UINT64_C(1) << 11,                 0),  // 11, H7
	Bitboard(UINT64_C(1) << 12,                 0),  // 12, H6
	Bitboard(UINT64_C(1) << 13,                 0),  // 13, H5
	Bitboard(UINT64_C(1) << 14,                 0),  // 14, H4
	Bitboard(UINT64_C(1) << 15,                 0),  // 15, H3
	Bitboard(UINT64_C(1) << 16,                 0),  // 16, H2
	Bitboard(UINT64_C(1) << 17,                 0),  // 17, H1
	Bitboard(UINT64_C(1) << 18,                 0),  // 18, G9
	Bitboard(UINT64_C(1) << 19,                 0),  // 19, G8
	Bitboard(UINT64_C(1) << 20,                 0),  // 20, G7
	Bitboard(UINT64_C(1) << 21,                 0),  // 21, G6
	Bitboard(UINT64_C(1) << 22,                 0),  // 22, G5
	Bitboard(UINT64_C(1) << 23,                 0),  // 23, G4
	Bitboard(UINT64_C(1) << 24,                 0),  // 24, G3
	Bitboard(UINT64_C(1) << 25,                 0),  // 25, G2
	Bitboard(UINT64_C(1) << 26,                 0),  // 26, G1
	Bitboard(UINT64_C(1) << 27,                 0),  // 27, F9
	Bitboard(UINT64_C(1) << 28,                 0),  // 28, F8
	Bitboard(UINT64_C(1) << 29,                 0),  // 29, F7
	Bitboard(UINT64_C(1) << 30,                 0),  // 30, F6
	Bitboard(UINT64_C(1) << 31,                 0),  // 31, F5
	Bitboard(UINT64_C(1) << 32,                 0),  // 32, F4
	Bitboard(UINT64_C(1) << 33,                 0),  // 33, F3
	Bitboard(UINT64_C(1) << 34,                 0),  // 34, F2
	Bitboard(UINT64_C(1) << 35,                 0),  // 35, F1
	Bitboard(UINT64_C(1) << 36,                 0),  // 36, E9
	Bitboard(UINT64_C(1) << 37,                 0),  // 37, E8
	Bitboard(UINT64_C(1) << 38,                 0),  // 38, E7
	Bitboard(UINT64_C(1) << 39,                 0),  // 39, E6
	Bitboard(UINT64_C(1) << 40,                 0),  // 40, E5
	Bitboard(UINT64_C(1) << 41,                 0),  // 41, E4
	Bitboard(UINT64_C(1) << 42,                 0),  // 42, E3
	Bitboard(UINT64_C(1) << 43,                 0),  // 43, E2
	Bitboard(UINT64_C(1) << 44,                 0),  // 44, E1
	Bitboard(UINT64_C(1) << 45,                 0),  // 45, D9
	Bitboard(UINT64_C(1) << 46,                 0),  // 46, D8
	Bitboard(UINT64_C(1) << 47,                 0),  // 47, D7
	Bitboard(UINT64_C(1) << 48,                 0),  // 48, D6
	Bitboard(UINT64_C(1) << 49,                 0),  // 49, D5
	Bitboard(UINT64_C(1) << 50,                 0),  // 50, D4
	Bitboard(UINT64_C(1) << 51,                 0),  // 51, D3
	Bitboard(UINT64_C(1) << 52,                 0),  // 52, D2
	Bitboard(UINT64_C(1) << 53,                 0),  // 53, D1
	Bitboard(UINT64_C(1) << 54,                 0),  // 54, C9
	Bitboard(UINT64_C(1) << 55,                 0),  // 55, C8
	Bitboard(UINT64_C(1) << 56,                 0),  // 56, C7
	Bitboard(UINT64_C(1) << 57,                 0),  // 57, C6
	Bitboard(UINT64_C(1) << 58,                 0),  // 58, C5
	Bitboard(UINT64_C(1) << 59,                 0),  // 59, C4
	Bitboard(UINT64_C(1) << 60,                 0),  // 60, C3
	Bitboard(UINT64_C(1) << 61,                 0),  // 61, C2
	Bitboard(UINT64_C(1) << 62,                 0),  // 62, C1
	Bitboard(                0, UINT64_C(1) <<  0),  // 63, B9
	Bitboard(                0, UINT64_C(1) <<  1),  // 64, B8
	Bitboard(                0, UINT64_C(1) <<  2),  // 65, B7
	Bitboard(                0, UINT64_C(1) <<  3),  // 66, B6
	Bitboard(                0, UINT64_C(1) <<  4),  // 67, B5
	Bitboard(                0, UINT64_C(1) <<  5),  // 68, B4
	Bitboard(                0, UINT64_C(1) <<  6),  // 69, B3
	Bitboard(                0, UINT64_C(1) <<  7),  // 70, B2
	Bitboard(                0, UINT64_C(1) <<  8),  // 71, B1
	Bitboard(                0, UINT64_C(1) <<  9),  // 72, A9
	Bitboard(                0, UINT64_C(1) << 10),  // 73, A8
	Bitboard(                0, UINT64_C(1) << 11),  // 74, A7
	Bitboard(                0, UINT64_C(1) << 12),  // 75, A6
	Bitboard(                0, UINT64_C(1) << 13),  // 76, A5
	Bitboard(                0, UINT64_C(1) << 14),  // 77, A4
	Bitboard(                0, UINT64_C(1) << 15),  // 78, A3
	Bitboard(                0, UINT64_C(1) << 16),  // 79, A2
	Bitboard(                0, UINT64_C(1) << 17)   // 80, A1
};

const Bitboard ClearMaskBB[SquareNum] = {
	~SetMaskBB[ 0], ~SetMaskBB[ 1], ~SetMaskBB[ 2], ~SetMaskBB[ 3],
	~SetMaskBB[ 4], ~SetMaskBB[ 5], ~SetMaskBB[ 6], ~SetMaskBB[ 7],
	~SetMaskBB[ 8], ~SetMaskBB[ 9], ~SetMaskBB[10], ~SetMaskBB[11],
	~SetMaskBB[12], ~SetMaskBB[13], ~SetMaskBB[14], ~SetMaskBB[15],
	~SetMaskBB[16], ~SetMaskBB[17], ~SetMaskBB[18], ~SetMaskBB[19],
	~SetMaskBB[20], ~SetMaskBB[21], ~SetMaskBB[22], ~SetMaskBB[23],
	~SetMaskBB[24], ~SetMaskBB[25], ~SetMaskBB[26], ~SetMaskBB[27],
	~SetMaskBB[28], ~SetMaskBB[29], ~SetMaskBB[30], ~SetMaskBB[31],
	~SetMaskBB[32], ~SetMaskBB[33], ~SetMaskBB[34], ~SetMaskBB[35],
	~SetMaskBB[36], ~SetMaskBB[37], ~SetMaskBB[38], ~SetMaskBB[39],
	~SetMaskBB[40], ~SetMaskBB[41], ~SetMaskBB[42], ~SetMaskBB[43],
	~SetMaskBB[44], ~SetMaskBB[45], ~SetMaskBB[46], ~SetMaskBB[47],
	~SetMaskBB[48], ~SetMaskBB[49], ~SetMaskBB[50], ~SetMaskBB[51],
	~SetMaskBB[52], ~SetMaskBB[53], ~SetMaskBB[54], ~SetMaskBB[55],
	~SetMaskBB[56], ~SetMaskBB[57], ~SetMaskBB[58], ~SetMaskBB[59],
	~SetMaskBB[60], ~SetMaskBB[61], ~SetMaskBB[62], ~SetMaskBB[63],
	~SetMaskBB[64], ~SetMaskBB[65], ~SetMaskBB[66], ~SetMaskBB[67],
	~SetMaskBB[68], ~SetMaskBB[69], ~SetMaskBB[70], ~SetMaskBB[71],
	~SetMaskBB[72], ~SetMaskBB[73], ~SetMaskBB[74], ~SetMaskBB[75],
	~SetMaskBB[76], ~SetMaskBB[77], ~SetMaskBB[78], ~SetMaskBB[79],
	~SetMaskBB[80]
};

// 各マスのrookが調べる必要があるマスの数
const int RookBlockBits[SquareNum] = {
	14, 13, 13, 13, 13, 13, 13, 13, 14,
	13, 12, 12, 12, 12, 12, 12, 12, 13,
	13, 12, 12, 12, 12, 12, 12, 12, 13,
	13, 12, 12, 12, 12, 12, 12, 12, 13,
	13, 12, 12, 12, 12, 12, 12, 12, 13,
	13, 12, 12, 12, 12, 12, 12, 12, 13,
	13, 12, 12, 12, 12, 12, 12, 12, 13,
	13, 12, 12, 12, 12, 12, 12, 12, 13,
	14, 13, 13, 13, 13, 13, 13, 13, 14
};

// 各マスのbishopが調べる必要があるマスの数
const int BishopBlockBits[SquareNum] = {
	7,  6,  6,  6,  6,  6,  6,  6,  7,
	6,  6,  6,  6,  6,  6,  6,  6,  6,
	6,  6,  8,  8,  8,  8,  8,  6,  6,
	6,  6,  8, 10, 10, 10,  8,  6,  6,
	6,  6,  8, 10, 12, 10,  8,  6,  6,
	6,  6,  8, 10, 10, 10,  8,  6,  6,
	6,  6,  8,  8,  8,  8,  8,  6,  6,
	6,  6,  6,  6,  6,  6,  6,  6,  6,
	7,  6,  6,  6,  6,  6,  6,  6,  7
};

// Magic Number を求める際のシフト量
// RookShiftBits[17], RookShiftBits[53] はマジックナンバーが見つからなかったため、
// シフト量を 1 つ減らす。(テーブルサイズを 2 倍にする。)
// この方法は issei_y さんに相談したところ、教えて頂いた方法。
const int RookShiftBits[SquareNum] = {
	50, 51, 51, 51, 51, 51, 51, 51, 50,
	51, 52, 52, 52, 52, 52, 52, 52, 50, // [17]: 51 -> 50
	51, 52, 52, 52, 52, 52, 52, 52, 51,
	51, 52, 52, 52, 52, 52, 52, 52, 51,
	51, 52, 52, 52, 52, 52, 52, 52, 51,
	51, 52, 52, 52, 52, 52, 52, 52, 50, // [53]: 51 -> 50
	51, 52, 52, 52, 52, 52, 52, 52, 51,
	51, 52, 52, 52, 52, 52, 52, 52, 51,
	50, 51, 51, 51, 51, 51, 51, 51, 50
};

// Magic Number を求める際のシフト量
const int BishopShiftBits[SquareNum] = {
	57, 58, 58, 58, 58, 58, 58, 58, 57,
	58, 58, 58, 58, 58, 58, 58, 58, 58,
	58, 58, 56, 56, 56, 56, 56, 58, 58,
	58, 58, 56, 54, 54, 54, 56, 58, 58,
	58, 58, 56, 54, 52, 54, 56, 58, 58,
	58, 58, 56, 54, 54, 54, 56, 58, 58,
	58, 58, 56, 56, 56, 56, 56, 58, 58,
	58, 58, 58, 58, 58, 58, 58, 58, 58,
	57, 58, 58, 58, 58, 58, 58, 58, 57
};

const u64 RookMagic[SquareNum] = {
	UINT64_C(0x140000400809300),  UINT64_C(0x1320000902000240), UINT64_C(0x8001910c008180),
	UINT64_C(0x40020004401040),   UINT64_C(0x40010000d01120),   UINT64_C(0x80048020084050),
	UINT64_C(0x40004000080228),   UINT64_C(0x400440000a2a0a),   UINT64_C(0x40003101010102),
	UINT64_C(0x80c4200012108100), UINT64_C(0x4010c00204000c01), UINT64_C(0x220400103250002),
	UINT64_C(0x2600200004001),    UINT64_C(0x40200052400020),   UINT64_C(0xc00100020020008),
	UINT64_C(0x9080201000200004), UINT64_C(0x2200201000080004), UINT64_C(0x80804c0020200191),
	UINT64_C(0x45383000009100),   UINT64_C(0x30002800020040),   UINT64_C(0x40104000988084),
	UINT64_C(0x108001000800415),  UINT64_C(0x14005000400009),   UINT64_C(0xd21001001c00045),
	UINT64_C(0xc0003000200024),   UINT64_C(0x40003000280004),   UINT64_C(0x40021000091102),
	UINT64_C(0x2008a20408000d00), UINT64_C(0x2000100084010040), UINT64_C(0x144080008008001),
	UINT64_C(0x50102400100026a2), UINT64_C(0x1040020008001010), UINT64_C(0x1200200028005010),
	UINT64_C(0x4280030030020898), UINT64_C(0x480081410011004),  UINT64_C(0x34000040800110a),
	UINT64_C(0x101000010c0021),   UINT64_C(0x9210800080082),    UINT64_C(0x6100002000400a7),
	UINT64_C(0xa2240800900800c0), UINT64_C(0x9220082001000801), UINT64_C(0x1040008001140030),
	UINT64_C(0x40002220040008),   UINT64_C(0x28000124008010c),  UINT64_C(0x40008404940002),
	UINT64_C(0x40040800010200),   UINT64_C(0x90000809002100),   UINT64_C(0x2800080001000201),
	UINT64_C(0x1400020001000201), UINT64_C(0x180081014018004),  UINT64_C(0x1100008000400201),
	UINT64_C(0x80004000200201),   UINT64_C(0x420800010000201),  UINT64_C(0x2841c00080200209),
	UINT64_C(0x120002401040001),  UINT64_C(0x14510000101000b),  UINT64_C(0x40080000808001),
	UINT64_C(0x834000188048001),  UINT64_C(0x4001210000800205), UINT64_C(0x4889a8007400201),
	UINT64_C(0x2080044080200062), UINT64_C(0x80004002861002),   UINT64_C(0xc00842049024),
	UINT64_C(0x8040000202020011), UINT64_C(0x400404002c0100),   UINT64_C(0x2080028202000102),
	UINT64_C(0x8100040800590224), UINT64_C(0x2040009004800010), UINT64_C(0x40045000400408),
	UINT64_C(0x2200240020802008), UINT64_C(0x4080042002200204), UINT64_C(0x4000b0000a00a2),
	UINT64_C(0xa600000810100),    UINT64_C(0x1410000d001180),   UINT64_C(0x2200101001080),
	UINT64_C(0x100020014104e120), UINT64_C(0x2407200100004810), UINT64_C(0x80144000a0845050),
	UINT64_C(0x1000200060030c18), UINT64_C(0x4004200020010102), UINT64_C(0x140600021010302)
};

const u64 BishopMagic[SquareNum] = {
	UINT64_C(0x20101042c8200428), UINT64_C(0x840240380102),     UINT64_C(0x800800c018108251),
	UINT64_C(0x82428010301000),   UINT64_C(0x481008201000040),  UINT64_C(0x8081020420880800),
	UINT64_C(0x804222110000),     UINT64_C(0xe28301400850),     UINT64_C(0x2010221420800810),
	UINT64_C(0x2600010028801824), UINT64_C(0x8048102102002),    UINT64_C(0x4000248100240402),
	UINT64_C(0x49200200428a2108), UINT64_C(0x460904020844),     UINT64_C(0x2001401020830200),
	UINT64_C(0x1009008120),       UINT64_C(0x4804064008208004), UINT64_C(0x4406000240300ca0),
	UINT64_C(0x222001400803220),  UINT64_C(0x226068400182094),  UINT64_C(0x95208402010d0104),
	UINT64_C(0x4000807500108102), UINT64_C(0xc000200080500500), UINT64_C(0x5211000304038020),
	UINT64_C(0x1108100180400820), UINT64_C(0x10001280a8a21040), UINT64_C(0x100004809408a210),
	UINT64_C(0x202300002041112),  UINT64_C(0x4040a8000460408),  UINT64_C(0x204020021040201),
	UINT64_C(0x8120013180404),    UINT64_C(0xa28400800d020104), UINT64_C(0x200c201000604080),
	UINT64_C(0x1082004000109408), UINT64_C(0x100021c00c410408), UINT64_C(0x880820905004c801),
	UINT64_C(0x1054064080004120), UINT64_C(0x30c0a0224001030),  UINT64_C(0x300060100040821),
	UINT64_C(0x51200801020c006),  UINT64_C(0x2100040042802801), UINT64_C(0x481000820401002),
	UINT64_C(0x40408a0450000801), UINT64_C(0x810104200000a2),   UINT64_C(0x281102102108408),
	UINT64_C(0x804020040280021),  UINT64_C(0x2420401200220040), UINT64_C(0x80010144080c402),
	UINT64_C(0x80104400800002),   UINT64_C(0x1009048080400081), UINT64_C(0x100082000201008c),
	UINT64_C(0x10001008080009),   UINT64_C(0x2a5006b80080004),  UINT64_C(0xc6288018200c2884),
	UINT64_C(0x108100104200a000), UINT64_C(0x141002030814048),  UINT64_C(0x200204080010808),
	UINT64_C(0x200004013922002),  UINT64_C(0x2200000020050815), UINT64_C(0x2011010400040800),
	UINT64_C(0x1020040004220200), UINT64_C(0x944020104840081),  UINT64_C(0x6080a080801c044a),
	UINT64_C(0x2088400811008020), UINT64_C(0xc40aa04208070),    UINT64_C(0x4100800440900220),
	UINT64_C(0x48112050),         UINT64_C(0x818200d062012a10), UINT64_C(0x402008404508302),
	UINT64_C(0x100020101002),     UINT64_C(0x20040420504912),   UINT64_C(0x2004008118814),
	UINT64_C(0x1000810650084024), UINT64_C(0x1002a03002408804), UINT64_C(0x2104294801181420),
	UINT64_C(0x841080240500812),  UINT64_C(0x4406009000004884), UINT64_C(0x80082004012412),
	UINT64_C(0x80090880808183),   UINT64_C(0x300120020400410),  UINT64_C(0x21a090100822002)
};

const Bitboard FileMask[FileNum] = {
	FileIMask, FileHMask, FileGMask, FileFMask, FileEMask, FileDMask, FileCMask, FileBMask, FileAMask
};

const Bitboard RankMask[RankNum] = {
	Rank9Mask, Rank8Mask, Rank7Mask, Rank6Mask, Rank5Mask, Rank4Mask, Rank3Mask, Rank2Mask, Rank1Mask
};

const Bitboard InFrontMask[ColorNum][RankNum] = {
	{
		allZeroBB(),
		rankMask<Rank9>(),
		rankMask<Rank9>() | rankMask<Rank8>(),
		rankMask<Rank9>() | rankMask<Rank8>() | rankMask<Rank7>(),
		rankMask<Rank9>() | rankMask<Rank8>() | rankMask<Rank7>() |
		rankMask<Rank6>(),
		rankMask<Rank9>() | rankMask<Rank8>() | rankMask<Rank7>() |
		rankMask<Rank6>() | rankMask<Rank5>(),
		rankMask<Rank9>() | rankMask<Rank8>() | rankMask<Rank7>() |
		rankMask<Rank6>() | rankMask<Rank5>() | rankMask<Rank4>(),
		rankMask<Rank9>() | rankMask<Rank8>() | rankMask<Rank7>() |
		rankMask<Rank6>() | rankMask<Rank5>() | rankMask<Rank4>() |
		rankMask<Rank3>(),
		rankMask<Rank9>() | rankMask<Rank8>() | rankMask<Rank7>() |
		rankMask<Rank6>() | rankMask<Rank5>() | rankMask<Rank4>() |
		rankMask<Rank3>() | rankMask<Rank2>()
	},
	{
		rankMask<Rank1>() | rankMask<Rank2>() | rankMask<Rank3>() |
		rankMask<Rank4>() | rankMask<Rank5>() | rankMask<Rank6>() |
		rankMask<Rank7>() | rankMask<Rank8>(),
		rankMask<Rank1>() | rankMask<Rank2>() | rankMask<Rank3>() |
		rankMask<Rank4>() | rankMask<Rank5>() | rankMask<Rank6>() |
		rankMask<Rank7>(),
		rankMask<Rank1>() | rankMask<Rank2>() | rankMask<Rank3>() |
		rankMask<Rank4>() | rankMask<Rank5>() | rankMask<Rank6>(),
		rankMask<Rank1>() | rankMask<Rank2>() | rankMask<Rank3>() |
		rankMask<Rank4>() | rankMask<Rank5>(),
		rankMask<Rank1>() | rankMask<Rank2>() | rankMask<Rank3>() |
		rankMask<Rank4>(),
		rankMask<Rank1>() | rankMask<Rank2>() | rankMask<Rank3>(),
		rankMask<Rank1>() | rankMask<Rank2>(),
		rankMask<Rank1>(),
		allZeroBB()
	}
};

// これらは一度値を設定したら二度と変更しない。
// 本当は const 化したい。
Bitboard RookAttack[512000];
int RookAttackIndex[SquareNum];
Bitboard RookBlockMask[SquareNum];
Bitboard BishopAttack[20224];
int BishopAttackIndex[SquareNum];
Bitboard BishopBlockMask[SquareNum];
Bitboard LanceAttack[ColorNum][SquareNum][128];

Bitboard KingAttack[SquareNum];
Bitboard GoldAttack[ColorNum][SquareNum];
Bitboard SilverAttack[ColorNum][SquareNum];
Bitboard KnightAttack[ColorNum][SquareNum];
Bitboard PawnAttack[ColorNum][SquareNum];

Bitboard BetweenBB[SquareNum][SquareNum];

Bitboard RookAttackToEdge[SquareNum];
Bitboard BishopAttackToEdge[SquareNum];
Bitboard LanceAttackToEdge[ColorNum][SquareNum];
Bitboard NotQueenAttackToEdgeOrKingAttack[SquareNum];

Bitboard GoldCheckTable[ColorNum][SquareNum];
Bitboard SilverCheckTable[ColorNum][SquareNum];
Bitboard KnightCheckTable[ColorNum][SquareNum];
Bitboard LanceCheckTable[ColorNum][SquareNum];
