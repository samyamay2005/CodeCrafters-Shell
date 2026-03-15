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
      continue;
    }
    if(command.rfind("type ",0)==0){
      std::string msg= command.substr(5);
      if(msg=="echo" || msg=="exit" || msg=="type"){
        std::cout<<msg<<" is a shell bulletin"<<std::endl;
        continue;
      }
      else{
        std::cout<<msg<<": not found"<<std::endl;
        continue;
      }
      continue;
    }
    std::cout<<command<<": command not found"<<std::endl;
  }
}
