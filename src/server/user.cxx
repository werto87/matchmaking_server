#include "src/server/user.hxx"
#include "server.hxx"
#include <iostream>

using namespace boost::beast;
using namespace boost::asio;

awaitable<void>
User::writeToClient (std::weak_ptr<SSLWebsocket> &connection)
{
  try
    {
      while (not connection.expired ())
        {
          timer = std::make_shared<CoroTimer> (CoroTimer{ co_await this_coro::executor });
          timer->expires_after (std::chrono::system_clock::time_point::max () - std::chrono::system_clock::now ());
          try
            {
              co_await timer->async_wait ();
            }
          catch (boost::system::system_error &e)
            {
              using namespace boost::system::errc;
              if (operation_canceled == e.code ())
                {
                  // swallow cancel
                }
              else
                {
                  std::cout << "error in timer boost::system::errc: " << e.code () << std::endl;
                  abort ();
                }
            }
          while (not connection.expired () && not msgQueue.empty ())
            {
              auto tmpMsg = std::move (msgQueue.front ());
              std::cout << " msg: " << tmpMsg << std::endl;
              msgQueue.pop_front ();
              co_await connection.lock ()->async_write (buffer (tmpMsg), use_awaitable);
            }
        }
    }
  catch (std::exception &e)
    {
      std::cout << "write Exception: " << e.what () << std::endl;
    }
}
void
User::sendMessageToUser (std::string const &message)
{
  msgQueue.push_back (message);
  if (timer) timer->cancel ();
}