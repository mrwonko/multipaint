#include "bitmap.hpp"

#include <SFML/Network/Packet.hpp>
#include <SFML/Graphics/Image.hpp>

#include <fstream>
#include <cstring>
#include <iostream>
#include <sstream>

Bitmap::Bitmap()
{
  memset(m_data[0], 0, sizeof(m_data));
}

void Bitmap::toImage(sf::Image& image) const
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

const bool Bitmap::fromImage(sf::Image& image)
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

static void skipComment( std::istream& is )
{
  int c = 0;
  while( c != EOF && c != '\n' && c != '\r' )
  {
    c = is.get();
  }
}

static const bool isWhitespace( const int c )
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static void skipWhitespaceAndComments( std::istream& is )
{
  int c;
  while( isWhitespace( c = is.get() ) || c == '#' )
  {
    if( c == '#' )
    {
      skipComment( is );
    }
  }
  is.unget();
}

static std::string readWord( std::istream& is )
{
  std::string word;
  int c;
  while( !isWhitespace( c = is.get() ) && c != EOF )
  {
    if( c == '#' )
    {
      skipComment( is );
    }
    else
    {
      word += (char) c;
    }
  }
  is.unget(); // unread whitespace/EOF
  return word;
}

const bool Bitmap::loadFromFile( const std::string& filename )
{
  // Netpbm loader - http://netpbm.sourceforge.net/doc/pbm.html

  std::ifstream is( filename.c_str(), std::ios::binary );
  if( !is )
  {
    std::cout << "Could not open " << filename << std::endl;
    return false;
  }
  if( readWord( is ) != "P4" )
  {
    std::cout << filename << " is no Netpbm (P4) image!" << std::endl;
    return false;
  }
  skipWhitespaceAndComments( is );
  std::string widthString = readWord( is );
  skipWhitespaceAndComments( is );
  std::string heightString = readWord( is );
  if( widthString.empty() )
  {
    std::cout << filename << " contains no width!" << std::endl;
    return false;
  }
  if( heightString.empty() )
  {
    std::cout << filename << " contains no height!" << std::endl;
    return false;
  }
  {
    std::stringstream ss;
    ss << widthString;
    unsigned int width;
    ss >> width;
    if( width != IMAGE_WIDTH )
    {
      std::cout << filename << " has an invalid width (" << widthString << "), allowed: " << IMAGE_WIDTH << std::endl;
      return false;
    }
  }
  {
    std::stringstream ss;
    ss << heightString;
    unsigned int height;
    ss >> height;
    if( height != IMAGE_HEIGHT )
    {
      std::cout << filename << " has an invalid height (" << heightString << "), allowed: " << IMAGE_HEIGHT << std::endl;
      return false;
    }
  }
  if( !isWhitespace( is.get() ) )
  {
    std::cout << filename << " is invalid: image height not followed by whitespace!" << std::endl;
    return false;
  }
  is.read( m_data[0], sizeof( m_data ) );
  if( !is )
  {
    std::cout << "Error reading " << filename << " (unexpected EOF?)" << std::endl;
    return false;
  }
  std::cout << "Loaded " << filename << std::endl;
  return true;
}

const bool Bitmap::saveToFile( const std::string& filename ) const
{
  std::ofstream os( filename.c_str() );
  os << "P4\n" << IMAGE_WIDTH << " " << IMAGE_HEIGHT << "\n";
  os.write( m_data[0], sizeof( m_data ) );
  os.close();
  return !os.fail();
}

