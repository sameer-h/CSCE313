#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <numeric>
#include <algorithm>
#include <list>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <functional>
#include <cctype>
#include <locale>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <cassert>
#include <assert.h>
#include <cmath>
#include <limits.h>
#include <pwd.h>

/* Sameer Hussain - CSCE 313 - PA 2 */

using namespace std;

string trim (string input) 
{
    bool space = true;
    while (space) 
    {
        if (input[input.size()-1] == ' ') 
        {
            input = input.substr(0, input.size()-1);
        } if (input[0] == ' ') {
            input = input.substr(1);
        } else {
            space = false;
        }
    }
    return input;
}

vector<string> split(string line) 
{
    vector<string> result;
    string word = "";
    for (int x = 0; x < line.length(); x++)
    {
        if (line.at(x) == ' ')
        {
            result.push_back(word);
            word = "";
        }
        else
        {
            word = word + line.at(x);
        }
    }
    result.push_back(word);
    return result;
}

vector<string> delimiterSplitter(string line, char delimiter) 
{
    vector<string> result;
    string str = "";
    for (int x = 0; x < line.length(); x++)
    {
        if (line.at(x) == delimiter)
        {
            result.push_back(str);
            str = "";
        }
        else
        {
            str = str + line.at(x);
        }
    }
    result.push_back(str);
    return result;
}

string echoCommand(string str)
{
    string chars = "\'";
    for (char c: chars) 
    {
        str.erase(std::remove(str.begin(), str.end(), c), str.end());
    }
    chars = "\"";
    for (char c: chars) 
    {
        str.erase(std::remove(str.begin(), str.end(), c), str.end());
    }
    return str;
}

char** vec_to_char_array(vector<string> commands) 
{
    char** arr = new char* [commands.size() + 1];
    for (int i = 0; i < commands.size(); i++) {
        char* cstring = new char[commands[i].size() + 1];
        strcpy(cstring, commands[i].c_str());
        arr[i] = cstring;
    }
    arr[commands.size()] = NULL;

    return arr;
}


int main () 
{
    vector<int> bgs;

    char cwd[PATH_MAX];

    dup2(0,10);

    string cd = "";

    
    while (true) 
    {
        // zombie killer
        for (int i = 0; i < bgs.size(); i++) {
            if (waitpid(bgs [i], 0, WNOHANG) == bgs [i]) {
                bgs.erase(bgs.begin() + i);
                i--;
            }
        }

        dup2(10,0); // keeping the previous

        cout << getpwuid(geteuid())->pw_name << "$ "; // outputting user name

        // get the user input
        string inputline;

        getline (cin, inputline); //get a line from standard input
        if (inputline == string("exit")) 
        {
            cout << "Bye!! End of shell" << endl; // exiting the shell
            break;
        }


        vector<string> cdpipe;

        int echoLoc = inputline.find("echo"); // find the echo commands

        if (echoLoc < 0) {
            cdpipe = delimiterSplitter(inputline, '|'); // split by the pipe character
        }

        else 
        {
            cdpipe.push_back(inputline);
        }

        for (int i = 0; i < cdpipe.size(); i++) 
        {
            inputline = cdpipe[i];
            inputline = trim(inputline);
            int fds[2];
            pipe(fds);
            vector<string> parts;

            bool bg = false;

            // fork()
            if (inputline[inputline.size() - 1] == '&') { // check for background processes
                //cout << "Bg process found" << endl;
                bg = true;
                inputline = trim(inputline.substr (0, inputline.size() - 1));
            }
            int pid = fork ();
            if (pid == 0){ //if there is a child process

                if (trim(inputline).find("echo") == 0) {

                    inputline = echoCommand(inputline);
                    parts.push_back("echo");
                    parts.push_back(trim(inputline.substr(5)));

                }
                else 
                {
                    // CD
                    if (trim(inputline).find("cd") == 0) 
                    {
                        string dir = trim(delimiterSplitter(inputline, ' ')[1]);
                        if (dir == "-") 
                        {
                            if (cd == "") 
                            {
                                cerr << "ERROR: INVALID DIRECTORY COMMAND" << endl;
                            }
                            else 
                            {
                                chdir(cd.c_str());
                            }

                        }
                        else 
                        {
                            char d[FILENAME_MAX];
                            
                            
                            
                            
                            
                            
                            
                             // max for UNIX
                            getcwd(d, sizeof(d)); // getcwd change directory
                            cd = d; //
                            chdir(dir.c_str());

                        }
                        continue;
                    }
                    
                    // PWD
                    if (trim(inputline).find("pwd") == 0) // print path
                    { 
                        char cwd[PATH_MAX];

                        //using making sure correct size
                        getcwd(cwd, sizeof(cwd));
                    }

                    int pos = inputline.find("awk"); // awk command
                    if (pos >= 0) 
                    {
                        inputline = echoCommand(inputline);
                        inputline = trim(inputline);
                    }

                    pos = inputline.find('<'); // read command
                    if (pos >= 0) 
                    {
                        string command = trim(inputline.substr(0, pos));
                        string filename = trim(inputline.substr(pos+1));
                        inputline = command;

                        int fd = open(filename.c_str(), O_RDONLY, S_IWUSR | S_IRUSR);
                        dup2(fd, 0);
                        close(fd);
                    }

                    pos = inputline.find('>'); // write command
                    if (pos >= 0) 
                    {

                        string command = trim(inputline.substr(0, pos));

                        // cerr << command;
                        string filename = trim(inputline.substr(pos+1));
                        inputline = command;

                        int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_RDONLY, S_IWUSR | S_IRUSR); // copied from TA
                        dup2(fd, 1);
                        close(fd);
                    }


                    if (i < cdpipe.size() -1) 
                    {
                        dup2(fds[1], 1);
                    }
                }

                char** args = vec_to_char_array(parts); // given to us
                execvp (args [0], args);

            }
            else 
            {
                if ((i == cdpipe.size() - 1) && !bg) 
                {
                    waitpid (pid, 0, 0);
                }
                else {
                    bgs.push_back (pid);
                }
                dup2(fds[0], 0);
                close(fds[1]);
            }
        }
    }
}