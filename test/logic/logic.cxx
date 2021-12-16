#include "src/logic/logic.hxx"
#include <catch2/catch.hpp>

TEST_CASE ("handleMessage send every possible message", "[handleMessage]")
{
  SECTION ("updateRatingWinLose 2 player", "[rating]")
  {
    auto const losers = std::vector<database::Account>{ { "player1", "", 1500 } };
    auto const winners = std::vector<database::Account>{ { "player2", "", 2000 } };
    auto const [updatedLosers, updatedWinners] = calcRatingLoserAndWinner (losers, winners);
    CHECK (updatedLosers.at (0).rating == 1499);
    CHECK (updatedWinners.at (0).rating == 2001);
  }
  SECTION ("updateRatingWinLose 4 player 1 heigh 3 low", "[rating]")
  {
    auto const losers = std::vector<database::Account>{ { "player1", "", 2000 } };
    auto const winners = std::vector<database::Account>{ { "player2", "", 1000 }, { "player3", "", 1000 }, { "player4", "", 1000 } };
    auto const [updatedLosers, updatedWinners] = calcRatingLoserAndWinner (losers, winners);
    CHECK (updatedLosers.at (0).rating == 1980);
    CHECK (updatedWinners.at (0).rating == 1007);
    CHECK (updatedWinners.at (1).rating == 1007);
    CHECK (updatedWinners.at (2).rating == 1007);
  }
}