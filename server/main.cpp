#include "../common/bitmap.hpp"
#include "../common/const.hpp"
#include "const.hpp"

#include <SFML/Network/TcpListener.hpp>
#include <SFML/Network/TcpSocket.hpp>
#include <SFML/Network/SocketSelector.hpp>
#include <SFML/Network/Packet.hpp>
#include <SFML/Network/IpAddress.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>

#include <sstream>
#include <list>
#include <csignal>
#include <iostream>
#include <algorithm>
#include <cassert>

static bool g_exit = false;

void interruptHandler( int signal )
{
  g_exit = true;
  std::cout << "Request to terminate received, will shut down shortly." << std::endl;
}

const bool sync( sf::TcpSocket& client, const Bitmap& bitmap, const sf::Clock& turnTime )
{
  sf::Packet packet;
  packet << NET_MESSAGE_SYNC << std::max( 0.f, TURN_TIME - turnTime.getElapsedTime().asSeconds() ) << bitmap;
  return packet && client.send( packet ) == sf::Socket::Done;
}

void syncAll( std::list< sf::TcpSocket* >& clients, const Bitmap& bitmap, const sf::Clock& turnTime )
{
  for( std::list< sf::TcpSocket* >::iterator it = clients.begin(); it != clients.end(); ++it )
  {
    // okay let's not drop anybody during sync (i.e. ignore failure), because I don't know if that might cause any bugs and have no time to think about it.
    sync( **it, bitmap, turnTime );
  }
}

const bool startTurn( sf::TcpSocket& client )
{
  sf::Packet packet;
  packet << NET_MESSAGE_START_TURN;
  return packet && client.send( packet ) == sf::Socket::Done;
}

const bool acceptClient( sf::TcpSocket& client )
{
  sf::Packet packet;
  packet << NET_MESSAGE_ACCEPT_CLIENT;
  return packet && client.send( packet ) == sf::Socket::Done;
}

const bool rejectClient( sf::TcpSocket& client )
{
  sf::Packet packet;
  packet << NET_MESSAGE_REJECT_CLIENT;
  return packet && client.send( packet ) == sf::Socket::Done;
}

const bool sendQueueInfo( sf::TcpSocket& client, sf::Uint32 queueSlot )
{
  sf::Packet packet;
  packet << NET_MESSAGE_QUEUE_INFO << queueSlot;
  return packet && client.send( packet ) == sf::Socket::Done;
}

const bool notifySlacker( sf::TcpSocket& client ) // i.e. don't bother handing in your turn, too slow.
{
  sf::Packet packet;
  packet << NET_MESSAGE_TURN_OVERTIME;
  return packet && client.send( packet ) == sf::Socket::Done;
}

const bool acceptTurn( sf::TcpSocket& client )
{
  sf::Packet packet;
  packet << NET_MESSAGE_TURN_ACCEPTED;
  return packet && client.send( packet ) == sf::Socket::Done;
}

const bool sendGoodbyes( sf::TcpSocket& client )
{
  sf::Packet packet;
  packet << NET_MESSAGE_BYE;
  return packet && client.send( packet ) == sf::Socket::Done;
}

int main(int argc, char** argv)
{
  unsigned short port;
  if( argc > 1 )
  {
    std::stringstream ss;
    ss << argv[1];
    ss >> port;
  }
  else
  {
    port = 14792;

    std::cout << "Using port 14792, different port can be supplied as a command line argument." << std::endl;
  }

  sf::TcpListener listener;
  listener.listen( port );

  std::list< sf::TcpSocket* > newClients; // not yet validated
  std::list< sf::TcpSocket* > playingClients; // validated

  sf::SocketSelector selector;
  selector.add( listener );

  signal( SIGINT, &interruptHandler );

  std::cout << "Starting server, ctrl+c (SIGINT) to exit." << std::endl;

  sf::Clock turnTime;

  enum State
  {
    SAwaitingClients,
    SAwaitingTurnStartConfirmation,
    SAwaitingResults
  };

  State state = SAwaitingClients;
  Bitmap bitmap;

  while( !g_exit )
  {
    float waitTime = 1.f; // How long to listen on sockets each tick - too long makes the server unresponsive (to SIGINT)

    if( ( state == SAwaitingResults && TURN_TIME + TURN_TIME_EXTRA - turnTime.getElapsedTime().asSeconds() < 0 ) 
      || ( state == SAwaitingTurnStartConfirmation && TURN_CONFIRM_TIME - turnTime.getElapsedTime().asSeconds() < 0 )
      )
    {
      // Client took too long
      // Throw him out
      sf::TcpSocket &client = *playingClients.front();
      notifySlacker( client );
      sendGoodbyes( client );
      client.disconnect();
      delete &client;
      playingClients.pop_front();

      // Find the next one who will answer, tell him to get busy
      while( !playingClients.empty() )
      {
        sf::TcpSocket &client = *playingClients.front();
        state = SAwaitingTurnStartConfirmation;
        if( !startTurn( client ) )
        {
          client.disconnect();
          delete &client;
          playingClients.pop_front();
        }
        else
        {
          turnTime.restart();
          syncAll( playingClients, bitmap, turnTime );
          break;
        }
      }
      // No clients left?
      if( playingClients.empty() )
      {
        state = SAwaitingClients;
      }
    }
    else
    {
      float timeToHandIn = waitTime;
      if( state == SAwaitingResults )
      {
          timeToHandIn = TURN_TIME + TURN_TIME_EXTRA - turnTime.getElapsedTime().asSeconds();
      }
      else if( state == SAwaitingTurnStartConfirmation )
      {
        timeToHandIn = TURN_CONFIRM_TIME - turnTime.getElapsedTime().asSeconds();
      }
      waitTime = std::min( timeToHandIn, waitTime );
    }


    if( selector.wait( sf::seconds( waitTime ) ) )
    {
      if( selector.isReady( listener ) )
      {
        sf::TcpSocket *client = new sf::TcpSocket;
        client->setBlocking( false );
        if( listener.accept( *client ) == sf::Socket::Done )
        {
          newClients.push_back( client );
          selector.add( *client );

          std::cout << "Someone is connecting from " << client->getRemoteAddress().toString() << ":" << client->getRemotePort() << std::endl;
        }
        else
        {
          delete client;
        }
      }
      for( std::list< sf::TcpSocket* >::iterator it = newClients.begin(); it != newClients.end(); )
      {
        bool erased = false;
        sf::TcpSocket &client = **it;
        struct
        {
          bool& erased;
          sf::TcpSocket &client;
          std::list< sf::TcpSocket* >::iterator& it;
          std::list< sf::TcpSocket* >& clients;
          void operator()()
          {
            client.disconnect();
            delete *it;
            it = clients.erase( it );
            erased = true;
          }
        } dropClient = { erased, client, it, newClients };

        if( selector.isReady( client ) )
        {
          sf::Packet packet;
          sf::Socket::Status status = client.receive( packet );
          if( status == sf::Socket::Done )
          {
            std::string messageType;
            if( !( packet >> messageType ) || messageType != NET_MESSAGE_HELLO )
            {
              std::cout << "Client " << client.getRemoteAddress().toString() << ":" << client.getRemotePort() << " does not appear to be a MultiPaint client, dropping." << std::endl;
              dropClient();
            }
            else
            {
              sf::Uint32 version;
              if( packet >> version )
              {
                if( version != NET_VERSION )
                {
                  std::cout << "Client " << client.getRemoteAddress().toString() << ":" << client.getRemotePort() << " uses invalid protocol version " << version << ", server uses " << NET_VERSION << ". Dropping." << std::endl;
                  rejectClient( client );
                  dropClient();
                }
                else
                {
                  if( !acceptClient( client ) )
                  {
                    std::cout << "Error sending message to client " << client.getRemoteAddress().toString() << ":" << client.getRemotePort() << ", dropping." << std::endl;
                    dropClient();
                  }
                  else
                  {
                    std::cout << "Client " << client.getRemoteAddress().toString() << ":" << client.getRemotePort() << " is valid, adding to queue." << std::endl;
                    it = newClients.erase( it );
                    erased = true;
                    
                    if( playingClients.empty() )
                    {
                      turnTime.restart();
                    }
                    if( sync( client, bitmap, turnTime ) )
                    {
                      if( playingClients.empty() )
                      {
                        if( !startTurn( client ) )
                        {
                          client.disconnect();
                          delete &client;
                        }
                        else
                        {
                          state = SAwaitingTurnStartConfirmation;
                          playingClients.push_back( &client );
                        }
                      }
                      else
                      {
                        playingClients.push_back( &client );
                      }
                    }
                    else
                    {
                      std::cout << "Error synchronizing client " << client.getRemoteAddress().toString() << ":" << client.getRemotePort() << ", dropping." << std::endl;
                      delete &client;
                    }
                  }
                }
              }
            }
          }
        }
        if( !erased ) // a List::erase() already moves the iterator.
        {
          ++it;
        }
      }

      bool first = true;
      for( std::list< sf::TcpSocket* >::iterator it = playingClients.begin(); it != playingClients.end(); )
      {
        bool erased = false;
        sf::TcpSocket &client = **it;
        struct
        {
          bool& erased;
          sf::TcpSocket &client;
          std::list< sf::TcpSocket* >::iterator& it;
          std::list< sf::TcpSocket* >& clients;
          State& state;
          const Bitmap& bitmap;
          sf::Clock& turnTime;
          bool& first;
          void operator()()
          {
            client.disconnect();
            delete *it;
            it = clients.erase( it );

            // Was this the one whose answer we were awaiting?
            if( first )
            {
              // Find the next one who will answer, tell him to get busy
              while( it != clients.end() )
              {
                sf::TcpSocket &client = **it;
                if( !startTurn( client ) )
                {
                  // Couldn't send "start" message, probably disconnected or something.
                  client.disconnect();
                  delete &client;
                  it = clients.erase( it );
                }
                else
                {
                  // Player is aware he's supposed to do stuff.
                  state = SAwaitingTurnStartConfirmation;
                  turnTime.restart();
                  syncAll( clients, bitmap, turnTime );
                  break;
                }
              }
            }
            // No clients left?
            if( clients.empty() )
            {
              state = SAwaitingClients;
            }

            erased = true;
          }
        } dropClient = { erased, client, it, playingClients, state, bitmap, turnTime, first };

        assert( state != SAwaitingClients ); // Because then there shouldn't be clients!
        
        if( selector.isReady( client ) )
        {
          sf::Packet packet;
          if( client.receive(packet) != sf::Socket::Done )
          {
            // Could not receive packet
            std::cout << "Dropping " << client.getRemoteAddress().toString() << ":" << client.getRemotePort() << std::endl;
            sendGoodbyes( client );
            dropClient();
          }
          else
          {
            std::string messageType;
            if( !( packet >> messageType ) )
            {
              // Packet did not contain a string first
              std::cout << "Invalid message from " << client.getRemoteAddress().toString() << ":" << client.getRemotePort() << ", dropping." << std::endl;
              sendGoodbyes( client );
              dropClient();
            }
            else
            {
              // OK, what've we got?
              if( messageType == NET_MESSAGE_BYE )
              {
                dropClient();
              }
              else if( messageType == NET_MESSAGE_STARTING_TURN )
              {
                if( first && state == SAwaitingTurnStartConfirmation )
                {
                  state = SAwaitingResults;
                  turnTime.restart();
                }
              }
              else if( messageType == NET_MESSAGE_TURN_DONE )
              {
                if( first && state == SAwaitingResults )
                {
                  if( !( packet >> bitmap ) )
                  {
                    std::cout << "Invalid Bitmap from " << client.getRemoteAddress().toString() << ":" << client.getRemotePort() << ", dropping." << std::endl;
                    sendGoodbyes( client );
                    dropClient();
                  }
                  else
                  {
                    // Player handed in new Bitmap, is done now. Next one please.

                    // Put him back to the end of the queue, remove from the front
                    playingClients.push_back( &client );
                    it = playingClients.erase( it );
                    erased = true;

                    // Find the next one who will answer, tell him to get busy
                    while( it != playingClients.end() )
                    {
                      sf::TcpSocket &client = **it;
                      if( !startTurn( client ) )
                      {
                        // Couldn't send "start" message, probably disconnected or something.
                        client.disconnect();
                        delete &client;
                        it = playingClients.erase( it );
                      }
                      else
                      {
                        // Player is aware he's supposed to do stuff.
                        state = SAwaitingTurnStartConfirmation;
                        turnTime.restart();
                        syncAll( playingClients, bitmap, turnTime );
                        break;
                      }
                    }
                    if( it == playingClients.end() )
                    {
                      assert( playingClients.empty() );
                      state = SAwaitingClients;
                    }
                  }
                }
              }
            }
          }
        }

        if( !erased ) // a List::erase() already moves the iterator.
        {
          ++it;
        }
        first = false;
      }
    }
  }

  for( std::list< sf::TcpSocket* >::iterator it = newClients.begin(); it != newClients.end(); it = newClients.erase( it ) )
  {
    sendGoodbyes( **it );
    delete *it;
  }

  for( std::list< sf::TcpSocket* >::iterator it = playingClients.begin(); it != playingClients.end(); it = playingClients.erase(it) )
  {
    sendGoodbyes( **it );
    delete *it;
  }

  return 0;
}
