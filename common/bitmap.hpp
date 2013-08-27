#ifndef COMMON_BITMAP_HPP_INCLUDED
#define COMMON_BITMAP_HPP_INCLUDED

#include <climits>
#include <string>

#include "const.hpp"

namespace sf
{
  class Packet;
  class Image;
}

class Bitmap
{
public:
  /** @brief Creates a black bitmap. */
  Bitmap();

  const bool fromImage(sf::Image& image);
  void toImage(sf::Image& image) const;

  const bool loadFromFile( const std::string& filename );
  const bool saveToFile( const std::string& filename ) const;

private:
  char m_data[ IMAGE_HEIGHT ][ IMAGE_WIDTH / CHAR_BIT + !!( IMAGE_WIDTH % CHAR_BIT ) ];

  friend sf::Packet& operator>>( sf::Packet&, Bitmap& );
  friend sf::Packet& operator<<( sf::Packet&, const Bitmap& );
};

/** @brief Loads the bitmap from a network packet. */
sf::Packet& operator>>( sf::Packet& packet, Bitmap& bitmap );

/** @brief Writes the bitmap to a network packet. */
sf::Packet& operator<<( sf::Packet& packet, const Bitmap& bitmap );

#endif
