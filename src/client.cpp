//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include "controlMessage.pb.h"

using boost::asio::ip::tcp;
std::string port_num = "12312";
std::string nomePrimoStream = "primo stream";
int main(int argc, char* argv[]) {
  controlMessage::StreamName first;
  first.set_streamname("primo stream");
  std::stringstream st;
  std::ostream* output = &st;
  std::string s;
  if (first.SerializeToOstream(output)) {
    s = st.str();
    try {
      s = s.substr(2,s.length()-2);
    } catch (const std::out_of_range& e) {
      std::cout << "Il messaggio deve essere di lunghezza maggiore di 1\n";
    }
    //std::cout << s << std::endl;
  }

  try {
    if (argc != 2) {
      std::cerr << "Usage: client <host>" << std::endl;
      return 1;
    }

    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    tcp::resolver::query query(argv[1], port_num);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    tcp::socket socket(io_service);
    boost::asio::connect(socket, endpoint_iterator);

    for (;;)
      {
	std::size_t written = socket.write_some(boost::asio::buffer(s));
	std::cout << "Written " << written << std::endl;

	boost::array<char, 128> buf;
	boost::system::error_code error;

	size_t len = socket.read_some(boost::asio::buffer(buf), error);

	if (error == boost::asio::error::eof)
	  break; // Connection closed cleanly by peer.
	else if (error)
	  throw boost::system::system_error(error); // Some other error.

	std::cout << "Received: " << std::string(buf.data(), len) << std::endl;
      }
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
