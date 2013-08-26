#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/System.hpp>

#include <string>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <sstream>

#include "../common/const.hpp"
#include "const.hpp"

enum GameState
{
  GSAwaitingTurn,
  GSDrawing
};

bool setup( int argc, char** argv, sf::TcpSocket& out_socket, std::string& out_error )
{
  return true;
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
  sf::TcpSocket socket;
  {
    std::string error;
    if( !setup( argc, argv, socket, error ) )
    {
      std::cout << error << std::endl;
      return 0;
    }
  }

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
  sf::Clock roundClock;

  // I do to much Lua... local function handleWindowEvents()
  struct
  {
    sf::Window& wnd;
    GameState &gameState;
    sf::Texture &tex;
    sf::Image &img;

    sf::Vector2i lastMousePos;
    bool useWhite;

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
  } handleWindowEvents = { wnd, gameState, tex, img, sf::Vector2i( -1, -1 ), true };

  // TODO REMOVE
  gameState = GSDrawing;

  // Main loop
  while( wnd.isOpen() )
  {
    handleWindowEvents();

    wnd.clear( backgroundColor );
    wnd.draw( sprite );
    wnd.display();
  }
  // TODO: Send BYE

  return 0;
}
