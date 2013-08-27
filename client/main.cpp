#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/System.hpp>

#include <string>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <sstream>

#include "../common/const.hpp"
#include "../common/bitmap.hpp"
#include "const.hpp"

enum GameState
{
  GSAwaitingTurn,
  GSDrawing
};

bool sayHello( sf::TcpSocket& server )
{
  sf::Packet packet;
  bool blocking = server.isBlocking();
  if( !blocking ) server.setBlocking( true );
  packet << NET_MESSAGE_HELLO << NET_VERSION;
  bool success = packet && server.send( packet ) == sf::Socket::Done;
  if( !blocking ) server.setBlocking( false );
  return success;
}

bool startTurn( sf::TcpSocket& server )
{
  sf::Packet packet;
  bool blocking = server.isBlocking();
  if( !blocking ) server.setBlocking( true );
  packet << NET_MESSAGE_STARTING_TURN;
  bool success = packet && server.send( packet ) == sf::Socket::Done;
  if( !blocking ) server.setBlocking( false );
  return success;
}

bool sendGoodbyes( sf::TcpSocket& server )
{
  sf::Packet packet;
  bool blocking = server.isBlocking();
  if( !blocking ) server.setBlocking( true );
  packet << NET_MESSAGE_BYE;
  bool success = packet && server.send( packet ) == sf::Socket::Done;
  if( !blocking ) server.setBlocking( false );
  return success;
}

bool sendTurn( sf::TcpSocket& server, const Bitmap& bitmap )
{
  sf::Packet packet;
  bool blocking = server.isBlocking();
  if( !blocking ) server.setBlocking( true );
  packet << NET_MESSAGE_TURN_DONE << bitmap;
  bool success = packet && server.send( packet ) == sf::Socket::Done;
  if( !blocking ) server.setBlocking( false );
  return success;
}

bool connect( int argc, char** argv, sf::TcpSocket& out_socket )
{
  std::string serverStr = "mrwonko.dyndns.org";
  if( argc > 1 )
  {
    serverStr = argv[1];
  }
  unsigned short port;
  if( argc > 2 )
  {
    std::stringstream ss;
    ss << argv[2];
    ss >> port;
  }
  else
  {
    port = 14792;
  }
  std::cout << "Trying to connect to " << serverStr << ":" << port << "... (Change IP/Port via command line arguments - " << argv[0] << " <ip> <port>)" << std::endl;
  sf::Socket::Status status = out_socket.connect( sf::IpAddress( serverStr ), port, sf::seconds(5.f) );
  return status == sf::Socket::Done;
}

void drawLine( sf::Image& image, sf::Vector2i p1, sf::Vector2i p2, const sf::Color& color )
{
  int diffX = std::abs( p1.x - p2.x );
  int diffY = std::abs( p1.y - p2.y );
  if( diffX > diffY )
  {
    sf::Vector2i& start = p1.x < p2.x ? p1 : p2;
    sf::Vector2i& end = p1.x < p2.x ? p2 : p1;
    int endX = std::min( end.x, (int)image.getSize().x - 1 );
    for( int x = std::max( start.x, 0 ); x <= endX; ++x )
    {
      int y = (int)(start.y + (float) (x - start.x) * ( end.y - start.y ) / diffX + 0.5f);
      if( y >= 0 && y < (int)image.getSize().y )
      {
        image.setPixel( x, y, color );
      }
    }
  }
  else
  {
    sf::Vector2i& start = p1.y < p2.y ? p1 : p2;
    sf::Vector2i& end = p1.y < p2.y ? p2 : p1;
    int endY = std::min( end.y, (int)image.getSize().y - 1 );
    for( int y = std::max(start.y, 0); y <= endY; ++y )
    {
      int x = (int)(start.x + (float) (y - start.y) * ( end.x - start.x ) / diffY + 0.5f);
      if( x >= 0 && x < (int)image.getSize().x )
      {
        image.setPixel( x, y, color );
      }
    }
  }
}

int main( int argc, char** argv )
{
  // Connect to server
  sf::TcpSocket server;
  if( !connect( argc, argv, server ) )
  {
    std::cout << "Could not connect to server." << std::endl;
    return 0;
  }
  std::cout << "Connected." << std::endl;
  if( !sayHello( server ) )
  {
    std::cout << "Could not send greetings." << std::endl;
    return 0;
  }
  sf::Packet packet;
  std::cout << "Awaiting server answer." << std::endl;
  if( server.receive( packet ) != sf::Socket::Done )
  {
    std::cout << "Server did not answer!" << std::endl;
    return 0;
  }
  std::string messageType;
  if( !( packet >> messageType ) )
  {
    std::cout << "Invalid message received!" << std::endl;
    sendGoodbyes( server );
    return 0;
  }
  if( messageType == NET_MESSAGE_REJECT_CLIENT )
  {
    std::cout << "Server rejected us." << std::endl;
    return 0;
  }
  if( messageType != NET_MESSAGE_ACCEPT_CLIENT )
  {
    std::cout << "Invalid message received!" << std::endl;
    sendGoodbyes( server );
    return 0;
  }
  std::cout << "Server accepted us!" << std::endl;
  // Connected and ready, apparently.
  server.setBlocking( false );

  // Create Window
  sf::RenderWindow wnd(
    sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT),
    WINDOW_TITLE,
    sf::Style::Close,
    sf::ContextSettings(0, 0, 0, 2, 0)
    );


  static const sf::Color backgroundColor( 63, 63, 63 );

  sf::Image img;
  img.create( 480, 320 );
  GameState gameState = GSAwaitingTurn;
  sf::Texture tex;
  tex.loadFromImage(img);
  sf::Sprite sprite(tex);
  sprite.setPosition(OFS_X, OFS_Y);
  sf::Clock clock;
  float countdown = 0.f;
  sf::Int32 queuePos = -1;
  bool useWhite = true;

  // I do to much Lua... local function handleWindowEvents()
  struct
  {
    sf::Window& wnd;
    GameState &gameState;
    sf::Texture &tex;
    sf::Image &img;

    sf::Vector2i lastMousePos;
    bool& useWhite;

    void operator()()
    {
      static const sf::Vector2i offset( OFS_X, OFS_Y );

      sf::Event ev;
      bool painted = false;
      while( wnd.pollEvent( ev ) )
      {
        if( ev.type == sf::Event::Closed || ( ev.type == sf::Event::KeyPressed && ev.key.code == sf::Keyboard::Escape ) )
        {
          wnd.close();
        }
        else if( ev.type == sf::Event::MouseMoved )
        {
          sf::Vector2i mousePos( ev.mouseMove.x, ev.mouseMove.y );
          if( lastMousePos.x != -1 && gameState == GSDrawing )
          {
            drawLine( img, lastMousePos - offset, mousePos - offset, useWhite ? sf::Color::White : sf::Color::Black );
            painted = true;
            lastMousePos = mousePos;
          }
        }
        else if( ev.type == sf::Event::MouseButtonPressed && ev.mouseButton.button == sf::Mouse::Left )
        {
          lastMousePos.x = ev.mouseButton.x;
          lastMousePos.y = ev.mouseButton.y;
          if( gameState == GSDrawing )
          {
            drawLine( img, lastMousePos - offset, lastMousePos - offset, useWhite ? sf::Color::White : sf::Color::Black );
            painted = true;
          }
        }
        else if( ev.type == sf::Event::MouseButtonReleased && ev.mouseButton.button == sf::Mouse::Left )
        {
          if( lastMousePos.x != -1 && gameState == GSDrawing )
          {
            sf::Vector2i mousePos( ev.mouseButton.x, ev.mouseButton.y );
            drawLine( img, lastMousePos - offset, mousePos - offset, useWhite ? sf::Color::White : sf::Color::Black );
            painted = true;
          }
          lastMousePos.x = lastMousePos.y = -1;
        }
        else if( ev.type == sf::Event::KeyPressed || ev.type == sf::Event::MouseButtonPressed )
        {
          useWhite = !useWhite;
        }
      }
      if( painted )
      {
        tex.loadFromImage( img );
      }
    }
  } handleWindowEvents = { wnd, gameState, tex, img, sf::Vector2i( -1, -1 ), useWhite };

  sf::Font font;
  if( !font.loadFromFile( FONT_PATH ) )
  {
    std::cout << "Could not load Font: " << FONT_PATH << std::endl;
    return false;
  }

  sf::Text statusText;
  statusText.setFont( font );
  statusText.setString( STATUS_WAIT );
  statusText.setPosition( 0, OFS_Y + IMAGE_HEIGHT );

  sf::Text colorText;
  colorText.setFont( font );
  colorText.setString( COLOR_TEXT );
  colorText.setPosition( WINDOW_WIDTH * 2 / 3, OFS_Y + IMAGE_HEIGHT );

  sf::RectangleShape colorRect( sf::Vector2f( OFS_X / 2, OFS_X / 2 ) );
  colorRect.setPosition( WINDOW_WIDTH * 2 / 3 + colorText.getLocalBounds().width + 8, OFS_Y  + IMAGE_HEIGHT );
  colorRect.setOutlineColor( sf::Color( 127, 127, 127 ) );
  colorRect.setOutlineThickness( - 2.f );

  // Main loop
  while( wnd.isOpen() )
  {
    // Network
    
    packet.clear();
    sf::Socket::Status status; 
    while( (status = server.receive( packet ) ) != sf::Socket::NotReady )
    {
      if( status != sf::Socket::Done )
      {
        std::cout << "Could not receive packet!" << std::endl;
        return 0;
      }
      if( !( packet >> messageType ) )
      {
        std::cout << "Invalid message received!" << std::endl;
        return 0;
      }
      if( messageType == NET_MESSAGE_BYE )
      {
        std::cout << "Server signing off." << std::endl;
        return 0;
      }
      else if( messageType == NET_MESSAGE_START_TURN )
      {
        std::cout << "Our turn!" << std::endl;
        if( !startTurn( server ) )
        {
          std::cout << "Could not send packet!" << std::endl;
          return 0;
        }
        countdown = TURN_TIME;
        clock.restart();
        gameState = GSDrawing;
        statusText.setString( STATUS_DRAW );
      }
      else if( messageType == NET_MESSAGE_TURN_ACCEPTED )
      {
        std::cout << "Turn accepted!" << std::endl;
      }
      else if( messageType == NET_MESSAGE_TURN_OVERTIME )
      {
        std::cout << "We were too slow to respond. Monster ping?" << std::endl;
        gameState = GSAwaitingTurn;
      }
      else if( messageType == NET_MESSAGE_SYNC )
      {
        std::cout << "Sync..." << std::endl;
        packet >> countdown;
        Bitmap bitmap;
        packet >> bitmap;
        bitmap.toImage( img );
        tex.loadFromImage( img );
      }
      else if( messageType == NET_MESSAGE_QUEUE_INFO )
      {
        packet >> queuePos;
      }
    }

    // Local logic
    countdown = std::max( 0.f, countdown - clock.getElapsedTime().asSeconds() );
    clock.restart();

    if( countdown == 0.f && gameState == GSDrawing )
    {
      // Time's up!
      Bitmap bitmap;
      bitmap.fromImage( img );
      if( !sendTurn( server, bitmap ) )
      {
        std::cout << "Could not send turn to server!" << std::endl;
        return 0;
      }
      gameState = GSAwaitingTurn;
      statusText.setString( STATUS_WAIT );
    }

    handleWindowEvents();

    wnd.clear( backgroundColor );

    {
      std::stringstream ss;
      ss << "Time: ";
      ss << std::max( 0, (int)( countdown + 0.5f ) );
      sf::Text timeText;
      timeText.setString( ss.str() );
      timeText.setFont( font );
      wnd.draw( timeText );
    }
    {
      std::stringstream ss;
      ss << "Queue: ";
      ss << queuePos;
      sf::Text queueText;
      queueText.setString( ss.str() );
      queueText.setPosition( WINDOW_WIDTH  / 2 , 0 );
      queueText.setFont( font );
      wnd.draw( queueText );
    }
    wnd.draw( colorText );
    colorRect.setFillColor( useWhite ? sf::Color::White : sf::Color::Black );
    wnd.draw( colorRect );
    wnd.draw( statusText );

    wnd.draw( sprite );
    wnd.display();
  }
  sendGoodbyes( server );

  return 0;
}
