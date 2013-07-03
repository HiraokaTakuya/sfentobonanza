#ifndef PIECESCORE_HPP
#define PIECESCORE_HPP

#include "score.hpp"
#include "piece.hpp"

// same as Bonanza
const Score PawnScore      = static_cast<Score>(   87);
const Score LanceScore     = static_cast<Score>(  232);
const Score KnightScore    = static_cast<Score>(  257);
const Score SilverScore    = static_cast<Score>(  369);
const Score GoldScore      = static_cast<Score>(  444);
const Score BishopScore    = static_cast<Score>(  569);
const Score RookScore      = static_cast<Score>(  642);
const Score ProPawnScore   = static_cast<Score>(  534);
const Score ProLanceScore  = static_cast<Score>(  489);
const Score ProKnightScore = static_cast<Score>(  510);
const Score ProSilverScore = static_cast<Score>(  495);
const Score HorseScore     = static_cast<Score>(  827);
const Score DragonScore    = static_cast<Score>(  945);
const Score KingScore      = static_cast<Score>(15000);

const Score CapturePawnScore      = PawnScore      * 2;
const Score CaptureLanceScore     = LanceScore     * 2;
const Score CaptureKnightScore    = KnightScore    * 2;
const Score CaptureSilverScore    = SilverScore    * 2;
const Score CaptureGoldScore      = GoldScore      * 2;
const Score CaptureBishopScore    = BishopScore    * 2;
const Score CaptureRookScore      = RookScore      * 2;
const Score CaptureProPawnScore   = ProPawnScore   + PawnScore;
const Score CaptureProLanceScore  = ProLanceScore  + LanceScore;
const Score CaptureProKnightScore = ProKnightScore + KnightScore;
const Score CaptureProSilverScore = ProSilverScore + SilverScore;
const Score CaptureHorseScore     = HorseScore     + BishopScore;
const Score CaptureDragonScore    = DragonScore    + RookScore;
const Score CaptureKingScore      = KingScore      * 2;

const Score PromotePawnScore      = ProPawnScore   - PawnScore;
const Score PromoteLanceScore     = ProLanceScore  - LanceScore;
const Score PromoteKnightScore    = ProKnightScore - KnightScore;
const Score PromoteSilverScore    = ProSilverScore - SilverScore;
const Score PromoteBishopScore    = HorseScore     - BishopScore;
const Score PromoteRookScore      = DragonScore    - RookScore;

const Score ScoreKnownWin = KingScore;

extern Score PieceScore[PieceNone];
extern Score CapturePieceScore[PieceNone];
extern Score PromotePieceScore[7];

#endif // #ifndef PIECESCORE_HPP
