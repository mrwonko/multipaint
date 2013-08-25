#include <SFML/Graphics.hpp>

int main(int argc, char** argv)
{
  sf::RenderWindow wnd(sf::VideoMode(480, 320), "MultiPaint", sf::Style::Close, sf::ContextSettings(0, 0, 0, 2, 0));
  while(wnd.isOpen())
  {
    sf::Event ev;
    while(wnd.pollEvent(ev))
    {
      if(ev.type == sf::Event::Closed)
      {
        wnd.close();
      }
    }

    wnd.clear();
    wnd.display();
  }

  return 0;
}
