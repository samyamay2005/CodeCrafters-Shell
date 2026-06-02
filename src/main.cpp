#include<iostream>
#include<string>
#include<sstream>
#include<unistd.h>
#include<cstdlib>
#include<vector>
#include<sys/wait.h>
#include<filesystem>
#include<fstream>

using namespace std;
namespace fs =filesystem;


vector<string> tokenize(const string& input) {
    vector<string> tokens;
    string current;
    bool inSingleQuote = false;
    bool isDoubleQuote = false;
    bool escaped = false;

    for(char c : input) {
        if(escaped) {
          current += c;
          escaped = false;
          continue;
        }

        if(c == '\\' && !inSingleQuote && !isDoubleQuote) {
          escaped = true;
          continue;
        }

        
        if(c == '\'' && !isDoubleQuote) {
          inSingleQuote = !inSingleQuote;
          continue;
        }

        
        if(c =='"' && !inSingleQuote){
          isDoubleQuote= !isDoubleQuote;
          continue;
        }
        
        
        if(c == ' ' && !inSingleQuote && !isDoubleQuote) {
            if(!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        }
        else {
            current += c;
        }
    }

    if(!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

int main() {
  // Flush after every std::cout / std:cerr
  cout << unitbuf;
  cerr << unitbuf;
  string command;
  // TODO: Uncomment the code below to pass the first stage
  while(1){
    cout << "$ ";
    getline(cin, command);
    
    vector<string> tokens= tokenize(command);

    if(tokens.empty()){
      continue;
    }

    string cmd= tokens[0];

    if(cmd=="exit"){
      break;
    }


    if(cmd=="echo"){

      for(size_t i = 1; i < tokens.size(); i++){
          if(i > 1){
              cout << " ";
          }
          cout << tokens[i];
      }

      cout << endl;
      continue;
    }
    

    if(cmd=="cat"){

      for(size_t i = 1; i < tokens.size(); i++) {

        ifstream file(tokens[i]);

        string line;
        while(getline(file, line)) {
          cout << line;
        }
    }

    cout << endl;
    continue;
    }

    if (cmd == "pwd") {
      cout << fs::current_path().string() << endl;
      continue;
    }

    if (cmd=="cd") {
      if(tokens.size()<2)
        continue;

      string chd = tokens[1];
      
      if (chd == "~") {
        const char *homedir = getenv("HOME");
        fs::current_path(homedir);
        continue;
      }
      
      try {
        fs::current_path(chd);
      } catch (const fs::filesystem_error &e) {
        cerr << "cd: " << chd << ": No such file or directory" << endl;
      }
      continue;
    }
    if(cmd == "type"){
      string msg = command.substr(5);

      if(msg=="echo" || msg=="exit" || msg=="type"|| msg=="pwd"|| msg=="cd" ){
        cout<<msg<<" is a shell builtin"<<endl;
        continue;
      }
      

      char* pathEnv = getenv("PATH");
      string pathStr = pathEnv;
      stringstream ss(pathStr);
      string dir;

      while(getline(ss, dir, ':')){
        string fullPath = dir + "/" + msg;

        if(access(fullPath.c_str(), X_OK) == 0){
          cout<<msg<<" is "<<fullPath<<endl;
          goto found;
        }
      }

      cout<<msg<<": not found"<<endl;

      found:
      continue;
    }
    
    vector<string> token_list= tokenize(command);

    // Convert to char* array
    vector<char*> args;
    for (auto& token : token_list) {
      args.push_back(token.data());
    }
    args.push_back(nullptr);

    // Create child process
    pid_t pid = fork();

    if (pid == 0) {
    // Child process executes command
    execvp(args[0], args.data());

    // If exec fails
    cout << token_list[0] << ": command not found" << endl;
    exit(1);
    } else {
      // Parent waits
      waitpid(pid, NULL, 0);
    }
  }
}
