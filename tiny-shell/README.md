## Tiny-Shell (Command Runner)

### Objective - This project is a tiny command-runner like a shell, which takes commands, creates processes, executes them, and redirects I/O to output for the caller.

It employs Process Creation, Exec & Pipe calls, Execution and Environment Passing

### Syscalls - fork(), execvp(), waitpid(), pipe(), exit(), signal(), sigaction()

#### Design Choices

#### Testing Strategy

#### Improvements and Insights

### Dev-Notes
1. Russian Doll -> Interesting observation of the Recursive nature of shells, how from within one, i can call the same binary again - and that nests the shells, like bash does too
2. strtok -> tokenizes the string, by simply replacing all the delimiters to '\0', which can then be used to create an array of char* - pointers to strings, which are basically arguments array exactly what the exec* family of functions expect (just like *argv gets from main() call, here kernel does it for us)
