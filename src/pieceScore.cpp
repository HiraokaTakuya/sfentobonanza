#include "piece.hpp"
#include "pieceScore.hpp"

// 学習の為に const ではない。
Score PieceScore[PieceNone] = {
	ScoreZero,
	PawnScore, LanceScore, KnightScore, SilverScore, BishopScore, RookScore, GoldScore,
	ScoreZero, // King
	ProPawnScore, ProLanceScore, ProKnightScore, ProSilverScore, HorseScore, DragonScore,
	ScoreZero, ScoreZero,
	PawnScore, LanceScore, KnightScore, SilverScore, BishopScore, RookScore, GoldScore,
	ScoreZero, // King
	ProPawnScore, ProLanceScore, ProKnightScore, ProSilverScore, HorseScore, DragonScore,
};
Score CapturePieceScore[PieceNone] = {
	ScoreZero,
	CapturePawnScore, CaptureLanceScore, CaptureKnightScore, CaptureSilverScore, CaptureBishopScore, CaptureRookScore, CaptureGoldScore,
	ScoreZero, // King
	CaptureProPawnScore, CaptureProLanceScore, CaptureProKnightScore, CaptureProSilverScore, CaptureHorseScore, CaptureDragonScore,
	ScoreZero, ScoreZero,
	CapturePawnScore, CaptureLanceScore, CaptureKnightScore, CaptureSilverScore, CaptureBishopScore, CaptureRookScore, CaptureGoldScore,
	ScoreZero, // King
	CaptureProPawnScore, CaptureProLanceScore, CaptureProKnightScore, CaptureProSilverScore, CaptureHorseScore, CaptureDragonScore,
};
Score PromotePieceScore[7] = {
	ScoreZero,
	PromotePawnScore, PromoteLanceScore, PromoteKnightScore,
	PromoteSilverScore, PromoteBishopScore, PromoteRookScore
};
