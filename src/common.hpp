#ifndef COMMON_HPP
#define COMMON_HPP

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#include <inttypes.h>
#define INT64_C(val) val##i64
#define UINT64_C(val) val##ui64

#define PRId64       "I64d"
#define PRIi64       "I64i"

#define PRIo64       "I64o"
#define PRIu64       "I64u"
#define PRIx64       "I64x"
#define PRIX64       "I64X"

#elif defined(__INTEL_COMPILER)
#include <inttypes.h>
#define INT64_C(val) val##ll
#define UINT64_C(val) val##ull

#define PRId64       "I64d"
#define PRIi64       "I64i"

#define PRIo64       "I64o"
#define PRIu64       "I64u"
#define PRIx64       "I64x"
#define PRIX64       "I64X"

#else
#include <cinttypes>
#endif

#include "ifdef.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cassert>
#include <ctime>
#include <cmath>
#include <cstddef>

#define STATIC_ASSERT(x) static_assert(x, "")

#if defined (HAVE_SSE4)
#include <smmintrin.h>
#elif defined (HAVE_SSE2)
#include <emmintrin.h>
#endif

#if !defined(NDEBUG)
// デバッグ時は、ここへ到達してはいけないので、assert でプログラムを止める。
#define UNREACHABLE assert(false)
#elif defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#define UNREACHABLE __assume(false)
#elif defined(__INTEL_COMPILER)
// todo: icc も __assume(false) で良いのか？ 一応ビルド出来るけど。
#define UNREACHABLE __assume(false)
#elif defined(__GNUC__) && (4 < __GNUC__ || (__GNUC__ == 4 && 4 < __GNUC_MINOR__))
#define UNREACHABLE __builtin_unreachable()
#else
#define UNREACHABLE assert(false)
#endif

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#define FORCE_INLINE __forceinline
#elif defined(__INTEL_COMPILER)
#define FORCE_INLINE inline
#elif defined(__GNUC__)
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
#define FORCE_INLINE inline
#endif

// インラインアセンブリのコメントを使用することで、
// C++ コードのどの部分がアセンブラのどの部分に対応するかを
// 分り易くする。
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#define ASMCOMMENT(s)
#elif defined(__INTEL_COMPILER)
#define ASMCOMMENT(s)
#elif defined(__GNUC__)
#define ASMCOMMENT(s) __asm__("#"s)
#else
#define ASMCOMMENT(s)
#endif

// bit幅を指定する必要があるときは、以下の型を使用する。
typedef int8_t   s8;
typedef uint8_t  u8;
typedef int16_t  s16;
typedef uint16_t u16;
typedef int32_t  s32;
typedef uint32_t u32;
typedef int64_t  s64;
typedef uint64_t u64;

#define nullptr 0

// Binary表記
// Binary<11110>::value とすれば、30 となる。
// 符合なし64bitなので19桁まで表記可能。
template<u64 n> struct Binary {
	static const u64 value = n % 10 + (Binary<n / 10>::value << 1);
};
// template 特殊化
template<> struct Binary<0> {
	static const u64 value = 0;
};

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER) && defined(_WIN64)
#include <intrin.h>
FORCE_INLINE int firstOneFromLSB(const u64 b) {
	unsigned long index;
	_BitScanForward64(&index, b);
	return index;
}
FORCE_INLINE int firstOneFromMSB(const u64 b) {
	unsigned long index;
	_BitScanReverse64(&index, b);
	return index;
}
#elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )
FORCE_INLINE int firstOneFromLSB(const u64 b) {
	return __builtin_ctzll(b);
}
FORCE_INLINE int firstOneFromMSB(const u64 b) {
	return __builtin_clzll(b);
}
#else
// firstOneFromLSB() で使用する table
const int BitTable[64] = {
	63, 30, 3, 32, 25, 41, 22, 33, 15, 50, 42, 13, 11, 53, 19, 34, 61, 29, 2,
	51, 21, 43, 45, 10, 18, 47, 1, 54, 9, 57, 0, 35, 62, 31, 40, 4, 49, 5, 52,
	26, 60, 6, 23, 44, 46, 27, 56, 16, 7, 39, 48, 24, 59, 14, 12, 55, 38, 28,
	58, 20, 37, 17, 36, 8
};
// LSB から数えて初めに bit が 1 になるのは何番目の bit かを返す。
// b = 8 だったら 3 を返す。
// b = 0 のとき、63 を返す。
FORCE_INLINE int firstOneFromLSB(const u64 b) {
	const u64 tmp = b ^ (b - 1);
	const u32 old = static_cast<u32>((tmp & 0xffffffff) ^ (tmp >> 32));
	return BitTable[(old * 0x783a9b23) >> 26];
}
// 超絶遅いコードなので後で書き換えること。
FORCE_INLINE int firstOneFromMSB(const u64 b) {
	for(int i = 63; 0 <= i; --i) {
		if(b >> i) {
			return 63 - i;
		}
	}
	return 0;
}
#endif

#if defined(HAVE_SSE42)
#include <nmmintrin.h>
inline int count1s(u64 x) {
	return _mm_popcnt_u64(x);
}
#else
inline int count1s(u64 x) //任意の値の1のビットの数を数える。( x is not a const value.)
{
	x = x - ((x >> 1) & UINT64_C(0x5555555555555555));
	x = (x & UINT64_C(0x3333333333333333)) + ((x >> 2) & UINT64_C(0x3333333333333333));
	x = (x + (x >> 4)) & UINT64_C(0x0f0f0f0f0f0f0f0f);
	x = x + (x >> 8);
	x = x + (x >> 16);
	x = x + (x >> 32);
	return (static_cast<int>(x)) & 0x0000007f;
}
#endif

// for debug
// 2進表示
template<typename T>
inline std::string putb(const T value, const int msb = sizeof(T)*8 - 1, const int lsb = 0) {
	std::string str;
	u64 tempValue = (static_cast<u64>(value) >> lsb);

	for(int length = msb - lsb + 1; length; --length) {
		str += ((tempValue & (UINT64_C(1) << (length - 1))) ? "1" : "0");
	}

	return str;
}

namespace Apery {

	// pc_on_sqで使うためのテーブル
	// 評価関数用のテーブルの開始,終了オフセット。
	// f_XXX : first , e_XXX : end
	// マジックナンバーをそのまま書くべきではない。
	// 持ち駒の部分は、最大持ち駒数＋１（ 持ち駒無しの場合があるので ）
	enum { f_hand_pawn   = 0, // 0
		   e_hand_pawn   = f_hand_pawn   + 19, // ↑ + 18 + 1
		   f_hand_lance  = e_hand_pawn   + 19, // ↑ + 18 + 1
		   e_hand_lance  = f_hand_lance  +  5, // ↑ +  4 + 1
		   f_hand_knight = e_hand_lance  +  5, // ↑ +  4 + 1
		   e_hand_knight = f_hand_knight +  5, // ↑ +  4 + 1
		   f_hand_silver = e_hand_knight +  5, // ↑ +  4 + 1
		   e_hand_silver = f_hand_silver +  5, // ↑ +  4 + 1
		   f_hand_gold   = e_hand_silver +  5, // ↑ +  4 + 1
		   e_hand_gold   = f_hand_gold   +  5, // ↑ +  4 + 1
		   f_hand_bishop = e_hand_gold   +  5, // ↑ +  4 + 1
		   e_hand_bishop = f_hand_bishop +  3, // ↑ +  2 + 1
		   f_hand_rook   = e_hand_bishop +  3, // ↑ +  2 + 1
		   e_hand_rook   = f_hand_rook   +  3, // ↑ +  2 + 1
		   fe_hand_end   = e_hand_rook   +  3, // ↑ +  2 + 1 , 手駒の終端

		   f_pawn        = fe_hand_end,        // = ↑
		   e_pawn        = f_pawn        + 81, // = ↑+9*9
		   f_lance       = e_pawn        + 81, // 以下、+9*9ずつ増える。
		   e_lance       = f_lance       + 81,
		   f_knight      = e_lance       + 81,
		   e_knight      = f_knight      + 81,
		   f_silver      = e_knight      + 81,
		   e_silver      = f_silver      + 81,
		   f_gold        = e_silver      + 81,
		   e_gold        = f_gold        + 81,
		   f_bishop      = e_gold        + 81,
		   e_bishop      = f_bishop      + 81,
		   f_horse       = e_bishop      + 81,
		   e_horse       = f_horse       + 81,
		   f_rook        = e_horse       + 81,
		   e_rook        = f_rook        + 81,
		   f_dragon      = e_rook        + 81,
		   e_dragon      = f_dragon      + 81,
		   fe_end        = e_dragon      + 81,

		   // 持ち駒の部分は、最大持ち駒数＋１（ 持ち駒無しの場合があるので ）
		   kkp_hand_pawn   = 0,
		   kkp_hand_lance  = kkp_hand_pawn   + 19,
		   kkp_hand_knight = kkp_hand_lance  +  5,
		   kkp_hand_silver = kkp_hand_knight +  5,
		   kkp_hand_gold   = kkp_hand_silver +  5,
		   kkp_hand_bishop = kkp_hand_gold   +  5,
		   kkp_hand_rook   = kkp_hand_bishop +  3,
		   kkp_hand_end    = kkp_hand_rook   +  3,
		   kkp_pawn        = kkp_hand_end,
		   kkp_lance       = kkp_pawn        + 81,
		   kkp_knight      = kkp_lance       + 81,
		   kkp_silver      = kkp_knight      + 81,
		   kkp_gold        = kkp_silver      + 81,
		   kkp_bishop      = kkp_gold        + 81,
		   kkp_horse       = kkp_bishop      + 81,
		   kkp_rook        = kkp_horse       + 81,
		   kkp_dragon      = kkp_rook        + 81,
		   kkp_end         = kkp_dragon      + 81 };

	const int FVScale = 32;
}

// N 回ループを展開させる。t は lambda で書くと良い。
// こんな感じに書くと、lambda がテンプレート引数の数値の分だけ繰り返し生成される。
// Unroller<5>()([&](const int i){std::cout << i << std::endl;});
template<int N> struct Unroller {
	template<typename T> FORCE_INLINE void operator () (T t) {
		Unroller<N-1>()(t);
		t(N-1);
	}
};
template<> struct Unroller<0> {
	template<typename T> FORCE_INLINE void operator () (T t) {}
};

const size_t CacheLineSize = 64; // 64byte

// Stockfish ほとんどそのまま
template<typename T> inline void prefetch(T* addr) {
// SSE が使えない時は、_mm_prefetch() とかが使えないので、prefetch無しにする。
#if defined HAVE_SSE2 || defined HAVE_SSE4
#if defined(__INTEL_COMPILER)
	// これでプリフェッチが最適化で消えるのを防げるらしい。
	__asm__("");
#endif

	// 最低でも sizeof(T) のバイト数分をプリフェッチする。
	// Stockfish は TTCluster が 64byte なのに、なぜか 128byte 分 prefetch しているが、
	// 必要無いと思う。
	char* charAddr = reinterpret_cast<char*>(addr);
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
	Unroller<(sizeof(T) + CacheLineSize - 1)/CacheLineSize>()([&](const int) {
			// 1キャッシュライン分(64byte)のプリフェッチ。
			_mm_prefetch(charAddr, _MM_HINT_T0);
			charAddr += CacheLineSize;
		});
#else
	Unroller<(sizeof(T) + CacheLineSize - 1)/CacheLineSize>()([&](const int) {
			// 1キャッシュライン分(64byte)のプリフェッチ。
			__builtin_prefetch(charAddr);
			charAddr += CacheLineSize;
		});
#endif
#endif
}

typedef u64 Key;

// Size は 2のべき乗であること。
template <typename T, size_t Size>
struct HashTable {
	HashTable() : entries_(Size, T()) {}
	T* operator [] (const Key k) { return &entries_[static_cast<size_t>(k) & (Size-1)]; }
	// Size が 2のべき乗であることのチェック
	STATIC_ASSERT((Size & (Size-1)) == 0);

private:
	std::vector<T> entries_;
};

// ミリ秒単位の時間を表すクラス
class Time {
public:
	typedef std::chrono::system_clock system_clock;

	void restart() { t_ = system_clock::now(); }
	int elapsed() {
		using std::chrono::duration_cast;
		using std::chrono::milliseconds;
		using std::chrono::system_clock;
		return duration_cast<milliseconds>(system_clock::now() - t_).count();
	}
	static Time currentTime() {
		Time t;
		t.restart();
		return t;
	}

private:
	decltype(system_clock::now()) t_;
};

#endif // #ifndef COMMON_HPP
