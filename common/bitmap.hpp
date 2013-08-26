#ifndef COMMON_IMAGE_HPP_INCLUDED
#define COMMON_IMAGE_HPP_INCLUDED

#include <climits>

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
  
  /** @brief Loads the bitmap from a network packet. */
  const bool fromPacket(sf::Packet& packet);

  /** @brief Writes the bitmap to a network packet. */
  void toPacket(sf::Packet& packet);

  const bool fromImage(sf::Image& image);
  void toImage(sf::Image& image);

private:
  char m_data[ IMAGE_HEIGHT ][ IMAGE_WIDTH / CHAR_BIT + !!( IMAGE_WIDTH % CHAR_BIT ) ];
};

#endif
