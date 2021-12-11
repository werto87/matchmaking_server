#ifndef E18680A5_3B06_4019_A849_6CDB82D14796
#define E18680A5_3B06_4019_A849_6CDB82D14796
#include "../server/gameLobby.hxx"
#include "../server/user.hxx"
#include "src/game/game.hxx"
#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <game_01_shared_class/serialization.hxx>
#include <string>
#include <vector>

boost::asio::awaitable<void> handleMessage (std::string const &msg, boost::asio::io_context &io_context, boost::asio::thread_pool &pool, std::list<std::shared_ptr<User>> &users, std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies, std::list<Game> &games);
boost::asio::awaitable<void> createAccountAndLogin (std::string objectAsString, boost::asio::io_context &io_context, std::shared_ptr<User> user, boost::asio::thread_pool &pool, std::list<GameLobby> &gameLobbies, std::list<Game> &games);
boost::asio::awaitable<void> loginAccount (std::string objectAsString, boost::asio::io_context &io_context, std::list<std::shared_ptr<User>> &users, std::shared_ptr<User> user, boost::asio::thread_pool &pool, std::list<GameLobby> &gameLobbies, std::list<Game> &games);
void logoutAccount (std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies, std::list<Game> &games);
void broadCastMessage (std::string const &objectAsString, std::list<std::shared_ptr<User>> &users, User &sendingUser);
void joinChannel (std::string const &objectAsString, std::shared_ptr<User> user);
void leaveChannel (std::string const &objectAsString, std::shared_ptr<User> user);
void createGameLobby (std::string const &objectAsString, std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies);
void joinGameLobby (std::string const &objectAsString, std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies);
void setMaxUserSizeInCreateGameLobby (std::string const &objectAsString, std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies);
void setGameOption (std::string const &objectAsString, std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies);
void leaveGameLobby (std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies);
void relogTo (std::string const &objectAsString, std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies, std::list<Game> &games);
void createGame (std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies, boost::asio::io_context &io_context);
void leaveGame (std::shared_ptr<User> user, std::list<Game> &games);
void loginAccountCancel (std::shared_ptr<User> user);
void createAccountCancel (std::shared_ptr<User> user);
void joinMatchMakingQueue (std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies, boost::asio::io_context &io_context, GameLobby::LobbyType const &lobbyType);
void wantsToJoinGame (std::string const &objectAsString, std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies, std::list<Game> &games);
void leaveMatchMakingQueue (std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies);
void loginAsGuest (std::shared_ptr<User> user);
void removeUserFromLobbyAndGame (std::shared_ptr<User> user, std::list<GameLobby> &gameLobbies, std::list<Game> &games);

#endif /* E18680A5_3B06_4019_A849_6CDB82D14796 */
