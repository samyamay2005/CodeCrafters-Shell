#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <cstdlib>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  std::string command;
  // TODO: Uncomment the code below to pass the first stage
  while(1){
    std::cout << "$ ";
    std::getline(std::cin, command);
    if(command=="exit"){
      break;
    }
    if(command.rfind("echo ",0)==0){
      std::string msg= command.substr(5);
      std::cout<<msg<<std::endl;
      continue;
    }
    if(command.rfind("type ",0)==0){
    std::string msg = command.substr(5);

    if(msg=="echo" || msg=="exit" || msg=="type"){
    std::cout<<msg<<" is a shell builtin"<<std::endl;
    continue;
    }

  char* pathEnv = getenv("PATH");
  std::string pathStr = pathEnv;
  std::stringstream ss(pathStr);
  std::string dir;

  while(std::getline(ss, dir, ':')){
    std::string fullPath = dir + "/" + msg;

    if(access(fullPath.c_str(), X_OK) == 0){
      std::cout<<msg<<" is "<<fullPath<<std::endl;
      goto found;
    }
  }

  std::cout<<msg<<": not found"<<std::endl;

  found:
  continue;
}
    std::cout<<command<<": command not found"<<std::endl;
  }
}
