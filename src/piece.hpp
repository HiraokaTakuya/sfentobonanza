#ifndef PIECE_HPP
#define PIECE_HPP

#include "common.hpp"
#include "overloadEnumOperators.hpp"
#include "color.hpp"
#include <iostream>
#include <string>
#include <cassert>

enum PieceType {
	// Pro* は 元の 駒の種類に 8 を加算したもの。それを利用するかは未定。
	PTPromote = 8,
	Occupied = 0, // 他の PieceType 全ての or をとったもの。
	Pawn, Lance, Knight, Silver, Bishop, Rook, Gold, King,
	ProPawn, ProLance, ProKnight, ProSilver, Horse, Dragon,
	PieceTypeNum
};
OverloadEnumOperators(PieceType);

enum Piece {
	// B* に 16 を加算することで、W* を表す。
	// Promoted を加算することで、成りを表す。
	Empty = 0, UnPromoted = 0, Promoted = 8,
	BPawn = 1, BLance, BKnight, BSilver, BBishop, BRook, BGold, BKing,
	BProPawn, BProLance, BProKnight, BProSilver, BHorse, BDragon, // BDragon = 14
	WPawn = 17, WLance, WKnight, WSilver, WBishop, WRook, WGold, WKing,
	WProPawn, WProLance, WProKnight, WProSilver, WHorse, WDragon,
	PieceNone // PieceNone = 31  これを 32 にした方が多重配列のときに有利か。
};
OverloadEnumOperators(Piece);

// 持ち駒を表すときに使用する。
enum HandPiece {
	HPawn, HLance, HKnight, HSilver, HGold, HBishop, HRook, HandPieceNum
};
OverloadEnumOperators(HandPiece);

// p == Empty のとき、PieceType は OccuPied になってしまうので、
// Position::bbOf(pieceToPieceType(p)) とすると、
// Position::emptyBB() ではなく Position::occupiedBB() になってしまうので、
// 注意すること。出来れば修正したい。
inline PieceType pieceToPieceType(const Piece p) {
	return static_cast<PieceType>(p & 15);
}

inline Color pieceToColor(const Piece p) {
	assert(p != Empty);
	return static_cast<Color>(p >> 4);
}

inline Piece colorAndPieceTypeToPiece(const Color c, const PieceType pt) {
	return static_cast<Piece>((c << 4) | pt);
}

const u32 IsSliderVal = 0x60646064;
// pc が遠隔駒であるか
inline bool isSlider(const Piece pc) {
	return static_cast<bool>(IsSliderVal & (1 << pc));
}
inline bool isSlider(const PieceType pt) {
	return static_cast<bool>(IsSliderVal & (1 << pt));
}

inline PieceType handPieceToPieceType(const HandPiece hp) {
	switch(hp) {
	case HPawn:   return Pawn;
	case HLance:  return Lance;
	case HKnight: return Knight;
	case HSilver: return Silver;
	case HGold:   return Gold;
	case HBishop: return Bishop;
	case HRook:   return Rook;
	default: UNREACHABLE;
	}
}

inline HandPiece pieceToHandPiece(const Piece p) {
	switch(p) {
	case BPawn:   case WPawn:   case BProPawn:   case WProPawn:   return HPawn;
	case BLance:  case WLance:  case BProLance:  case WProLance:  return HLance;
	case BKnight: case WKnight: case BProKnight: case WProKnight: return HKnight;
	case BSilver: case WSilver: case BProSilver: case WProSilver: return HSilver;
	case BGold:   case WGold:                                     return HGold;
	case BBishop: case WBishop: case BHorse:     case WHorse:     return HBishop;
	case BRook:   case WRook:   case BDragon:    case WDragon:    return HRook;
	default: UNREACHABLE;
	}
}

inline HandPiece pieceTypeToHandPiece(const PieceType pt) {
	switch(pt) {
	case Pawn:   case ProPawn:   return HPawn;
	case Lance:  case ProLance:  return HLance;
	case Knight: case ProKnight: return HKnight;
	case Silver: case ProSilver: return HSilver;
	case Bishop: case Horse:     return HBishop;
	case Rook:   case Dragon:    return HRook;
	case Gold:                   return HGold;
	default: UNREACHABLE;
	}
}

inline std::string handPieceToString(const HandPiece hp) {
	switch(hp) {
	case HPawn:   return "P*";
	case HLance:  return "L*";
	case HKnight: return "N*";
	case HSilver: return "S*";
	case HBishop: return "B*";
	case HRook:   return "R*";
	case HGold:   return "G*";
	default: UNREACHABLE;
	}
}

inline std::string pieceTypeToString(const PieceType pt) {
	switch(pt) {
	case Pawn:      return "FU";
	case Lance:     return "KY";
	case Knight:    return "KE";
	case Silver:    return "GI";
	case Bishop:    return "KA";
	case Rook:      return "HI";
	case Gold:      return "KI";
	case King:      return "OU";
	case ProPawn:   return "TO";
	case ProLance:  return "NY";
	case ProKnight: return "NK";
	case ProSilver: return "NG";
	case Horse:     return "UM";
	case Dragon:    return "RY";
	default:        return "XX";
	}
}

inline void printPieceType(const PieceType pt) {
	std::cout << pieceTypeToString(pt);
}

#endif // #ifndef PIECE_HPP
