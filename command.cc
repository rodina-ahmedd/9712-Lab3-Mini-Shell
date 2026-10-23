
/*
 * 
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <glob.h>
#include <wordexp.h>
#include <vector>
#include <string>
#include <cctype>
#include <iostream>
#include <sstream>
#include "command.h"
#include "tokenizer.h"


void parse(std::vector<Token> &tokens)
{
    Command currentCommand;
    SimpleCommand *currentSimpleCommand = new SimpleCommand();

    for(size_t i=0; i<tokens.size();i++){
        Token &token=tokens[i];
        if (token.type == TOKEN_EOF){
            break;
        }
        else if (token.type == TOKEN_COMMAND || token.type == TOKEN_ARGUMENT){
            currentSimpleCommand->insertArgument(strdup(token.value.c_str()));
        }
        else if(token.type == TOKEN_BACKGROUND){
             if (i + 1 < tokens.size() && tokens[i + 1].type != TOKEN_EOF){
                fprintf(stderr, "Syntax error: '&' must be at the end of the command\n");
                currentCommand.clear();
                delete currentSimpleCommand;
                return;
           }
            currentCommand._background=1;
        }
        else if(token.type == TOKEN_PIPE){
            if (currentSimpleCommand->_numberOfArguments == 0)
            {
                fprintf(stderr, "Syntax error: empty command before pipe\n");
                currentCommand.clear();
                delete currentSimpleCommand;
                return;
            }

            currentCommand.insertSimpleCommand(currentSimpleCommand);
            currentSimpleCommand = new SimpleCommand();
        }
        // redirections
        else if(token.type==TOKEN_REDIRECT ||token.type==TOKEN_APPEND ||token.type==TOKEN_INPUT||
                token.type==TOKEN_ERROR ||token.type==TOKEN_REDIRECT_AND_ERROR){

            if (i + 1 >= tokens.size() ||tokens[i + 1].type == TOKEN_EOF ||tokens[i + 1].type == TOKEN_PIPE ||
                tokens[i + 1].type == TOKEN_BACKGROUND ||tokens[i + 1].type == TOKEN_REDIRECT ||
                tokens[i + 1].type == TOKEN_APPEND ||tokens[i + 1].type == TOKEN_INPUT ||
                tokens[i + 1].type == TOKEN_ERROR ||tokens[i + 1].type == TOKEN_REDIRECT_AND_ERROR){
                fprintf(stderr, "Syntax error: missing redirection filename or invalid\n");
                currentCommand.clear();
                delete currentSimpleCommand;
                return;
            }

            char *filename = strdup(tokens[++i].value.c_str());

             // >
            if (token.type == TOKEN_REDIRECT){ 
                free(currentCommand._outFile);
                currentCommand._outFile = filename;
                currentCommand._append = 0;
            }
             // >>
            else if (token.type == TOKEN_APPEND){ 
                free(currentCommand._outFile);
                currentCommand._outFile = filename;
                currentCommand._append = 1;
            }
            // <
            else if (token.type == TOKEN_INPUT){ 
                free(currentCommand._inputFile);
                currentCommand._inputFile = filename;
            }
            // 2>
            else if (token.type == TOKEN_ERROR){          
                free(currentCommand._errFile);
                currentCommand._errFile = filename;
            }
            // >>&
            else if (token.type == TOKEN_REDIRECT_AND_ERROR){
                free(currentCommand._outFile);
                free(currentCommand._errFile);
                currentCommand._outFile = filename;
                currentCommand._errFile = strdup(filename);
                currentCommand._out_error = 1;
                currentCommand._append = 1;
            }
        }
    }
        if (currentSimpleCommand->_numberOfArguments > 0){
        currentCommand.insertSimpleCommand(currentSimpleCommand);
        }
         else
    {
        //mfesh arg ls | 
        if (currentCommand._numberOfSimpleCommands > 0)
        {
            fprintf(stderr, "Syntax error: missing command after pipe\n");
            currentCommand.clear();
            delete currentSimpleCommand;
            return;
        }
        delete currentSimpleCommand;
    }

    // mfesh ay amr lwahdo zy (>out)
    if (currentCommand._numberOfSimpleCommands == 0)
    {
        if (tokens.size() > 1)
            fprintf(stderr, "Syntax error: missing command\n");
        currentCommand.clear();
        return;
    }

    currentCommand.execute();
    /*
      Implement this function 
      you must parse for all possible tokens scenario
      right now command.execute only print the format , you will inject it in your logic.

    */
}

SimpleCommand::SimpleCommand()
{
    _numberOfAvailableArguments = 5;
    _numberOfArguments = 0;
    _arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument)
{
    if (_numberOfAvailableArguments == _numberOfArguments + 1)
    {
        _numberOfAvailableArguments *= 2;
        _arguments = (char **)realloc(_arguments,
                                      _numberOfAvailableArguments * sizeof(char *));
    }

    _arguments[_numberOfArguments] = argument;
    _arguments[_numberOfArguments + 1] = NULL;

    _numberOfArguments++;
}

Command::Command()
{
    _numberOfAvailableSimpleCommands = 1;
    _simpleCommands = (SimpleCommand **)
        malloc(_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _out_error = 0;
    _background = 0;
    _append = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
    if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
    {
        _numberOfAvailableSimpleCommands *= 2;
        _simpleCommands = (SimpleCommand **)realloc(_simpleCommands,
                                                    _numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
    }

    _simpleCommands[_numberOfSimpleCommands] = simpleCommand;
    _numberOfSimpleCommands++;
}

void Command::clear()
{
    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            free(_simpleCommands[i]->_arguments[j]);
        }

        free(_simpleCommands[i]->_arguments);
        free(_simpleCommands[i]);
    }

    if (_outFile)
    {
        free(_outFile);
    }

    if (_inputFile)
    {
        free(_inputFile);
    }

    if (_errFile)
    {
        free(_errFile);
    }

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
}

void Command::print()
{
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        printf("  %-3d ", i);
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
        }
    }

    printf("\n\n");
    printf("  Output       Input        Error        Err&Out       Background\n");
    printf("  ------------ ------------ ------------ ------------ ------------\n");
    printf("  %-12s %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
           _inputFile ? _inputFile : "default", _errFile ? _errFile : "default", _out_error == 1 ? _errFile : "default"
           ,_background ? "YES" : "NO");
    printf("\n\n");
}

void Command::execute()
{
    print();
    fflush(stdout);
    if (_numberOfSimpleCommands == 0){//lw mfesh ay amr my3mlsh haga 
        clear();
        return;
    }
    std::vector<pid_t> pids;//list pids ahot fyha num of process
    int previousPipe = -1; // file descriptor ely by2ra mno el current amr
    for (int i = 0; i < _numberOfSimpleCommands; i++){
        int pipefd[2] = {-1, -1};//matrix for read , write
        bool isLast = (i == _numberOfSimpleCommands - 1);// bshof lw da last
        if (!isLast){//lw msh akher wahd by3ml pipe 
            if (pipe(pipefd) == -1){
                perror("pipe");
                if (previousPipe != -1)
                    close(previousPipe);

                for (pid_t pid : pids)
                    waitpid(pid, NULL, 0);

                clear();
                return;
            }
        }
        // fork create
        pid_t pid = fork();

        if (pid == -1)
        {
            perror("fork");

            if (previousPipe != -1)
                close(previousPipe);
            if (pipefd[0] != -1)
                close(pipefd[0]);
            if (pipefd[1] != -1)
                close(pipefd[1]);

            for (pid_t childPid : pids)
                waitpid(childPid, NULL, 0);

            clear();
            return;
        }
        if (pid == 0){
            // Read input from previous command
            if (previousPipe != -1) {
                if (dup2(previousPipe, STDIN_FILENO) == -1){
                    perror("dup2");
                    _exit(1);
                }
            }
                        
            // Send output to next command
            if (!isLast){
                if (dup2(pipefd[1], STDOUT_FILENO) == -1){
                    perror("dup2");
                    _exit(1);
                }
            }

            if (previousPipe != -1)
                close(previousPipe);
            if (pipefd[0] != -1)
                close(pipefd[0]);
            if (pipefd[1] != -1)
                close(pipefd[1]);

            // I/p redirection <
            if (i == 0 && _inputFile != NULL){
                int fd = open(_inputFile, O_RDONLY);

                if (fd == -1){
                    perror(_inputFile);
                    _exit(1);
                }

                if (dup2(fd, STDIN_FILENO) == -1){
                    perror("dup2");
                    close(fd);
                    _exit(1);
                }

                close(fd);
            }
            // o/p redirection > or >>
            if (isLast && _outFile != NULL){
                int flags = O_WRONLY | O_CREAT;//write only| lw file msh mwgod y3ml wahd

                flags |= _append ? O_APPEND : O_TRUNC; // >delete contant | >>write fy elakher

                int fd = open(_outFile, flags, 0666);

                if (fd == -1){
                    perror(_outFile);
                    _exit(1);
                }

                if (dup2(fd, STDOUT_FILENO) == -1){
                    perror("dup2");
                    close(fd);
                    _exit(1);
                }

                close(fd);
            }
            // Error redirection 2>
            if (isLast && _errFile != NULL){
                if (_out_error){
                    // >>& : send stderr to the same destination as stdout
                    if (dup2(STDOUT_FILENO, STDERR_FILENO) == -1){
                        perror("dup2");
                        _exit(1);
                    }
                }
                else{
                    int fd = open(
                        _errFile,
                        O_WRONLY | O_CREAT | O_TRUNC,
                        0666
                    );

                    if (fd == -1){
                        perror(_errFile);
                        _exit(1);
                    }

                    if (dup2(fd, STDERR_FILENO) == -1){
                        perror("dup2");
                        close(fd);
                        _exit(1);
                    }

                    close(fd);
                }
            }
            // exec the command
            signal(SIGINT, SIG_DFL);
            SimpleCommand *cmd = _simpleCommands[i];
            execvp(cmd->_arguments[0], cmd->_arguments);

            //if execvp fails
            perror(cmd->_arguments[0]);
            _exit(127);// from child 127 en el amr msh mwgod 
        }
        //parent
        pids.push_back(pid);

        if (previousPipe != -1)
            close(previousPipe);

        if (pipefd[1] != -1)
            close(pipefd[1]);

        previousPipe = pipefd[0];
    }

    if (previousPipe != -1)// bn2fl akher pipe open
        close(previousPipe);

    // lw mfish & wait for all children to finish lw fy & msh h wait 
    if (!_background){
        for (pid_t pid : pids)
            waitpid(pid, NULL, 0);
    }

    clear();
}

   // prompt();


void Command::prompt()
{
    std::string input;
    while (true)
    {
        printf("myshell>");
        fflush(stdout);

        if (!std::getline(std::cin, input))   // Ctrl-D
            break;

        std::vector<Token> tokens = tokenize(input);
        parse(tokens);
    }
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int main()
{
    signal(SIGINT, SIG_IGN);
    Command::_currentCommand.prompt();
    return 0;
}
