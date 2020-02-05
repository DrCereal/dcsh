/*  main.cpp -- the main file in dcsh
    Copyright (C) 2020 Jakob Brothwell

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

std::string path = ".";

std::vector<std::string> tokens;
std::vector<std::string> buffer;

int child_exit_status = 0;

// used for debugging
extern char** environ;

#define error(msg) std::cerr << msg << '\n'

#define exit() { \
switch_canonical(1); \
return 0; }

inline void
switch_canonical (int enabled)
{
	struct termios termios_p;

	int status = tcgetattr(STDIN_FILENO, &termios_p);
	if (status)
		error("tcgetattr(): failed");
	else
	{
		if (enabled)
			termios_p.c_lflag |= (ICANON | ECHO);
		else
			termios_p.c_lflag &= ~ICANON;

		status = tcsetattr(STDIN_FILENO, TCSANOW, &termios_p);
		if (status)
			error("tcsetattr(): failed");
	}
}

inline int 
file_exists(const std::string& path)
{
	struct stat buffer;
	return stat(path.c_str(), &buffer) == 0;
}

// parse tokens for shell variables
void
parse (int flags)
{
	std::string* token;
	for (int i = 0; i < tokens.size(); i++)
	{
		token = &tokens[i];

		if ((*token)[0] == '$')
		{
			const char* var = token->substr(1, token->size()).c_str();
			char* val = getenv(var);
			if (val != NULL)
			{
				*token = val;
				return;
			}
			else
			{
				if (strcmp(var, "?") == 0)
				{
					*token = std::to_string(child_exit_status);
					return;
				}
			}
		}

		char c;
		for (int j = 0; c = (*token)[j]; j++)
		{
			switch (c)
			{
				default: break;
			}
		}
	}
} 

void
tokenize ()
{
	std::string& command = buffer.back();
	std::string token;

	char c;
	int end = 0;
	for (int i = 0; end != 1; i++)
	{
		c = command[i];
		switch (c)
		{
			case 0: end = 1;
			case ' ':
				if (token != "")
				{
					tokens.push_back(token);
					token = "";
				}
				break;
			default: token.append(1, c); break;
		}
	} 
}

void change_dir()
{
	if (tokens.size() != 2)
		error("cd: invalid arguments");
	else
	{
		if (file_exists(tokens[1]))
		{
			if (chdir(tokens[1].c_str()))
				error("cd: couldn't change directory");
			return;
		}
		else
			error("cd: '" << tokens[1] << "' does not exist");
	}
}

// execute program pointed to by path
int
exec_program (const std::string& path)
{
	pid_t child = fork();

	if (child > 0) // parent
	{
		int wstatus = 0;
		wait(&wstatus);

		if (WIFEXITED(wstatus))
			child_exit_status = WEXITSTATUS(wstatus);
		else
			error("dsh: program did not exit properly");
 
		return 0;
	}
	else if (child == 0) // child
	{
	
		// set up argv for the program
		char** argv = (char**)malloc(sizeof(char*) * tokens.size());
		for (int i = 0; i < tokens.size(); i++)
			argv[i] = const_cast<char*>(tokens[i].c_str());

		// replace process
		int status = execv(path.c_str(), argv);
		free(argv);
		return status; // on failure, return status
	}
	else // error
		error("failed to fork: " << errno);

	return 0;
}

// temp function which handles finding a program
void 
run_command()
{
	path = getenv("PATH");
	
	if (tokens[0] == "cd")
		change_dir();
	else
	{
		if (file_exists(tokens[0]))
			exec_program(tokens[0]);
		else
		{
			// scan path
			std::string prefix;

			int old_i = 0;
			int i = 0;
			for (;; i++)
			{
				if (path[i] == ':' || path[i] == 0)
				{
					prefix = path.substr(old_i, i - old_i); // O(n^2) potentially
					if (prefix.back() != '/')
						prefix.append("/");

					if (file_exists(prefix + tokens[0]))
					{
						if (exec_program(prefix + tokens[0]))
							old_i = i + 1;
						else
							break;
					}
					else
						old_i = i + 1;

					if (path[i] == 0)
					{
						error("dsh: " << tokens[0] << " is not a valid command");
						break;
					}
				}
			}

			if (old_i == i)
				error("dsh: " << tokens[0] << " is not a valid command");
		}
	}
	
	tokens.clear();
}

int 
main(int argc, char **argv)
{
	switch_canonical(0);

	std::string command;

	printf("# ");
	int c;
	for (;;)
	{
		c = fgetc(stdin);

		switch (c)
		{
			// differentiate in the future
			case 3:
			case EOF: exit()
			case 10:
				if (command == "exit")
					exit()
				else
				{
					buffer.push_back(command);
					command = "";

					tokenize();
					parse(0);

					run_command();
				}
				printf("# ");
				break;
			default:
				printf("%i\n", c);
				command.append(1, c);
				break;
		}
	}
	
	switch_canonical(1);
}

