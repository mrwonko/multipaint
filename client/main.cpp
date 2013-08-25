#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

#include <string>
#include <iostream>

#include "../common/const.hpp"
#include "const.hpp"

enum GameState
{
  GSAwaitingTurn,
  GSDrawing
};

bool setup(int argc, char** argv, sf::TcpSocket& out_socket, std::string& out_error)
{
  // TODO
  return true;
}

void handleWindowEvents(sf::Window& wnd, const GameState gameState)
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

int main(int argc, char** argv)
{
  // Connect to server
  sf::TcpSocket socket;
  {
    std::string error;
    if(!setup(argc, argv, socket, error))
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

  // Main loop
  while(wnd.isOpen())
  {
    handleWindowEvents(wnd, gameState);

    wnd.clear();
    wnd.display();
  }

  return 0;
}
