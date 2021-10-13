#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <time.h>
#include <regex>
#include <fstream>
using namespace std;



string trim(const string& line)
{
    size_t first = line.find_first_not_of(' ');
    if (string::npos == first)
    {
        return line;
    }
    size_t last = line.find_last_not_of(' ');
    return line.substr(first, (last - first + 1));
}


const string currentDateTime() {
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

void IORedirect(int oldfd, int newfd) {
    if (oldfd != newfd) {
        dup2(oldfd, newfd);
        close(oldfd);
    }
}

string removeQuotes(string arg){
    if (arg[0] == '"' || arg[0] == '\''){
        return arg.substr(1, arg.length() - 2);
    }
    return arg;
}

vector<string> split(string line, string delim = " "){
    regex rgx(delim + "(?=(?:[^\"\']*[\"\'][^\"\']*[\"\'])*[^\"\']*$)");
    sregex_token_iterator iter(line.begin(),
    line.end(),
    rgx,
    -1);
    sregex_token_iterator end;

    vector<string> result = vector<string>();
    for ( ; iter != end; ++iter){
        if (*iter != ""){
            result.push_back(trim(removeQuotes(*iter)));
        }
    }

    return result;
}

vector<string> getIOArgs(vector<string> &args){
    vector<string> result = vector<string>();
    for (int i = 0; i < args.size(); i++){
        if (args[i] == "<" || args[i] == ">"){
            if (i + 1 >= args.size()){
                cout << "SYNTAX ERROR";
                result;
            }
            else{
                result.push_back(args[i]);
                result.push_back(args[i+1]);
                args.erase(std::next(args.begin(), i), std::next(args.begin(), i+2));
                i-=2;
            }
        }
    }
    return result;
}




char** vec_to_char_array(vector<string>& parts){
    char ** result = new char * [parts.size() +1];
    for (int i = 0; i < parts.size(); i++){
        result [i] = (char*) parts [i].c_str();
    }
    result[parts.size()] = NULL;
    return result;
}

int main(){
    char buf[256];
    getcwd(buf, 256);

    string initaldir(buf);
    vector<int> bgs;
    while(true){

        
        for (int i = 0; i < bgs.size(); i++){
            if (waitpid (bgs[i], 0, WNOHANG) == bgs[i]){
                //cout << "Process: " << bgs[i] << " ended" << endl;
                bgs.erase (bgs.begin() + i);
                i--;
            }
        }
        
        cout << currentDateTime() << " " << getlogin() << "$ ";
        string inputline;
        getline(cin, inputline);
        inputline = trim(inputline);
            
            if (inputline == string("exit") || cin.eof()){
                cout<< "Bye!! End of shell" << endl;
                break;
            }       

        vector<string> c = split(inputline, "\\|");

        int backup = dup(0);
        int in = 0; //Standard in
        for (int i = 0; i < c.size(); i++){
            inputline = c[i];
            bool bg = false;
            if (inputline[inputline.size()-1] == '&'){
                //cout << "Bg process found" << endl;
                bg = true;
                inputline = inputline.substr(0, inputline.size()-1);
            }

            int fd[2];
            pipe(fd);
            vector<string> ogparts = split (inputline);


            if (ogparts[0] == "cd"){

                if (ogparts[1] == "-"){
                    getcwd(buf,256);
                    string temp(buf);

                    chdir(initaldir.c_str());
                    initaldir = temp;
                }
                else{

                    getcwd(buf,256);
                    string temp(buf);
                    initaldir = temp;
                    chdir(ogparts[1].c_str());
                }
            }
                

            int pid = fork();
        

            if (pid == 0) {
                
                //close(fd[0]);
                //IORedirect(in, 0);
                //IORedirect(fd[1], 1);

                if (i < c.size() -1) {
                    dup2(fd[1],1);
                }

                vector<string> parts = split (inputline);
                vector<string> ioargs = getIOArgs(parts);


                if (ioargs.size() > 0) {
                    int infd = 0;
                    int outfd = 1;
                    if (ioargs[0] == "<"){
                        infd = open(ioargs[1].c_str(), O_RDONLY, 0);
                        if (ioargs[ioargs.size() - 2] == ">"){
                            outfd = open (ioargs[ioargs.size() - 1].c_str(), O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                        }
                    }
                    else {
                        outfd = open (ioargs[1].c_str(), O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                    }
                    dup2(outfd, 1);
                    dup2(infd, 0);

                }


                /*for (int i = 0; i < ioargs.size(); i++) {
                    cout << ioargs[i] << " ";
                }
                cout << endl;
                
                for (int i = 0; i < parts.size(); i++) {fstream input (ioargs[1]);
                    int infd = fileno(input);
                    cout << parts[i] << " ";
                }
                cout << endl;*/
                char** args = vec_to_char_array(parts);

                /*for (int i = 0; i < parts.size(); i++) {
                    cout << parts[i] << " ";
                }*/


                execvp (args[0], args);
                exit(100);
                
                break;

            }
            else{
                if (!bg)
                    waitpid (pid, 0, 0);
                else {
                    bgs.push_back(pid);
                }
                dup2 (fd [0], 0);//now redirect the input for the next loop
                close (fd [1]); //fd
            }
        }
        dup2(backup, 0);
    }
    
}



