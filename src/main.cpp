#include<iostream>
#include<string>
#include<sstream>
#include<unistd.h>
#include<cstdlib>
#include<vector>
#include<sys/wait.h>
#include<filesystem>

using namespace std;
namespace fs =filesystem;

int main() {
  // Flush after every std::cout / std:cerr
  cout << unitbuf;
  cerr << unitbuf;
  string command;
  // TODO: Uncomment the code below to pass the first stage
  while(1){
    cout << "$ ";
    getline(cin, command);
    
    
    if(command=="exit"){
      break;
    }


    if(command.rfind("echo ",0)==0){
      string msg= command.substr(5);
      int ss=msg.size();
      if(!msg.empty()&& msg.front() == '\''&& msg.back() == '\''){
        string squote=msg.substr(1, ss-2);
        cout<<squote<<endl;
        continue;
      }
      if(!msg.empty()&& msg.front() == '\"'&& msg.back() == '\"'){
        string squote=msg.substr(1, ss-2);
        cout<<squote<<endl;
        continue;
      }

      stringstream ss(msg);
      string word;
      bool first = true;

      while(ss >> word){
          if(!first){
              cout << " ";
          }
          cout << word;
          first = false;
      }

      cout << endl;
      continue;
    }
    
    
    if(command=="pwd"){
      cout<<fs::current_path().string() <<endl;
      continue;
    }

    if(command.rfind("cd ",0)==0){
      string chd= command.substr(3);
      if(chd=="~"){
        const char* homedir=getenv("HOME");
        fs::current_path(homedir);
        continue;
      }
      try
      {
        fs::current_path(chd);
      }
      catch(const fs::filesystem_error& e)
      {
        cerr << "cd: " << chd << ": No such file or directory" << endl;
      }
      continue;
    }
    
    
    if(command.rfind("type ",0)==0){
      string msg = command.substr(5);

      if(msg=="echo" || msg=="exit" || msg=="type"|| msg=="pwd"|| msg=="cd"){
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
    
    vector<string> tokens;
    stringstream ss(command);
    string word;

    while (ss >> word) {
      tokens.push_back(word);
    }

    // Convert to char* array
    vector<char*> args;
    for (auto& token : tokens) {
      args.push_back(token.data());
    }
    args.push_back(nullptr);

    // Create child process
    pid_t pid = fork();

    if (pid == 0) {
    // Child process executes command
    execvp(args[0], args.data());

    // If exec fails
    cout << tokens[0] << ": command not found" << endl;
    exit(1);
    } else {
      // Parent waits
      waitpid(pid, NULL, 0);
    }
  }
}
