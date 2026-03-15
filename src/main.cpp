#include <iostream>
#include <string>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  std::string command;
  // TODO: Uncomment the code below to pass the first stage
  std::cout << "$ ";
  std::cin>>command;
  std::cout<<command<<" : command not found";
}
