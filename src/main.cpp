#include<iostream>
#include<string>
#include<sstream>
#include<unistd.h>
#include<cstdlib>
#include<vector>
#include<sys/wait.h>
#include<filesystem>
#include<fstream>
#include<fcntl.h>
#include<algorithm>
#include<readline/readline.h>
#include<readline/history.h>
#include<array>

using namespace std;
namespace fs = filesystem;
vector<string> builtins={"echo","cd","pwd","exit", "type", "history"};
int appendOffset = 0;
// Returns true if cmd was a builtin and was handled
bool runBuiltin(vector<string>& tokens) {
        string cmd = tokens[0];
        if (cmd == "echo") {
            for (size_t i = 1; i < tokens.size(); i++) {
                if (i > 1) cout << " ";
                cout << tokens[i];
            }
            cout << endl;
            return true;
        }
        if (cmd == "type") {
            if (tokens.size() < 2) return true;
            string msg = tokens[1];
            if (msg=="echo"||msg=="exit"||msg=="type"||msg=="pwd"||msg=="cd"||msg=="history") {
                cout << msg << " is a shell builtin" << endl;
                return true;
            }
            char* pathEnv = getenv("PATH");
            string pathstr(pathEnv);
            stringstream ss(pathstr);
            string dir; 
            bool found = false;
            while(getline(ss, dir, ':')) {
                string fullPath = dir + "/" + msg;
                if (access(fullPath.c_str(), X_OK) == 0) {
                    cout << msg << " is " << fullPath << endl;
                    found = true; break;
                }
            }
            if (!found) cout << msg << ": not found" << endl;
            return true;
        }
        if (cmd == "pwd") {
            cout << fs::current_path().string() << endl;
            return true;
        }
        if (cmd == "history") {
            if (tokens.size() >= 3 && tokens[1] == "-r") {
                read_history(tokens[2].c_str());
                return true;
            }
            if (tokens.size() >= 3 && tokens[1] == "-w") {
                write_history(tokens[2].c_str());
                return true;
            }
            if (tokens.size() >= 3 && tokens[1] == "-a") {
                append_history(history_length - appendOffset, tokens[2].c_str());
                appendOffset = history_length;
                return true;
            }

        HIST_ENTRY** hist = history_list();
        int total = history_length;
        int start = 0;
        if (tokens.size() >= 2) {
            int n = atoi(tokens[1].c_str());
            start = max(0, total - n);
        }
        if (hist) {
            for (int i = start; i < total; i++) {
                cout << "    " << (i + 1) << "  " << hist[i]->line << endl;
                }
            }
            return true;
        } 
    // cd doesn't make sense in a pipeline child, but handle gracefully
    return false;
}

void execCommand(vector<string>& tokens, int inFd, int outFd) {
    if (inFd != STDIN_FILENO) { dup2(inFd, STDIN_FILENO); close(inFd); }
    if (outFd != STDOUT_FILENO) { dup2(outFd, STDOUT_FILENO); close(outFd); }

    if (runBuiltin(tokens)) {
        cout.flush();   // <-- add this
        exit(0);
    }

    vector<char*> args;
    for (auto& t : tokens) args.push_back(t.data());
    args.push_back(nullptr);
    execvp(args[0], args.data());
    cerr << tokens[0] << ": command not found" << endl;
    exit(1);
}

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
  bool stdoutAppend = false;
  bool stderrAppend = false;
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
            info.stdoutAppend = false;
        } else if ((tokens[i] == ">>" || tokens[i] == "1>>") && i + 1 < tokens.size()){
            info.stdoutFile = tokens[++i];
            info.stdoutAppend = true;
        } else if (tokens[i] == "2>" && i + 1 < tokens.size()) {
            info.stderrFile = tokens[++i];
            info.stderrAppend = false;
        } else if(tokens[i] == "2>>" && i + 1 < tokens.size()){
            info.stderrFile = tokens[++i];
            info.stderrAppend =true;
        } else {
            cleaned.push_back(tokens[i]);
        }
    }
    tokens = cleaned;
    return info;
}


char* completionGenerator(const char* text, int state) {
    static vector<string> matches;
    static size_t matchIndex;

    if (state == 0) {
        matches.clear();
        matchIndex = 0;
        string prefix(text);

        // Match builtins
        for (auto& b : builtins) {
            if (b.substr(0, prefix.size()) == prefix)
                matches.push_back(b);
        }

        // Match executables in PATH
        char* pathEnv = getenv("PATH");
        if (pathEnv) {
            stringstream ss(pathEnv);
            string dir;
            while (getline(ss, dir, ':')) {
                try {
                    for (auto& entry : fs::directory_iterator(dir)) {
                        string name = entry.path().filename().string();
                        if (name.substr(0, prefix.size()) == prefix) {
                            // Avoid duplicates
                            if (find(matches.begin(), matches.end(), name) == matches.end())
                                matches.push_back(name);
                        }
                    }
                } catch (...) {}  // Skip unreadable dirs
            }
        }
    }

    if (matchIndex < matches.size())
        return strdup(matches[matchIndex++].c_str());

    return nullptr;
}

char** completionCallback(const char* text, int start, int end) {
    // Only complete the first word (the command)
    if (start == 0)
        return rl_completion_matches(text, completionGenerator);
    
    // For arguments, fall back to filename completion (readline default)
    return nullptr;
}

int main() {
    cout << unitbuf;
    cerr << unitbuf;
    rl_attempted_completion_function = completionCallback;
    const char* histFile = getenv("HISTFILE");
    if (histFile) {
        read_history(histFile);
        appendOffset = history_length;
    }

    while(1) {

        char* rawInput = readline("$ ");
        if (!rawInput) break;  // EOF (Ctrl+D)

        string command(rawInput);
        free(rawInput);
        if (command.empty()) continue;
        add_history(command.c_str());

        vector<string> tokens = tokenize(command);
        if(tokens.empty()) continue;

        
        // Check for pipe operator
        auto pipePos = find(tokens.begin(), tokens.end(), "|");
        if (pipePos != tokens.end()) {
        // Split tokens into stages by "|"
        vector<vector<string>> stages;
        vector<string> current;
        for (auto& t : tokens) {
            if (t == "|") {
                stages.push_back(current);
                current.clear();
            } else {
                current.push_back(t);
            }
        }
        stages.push_back(current);

        for (auto& s : stages) {
            if (s.empty()) {
                cerr << "syntax error near |" << endl;
                goto pipeline_done;
            }
        }

        {
            int n = stages.size();
            vector<array<int,2>> pipes(n - 1);
            for (int i = 0; i < n - 1; i++) {
                if (pipe(pipes[i].data()) < 0) {
                    cerr << "pipe failed" << endl;
                    goto pipeline_done;
                }
            }

            vector<pid_t> pids;
            for (int i = 0; i < n; i++) {
                pid_t pid = fork();
                if (pid == 0) {
                    int inFd  = (i == 0)     ? STDIN_FILENO  : pipes[i-1][0];
                    int outFd = (i == n-1)   ? STDOUT_FILENO : pipes[i][1];

                    // Close all pipe fds not relevant to this stage
                    for (int j = 0; j < n - 1; j++) {
                        if (j != i-1 && j != i) {
                            close(pipes[j][0]);
                            close(pipes[j][1]);
                        }
                    }
                    // Close the unused end of relevant pipes
                    if (i > 0)   close(pipes[i-1][1]);
                    if (i < n-1) close(pipes[i][0]);

                    execCommand(stages[i], inFd, outFd);
                }
                pids.push_back(pid);
            }

                // Parent: close all pipe fds, wait for all children
                for (int i = 0; i < n - 1; i++) {
                    close(pipes[i][0]);
                    close(pipes[i][1]);
                }
                for (pid_t pid : pids) waitpid(pid, NULL, 0);
            }

            pipeline_done:
            continue;
        }
        
        // Parse out any redirection FIRST, before dispatch
        redirectInfo redir = parseRedirect(tokens);
        string cmd = tokens[0];

        if(cmd == "exit") break;

        // int redirFd = -1;
        int stdoutFd = -1;
        if (!redir.stdoutFile.empty()) {
          int flags = O_WRONLY | O_CREAT | (redir.stdoutAppend ? O_APPEND : O_TRUNC);
          stdoutFd = open(redir.stdoutFile.c_str(), flags, 0644);
          if (stdoutFd < 0) { cerr << "cannot open " << redir.stdoutFile << endl; continue; }
        }

        int stderrFd = -1;
        if (!redir.stderrFile.empty()) {
          int flags = O_WRONLY | O_CREAT | (redir.stderrAppend ? O_APPEND : O_TRUNC);
          stderrFd = open(redir.stderrFile.c_str(), flags, 0644);
          if (stderrFd < 0) { cerr << "cannot open " << redir.stderrFile << endl; continue; }
        }

        if (cmd == "echo" || cmd == "pwd" || cmd == "type"|| cmd=="history") {
            int savedStdout = -1, savedStderr = -1;
            if (stdoutFd >= 0) { savedStdout = dup(STDOUT_FILENO); dup2(stdoutFd, STDOUT_FILENO); close(stdoutFd); }
            if (stderrFd >= 0) { savedStderr = dup(STDERR_FILENO); dup2(stderrFd, STDERR_FILENO); close(stderrFd); }

            runBuiltin(tokens);

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
        // if(cmd == "echo") {
        //     int savedStdout = -1, savedStderr = -1;
        //     if (stdoutFd >= 0) { savedStdout = dup(STDOUT_FILENO); dup2(stdoutFd, STDOUT_FILENO); close(stdoutFd); }
        //     if (stderrFd >= 0) { savedStderr = dup(STDERR_FILENO); dup2(stderrFd, STDERR_FILENO); close(stderrFd); }

        //     for(size_t i = 1; i < tokens.size(); i++) {
        //         if(i > 1) cout << " ";
        //         cout << tokens[i];
        //     }
        //     cout << endl;

        //     if (savedStdout >= 0) { dup2(savedStdout, STDOUT_FILENO); close(savedStdout); }
        //     if (savedStderr >= 0) { dup2(savedStderr, STDERR_FILENO); close(savedStderr); }
        //     continue;
        // }
        // if(cmd == "pwd") {
        //     int savedStdout = -1, savedStderr = -1;
        //     if (stdoutFd >= 0) { savedStdout = dup(STDOUT_FILENO); dup2(stdoutFd, STDOUT_FILENO); close(stdoutFd); }
        //     if (stderrFd >= 0) { savedStderr = dup(STDERR_FILENO); dup2(stderrFd, STDERR_FILENO); close(stderrFd); }

        //     cout << fs::current_path().string() << endl;

        //     if (savedStdout >= 0) { dup2(savedStdout, STDOUT_FILENO); close(savedStdout); }
        //     if (savedStderr >= 0) { dup2(savedStderr, STDERR_FILENO); close(savedStderr); }
        //     continue;
        // }
        // if(cmd == "type") {
        //     if(tokens.size() < 2) continue;
        //     string msg = tokens[1];
        //     if(msg=="echo"||msg=="exit"||msg=="type"||msg=="pwd"||msg=="cd") {
        //         cout << msg << " is a shell builtin" << endl;
        //         continue;
        //     }
        //     char* pathEnv = getenv("PATH");
        //     string pathStr = pathEnv;
        //     stringstream ss(pathStr);
        //     string dir;
        //     bool found = false;
        //     while(getline(ss, dir, ':')) {
        //         string fullPath = dir + "/" + msg;
        //         if(access(fullPath.c_str(), X_OK) == 0) {
        //             cout << msg << " is " << fullPath << endl;
        //             found = true; break;
        //         }
        //     }
        //     if(!found) cout << msg << ": not found" << endl;
        //     continue;
        // }
        // ... builtin handlers above ...

        // Pipeline check
        

        // Single external command (existing code)
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
    if (histFile) {
        write_history(histFile);
        // append_history(history_length - appendOffset, histFile);
    }
    return 0;
}