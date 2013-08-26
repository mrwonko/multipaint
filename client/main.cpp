#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

#include <string>
#include <iostream>
#include <algorithm>
#include <cstdlib>

#include "../common/const.hpp"
#include "const.hpp"

enum GameState
{
  GSAwaitingTurn,
  GSDrawing
};

bool setup( int argc, char** argv, sf::TcpSocket& out_socket, std::string& out_error )
{
  // TODO
  return true;
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
    for( int x = start.x; x <= endX; ++x )
    {
      int y = (int)(start.y + (float) (x - start.x) * ( end.y - start.y ) / diffX + 0.5f);
      if( y >= 0 && y < image.getSize().y )
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
      if( x >= 0 && x < image.getSize().x )
      {
        image.setPixel( x, y, color );
      }
    }
  }
}

void handleWindowEvents( sf::Window& wnd, const GameState gameState )
{
  sf::Event ev;
  while(wnd.pollEvent(ev))
  {
    if(ev.type == sf::Event::Closed)
    {
      wnd.close();
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

  GameState gameState = GSAwaitingTurn;

  static const sf::Color backgroundColor( 63, 63, 63 );

  sf::Image img;
  img.create( 480, 320 );
  drawLine(img, sf::Vector2i(20, 20), sf::Vector2i(30, 25), sf::Color::White);
  drawLine(img, sf::Vector2i(20, 20), sf::Vector2i(25, 30), sf::Color::Green);
  drawLine(img, sf::Vector2i(40, 40), sf::Vector2i(35, 30), sf::Color::Blue);
  drawLine(img, sf::Vector2i(40, 40), sf::Vector2i(30, 35), sf::Color::Yellow);
  drawLine(img, sf::Vector2i(40, 40), sf::Vector2i(50, 35), sf::Color::Red);
  drawLine(img, sf::Vector2i(40, 40), sf::Vector2i(45, 30), sf::Color::Magenta);
  
  drawLine(img, sf::Vector2i(200, 100), sf::Vector2i(400, 200), sf::Color::White);
  drawLine(img, sf::Vector2i(400, 200), sf::Vector2i(300, 400), sf::Color::White);
  drawLine(img, sf::Vector2i(300, 400), sf::Vector2i(100, 300), sf::Color::White);
  drawLine(img, sf::Vector2i(100, 300), sf::Vector2i(200, 100), sf::Color::White);

  sf::Texture tex;
  tex.loadFromImage(img);
  sf::Sprite sprite(tex);
  sprite.setPosition(80, 80);

  // Main loop
  while( wnd.isOpen() )
  {
    handleWindowEvents( wnd, gameState );

    wnd.clear( backgroundColor );
    wnd.draw( sprite );
    wnd.display();
  }

  return 0;
}
