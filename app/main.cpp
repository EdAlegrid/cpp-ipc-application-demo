/*
 * File:   main.ccp
 * Author: Ed Alegrid
 *
 * Use any Linux C++11 compliant compiler or IDE.
 *
 */

#include <string>  
#include <memory>
#include <iostream>
#include <nlohmann/json.hpp>
#include "tcp/server.h"

using namespace std;
using json = nlohmann::json;

int main()
{
  cout << "\n*** Remote C/C++ Application ***\n" << endl;

  int port = 5300;
  auto server = make_shared<Tcp::Server>(port);

  for (;;)
  {
      // listen for new client connection
      // set socket listener to true for continous loop
      server->socketListen(true);

      // generate random number 1 ~ 100
      int rn = rand() % 100 + 1;

      // read data from client
      auto data = server->socketRead();
      cout << "rcvd client data: " << data << endl;

      // parse received json data 
      try{
        auto j = json::parse(data);
       
        if(j["type"] == "random"){
          j["value"] = rn;
          cout << "send back json data: " << server->socketWrite(j.dump()) << endl; 
        }
        else{
          cout << "send back invalid json data: " << server-> socketWrite("invalid json data") << endl; 
        }
      }
      catch (json::parse_error& ex)
      {
        cerr << "parse error at byte " << ex.byte << endl;
        cout << "send back string data: " << server->socketWrite(data) << endl; 
      }
      
      cout << "waiting for client data ...\n\n";
      server->socketClose();
  }

  return 0;
}
