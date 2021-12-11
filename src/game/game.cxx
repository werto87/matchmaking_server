#include "src/game/game.hxx"

void Game::relogUser (std::shared_ptr<User>)
{
  // TODO add logic here
}

void Game::removeUser (std::shared_ptr<User>)
{
  // TODO add logic here
}

std::vector<std::shared_ptr<User>>
Game::getWinners () const
{
  // TODO add logic here
  return {};
}

void
Game::gameOverChangeRating ()
{
  auto winners = getWinners ();
  if (winners.empty ())
    {
      auto accounts = std::vector<database::Account>{};
      soci::session sql (soci::sqlite3, databaseName);
      users >>= pipes::transform ([&sql] (auto const &user) { return confu_soci::findStruct<database::Account> (sql, "accountName", user->accountName.value ()).value (); }) >>= pipes::push_back (accounts);
      auto updatedAccounts = calcRatingDraw (accounts);
      for (size_t i = 0; i < updatedAccounts.size (); i++)
        //   TODO replace with function this is the same as the other for loops part1/3
        {
          confu_soci::upsertStruct (sql, updatedAccounts.at (i));
          if (auto user = ranges::find_if (users, [accountName = updatedAccounts.at (i).accountName] (std::shared_ptr<User> user) { return user->accountName.value () == accountName; }); user != users.end ())
            {
              user->get ()->msgQueue.push_back (objectToStringWithObjectName (shared_class::RatingChanged{ accounts.at (i).rating, updatedAccounts.at (i).rating }));
            }
        }
    }
  else
    {
      soci::session sql (soci::sqlite3, databaseName);
      auto winnerAccounts = std::vector<database::Account>{};
      auto loserAccounts = std::vector<database::Account>{};
      users >>= pipes::transform ([&sql] (auto const &user) { return confu_soci::findStruct<database::Account> (sql, "accountName", user->accountName.value ()).value (); }) >>= pipes::partition ([&winners] (database::Account const &account) { return ranges::find_if (winners, [accountName = account.accountName] (std::shared_ptr<User> user) { return user->accountName == accountName; }) != winners.end (); }, pipes::push_back (winnerAccounts), pipes::push_back (loserAccounts));
      auto [updatedLoserAccounts, updatedWinnerAccounts] = calcRatingLoserAndWinner (loserAccounts, winnerAccounts);
      for (size_t i = 0; i < updatedLoserAccounts.size (); i++)
        {
          //   TODO replace with function this is the same as the other for loops part2/3
          confu_soci::upsertStruct (sql, updatedLoserAccounts.at (i));
          if (auto user = ranges::find_if (users, [accountName = updatedLoserAccounts.at (i).accountName] (std::shared_ptr<User> user) { return user->accountName.value () == accountName; }); user != users.end ())
            {
              user->get ()->msgQueue.push_back (objectToStringWithObjectName (shared_class::RatingChanged{ loserAccounts.at (i).rating, updatedLoserAccounts.at (i).rating }));
            }
        }
      for (size_t i = 0; i < updatedWinnerAccounts.size (); i++)
        {
          //   TODO replace with function this is the same as the other for loops part3/3
          confu_soci::upsertStruct (sql, updatedWinnerAccounts.at (i));
          if (auto user = ranges::find_if (users, [accountName = updatedWinnerAccounts.at (i).accountName] (std::shared_ptr<User> user) { return user->accountName.value () == accountName; }); user != users.end ())
            {
              user->get ()->msgQueue.push_back (objectToStringWithObjectName (shared_class::RatingChanged{ winnerAccounts.at (i).rating, updatedWinnerAccounts.at (i).rating }));
            }
        }
    }
}
