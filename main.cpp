#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

std::string path = ".";

std::vector<std::string> tokens;

// used for debugging
extern char** environ;

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
				// if (strcmp(val, "?"))
					// status of last program
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

void change_dir()
{
	if (tokens.size() != 2)
		std::cerr << "cd: invalid argumentd\n";
	else
	{
		if (file_exists(tokens[1]))
		{
			if (chdir(tokens[1].c_str()))
				std::cerr << "couldn't change directory\n";
			return;
		}
		else
			std::cerr << "cd: error changing directory to: " << tokens[1] << '\n';
			// TODO: Report specific error
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
		std::cerr << "failed to fork: " << errno << '\n';

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
						std::cerr << "dsh: " << tokens[0] << " is not a valid command\n";
						break;
					}
				}
			}

			if (old_i == i)
				std::cerr << "dsh: " << tokens[0] << " is not a valid command\n";
		}
	}
	
	tokens.clear();
}

int main(int argc, char **argv)
{
	std::string in;
	for (;;)
	{
		std::cout << "# ";
		std::getline(std::cin, in);
		
		if (in == "exit")
			return 0;
		else
		{
			std::string token = "" ;
			for (int i = 0; in[i]; i++)
			{
				if (in[i] == ' ')
				{
					tokens.push_back(token);
					token = "";
				}
				else
					token += in[i];
			}
			tokens.push_back(token);
			
			parse(0);
			
			run_command();
		}
	}
}

