[![progress-banner](https://backend.codecrafters.io/progress/shell/4b5dcb77-0895-4306-bffc-721041a2ecb9)](https://app.codecrafters.io/users/codecrafters-bot?r=2qF)

This is a starting point for C++ solutions to the
["Build Your Own Shell" Challenge](https://app.codecrafters.io/courses/shell/overview).

In this challenge, you'll build your own POSIX compliant shell that's capable of
interpreting shell commands, running external programs and builtin commands like
cd, pwd, echo and more. Along the way, you'll learn about shell command parsing,
REPLs, builtin commands, and more.

## Features implemented

- **Built-in commands**: `echo`, `cd`, `pwd`, `exit`, `type`, `history`,
  `jobs`, `complete`, `declare`
- **External command execution** via `fork`/`execvp`, with proper PATH
  resolution
- **I/O redirection**: `>`, `1>`, `>>`, `1>>`, `2>`, `2>>`
- **Pipelines**: supports chaining any number of commands with `|`, including
  built-ins appearing anywhere in the chain
- **Background jobs**: run commands with `&`, list them with `jobs`
  (with `+`/`-` current/previous job markers), and get automatic
  `Done` notifications before the next prompt
- **Quoting and escaping**: single quotes, double quotes, and backslash
  escapes, matching POSIX shell tokenization rules
- **Command history**: in-memory history via GNU Readline, `history [n]`,
  `history -r/-w/-a <file>`, and `HISTFILE`-based persistence across sessions
- **Tab completion**:
  - Built-in commands and PATH executables autocomplete on the first word
  - `complete -C <script> <command>` registers an external completer script
    for argument completion
  - `complete -p <command>` and `complete -r <command>` to inspect/remove
    registrations
  - Registered completers are invoked with the standard arguments
    (`argv[1]` = command, `argv[2]` = current word, `argv[3]` = previous word)
    and environment variables (`COMP_LINE`, `COMP_POINT`)
- **Shell variables**: `declare NAME=VALUE` and `declare -p NAME`, with
  identifier validation
- **Parameter expansion**: `$VAR` and `${VAR}` forms, including unset
  variables expanding to an empty string and being dropped from the
  argument list when they contribute no literal text

## Building and running

This project uses CMake and requires the GNU Readline development
headers/library to be available on your system (e.g. `libreadline-dev` on
Debian/Ubuntu).

```sh
cmake -B build -S .
cmake --build build
./build/shell
```

Alternatively, once you've cloned this repository, the included
`your_program.sh` script will compile and run the shell for you:

```sh
./your_program.sh
```

## Project structure

```
.
├── src/
│   └── main.cpp        # Shell implementation
├── CMakeLists.txt      # Build configuration
├── compile.sh           # Compiles the project
├── run.sh                # Runs the compiled shell
└── your_program.sh      # Entry point used by CodeCrafters tests
```