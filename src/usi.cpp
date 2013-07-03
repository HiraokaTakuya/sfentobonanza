#include "usi.hpp"
#include "position.hpp"
#include "move.hpp"
//#include "movePicker.hpp"
#include "generateMoves.hpp"
//#include "search.hpp"
//#include "tt.hpp"
//#include "book.hpp"
//#include "thread.hpp"
//#include "learner.hpp"

bool CaseInsensitiveLess::operator () (const std::string& s1, const std::string& s2) const {
	for(size_t i = 0; i < s1.size() && i < s2.size(); ++i) {
		const int c1 = tolower(s1[i]);
		const int c2 = tolower(s2[i]);

		if(c1 != c2) {
			return c1 < c2;
		}
	}
	return s1.size() < s2.size();
}

namespace {
	// 論理的なコア数の取得
//	inline int cpuCoreCount() {
//		// std::thread::hardware_concurrency() は 0 を返す可能性がある。
//		return std::max(static_cast<int>(std::thread::hardware_concurrency()), 1);
//	}

	StateStackPtr SetUpStates;

	class StringToPieceTypeCSA : public std::map<std::string, PieceType> {
	public:
		StringToPieceTypeCSA() {
			(*this)["FU"] = Pawn;
			(*this)["KY"] = Lance;
			(*this)["KE"] = Knight;
			(*this)["GI"] = Silver;
			(*this)["KA"] = Bishop;
			(*this)["HI"] = Rook;
			(*this)["KI"] = Gold;
			(*this)["OU"] = King;
			(*this)["TO"] = ProPawn;
			(*this)["NY"] = ProLance;
			(*this)["NK"] = ProKnight;
			(*this)["NG"] = ProSilver;
			(*this)["UM"] = Horse;
			(*this)["RY"] = Dragon;
		}
		PieceType value(const std::string& str) const {
			return this->find(str)->second;
		}
		bool isLegalString(const std::string& str) const {
			return (this->find(str) != this->end());
		}
	};
	const StringToPieceTypeCSA g_stringToPieceTypeCSA;
}

Move usiToMoveBody(const Position& pos, const std::string& moveStr) {
	Move move;
	if(g_charToPieceUSI.isLegalChar(moveStr[0])) {
		// drop
		const PieceType ptTo = pieceToPieceType(g_charToPieceUSI.value(moveStr[0]));
		if(moveStr[1] != '*') {
			return Move::moveNone();
		}
		const File toFile = charUSIToFile(moveStr[2]);
		const Rank toRank = charUSIToRank(moveStr[3]);
		if(!isInSquare(toFile, toRank)) {
			return Move::moveNone();
		}
		const Square to = makeSquare(toFile, toRank);
		move = makeDropMove(ptTo, to);
	}
	else {
		const File fromFile = charUSIToFile(moveStr[0]);
		const Rank fromRank = charUSIToRank(moveStr[1]);
		if(!isInSquare(fromFile, fromRank)) {
			return Move::moveNone();
		}
		const Square from = makeSquare(fromFile, fromRank);
		const File toFile = charUSIToFile(moveStr[2]);
		const Rank toRank = charUSIToRank(moveStr[3]);
		if(!isInSquare(toFile, toRank)) {
			return Move::moveNone();
		}
		const Square to = makeSquare(toFile, toRank);
		if(moveStr[4] == '\0') {
			move = makeNonPromoteMove<Capture>(pieceToPieceType(pos.piece(from)), from, to, pos);
		}
		else if(moveStr[4] == '+') {
			if(moveStr[5] != '\0') {
				return Move::moveNone();
			}
			move = makePromoteMove<Capture>(pieceToPieceType(pos.piece(from)), from, to, pos);
		}
		else {
			return Move::moveNone();
		}
	}

	if(pos.moveIsPseudoLegal<true>(move)
	   && pos.pseudoLegalMoveIsLegal<false, false>(move, pos.pinnedBB()))
	{
		return move;
	}
	return Move::moveNone();
}
#if !defined NDEBUG
// for debug
Move usiToMoveDebug(const Position& pos, const std::string& moveStr) {
	for(MoveList<LegalAll> ml(pos); !ml.end(); ++ml) {
		if(moveStr == ml.move().toUSI()) {
			return ml.move();
		}
	}
	return Move::moveNone();
}
Move csaToMoveDebug(const Position& pos, const std::string& moveStr) {
	for(MoveList<LegalAll> ml(pos); !ml.end(); ++ml) {
		if(moveStr == ml.move().toCSA()) {
			return ml.move();
		}
	}
	return Move::moveNone();
}
#endif
Move usiToMove(const Position& pos, const std::string& moveStr) {
	const Move move = usiToMoveBody(pos, moveStr);
	assert(move == usiToMoveDebug(pos, moveStr));
	return move;
}

Move csaToMoveBody(const Position& pos, const std::string& moveStr) {
	if(moveStr.size() != 6) {
		return Move::moveNone();
	}
	const File toFile = charCSAToFile(moveStr[2]);
	const Rank toRank = charCSAToRank(moveStr[3]);
	if(!isInSquare(toFile, toRank)) {
		return Move::moveNone();
	}
	const Square to = makeSquare(toFile, toRank);
	const std::string ptToString(moveStr.begin() + 4, moveStr.end());
	if(!g_stringToPieceTypeCSA.isLegalString(ptToString)) {
		return Move::moveNone();
	}
	const PieceType ptTo = g_stringToPieceTypeCSA.value(ptToString);
	Move move;
	if(moveStr[0] == '0' && moveStr[1] == '0') {
		// drop
		move = makeDropMove(ptTo, to);
	}
	else {
		const File fromFile = charCSAToFile(moveStr[0]);
		const Rank fromRank = charCSAToRank(moveStr[1]);
		if(!isInSquare(fromFile, fromRank)) {
			return Move::moveNone();
		}
		const Square from = makeSquare(fromFile, fromRank);
		PieceType ptFrom = pieceToPieceType(pos.piece(from));
		if(ptFrom == ptTo) {
			// non promote
			move = makeNonPromoteMove<Capture>(ptFrom, from, to, pos);
		}
		else if(ptFrom + PTPromote == ptTo) {
			// promote
			move = makePromoteMove<Capture>(ptFrom, from, to, pos);
		}
		else {
			return Move::moveNone();
		}
	}

	if(pos.moveIsPseudoLegal<true>(move)
	   && pos.pseudoLegalMoveIsLegal<false, false>(move, pos.pinnedBB()))
	{
		return move;
	}
	return Move::moveNone();
}
Move csaToMove(const Position& pos, const std::string& moveStr) {
	const Move move = csaToMoveBody(pos, moveStr);
	assert(move == csaToMoveDebug(pos, moveStr));
	return move;
}

template<bool SfenToBonanza = false>
void setPosition(Position& pos, std::istringstream& ssCmd) {
	std::string token;
	std::string sfen;
	if(SfenToBonanza) {
		sfen = DefaultStartPositionSFEN;
	}

	ssCmd >> token;

	if(!SfenToBonanza) {
		if(token == "startpos") {
			sfen = DefaultStartPositionSFEN;
			ssCmd >> token; // "moves" が入力されるはず。
		}
		else if(token == "sfen") {
			while(ssCmd >> token && token != "moves") {
				sfen += token + " ";
			}
		}
		else {
			return;
		}
	}

	pos.set(sfen/*, g_threads.mainThread(), &g_threads.searchInfo_*/);
	SetUpStates = StateStackPtr(new std::stack<StateInfo>());

	const char color[ColorNum] = {'+', '-'};
	if(SfenToBonanza) {
		std::cout << "PI\n"
				  << color[pos.turn()] << "\n";
	}
	Ply currentPly = 0;
	while(ssCmd >> token) {
		SetUpStates->push(StateInfo());
		const Move move = usiToMove(pos, token);
		if(SfenToBonanza) {
			if(move.isNone()) {
				break;
			}
			std::cout << color[pos.turn()] << move.toCSA() << "\n";
		}
		pos.doMove(move, SetUpStates->top());
		++currentPly;
	}
	if(SfenToBonanza) {
		std::cout << "%TORYO\n"
				  << "/\n";
	}
	pos.addStartPosPly(currentPly);
}

#if !defined MINIMUL
// for debug
// 指し手生成の速度を計測
void measureGenerateMoves(const Position& pos) {
	pos.print();

	MoveStack legalMoves[MaxLegalMoves];
	for(int i = 0; i < MaxLegalMoves; ++i) legalMoves[i].move = moveNone();
	MoveStack* pms = &legalMoves[0];
	const u64 num = 5000000;
	Time t = Time::currentTime();
	if(pos.inCheck()) {
		for(u64 i = 0; i < num; ++i) {
			pms = &legalMoves[0];
			pms = generateMoves<Evasion>(pms, pos);
		}
	}
	else {
		for(u64 i = 0; i < num; ++i) {
			pms = &legalMoves[0];
			pms = generateMoves<CapturePlusPro>(pms, pos);
			pms = generateMoves<NonCaptureMinusPro>(pms, pos);
			pms = generateMoves<Drop>(pms, pos);
//			pms = generateMoves<PseudoLegal>(pms, pos);
//			pms = generateMoves<Legal>(pms, pos);
		}
	}
	const int elapsed = t.elapsed();
	std::cout << "elapsed = " << elapsed << " [msec]" << std::endl;
	std::cout << "times/s = " << num * 1000 / elapsed << " [times/sec]" << std::endl;
	const ptrdiff_t count = pms - &legalMoves[0];
	std::cout << "num of moves = " << count << std::endl;
	for(int i = 0; i < count; ++i) {
		std::cout << legalMoves[i].move.toCSA() << ", ";
	}
	std::cout << std::endl;
}
#endif

#ifdef NDEBUG
const std::string MyName = "Apery";
#else
const std::string MyName = "Apery Debug Build";
#endif

void doUSICommandLoop() {
	Position pos(DefaultStartPositionSFEN/*, g_threads.mainThread(), &g_threads.searchInfo_*/);

	std::string cmd;
	std::string token;

	do {
		getline(std::cin, cmd);
		std::istringstream ssCmd(cmd);

		token = "quit";
		ssCmd >> std::skipws >> token;

		if(token == "usi") {
			std::cout << "id name " << MyName
					  << "\nid author Hiraoka Takuya"
//					  << "\n" << g_options
					  << "\nusiok" << std::endl;
		}
		else if(token == "isready") {
			std::cout << "readyok" << std::endl;
		}
		else if(token == "position") {
			setPosition(pos, ssCmd);
		}
		else if(token == "startpos") {
			// sfen to bonanza
			setPosition<true>(pos, ssCmd);
		}
#if !defined MINIMUL
		// 以下、デバッグ用
		else if(token == "d") {
			pos.print();
		}
		else if(token == "s") {
			measureGenerateMoves(pos);
		}
		else if(token == "t") {
			std::cout << pos.mateMoveIn1Ply().toCSA() << std::endl;
		}
#endif
		else if(token == "quit") {
			break;
		}
		else {
			std::cout << "unknown command: " << cmd << std::endl;
		}
	} while(true);
}
