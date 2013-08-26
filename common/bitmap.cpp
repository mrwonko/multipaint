#include "bitmap.hpp"

#include <SFML/Network/Packet.hpp>

#include <cstring>

Bitmap::Bitmap()
{
  memset(m_data[0], 0, sizeof(m_data));
}

const bool Bitmap::fromPacket(sf::Packet& packet)
{

  std::string data;
  packet >> data;
  if( !packet || data.size() != sizeof(m_data) )
  {
    return false;
  }
  else
  {
    memcpy(m_data[0], data.c_str(), data.size());
    return true;
  }
}

void Bitmap::toPacket(sf::Packet& packet)
{
  std::string data( m_data[0], sizeof(m_data) );
  packet << data;
}
