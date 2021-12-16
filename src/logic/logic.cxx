#include "src/logic/logic.hxx"
#include "src/database/database.hxx"
#include "src/game/rating.hxx"
#include "src/pw_hash/passwordHash.hxx"
#include "src/server/gameLobby.hxx"
#include "src/util.hxx"
#include <algorithm>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/fusion/include/pair.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/support/pair.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/type_index.hpp>
#include <boost/uuid/uuid.hpp>
#include <cmath>
#include <confu_json/confu_json.hxx>
#include <crypt.h>
#include <cstddef>
#include <cstdlib>
#include <fmt/core.h>
#include <iostream>
#include <iterator>
#include <numeric>
#include <optional>
#include <pipes/filter.hpp>
#include <pipes/pipes.hpp>
#include <pipes/push_back.hpp>
#include <pipes/transform.hpp>
#include <range/v3/algorithm/copy_if.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/all.hpp>
#include <range/v3/iterator/insert_iterators.hpp>
#include <range/v3/numeric/accumulate.hpp>
#include <range/v3/range.hpp>
#include <range/v3/range_fwd.hpp>
#include <range/v3/view/filter.hpp>
#include <sodium.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

template <class... Ts> struct overloaded : Ts...
{
  using Ts::operator()...;
};
template <class... Ts> overloaded (Ts...) -> overloaded<Ts...>;

boost::asio::awaitable<void>
handleMessage (std::string const &msg, boost::asio::io_context &io_context, boost::asio::thread_pool &pool, std::list<std::shared_ptr<User>> &users, std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies, std::list<Game> &games)
{
  std::vector<std::string> splitMesssage{};
  boost::algorithm::split (splitMesssage, msg, boost::is_any_of ("|"));
  if (splitMesssage.size () == 2)
    {
      // TODO-TEMPLATE to add a new type to handle here put it in sharedClasses it is in src/serialization.hxx
      auto const &typeToSearch = splitMesssage.at (0);
      auto const &objectAsString = splitMesssage.at (1);
      if (typeToSearch == "CreateAccount")
        {
          co_await createAccountAndLogin (objectAsString, io_context, user, pool, gameLobbies, games);
          user->ignoreCreateAccount = false;
          user->ignoreLogin = false;
        }
      else if (typeToSearch == "LoginAccount")
        {
          co_await loginAccount (objectAsString, io_context, users, user, pool, gameLobbies, games);
          user->ignoreCreateAccount = false;
          user->ignoreLogin = false;
        }
      else if (typeToSearch == "LoginAccountCancel")
        {
          loginAccountCancel (user);
        }
      else if (typeToSearch == "CreateAccountCancel")
        {
          createAccountCancel (user);
        }
      else if (typeToSearch == "LoginAsGuest")
        {
          loginAsGuest (user);
        }
      else if (user->accountName)
        {
          if (typeToSearch == "BroadCastMessage")
            {
              broadCastMessage (objectAsString, users, *user);
            }
          else if (typeToSearch == "JoinChannel")
            {
              joinChannel (objectAsString, user);
            }
          else if (typeToSearch == "LeaveChannel")
            {
              leaveChannel (objectAsString, user);
            }
          else if (typeToSearch == "LogoutAccount")
            {
              logoutAccount (user, gameLobbies, games);
            }
          else if (typeToSearch == "CreateGameLobby")
            {
              createGameLobby (objectAsString, user, gameLobbies);
            }
          else if (typeToSearch == "JoinGameLobby")
            {
              joinGameLobby (objectAsString, user, gameLobbies);
            }
          else if (typeToSearch == "SetMaxUserSizeInCreateGameLobby")
            {
              setMaxUserSizeInCreateGameLobby (objectAsString, user, gameLobbies);
            }
          else if (typeToSearch == "GameOption")
            {
              setGameOption (objectAsString, user, gameLobbies);
            }
          else if (typeToSearch == "LeaveGameLobby")
            {
              leaveGameLobby (user, gameLobbies);
            }
          else if (typeToSearch == "RelogTo")
            {
              relogTo (objectAsString, user, gameLobbies, games);
            }
          else if (typeToSearch == "CreateGame")
            {
              createGame (user, gameLobbies, io_context);
            }
          else if (typeToSearch == "LeaveGame")
            {
              leaveGame (user, games);
            }
          else if (typeToSearch == "JoinMatchMakingQueue")
            {
              joinMatchMakingQueue (user, gameLobbies, io_context, (stringToObject<shared_class::JoinMatchMakingQueue> (objectAsString).isRanked) ? GameLobby::LobbyType::MatchMakingSystemRanked : GameLobby::LobbyType::MatchMakingSystemUnranked);
            }
          else if (typeToSearch == "WantsToJoinGame")
            {
              wantsToJoinGame (objectAsString, user, gameLobbies, games);
            }
          else if (typeToSearch == "LeaveQuickGameQueue")
            {
              leaveMatchMakingQueue (user, gameLobbies);
            }
          else if (typeToSearch == "JoinRankedGameQueue")
            {
              joinMatchMakingQueue (user, gameLobbies, io_context, GameLobby::LobbyType::MatchMakingSystemRanked);
            }
          else
            {
              std::cout << "UnhandledMessage|{\"message\": \"" << msg << "\"}" << std::endl;
              user->sendMessageToUser ("UnhandledMessage|{\"message\": \"" + msg + "\"}");
            }
        }
      else
        {
          std::cout << "UnhandledMessage|{\"message\": \"" << msg << "\"}" << std::endl;
          user->sendMessageToUser ("UnhandledMessage|{\"message\": \"" + msg + "\"}");
        }
    }
  else
    {
      std::cout << "UnhandledMessage|{\"message\": \"" << msg << "\"}" << std::endl;
      user->sendMessageToUser ("UnhandledMessage|{\"message\": \"" + msg + "\"}");
    }
  co_return;
}

boost::asio::awaitable<void>
createAccountAndLogin (std::string objectAsString, boost::asio::io_context &io_context, std::shared_ptr<User> user, boost::asio::thread_pool &pool, std::list<GameLobby> &gameLobbies, std::list<Game> &games)
{
  if (user->accountName)
    {
      logoutAccount (user, gameLobbies, games);
    }
  auto createAccountObject = stringToObject<shared_class::CreateAccount> (objectAsString);
  soci::session sql (soci::sqlite3, databaseName);
  if (confu_soci::findStruct<database::Account> (sql, "accountName", createAccountObject.accountName))
    {
      user->sendMessageToUser (objectToStringWithObjectName (shared_class::CreateAccountError{ createAccountObject.accountName, "account already created" }));
      co_return;
    }
  else
    {
      auto hashedPw = co_await async_hash (pool, io_context, createAccountObject.password, boost::asio::use_awaitable);
      if (user->ignoreCreateAccount)
        {
          user->sendMessageToUser (objectToStringWithObjectName (shared_class::CreateAccountError{ createAccountObject.accountName, "Canceled by User Request" }));
          co_return;
        }
      else
        {
          if (auto account = database::createAccount (createAccountObject.accountName, hashedPw))
            {
              user->accountName = account->accountName;
              user->sendMessageToUser (objectToStringWithObjectName (shared_class::LoginAccountSuccess{ createAccountObject.accountName }));
              co_return;
            }
          else
            {
              user->sendMessageToUser (objectToStringWithObjectName (shared_class::CreateAccountError{ createAccountObject.accountName, "account already created" }));
              co_return;
            }
        }
    }
}

boost::asio::awaitable<void>
loginAccount (std::string objectAsString, boost::asio::io_context &io_context, std::list<std::shared_ptr<User>> &users, std::shared_ptr<User> user, boost::asio::thread_pool &pool, std::list<GameLobby> &gameLobbies, std::list<Game> &games)
{
  if (user->accountName)
    {
      logoutAccount (user, gameLobbies, games);
    }
  auto loginAccountObject = stringToObject<shared_class::LoginAccount> (objectAsString);
  soci::session sql (soci::sqlite3, databaseName);
  if (auto account = confu_soci::findStruct<database::Account> (sql, "accountName", loginAccountObject.accountName))
    {
      if (std::find_if (users.begin (), users.end (), [accountName = account->accountName] (auto const &u) { return accountName == u->accountName; }) != users.end ())
        {
          user->sendMessageToUser (objectToStringWithObjectName (shared_class::LoginAccountError{ loginAccountObject.accountName, "Account already logged in" }));
          co_return;
        }
      else
        {
          if (co_await async_check_hashed_pw (pool, io_context, account->password, loginAccountObject.password, boost::asio::use_awaitable))
            {
              if (user->ignoreLogin)
                {
                  user->sendMessageToUser (objectToStringWithObjectName (shared_class::LoginAccountError{ loginAccountObject.accountName, "Canceled by User Request" }));
                  co_return;
                }
              else
                {
                  user->accountName = account->accountName;
                  if (auto gameLobbyWithUser = ranges::find_if (gameLobbies,
                                                                [accountName = user->accountName] (auto const &gameLobby) {
                                                                  auto const &accountNames = gameLobby.accountNames ();
                                                                  return ranges::find_if (accountNames, [&accountName] (auto const &nameToCheck) { return nameToCheck == accountName; }) != accountNames.end ();
                                                                });
                      gameLobbyWithUser != gameLobbies.end ())
                    {
                      if (gameLobbyWithUser->lobbyAdminType == GameLobby::LobbyType::FirstUserInLobbyUsers)
                        {
                          user->sendMessageToUser (objectToStringWithObjectName (shared_class::WantToRelog{ loginAccountObject.accountName, "Create Game Lobby" }));
                        }
                      else
                        {
                          gameLobbyWithUser->removeUser (user);
                          if (gameLobbyWithUser->accountCount () == 0)
                            {
                              gameLobbies.erase (gameLobbyWithUser);
                            }
                          user->sendMessageToUser (objectToStringWithObjectName (shared_class::LoginAccountSuccess{ loginAccountObject.accountName }));
                        }
                      co_return;
                    }
                  else if (auto gameWithUser = ranges::find_if (games,
                                                                [accountName = user->accountName] (auto const &game) {
                                                                  auto accountNames = std::vector<std::string>{};
                                                                  ranges::transform (game.users, ranges::back_inserter (accountNames), [] (std::shared_ptr<User> const &user) { return user->accountName.value (); });
                                                                  return ranges::find_if (accountNames, [&accountName] (auto const &nameToCheck) { return nameToCheck == accountName; }) != accountNames.end ();
                                                                });
                           gameWithUser != games.end ())
                    {
                      user->sendMessageToUser (objectToStringWithObjectName (shared_class::WantToRelog{ loginAccountObject.accountName, "Back To Game" }));
                      co_return;
                    }
                  else
                    {
                      user->sendMessageToUser (objectToStringWithObjectName (shared_class::LoginAccountSuccess{ loginAccountObject.accountName }));
                      co_return;
                    }
                }
            }
          else
            {
              user->sendMessageToUser (objectToStringWithObjectName (shared_class::LoginAccountError{ loginAccountObject.accountName, "Incorrect Username or Password" }));
              co_return;
            }
        }
    }
  else
    {
      user->sendMessageToUser (objectToStringWithObjectName (shared_class::LoginAccountError{ loginAccountObject.accountName, "Incorrect username or password" }));
      co_return;
    }
}

void
broadCastMessage (std::string const &objectAsString, std::list<std::shared_ptr<User>> &users, User &sendingUser)
{
  auto broadCastMessageObject = stringToObject<shared_class::BroadCastMessage> (objectAsString);
  if (sendingUser.accountName)
    {
      for (auto &user : users | ranges::views::filter ([channel = broadCastMessageObject.channel, accountName = sendingUser.accountName] (auto const &user) { return user->communicationChannels.find (channel) != user->communicationChannels.end (); }))
        {
          soci::session sql (soci::sqlite3, databaseName);
          auto message = shared_class::Message{ sendingUser.accountName.value (), broadCastMessageObject.channel, broadCastMessageObject.message };
          user->sendMessageToUser (objectToStringWithObjectName (std::move (message)));
        }
      return;
    }
  else
    {
      sendingUser.msgQueue.push_back (objectToStringWithObjectName (shared_class::BroadCastMessageError{ broadCastMessageObject.channel, "account not logged in" }));
      return;
    }
}

void
joinChannel (std::string const &objectAsString, std::shared_ptr<User> user)
{
  auto joinChannelObject = stringToObject<shared_class::JoinChannel> (objectAsString);
  if (user->accountName)
    {
      user->communicationChannels.insert (joinChannelObject.channel);
      user->sendMessageToUser (objectToStringWithObjectName (shared_class::JoinChannelSuccess{ joinChannelObject.channel }));
      return;
    }
  else
    {
      user->sendMessageToUser (objectToStringWithObjectName (shared_class::JoinChannelError{ joinChannelObject.channel, { "user not logged in" } }));
      return;
    }
}

void
leaveChannel (std::string const &objectAsString, std::shared_ptr<User> user)
{
  auto leaveChannelObject = stringToObject<shared_class::LeaveChannel> (objectAsString);
  if (user->accountName)
    {
      if (user->communicationChannels.erase (leaveChannelObject.channel))
        {
          user->sendMessageToUser (objectToStringWithObjectName (shared_class::LeaveChannelSuccess{ leaveChannelObject.channel }));
          return;
        }
      else
        {
          user->sendMessageToUser (objectToStringWithObjectName (shared_class::LeaveChannelError{ leaveChannelObject.channel, { "channel not found" } }));
          return;
        }
    }
  else
    {
      user->sendMessageToUser (objectToStringWithObjectName (shared_class::LeaveChannelError{ leaveChannelObject.channel, { "user not logged in" } }));
      return;
    }
}

std::optional<shared_class::GameOptionError>
errorInGameOption (shared_class::GameOption const &)
{
  // TODO-TEMPLATE check Game option
  return std::nullopt;
}

void
startGame (std::list<GameLobby>::iterator &gameLobby, std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies, std::list<Game> &games)
{
  gameLobby->cancelTimer ();
  gameLobby->sendToAllAccountsInGameLobby (objectToStringWithObjectName (shared_class::StartGame{}));
  auto names = std::vector<std::string>{};
  ranges::transform (gameLobby->_users, ranges::back_inserter (names), [] (auto const &tempUser) { return tempUser->accountName.value (); });
  games.emplace_back (gameLobby->_users, gameLobby->lobbyAdminType, [accountName = user->accountName, &games] () {
    if (auto gameWithUser = ranges::find_if (games,
                                             [accountName] (auto const &game) {
                                               auto accountNames = std::vector<std::string>{};
                                               ranges::transform (game.users, ranges::back_inserter (accountNames), [] (auto const &user) { return user->accountName.value (); });
                                               return ranges::find_if (accountNames, [&accountName] (auto const &nameToCheck) { return nameToCheck == accountName; }) != accountNames.end ();
                                             });
        gameWithUser != games.end ())
      {
        if (gameWithUser->lobbyType == GameLobby::LobbyType::MatchMakingSystemRanked)
          {
            gameWithUser->gameOverChangeRating ();
          }
        games.erase (gameWithUser);
      }
  });
  gameLobbies.erase (gameLobby);
}

void
askUsersToJoinGame (std::list<GameLobby>::iterator &gameLobby, std::list<GameLobby> &gameLobbies, boost::asio::io_context &io_context)
{
  gameLobby->sendToAllAccountsInGameLobby (objectToStringWithObjectName (shared_class::AskIfUserWantsToJoinGame{}));
  gameLobby->startTimerToAcceptTheInvite (io_context, [gameLobby, &gameLobbies] () {
    auto notReadyUsers = std::vector<std::shared_ptr<User>>{};
    ranges::copy_if (gameLobby->_users, ranges::back_inserter (notReadyUsers), [usersWhichAccepted = gameLobby->readyUsers] (std::shared_ptr<User> const &user) { return ranges::find_if (usersWhichAccepted, [user] (std::shared_ptr<User> const &userWhoAccepted) { return user->accountName.value () == userWhoAccepted->accountName.value (); }) == usersWhichAccepted.end (); });
    for (auto const &notReadyUser : notReadyUsers)
      {
        notReadyUser->sendMessageToUser (objectToStringWithObjectName (shared_class::AskIfUserWantsToJoinGameTimeOut{}));
        if (gameLobby->lobbyAdminType != GameLobby::LobbyType::FirstUserInLobbyUsers)
          {
            gameLobby->removeUser (notReadyUser);
          }
      }
    if (gameLobby->_users.empty ())
      {
        gameLobbies.erase (gameLobby);
      }
    else
      {
        gameLobby->readyUsers.clear ();
        gameLobby->sendToAllAccountsInGameLobby (objectToStringWithObjectName (shared_class::GameStartCanceled{}));
      }
  });
}

void
createGame (std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies, boost::asio::io_context &io_context)
{
  if (auto gameLobbyWithUser = ranges::find_if (gameLobbies,
                                                [accountName = user->accountName] (auto const &gameLobby) {
                                                  auto const &accountNames = gameLobby.accountNames ();
                                                  return ranges::find_if (accountNames, [&accountName] (auto const &nameToCheck) { return nameToCheck == accountName; }) != accountNames.end ();
                                                });
      gameLobbyWithUser != gameLobbies.end ())
    {
      if (gameLobbyWithUser->getWaitingForAnswerToStartGame ())
        {
          user->sendMessageToUser (objectToStringWithObjectName (shared_class::CreateGameError{ "It is not allowed to start a game while ask to start a game is running" }));
        }
      else
        {
          if (gameLobbyWithUser->isGameLobbyAdmin (user->accountName.value ()))
            {
              if (gameLobbyWithUser->accountNames ().size () >= 2)
                {
                  if (auto gameOptionError = errorInGameOption (gameLobbyWithUser->gameOption))
                    {
                      user->sendMessageToUser (objectToStringWithObjectName (gameOptionError.value ()));
                    }
                  else
                    {
                      askUsersToJoinGame (gameLobbyWithUser, gameLobbies, io_context);
                    }
                }
              else
                {
                  user->sendMessageToUser (objectToStringWithObjectName (shared_class::CreateGameError{ "You need atleast two user to create a game" }));
                }
            }
          else
            {
              user->sendMessageToUser (objectToStringWithObjectName (shared_class::CreateGameError{ "you need to be admin in a game lobby to start a game" }));
            }
        }
    }
  else
    {
      user->sendMessageToUser (objectToStringWithObjectName (shared_class::CreateGameError{ "Could not find a game lobby for the user" }));
    }
}

void
createGameLobby (std::string const &objectAsString, std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies)
{
  auto createGameLobbyObject = stringToObject<shared_class::CreateGameLobby> (objectAsString);
  if (ranges::find_if (gameLobbies, [gameLobbyName = createGameLobbyObject.name, lobbyPassword = createGameLobbyObject.password] (auto const &_gameLobby) { return _gameLobby.name && _gameLobby.name == gameLobbyName; }) == gameLobbies.end ())
    {
      if (auto gameLobbyWithUser = ranges::find_if (gameLobbies,
                                                    [accountName = user->accountName] (auto const &gameLobby) {
                                                      auto const &accountNames = gameLobby.accountNames ();
                                                      return ranges::find_if (accountNames, [&accountName] (auto const &nameToCheck) { return nameToCheck == accountName; }) != accountNames.end ();
                                                    });
          gameLobbyWithUser != gameLobbies.end ())
        {
          user->sendMessageToUser (objectToStringWithObjectName (shared_class::CreateGameLobbyError{ { "account has already a game lobby with the name: " + gameLobbyWithUser->name.value () } }));
          return;
        }
      else
        {
          auto &newGameLobby = gameLobbies.emplace_back (GameLobby{ createGameLobbyObject.name, createGameLobbyObject.password });
          if (newGameLobby.tryToAddUser (user))
            {
              throw std::logic_error{ "user can not join lobby which he created" };
            }
          else
            {
              auto result = std::vector<std::string>{};
              auto usersInGameLobby = shared_class::UsersInGameLobby{};
              usersInGameLobby.maxUserSize = newGameLobby.maxUserCount ();
              usersInGameLobby.name = newGameLobby.name.value ();
              usersInGameLobby.durakGameOption = newGameLobby.gameOption;
              ranges::transform (newGameLobby.accountNames (), ranges::back_inserter (usersInGameLobby.users), [] (auto const &accountName) { return shared_class::UserInGameLobby{ accountName }; });
              user->sendMessageToUser (objectToStringWithObjectName (shared_class::JoinGameLobbySuccess{}));
              user->sendMessageToUser (objectToStringWithObjectName (usersInGameLobby));
              return;
            }
        }
    }
  else
    {
      user->sendMessageToUser (objectToStringWithObjectName (shared_class::CreateGameLobbyError{ { "lobby already exists with name: " + createGameLobbyObject.name } }));
      return;
    }
}

void
joinGameLobby (std::string const &objectAsString, std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies)
{
  auto joinGameLobbyObject = stringToObject<shared_class::JoinGameLobby> (objectAsString);
  if (auto gameLobby = ranges::find_if (gameLobbies, [gameLobbyName = joinGameLobbyObject.name, lobbyPassword = joinGameLobbyObject.password] (auto const &_gameLobby) { return _gameLobby.name && _gameLobby.name == gameLobbyName && _gameLobby.password == lobbyPassword; }); gameLobby != gameLobbies.end ())
    {
      if (auto error = gameLobby->tryToAddUser (user))
        {
          user->sendMessageToUser (objectToStringWithObjectName (shared_class::JoinGameLobbyError{ joinGameLobbyObject.name, error.value () }));
          return;
        }
      else
        {
          user->sendMessageToUser (objectToStringWithObjectName (shared_class::JoinGameLobbySuccess{}));
          auto usersInGameLobby = shared_class::UsersInGameLobby{};
          usersInGameLobby.maxUserSize = gameLobby->maxUserCount ();
          usersInGameLobby.name = gameLobby->name.value ();
          usersInGameLobby.durakGameOption = gameLobby->gameOption;
          ranges::transform (gameLobby->accountNames (), ranges::back_inserter (usersInGameLobby.users), [] (auto const &accountName) { return shared_class::UserInGameLobby{ accountName }; });
          gameLobby->sendToAllAccountsInGameLobby (objectToStringWithObjectName (usersInGameLobby));
          return;
        }
    }
  else
    {
      user->sendMessageToUser (objectToStringWithObjectName (shared_class::JoinGameLobbyError{ joinGameLobbyObject.name, "wrong password name combination or lobby does not exists" }));
      return;
    }
}

void
setMaxUserSizeInCreateGameLobby (std::string const &objectAsString, std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies)
{
  auto setMaxUserSizeInCreateGameLobbyObject = stringToObject<shared_class::SetMaxUserSizeInCreateGameLobby> (objectAsString);
  auto accountNameToSearch = user->accountName.value ();
  if (auto gameLobbyWithAccount = ranges::find_if (gameLobbies,
                                                   [accountName = user->accountName] (auto const &gameLobby) {
                                                     auto const &accountNames = gameLobby.accountNames ();
                                                     return ranges::find_if (accountNames, [&accountName] (auto const &nameToCheck) { return nameToCheck == accountName; }) != accountNames.end ();
                                                   });
      gameLobbyWithAccount != gameLobbies.end ())
    {
      if (gameLobbyWithAccount->getWaitingForAnswerToStartGame ())
        {
          user->sendMessageToUser (objectToStringWithObjectName (shared_class::SetMaxUserSizeInCreateGameLobbyError{ "It is not allowed to change lobby while ask to start a game is running" }));
        }
      else
        {
          if (gameLobbyWithAccount->isGameLobbyAdmin (user->accountName.value ()))
            {
              if (auto errorMessage = gameLobbyWithAccount->setMaxUserCount (setMaxUserSizeInCreateGameLobbyObject.maxUserSize))
                {
                  user->sendMessageToUser (objectToStringWithObjectName (shared_class::SetMaxUserSizeInCreateGameLobbyError{ errorMessage.value () }));
                  return;
                }
              else
                {
                  gameLobbyWithAccount->sendToAllAccountsInGameLobby (objectToStringWithObjectName (shared_class::MaxUserSizeInCreateGameLobby{ setMaxUserSizeInCreateGameLobbyObject.maxUserSize }));
                  return;
                }
            }
          else
            {
              user->sendMessageToUser (objectToStringWithObjectName (shared_class::SetMaxUserSizeInCreateGameLobbyError{ "you need to be admin in a game lobby to change the user size" }));
              return;
            }
        }
    }
  else
    {
      user->sendMessageToUser (objectToStringWithObjectName (shared_class::SetMaxUserSizeInCreateGameLobbyError{ "could not find a game lobby for account" }));
      return;
    }
}

void
setGameOption (std::string const &objectAsString, std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies)
{
  auto gameOption = stringToObject<shared_class::GameOption> (objectAsString);
  auto accountNameToSearch = user->accountName.value ();
  if (auto gameLobbyWithAccount = ranges::find_if (gameLobbies,
                                                   [accountName = user->accountName] (auto const &gameLobby) {
                                                     auto const &accountNames = gameLobby.accountNames ();
                                                     return ranges::find_if (accountNames, [&accountName] (auto const &nameToCheck) { return nameToCheck == accountName; }) != accountNames.end ();
                                                   });
      gameLobbyWithAccount != gameLobbies.end ())
    {
      if (gameLobbyWithAccount->getWaitingForAnswerToStartGame ())
        {
          user->sendMessageToUser (objectToStringWithObjectName (shared_class::GameOptionError{ "It is not allowed to change game option while ask to start a game is running" }));
        }
      else
        {
          if (gameLobbyWithAccount->isGameLobbyAdmin (user->accountName.value ()))
            {
              gameLobbyWithAccount->gameOption = gameOption;
              gameLobbyWithAccount->sendToAllAccountsInGameLobby (objectToStringWithObjectName (gameOption));
              return;
            }
          else
            {
              user->sendMessageToUser (objectToStringWithObjectName (shared_class::GameOptionError{ "you need to be admin in the create game lobby to change game option" }));
              return;
            }
        }
    }
  else
    {
      user->sendMessageToUser (objectToStringWithObjectName (shared_class::GameOptionError{ "could not find a game lobby for account" }));
      return;
    }
}

void
leaveGameLobby (std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies)
{
  if (auto gameLobbyWithAccount = ranges::find_if (gameLobbies,
                                                   [accountName = user->accountName] (auto const &gameLobby) {
                                                     auto const &accountNames = gameLobby.accountNames ();
                                                     return ranges::find_if (accountNames, [&accountName] (auto const &nameToCheck) { return nameToCheck == accountName; }) != accountNames.end ();
                                                   });
      gameLobbyWithAccount != gameLobbies.end ())
    {
      if (gameLobbyWithAccount->lobbyAdminType == GameLobby::LobbyType::FirstUserInLobbyUsers)
        {
          gameLobbyWithAccount->removeUser (user);
          if (gameLobbyWithAccount->accountCount () == 0)
            {
              gameLobbies.erase (gameLobbyWithAccount);
            }
          user->sendMessageToUser (objectToStringWithObjectName (shared_class::LeaveGameLobbySuccess{}));
          return;
        }
      else
        {
          user->sendMessageToUser (objectToStringWithObjectName (shared_class::LeaveGameLobbyError{ "not allowed to leave a game lobby which is controlled by the matchmaking system with leave game lobby" }));
          return;
        }
    }
  else
    {
      user->sendMessageToUser (objectToStringWithObjectName (shared_class::LeaveGameLobbyError{ "could not remove user from lobby user not found in lobby" }));
      return;
    }
}

void
relogTo (std::string const &objectAsString, std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies, std::list<Game> &games)
{
  auto relogToObject = stringToObject<shared_class::RelogTo> (objectAsString);
  if (auto gameLobbyWithAccount = ranges::find_if (gameLobbies,
                                                   [accountName = user->accountName] (auto const &gameLobby) {
                                                     auto const &accountNames = gameLobby.accountNames ();
                                                     return ranges::find_if (accountNames, [&accountName] (auto const &nameToCheck) { return nameToCheck == accountName; }) != accountNames.end ();
                                                   });
      gameLobbyWithAccount != gameLobbies.end ())
    {
      if (relogToObject.wantsToRelog)
        {
          gameLobbyWithAccount->relogUser (user);
          user->sendMessageToUser (objectToStringWithObjectName (shared_class::RelogToCreateGameLobbySuccess{}));
          auto usersInGameLobby = shared_class::UsersInGameLobby{};
          usersInGameLobby.maxUserSize = gameLobbyWithAccount->maxUserCount ();
          usersInGameLobby.name = gameLobbyWithAccount->name.value ();
          usersInGameLobby.durakGameOption = gameLobbyWithAccount->gameOption;
          ranges::transform (gameLobbyWithAccount->accountNames (), ranges::back_inserter (usersInGameLobby.users), [] (auto const &accountName) { return shared_class::UserInGameLobby{ accountName }; });
          user->sendMessageToUser (objectToStringWithObjectName (usersInGameLobby));
          return;
        }
      else
        {
          gameLobbyWithAccount->removeUser (user);
          if (gameLobbyWithAccount->accountCount () == 0)
            {
              gameLobbies.erase (gameLobbyWithAccount);
            }
          else
            {
              auto usersInGameLobby = shared_class::UsersInGameLobby{};
              usersInGameLobby.maxUserSize = gameLobbyWithAccount->maxUserCount ();
              usersInGameLobby.name = gameLobbyWithAccount->name.value ();
              usersInGameLobby.durakGameOption = gameLobbyWithAccount->gameOption;
              ranges::transform (gameLobbyWithAccount->accountNames (), ranges::back_inserter (usersInGameLobby.users), [] (auto const &accountName) { return shared_class::UserInGameLobby{ accountName }; });
              gameLobbyWithAccount->sendToAllAccountsInGameLobby (objectToStringWithObjectName (usersInGameLobby));
              return;
            }
        }
    }
  else if (auto gameWithUser = ranges::find_if (games,
                                                [accountName = user->accountName] (auto const &game) {
                                                  auto accountNames = std::vector<std::string>{};
                                                  ranges::transform (game.users, ranges::back_inserter (accountNames), [] (std::shared_ptr<User> const &user) { return user->accountName.value (); });
                                                  return ranges::find_if (accountNames, [&accountName] (auto const &nameToCheck) { return nameToCheck == accountName; }) != accountNames.end ();
                                                });
           gameWithUser != games.end ())
    {
      if (relogToObject.wantsToRelog)
        {
          gameWithUser->relogUser (user);
        }
      else
        {
          gameWithUser->removeUser (user);
        }
    }
  else if (relogToObject.wantsToRelog)
    {
      user->sendMessageToUser (objectToStringWithObjectName (shared_class::RelogToError{ "trying to reconnect into game lobby but game lobby does not exist anymore" }));
      return;
    }
  return;
}

void
loginAccountCancel (std::shared_ptr<User> user)
{
  if (not user->accountName)
    {
      user->ignoreLogin = true;
    }
}

void
createAccountCancel (std::shared_ptr<User> user)
{
  if (not user->accountName)
    {
      user->ignoreCreateAccount = true;
    }
}

void
leaveGame (std::shared_ptr<User> user, std::list<Game> &games)
{
  if (auto game = ranges::find_if (games, [accountName = user->accountName.value ()] (Game const &_game) { return ranges::find_if (_game.users, [&accountName] (std::shared_ptr<User> const &user) { return user->accountName.value () == accountName; }) != _game.users.end (); }); game != games.end ())
    {
      game->removeUser (user);
      user->sendMessageToUser (objectToStringWithObjectName (shared_class::LeaveGameSuccess{}));
    }
  else
    {
      user->sendMessageToUser (objectToStringWithObjectName (shared_class::LeaveGameError{ "Could not find a game for Account Name: " + user->accountName.value () }));
    }
}
auto constexpr ALLOWED_DIFFERENCE_FOR_RANKED_GAME_MATCHMAKING = size_t{ 100 };
bool
isInRatingrange (size_t userRating, size_t lobbyAverageRating)
{
  auto const difference = userRating > lobbyAverageRating ? userRating - lobbyAverageRating : lobbyAverageRating - userRating;
  return difference < ALLOWED_DIFFERENCE_FOR_RANKED_GAME_MATCHMAKING;
}

bool
checkRating (size_t userRating, std::vector<std::string> const &accountNames)
{
  return isInRatingrange (userRating, averageRating (accountNames));
}

bool
matchingLobby (std::string const &accountName, GameLobby const &gameLobby, GameLobby::LobbyType const &lobbyType)
{
  if (gameLobby.lobbyAdminType == lobbyType && gameLobby._users.size () < gameLobby.maxUserCount ())
    {
      if (lobbyType == GameLobby::LobbyType::MatchMakingSystemRanked)
        {
          soci::session sql (soci::sqlite3, databaseName);
          if (auto userInDatabase = confu_soci::findStruct<database::Account> (sql, "accountName", accountName))
            {
              return checkRating (userInDatabase->rating, gameLobby.accountNames ());
            }
        }
      else
        {
          return true;
        }
    }
  else
    {
      return false;
    }
  return false;
}

void
joinMatchMakingQueue (std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies, boost::asio::io_context &io_context, GameLobby::LobbyType const &lobbyType)
{
  if (ranges::find_if (gameLobbies,
                       [accountName = user->accountName] (auto const &gameLobby) {
                         auto const &accountNames = gameLobby.accountNames ();
                         return ranges::find_if (accountNames, [&accountName] (auto const &nameToCheck) { return nameToCheck == accountName; }) != accountNames.end ();
                       })
      == gameLobbies.end ())
    {
      if (auto gameLobbyToAddUser = ranges::find_if (gameLobbies, [lobbyType, accountName = user->accountName.value ()] (GameLobby const &gameLobby) { return matchingLobby (accountName, gameLobby, lobbyType); }); gameLobbyToAddUser != gameLobbies.end ())
        {
          if (auto error = gameLobbyToAddUser->tryToAddUser (user))
            {
              user->sendMessageToUser (objectToStringWithObjectName (shared_class::JoinGameLobbyError{ user->accountName.value (), error.value () }));
            }
          else
            {
              user->sendMessageToUser (objectToStringWithObjectName (shared_class::JoinMatchMakingQueueSuccess{}));
              if (gameLobbyToAddUser->_users.size () == gameLobbyToAddUser->maxUserCount ())
                {
                  askUsersToJoinGame (gameLobbyToAddUser, gameLobbies, io_context);
                }
            }
        }
      else
        {
          auto gameLobby = GameLobby{};
          gameLobby.lobbyAdminType = lobbyType;
          if (auto error = gameLobby.tryToAddUser (user))
            {
              user->sendMessageToUser (objectToStringWithObjectName (shared_class::JoinGameLobbyError{ user->accountName.value (), error.value () }));
            }
          gameLobbies.emplace_back (gameLobby);
          user->sendMessageToUser (objectToStringWithObjectName (shared_class::JoinMatchMakingQueueSuccess{}));
        }
    }
  else
    {
      user->sendMessageToUser (objectToStringWithObjectName (shared_class::JoinMatchMakingQueueError{ "User is allready in gamelobby" }));
    }
}

void
wantsToJoinGame (std::string const &objectAsString, std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies, std::list<Game> &games)
{
  if (auto gameLobby = ranges::find_if (gameLobbies,
                                        [accountName = user->accountName] (auto const &gameLobby) {
                                          auto const &accountNames = gameLobby.accountNames ();
                                          return ranges::find_if (accountNames, [&accountName] (auto const &nameToCheck) { return nameToCheck == accountName; }) != accountNames.end ();
                                        });
      gameLobby != gameLobbies.end ())
    {
      if (stringToObject<shared_class::WantsToJoinGame> (objectAsString).answer)
        {
          if (ranges::find_if (gameLobby->readyUsers, [accountName = user->accountName.value ()] (std::shared_ptr<User> _user) { return _user->accountName == accountName; }) == gameLobby->readyUsers.end ())
            {
              gameLobby->readyUsers.push_back (user);
              if (gameLobby->readyUsers.size () == gameLobby->_users.size ())
                {
                  startGame (gameLobby, user, gameLobbies, games);
                }
            }
          else
            {
              user->sendMessageToUser (objectToStringWithObjectName (shared_class::WantsToJoinGameError{ "You already accepted to join the game" }));
            }
        }
      else
        {
          gameLobby->cancelTimer ();
          if (gameLobby->lobbyAdminType != GameLobby::LobbyType::FirstUserInLobbyUsers)
            {
              user->sendMessageToUser (objectToStringWithObjectName (shared_class::GameStartCanceledRemovedFromQueue{}));
              gameLobby->removeUser (user);
              if (gameLobby->_users.empty ())
                {
                  gameLobbies.erase (gameLobby);
                }
            }
        }
    }
  else
    {
      user->sendMessageToUser (objectToStringWithObjectName (shared_class::WantsToJoinGameError{ "No game to join" }));
    }
}

void
leaveMatchMakingQueue (std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies)
{
  if (auto gameLobby = ranges::find_if (gameLobbies,
                                        [accountName = user->accountName] (auto const &gameLobby) {
                                          auto const &accountNames = gameLobby.accountNames ();
                                          return gameLobby.lobbyAdminType != GameLobby::LobbyType::FirstUserInLobbyUsers && ranges::find_if (accountNames, [&accountName] (auto const &nameToCheck) { return nameToCheck == accountName; }) != accountNames.end ();
                                        });
      gameLobby != gameLobbies.end ())
    {
      user->sendMessageToUser (objectToStringWithObjectName (shared_class::LeaveQuickGameQueueSuccess{}));
      gameLobby->removeUser (user);
      gameLobby->cancelTimer ();
      if (gameLobby->_users.empty ())
        {
          gameLobbies.erase (gameLobby);
        }
    }
  else
    {
      user->sendMessageToUser (objectToStringWithObjectName (shared_class::LeaveQuickGameQueueError{ "User is not in queue" }));
    }
}

void
loginAsGuest (std::shared_ptr<User> user)
{
  if (not user->accountName)
    {
      user->accountName = to_string (boost::uuids::random_generator () ());
      user->sendMessageToUser (objectToStringWithObjectName (shared_class::LoginAsGuestSuccess{ user->accountName.value () }));
    }
}

void
removeUserFromLobbyAndGame (std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies, std::list<Game> &games)
{
  auto const findLobby = [userAccountName = user->accountName] (GameLobby const &gameLobby) {
    auto const &accountNamesToCheck = gameLobby.accountNames ();
    return ranges::find_if (accountNamesToCheck, [userAccountName] (std::string const &accountNameToCheck) { return userAccountName == accountNameToCheck; }) != accountNamesToCheck.end ();
  };
  auto gameLobbyWithUser = ranges::find_if (gameLobbies, findLobby);
  while (gameLobbyWithUser != gameLobbies.end ())
    {
      gameLobbyWithUser->removeUser (user);
      if (gameLobbyWithUser->_users.empty ())
        {
          gameLobbies.erase (gameLobbyWithUser);
        }
      gameLobbyWithUser = ranges::find_if (gameLobbies, findLobby);
    }
  auto const findGame = [userAccountName = user->accountName] (Game const &game) {
    auto const &gameUsers = game.users;
    return ranges::find_if (gameUsers, [userAccountName] (std::shared_ptr<User> const &user) { return userAccountName == user->accountName; }) != gameUsers.end ();
  };
  auto gameWithUser = ranges::find_if (games, findGame);
  while (gameWithUser != games.end ())
    {
      gameWithUser->removeUser (user);
      gameWithUser = ranges::find_if (games, findGame);
    }
}

void
logoutAccount (std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies, std::list<Game> &games)
{
  removeUserFromLobbyAndGame (user, gameLobbies, games);
  user->accountName = {};
  user->msgQueue.clear ();
  user->communicationChannels.clear ();
  user->ignoreLogin = false;
  user->ignoreCreateAccount = false;
  user->sendMessageToUser (objectToStringWithObjectName (shared_class::LogoutAccountSuccess{}));
}