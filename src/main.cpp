#include<iostream>
#include<string>
#include<sstream>
#include<unistd.h>
#include<cstdlib>
#include<vector>
#include<sys/wait.h>
#include<filesystem>
#include<fstream>
#include<fcntl.h>   // <-- add this for open()

using namespace std;
namespace fs = filesystem;

vector<string> tokenize(const string& input) {
    vector<string> tokens;
    string current;
    bool inSingleQuote = false;
    bool isDoubleQuote = false;
    bool escaped = false;

    for(char c : input) {
        if(escaped) {
            escaped = false;
            if(isDoubleQuote){
                if(c=='"' || c=='\\') current+=c;
                else { current+='\\'; current+=c; }
            } else {
                current += c;
            }
            continue;
        }
        if(c == '\\' && !inSingleQuote){
          escaped = true;
          continue; 
        }
        if(c == '\'' && !isDoubleQuote){
          inSingleQuote = !inSingleQuote;
          continue;
        }
        if(c == '"' && !inSingleQuote){
          isDoubleQuote = !isDoubleQuote;
          continue;
        }
        if(c == ' ' && !inSingleQuote && !isDoubleQuote) {
            if(!current.empty()){
              tokens.push_back(current);
              current.clear();
            }
        } else {
            current += c;
        }
    }
    if(!current.empty())
      tokens.push_back(current);
    return tokens;
}

struct redirectInfo{
  string stdoutFile;
  string stderrFile;
};

// Parse tokens: extract redirect file (if any) and return clean args
// Returns the output filename, or "" if no redirect
redirectInfo parseRedirect(vector<string>& tokens) {
    redirectInfo info;
    string outFile;
    vector<string> cleaned;
    for (size_t i = 0; i < tokens.size(); i++) {
        if ((tokens[i] == ">" || tokens[i] == "1>") && i + 1 < tokens.size()) {
            info.stdoutFile = tokens[++i];
        } else if (tokens[i] == "2>" && i + 1 < tokens.size()) {
            info.stderrFile = tokens[++i];
        } else {
            cleaned.push_back(tokens[i]);
        }
    }
    tokens = cleaned;
    return info;
}

int main() {
    cout << unitbuf;
    cerr << unitbuf;
    string command;

    while(1) {
        cout << "$ ";
        getline(cin, command);

        vector<string> tokens = tokenize(command);
        if(tokens.empty()) continue;

        // Parse out any redirection FIRST, before dispatch
        redirectInfo redir = parseRedirect(tokens);
        string cmd = tokens[0];

        if(cmd == "exit") break;

        // int redirFd = -1;
        int stdoutFd = -1;
        if (!redir.stdoutFile.empty()) {
          stdoutFd = open(redir.stdoutFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
          if (stdoutFd < 0) {
            cerr << "cannot open " << redir.stdoutFile << endl; continue;
          }
        }

        int stderrFd = -1;
        if (!redir.stderrFile.empty()) {
          stderrFd = open(redir.stderrFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
          if (stderrFd < 0) { cerr << "cannot open " << redir.stderrFile << endl; continue; }
        }


      if(cmd == "echo") {
        int savedStdout = -1, savedStderr = -1;
        if (stdoutFd >= 0) { savedStdout = dup(STDOUT_FILENO); dup2(stdoutFd, STDOUT_FILENO); close(stdoutFd); }
        if (stderrFd >= 0) { savedStderr = dup(STDERR_FILENO); dup2(stderrFd, STDERR_FILENO); close(stderrFd); }

        for(size_t i = 1; i < tokens.size(); i++) {
          if(i > 1) cout << " ";
            cout << tokens[i];
        }
        cout << endl;

        if (savedStdout >= 0) { dup2(savedStdout, STDOUT_FILENO); close(savedStdout); }
        if (savedStderr >= 0) { dup2(savedStderr, STDERR_FILENO); close(savedStderr); }
        continue;
      }

      if(cmd == "cat") {
        int savedStdout = -1, savedStderr = -1;
        if (stdoutFd >= 0) { savedStdout = dup(STDOUT_FILENO); dup2(stdoutFd, STDOUT_FILENO); close(stdoutFd); }
        if (stderrFd >= 0) { savedStderr = dup(STDERR_FILENO); dup2(stderrFd, STDERR_FILENO); close(stderrFd); }

        for(size_t i = 1; i < tokens.size(); i++) {
          ifstream file(tokens[i]);
          if(!file.is_open()) {
            cerr << "cat: " << tokens[i] << ": No such file or directory" << endl;
            continue;
          }
          cout << file.rdbuf();
        }
        cout << flush;

        if (savedStdout >= 0) { dup2(savedStdout, STDOUT_FILENO); close(savedStdout); }
        if (savedStderr >= 0) { dup2(savedStderr, STDERR_FILENO); close(savedStderr); }
        continue;
      }

      if(cmd == "pwd") {
        int savedStdout = -1, savedStderr = -1;
        if (stdoutFd >= 0) { savedStdout = dup(STDOUT_FILENO); dup2(stdoutFd, STDOUT_FILENO); close(stdoutFd); }
        if (stderrFd >= 0) { savedStderr = dup(STDERR_FILENO); dup2(stderrFd, STDERR_FILENO); close(stderrFd); }

        cout << fs::current_path().string() << endl;

        if (savedStdout >= 0) { dup2(savedStdout, STDOUT_FILENO); close(savedStdout); }
        if (savedStderr >= 0) { dup2(savedStderr, STDERR_FILENO); close(savedStderr); }
        continue;
      }

        if(cmd == "cd") {
            if(tokens.size() < 2) continue;
            string chd = tokens[1];
            if(chd == "~") { fs::current_path(getenv("HOME")); continue; }
            try { fs::current_path(chd); }
            catch(const fs::filesystem_error&) {
                cerr << "cd: " << chd << ": No such file or directory" << endl;
            }
            continue;
        }

        if(cmd == "type") {
            if(tokens.size() < 2) continue;
            string msg = tokens[1];
            if(msg=="echo"||msg=="exit"||msg=="type"||msg=="pwd"||msg=="cd") {
                cout << msg << " is a shell builtin" << endl;
                continue;
            }
            char* pathEnv = getenv("PATH");
            string pathStr = pathEnv;
            stringstream ss(pathStr);
            string dir;
            bool found = false;
            while(getline(ss, dir, ':')) {
                string fullPath = dir + "/" + msg;
                if(access(fullPath.c_str(), X_OK) == 0) {
                    cout << msg << " is " << fullPath << endl;
                    found = true; break;
                }
            }
            if(!found) cout << msg << ": not found" << endl;
            continue;
        }

        vector<char*> args;
        for(auto& t : tokens) args.push_back(t.data());
        args.push_back(nullptr);

        pid_t pid = fork();
        if (pid == 0) {
          if (stdoutFd >= 0) { dup2(stdoutFd, STDOUT_FILENO); close(stdoutFd); }
          if (stderrFd >= 0) { dup2(stderrFd, STDERR_FILENO); close(stderrFd); }
          execvp(args[0], args.data());
          cerr << tokens[0] << ": command not found" << endl;
          exit(1);
        }else {
          if (stdoutFd >= 0) close(stdoutFd);
          if (stderrFd >= 0) close(stderrFd);
          waitpid(pid, NULL, 0);
        }
    }
}