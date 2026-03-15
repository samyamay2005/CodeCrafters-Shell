#include <iostream>
#include <string>

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

    }
    std::cout<<command<<": command not found"<<std::endl;
  }
}
