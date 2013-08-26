#include "bitmap.hpp"

#include <SFML/Network/Packet.hpp>

#include <cstring>

Bitmap::Bitmap()
{
  memset(m_data[0], 0, sizeof(m_data));
}

sf::Packet& operator>>( sf::Packet& packet, Bitmap& bitmap )
{
  std::string data;
  packet >> data;
  if( packet && data.size() == sizeof(bitmap.m_data) )
  {
    memcpy(bitmap.m_data[0], data.c_str(), data.size());
  }
  else
  {
    // No way to signal failure?
  }
  return packet;
}

sf::Packet& operator<<( sf::Packet& packet, const Bitmap& bitmap )
{
  std::string data( bitmap.m_data[0], sizeof(bitmap.m_data) );
  return packet << data;
}

