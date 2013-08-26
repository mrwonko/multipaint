#include "../common/bitmap.hpp"

#include <SFML/Graphics/Image.hpp>

void Bitmap::toImage(sf::Image& image)
{
  image.create( IMAGE_WIDTH, IMAGE_HEIGHT );
  for( unsigned int y = 0; y < IMAGE_HEIGHT; ++y )
  {
    for( unsigned int x = 0; x < IMAGE_WIDTH; ++x )
    {
      if( m_data[y][x / 8] & (1 << (7 - x % 8) ) )
      {
        image.setPixel(x, y, sf::Color::White);
      }
    }
  }
}

const bool Bitmap::fromImage(sf::Image& image) const
{
  if( image.getSize() != sf::Vector2u(IMAGE_WIDTH, IMAGE_HEIGHT ) )
  {
    return false;
  }
  for( unsigned int y = 0; y < IMAGE_HEIGHT; ++y )
  {
    char curChar = 0;
    for( unsigned int x = 0; x < IMAGE_WIDTH; ++x )
    {
      sf::Color& color = image.getPixel(x, y);
      const bool isBlack = ( color.r / 3 + color.g / 3 + color.b / 3 ) < 128;
      if( !isBlack )
      {
        curChar |= 1 << ( 7 - x % 8 );
      }
      if( x % 8 == 7 || x + 1 == IMAGE_WIDTH )
      {
        m_data[y][x / 8] = curChar;
        curChar = 0;
      }
    }
  }
  return true;
}
