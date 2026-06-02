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

// Parse tokens: extract redirect file (if any) and return clean args
// Returns the output filename, or "" if no redirect
string parseRedirect(vector<string>& tokens) {
    string outFile;
    vector<string> cleaned;
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == ">" || tokens[i] == "1>") {
            if (i + 1 < tokens.size()) {
                outFile = tokens[i + 1];
                i++; // skip filename too
            }
        } else {
            cleaned.push_back(tokens[i]);
        }
    }
    tokens = cleaned;
    return outFile;
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
        string outFile = parseRedirect(tokens);
        string cmd = tokens[0];

        if(cmd == "exit") break;

        int redirFd = -1;
        if (!outFile.empty()) {
            redirFd = open(outFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (redirFd < 0) {
                cerr << "cannot open " << outFile << " for writing" << endl;
                continue;
            }
        }

        if(cmd == "echo") {
            int savedStdout = -1;
            if (redirFd >= 0) {
                savedStdout = dup(STDOUT_FILENO);
                dup2(redirFd, STDOUT_FILENO);
                close(redirFd);
            }

            for(size_t i = 1; i < tokens.size(); i++) {
                if(i > 1) cout << " ";
                cout << tokens[i];
            }
            cout << endl;

            // Restore stdout
            if (savedStdout >= 0) {
                dup2(savedStdout, STDOUT_FILENO);
                close(savedStdout);
            }
            continue;
        }

        if(cmd == "cat") {
            int savedStdout = -1;
            if (redirFd >= 0) {
                savedStdout = dup(STDOUT_FILENO);
                dup2(redirFd, STDOUT_FILENO);
                close(redirFd);
            }

            for(size_t i = 1; i < tokens.size(); i++) {
                ifstream file(tokens[i]);
                if (!file.is_open()) {
                    // Error goes to stderr (not redirected) -- restore stdout first
                    if (savedStdout >= 0) dup2(savedStdout, STDOUT_FILENO);
                    cerr << "cat: " << tokens[i] << ": No such file or directory" << endl;
                    if (savedStdout >= 0) dup2(redirFd >= 0 ? redirFd : savedStdout, STDOUT_FILENO);
                    // Re-redirect for any remaining files
                    continue;
                }
                string line;
                while(getline(file, line)) cout << line << "\n";
            }
            cout << flush;

            if (savedStdout >= 0) {
                dup2(savedStdout, STDOUT_FILENO);
                close(savedStdout);
            }
            continue;
        }

        if(cmd == "pwd") {
            int savedStdout = -1;
            if (redirFd >= 0) {
                savedStdout = dup(STDOUT_FILENO);
                dup2(redirFd, STDOUT_FILENO);
                close(redirFd);
            }
            cout << fs::current_path().string() << endl;
            if (savedStdout >= 0) { dup2(savedStdout, STDOUT_FILENO); close(savedStdout); }
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
        if(pid == 0) {
            // In child: redirect stdout to file if needed
            if(redirFd >= 0) {
                dup2(redirFd, STDOUT_FILENO);
                close(redirFd);
            }
            execvp(args[0], args.data());
            cerr << tokens[0] << ": command not found" << endl;
            exit(1);
        } else {
            if(redirFd >= 0) close(redirFd);
            waitpid(pid, NULL, 0);
        }
    }
}