#include "move.hpp"

std::string Move::toUSI() const {
	if(this->isNone()) {
		return "None";
	}

	const Square from = this->from();
	const Square to = this->to();
	if(this->isDrop()) {
		return handPieceToString(this->handPieceDropped()) + squareToStringUSI(to);
	}
	else {
		std::string usi = squareToStringUSI(from) + squareToStringUSI(to);
		if(this->isPromotion()) {
			usi += "+";
		}
		return usi;
	}
}

std::string Move::toCSA() const {
	if(this->isNone()) {
		return "None";
	}

	std::string s;
	if(this->isDrop()) {
		s = std::string("00");
	}
	else {
		s = squareToStringCSA(this->from());
	}
	s += squareToStringCSA(this->to())
		+ pieceTypeToString(this->pieceTypeTo());

	return s;
}
