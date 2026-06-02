#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include "Commands.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <regex>
#include <fcntl.h>
#include <ctype.h>
#include <pwd.h>
#include <cstdlib>
#include <cstring>
#include <dirent.h>

#include <algorithm>
#include <fstream>
#include <ctime>

#define MAX_BUFF_SIZE 8192

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

struct linux_dirent64 {
    ino64_t        d_ino;    // inode number
    off64_t        d_off;    // offset to next dirent
    unsigned short d_reclen; // length of this record
    unsigned char  d_type;   // file type
    char           d_name[]; // filename (flexible array)
};

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        //changed to nullptr
        args[++i] = nullptr;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

//I added this so it could work for const char* cmd_line
const char* _removeBackgroundSignConstant(const char* cmd_line) {
    string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == std::string::npos) {
        return strdup(str.c_str());
    }
    // if the command line does not end with & then return
    if (str[idx] != '&') {
        return strdup(str.c_str());
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    str[idx] = ' ';
    // truncate the command line string up to the last non-space character
    unsigned int last_non_space = str.find_last_not_of(WHITESPACE, idx);
    if (last_non_space != std::string::npos) {
        str.erase(last_non_space + 1);
    } else {
        str.clear();
    }
    return strdup(str.c_str());
}

// TODO: Add your implementation for classes in Commands.h

Command :: Command(const char *cmd_line) : cmd_line(cmd_line), cmd_str(cmd_line){
    args = new char*[COMMAND_MAX_ARGS];
    _parseCommandLine(cmd_line,args);
    isBackgroundCmd = (_isBackgroundComamnd(cmd_line));
    isForegroundCmd = (!isBackgroundCmd);
}
string Command::getCmdStr() {
    return cmd_str;
}

BuiltInCommand ::BuiltInCommand (const char *cmd_line) : Command(_removeBackgroundSignConstant(cmd_line)){}


ExternalCommand::ExternalCommand(const char *cmd_line, const char *unchanged_str) : Command(cmd_line),
                                                                                    unchanged_str(unchanged_str){};

bool ExternalCommand::isComplex() const {
    // return true for '?' or '*'
    return ((cmd_str.find('?') != std::string::npos || cmd_str.find('*') != std::string::npos));
}

void ExternalCommand::execute() {
    const char* org_cmd_line_no_amp = _removeBackgroundSignConstant(cmd_line);
    string original_cmd_s_no_amp = _trim(string(org_cmd_line_no_amp));


    if (isComplex()) {
        //complex command
        string first = "/bin/bash -c \"";
        string last = "\"";
        string bash_cmd = first + original_cmd_s_no_amp + last;
        execlp("/bin/bash", "bash", "-c", bash_cmd.c_str(), nullptr);
        //if execlp failed
        perror("smash error: execlp failed");
        exit(1);
    }
    else {
        //simple command
        char *args[COMMAND_MAX_ARGS];
        _parseCommandLine(original_cmd_s_no_amp.c_str(), args);
        execvp(args[0], args);
    }
    //if execvp failed
    perror("smash error: execvp failed");
    exit(1);
}



ChpromptCommand::ChpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line),prompt("") {
    if(args[1] == nullptr){
        setPrompt("smash");
    }
    else{
        setPrompt(args[1]);
    }
}

void ChpromptCommand::execute() {
    SmallShell::getInstance().setName(prompt);
}

ShowPidCommand::ShowPidCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    cout << "smash pid is " << getpid() << endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line){}

void GetCurrDirCommand::execute() {
    char currWorkDir[COMMAND_MAX_LENGTH];
    if (getcwd(currWorkDir, sizeof(currWorkDir)) != nullptr) {
        cout << currWorkDir << endl;
    }
    else{
        perror("smash error: getcwd failed");
    }
    //we assumed pwd will not fail because there is no red message for an error
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line){}

void ChangeDirCommand::execute() {
    if(args[2] != nullptr){
        cerr << "smash error: cd: too many arguments" << endl;
        return;
    }

    //checking the current path and assuming it works
    char currWorkDir[COMMAND_MAX_LENGTH];
    getcwd(currWorkDir, sizeof(currWorkDir));

    //checking if args[1]= nullptr
    if(args[1] == nullptr){
        return; //do nothing
    }
    //checking if args[1]='-'
    if(strcmp(args[1], "-") == 0) {
        if (SmallShell::getInstance().getLastPwd() == "") {
            cerr << "smash error: cd: OLDPWD not set" << endl;
        }
        else{
            int result = chdir(SmallShell::getInstance().getLastPwd().c_str());
            //chdir fails
            if(result != 0){
                perror("smash error: chdir failed");
            }
            else{
                SmallShell::getInstance().setLastPwd(currWorkDir);

            }
        }
    }
        //args[1]!='-' and not nullptr
    else{
        int result = chdir(args[1]);
        if(result != 0){
            perror("smash error: chdir failed");
        }
        else{
            SmallShell::getInstance().setLastPwd(currWorkDir);
        }
    }
}

SmallShell::SmallShell() {
    //default initilization
    shellName = "smash";
    lastPwd = "";
    jobsList = new JobsList();
    aliasHelper = new AliasHelper();
    smash_pid = getpid();

    //helper for the running fg command
    fgPid = -1;

}

SmallShell::~SmallShell() {
    delete(jobsList);
    delete(aliasHelper);
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line,const char *unchanged_cmd_line) {
    //if alias convert to original
    string original_cmd_s = SmallShell::getInstance().getAliasHelper()->convertAliasToOriginal(cmd_line);
    const char* org_cmd_line = strdup(original_cmd_s.c_str());
    string cmd_s = _trim(string(original_cmd_s));

    //added for test alias pass
    string unchanged_str = _trim(string(cmd_line));
    const char* unchanged_str_line = strdup(unchanged_str.c_str());
    //

    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));


    if(firstWord.compare("chprompt") == 0 || firstWord.compare("chprompt&") == 0){
        return new ChpromptCommand(org_cmd_line);
    }
    else if (firstWord.compare("alias") == 0 || firstWord.compare("alias&") == 0) {
        return new AliasCommand(org_cmd_line);
    }
    else if (cmd_s.find('|') != string::npos) {
        return new PipeCommand(cmd_line);
    }
    else if ((cmd_s.find('>') != string::npos) || (cmd_s.find(">>") != string::npos)) {
        return new RedirectionCommand(cmd_line);
    }
    else if (firstWord.compare("showpid") == 0 || firstWord.compare("showpid&") == 0) {
        return new ShowPidCommand(org_cmd_line);
    }
    else if (firstWord.compare("pwd") == 0 || firstWord.compare("pwd&") == 0) {
        return new GetCurrDirCommand(org_cmd_line);
    }
    else if (firstWord.compare("cd") == 0) {
        return new ChangeDirCommand(org_cmd_line);
    }
    else if (firstWord.compare("jobs") == 0 || firstWord.compare("jobs&") == 0) {
        return new JobsCommand(org_cmd_line,jobsList);
    }
    else if (firstWord.compare("fg") == 0 || firstWord.compare("fg&") == 0) {
        return new ForegroundCommand(org_cmd_line,jobsList);
    }
    else if (firstWord.compare("quit") == 0 || firstWord.compare("quit&") == 0) {
        return new QuitCommand(org_cmd_line,jobsList);
    }
    else if (firstWord.compare("kill") == 0) {
        return new KillCommand(org_cmd_line,jobsList);
    }
    else if (firstWord.compare("unalias") == 0) {
        return new UnAliasCommand(org_cmd_line);
    }
    else if (firstWord.compare("unsetenv") == 0) {
        return new UnSetEnvCommand(org_cmd_line);
    }
    else if (firstWord.compare("sysinfo") == 0 || firstWord.compare("sysinfo&") == 0) {
        return new SysInfoCommand(org_cmd_line);
    }
    else if (firstWord.compare("du") == 0) {
        return new DiskUsageCommand(org_cmd_line);
    }
    else if (firstWord.compare("whoami") == 0 || firstWord.compare("whoami&") == 0) {
        return new WhoAmICommand(org_cmd_line);
    }
    else if (firstWord.compare("usbinfo") == 0 || firstWord.compare("usbinfo&") == 0) {
        return new USBInfoCommand(org_cmd_line);
    }
    else {
        return new ExternalCommand(org_cmd_line,unchanged_cmd_line);
    }

}

void SmallShell::executeCommand(const char *cmd_line) {
    //we have to first remove finished jobs
    jobsList->removeFinishedJobs();

    string original_cmd_s = SmallShell::getInstance().getAliasHelper()->convertAliasToOriginal(cmd_line);
    const char* org_cmd_line = strdup(original_cmd_s.c_str());
    string cmd_s = _trim(string(original_cmd_s));

    //added for test alias pass
    string unchanged_str = _trim(string(cmd_line));
    const char* unchanged_str_line = strdup(unchanged_str.c_str());
    //

    Command* cmd = CreateCommand(org_cmd_line,unchanged_str_line);

    //check the type of the command
    BuiltInCommand* builtInCommand = dynamic_cast<BuiltInCommand*>(cmd);

    if (builtInCommand) {
        builtInCommand->execute();
    }
    else{
        //foreground command
        if(cmd->getIsForegroundCmd()){

            pid_t pid = fork();
            // Child process
            if (pid == 0) {
                setpgrp();
                cmd->execute();
                exit(0);
            }
            // Parent process
            else if (pid > 0) {
                //we save the fg process pid for the signal sending
                setForegroundPid(pid);

                int result = waitpid(pid, nullptr, 0);

                //if we did not send signal we return value to default
                setForegroundPid(-1);

                if(result == -1){
                    perror("smash error: waitpid failed");
                }
            }
            else {
                perror("smash error: fork failed");
            }
        }
        //background command
        else{

            pid_t pid = fork();
            // Child process
            if (pid == 0) {
                setpgrp();
                cmd->execute();
                exit(0);
            }
            // Parent process
            else if (pid > 0) {
                jobsList -> addJob(cmd,pid);
                //so we can print the failure if the process failed before the smash
                usleep(10000);
            }
            else {
                perror("smash error: fork failed");
            }
        }
    }

    // removing finished jobs after execution
    jobsList->removeFinishedJobs();
}

string SmallShell::getName() const {
    return shellName;
}

void SmallShell::setName(const string &name) {
    shellName = name;
}

string SmallShell::getLastPwd() const {
    return lastPwd;
}

void SmallShell::setLastPwd(const string &currPwd) {
    lastPwd = currPwd;
}

AliasHelper* SmallShell::getAliasHelper() {
    return aliasHelper;
}

pid_t SmallShell::getForegroundPid() const {
    return fgPid;
}

void SmallShell::setForegroundPid(pid_t pid) {
    fgPid = pid;
}

pid_t SmallShell::getSmashPid() const {
 return smash_pid; 
}



bool JobsList::JobEntry::isJobFinished() const{
    int status;
    pid_t result = waitpid(pid, &status, WNOHANG);
    if (result == 0) {
        return false;
    }

    if (result < 0) {
        if (errno == ECHILD){
            return false;
        }
        perror("smash error: waitpid failed");
        return true;
    }

    return WIFSIGNALED(status) || WIFEXITED(status);

}




void JobsList::addJob(Command *cmd, pid_t pid) {
    removeFinishedJobs();
    jobList.push_back(JobEntry(maxJobId+1,pid,cmd->getCmdStr()));
    maxJobId++;
}
void JobsList :: removeFinishedJobs() {
    if (getpid() != SmallShell::getInstance().getSmashPid()) {
        return;
    }

    for (auto it = jobList.begin(); it != jobList.end(); ) {
        if (it -> isJobFinished()){
            it = jobList.erase(it);  // erase returns the next valid iterator
        } else {
            ++it;
        }
    }
    sort(jobList.begin(), jobList.end());
    if (!jobList.empty()) {
        maxJobId = jobList.back().getJobId();
    }
    else{
        maxJobId = 0;
    }
}

JobsList::JobEntry * JobsList::getJobById(int jobId) {
    for (auto& it : jobList) {
        if(it.getJobId() == jobId) {
            return &it;
        }
    }
    return nullptr;
}

void JobsList :: removeJobById(int id) {
    for (auto it = jobList.begin(); it != jobList.end(); ) {
        if (it -> getJobId() == id){
            it = jobList.erase(it);
        }   else {
            ++it;
        }
    }
    sort(jobList.begin(), jobList.end());
    if (!jobList.empty()) {
        maxJobId = jobList.back().getJobId();
    }
    else{
        maxJobId = 0;
    }

}

JobsList::JobEntry * JobsList::getLastJob(int *lastJobId) {
    if (jobList.empty()) return nullptr;
    *lastJobId = jobList.back().getJobId();
    return &jobList.back();
}


void JobsCommand::execute() {
    jobsList -> printJobsList();
}

bool isNum(const char* str) {
    if (!str) return false;
    for (int i = 0; str[i]; ++i) {
        if (!isdigit(str[i])) return false;
    }
    return true;
}

void ForegroundCommand::execute() {
    int jobId;
    JobsList :: JobEntry* job;
    if (args[1] == nullptr) {
        job = jobsList->getLastJob(&jobId);
        if (job == nullptr) {
            cerr << "smash error: fg: jobs list is empty" << endl;
            return;
        }
    }
    else if(args[2] != nullptr || !isNum(args[1])) {
        cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }
    else {
        job = jobsList->getJobById(stoi(args[1]));
        if (job == nullptr) {
            cerr << "smash error: fg: job-id " << args[1] << " does not exist" << endl;
            return;
        }
    }

    pid_t pid = job->getPid();

    SmallShell::getInstance().setForegroundPid(pid);


    cout << job ->getCommandLine() << " " << job ->getPid() <<endl;
    int* status;
    pid_t result = waitpid(job->getPid(),status,0);
    if (result < 0) {
        perror("smash error: waitpid failed");
    }

    SmallShell::getInstance().setForegroundPid(-1);


    //we remove the job without waiting because we already waited
    jobsList->removeJobById(job->getJobId());
}

bool isNumber(char* arg,int& num) {
    char* ch = arg;
    num = 0;
    if(*ch == '-') ch++;
    while(*ch != '\0') {
        if (!isdigit(*ch)){
            return false;
        }
        ch++;
    }
    num = stoi(arg);
    return true;
}


void KillCommand::execute() {
    int jobID, sigNum;
    if(args[1] == nullptr || args[2] == nullptr || args[3] != nullptr || args[1][0] != '-' ||
            strcmp(args[1], "-") == 0 || !isNumber(args[1] + 1,sigNum) || !isNumber(args[2],jobID)) {
        cerr << "smash error: kill: invalid arguments" <<endl;
        return;
    }

    JobsList :: JobEntry* job = jobsList -> getJobById(jobID);
    if (job == nullptr) {
        cerr << "smash error: kill: job-id " << jobID <<" does not exist" << endl;
        return;
    }

    if(kill(job->getPid(),sigNum) == -1) {
        cout << "signal number " << sigNum <<" was sent to pid " << job->getPid() <<endl;
        perror("smash error: kill failed");
        return;
    }
    cout << "signal number " << sigNum <<" was sent to pid " << job->getPid() <<endl;
}

void QuitCommand::execute() {

    if(args[1] != nullptr && strcmp(args[1], "kill") == 0) {
        jobsList->removeFinishedJobs();
        cout << "smash: sending SIGKILL signal to " << jobsList->getJobsListSize() <<" jobs:"<<endl;
        vector<JobsList::JobEntry> jobs = jobsList->getJobEntries();
        for(auto& it : jobs) {
            cout << it.getPid() <<": "<<it.getCommandLine() <<endl;
        }
        jobsList->killAllJobs();
    }
    jobsList->getJobEntries().clear();
    jobsList->updateMaxId();

    //at the end we exit
    exit(0);
}


void JobsList :: printJobsList() {
    removeFinishedJobs();
    for (auto it = jobList.begin(); it != jobList.end(); ++it) {
        cout << "[" << it->getJobId() << "] " << it->getCommandLine() << endl;
    }
}

void JobsList::killAllJobs() {
    for (auto& job : jobList) {
        if(kill(job.getPid(), SIGKILL)) {
            perror("smash error: kill failed");
        }
    }
    jobList.clear();
    maxJobId = 0;
}


///aliasHelper Class
bool AliasHelper::addAliasName(const string& aliasName,const string& cmd) {
    for(const auto& name:reservedWords){
        if(name == aliasName){
            return false;
        }
    }
    for(const auto& pair:addedAliasList){
        if(pair.first == aliasName){
            return false;
        }
    }
    addedAliasList.emplace_back(aliasName,cmd);
    return true;
}

bool AliasHelper::removeAliasName(const string& aliasName) {
    if(addedAliasList.empty()){
        return false;
    }
    for (auto it = addedAliasList.begin(); it != addedAliasList.end(); ++it) {
        if (it->first == aliasName) {
            addedAliasList.erase(it);
            return true;
        }
    }
    return false;
}

void AliasHelper::printAllAliasNames() const {
    for (const auto& pair : addedAliasList) {
        cout << pair.first << "='" << pair.second << "'" << endl;
    }
}

string AliasHelper::convertAliasToOriginal(const string& cmd_line) {
    //we get firstWord
    istringstream str(cmd_line);
    string firstWord;
    str >> firstWord;
    //now we check if this alias is added, if so we return string to its original form
    for(const auto& pair:addedAliasList){
        if(pair.first == firstWord){
            string remainingCmd;
            getline(str,remainingCmd);
            // returning original string
            return pair.second + remainingCmd;
        }
    }
    // if no alias was found we return the same cmd_line bacause it is the original
    return cmd_line;
}

///alias command

AliasCommand::AliasCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}



void AliasCommand::execute() {
    if(args[1] == nullptr){
        SmallShell::getInstance().getAliasHelper()->printAllAliasNames();
        return;
    }

    istringstream cmd_l(cmd_line);
    string firstWord;
    cmd_l >> firstWord;

    //get <name>='<command>'
    string remainingCmd;
    getline(cmd_l,remainingCmd);

    //trim whitspaces from the sides
    string trimed_remainingCmd = _trim(remainingCmd);

    if (trimed_remainingCmd.find("='") == string::npos || (!trimed_remainingCmd.empty() && trimed_remainingCmd.back() != '\'')) {
        cerr <<"smash error: alias: invalid alias format"<<endl;
        return;
    }

    // Must have exactly two single quotes
    size_t first_quote = trimed_remainingCmd.find('\'');
    size_t last_quote = trimed_remainingCmd.rfind('\'');

    if (first_quote == string::npos || first_quote == last_quote) {
        cerr <<"smash error: alias: invalid alias format"<<endl;
        return;
    }

    // The character before the first quote must be '='
    if (first_quote < 1 || trimed_remainingCmd[first_quote - 1] != '=') {
        cerr << "smash error: alias: invalid alias format" << endl;
        return;
    }

    // Extract name and validate it and check if command empty (better than not)
    string name = trimed_remainingCmd.substr(0, first_quote - 1);
    string command = trimed_remainingCmd.substr(first_quote + 1, last_quote - first_quote - 1);

    if (!regex_match(name, regex("^[a-zA-Z0-9_]+$"))) {
        cerr << "smash error: alias: invalid alias format" << endl;
        return;
    }

    //now we check the result of inserting a new alias
    if(!SmallShell::getInstance().getAliasHelper()->addAliasName(name,command)){
        cerr << "smash error: alias: " << name << " already exists or is a reserved command" << endl;
    }
}

UnAliasCommand::UnAliasCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void UnAliasCommand::execute() {
    if(args[1] == nullptr){
        cerr << "smash error: unalias: not enough arguments" << endl;
    }
    int i = 1;
    while(args[i] != nullptr){
        if(!SmallShell::getInstance().getAliasHelper()->removeAliasName(args[i])){
            cerr << "smash error: unalias: " << args[i] << " alias does not exist" << endl;
            break;
        }
        i++;
    }
}

bool UnSetEnvCommand::checkVariableExistence(const char * var, const char * envFile, size_t envSize) {
    string variable = string(var) + "=";
    size_t index = 0;

    while (index < envSize) {
        const char* entry = &envFile[index];
        if (strncmp(entry, variable.c_str(), variable.length()) == 0) {
            return true;
        }
        index += strlen(entry) + 1;
    }

    return false;
}


void UnSetEnvCommand::execute() {
    //cout << "TEST| entered the function" <<endl;
    if(args[1] == nullptr) {
        cerr << "smash error: unsetenv: not enough arguments" << endl;
        return;
    }
    extern char **__environ;
    //cout << "TEST| just a test" <<endl;
    int fdEnv = open("/proc/self/environ",O_RDONLY);
    //cout << "TEST| read from file" <<endl;
    if(fdEnv == -1) {
        perror("smash error: open failed");
        return;
    }

    //cout << "TEST| about to start tokenizing" <<endl;
    char buffer[MAX_BUFF_SIZE];
    ssize_t bytesRead = read(fdEnv, buffer, MAX_BUFF_SIZE);

    if(bytesRead == -1){
        perror("smash error: read failed");
        return;
    }
    //cout << "TEST| read from file -> about to close" <<endl;
    if(close(fdEnv) == -1) {
        perror("smash error: close failed");
        return;
    }
    //cout << "TEST| closed" <<endl;
    int i = 1;
    while(args[i] != nullptr) {
        //cout << "TEST| while loop - > checking variable existance" <<endl;
        if(!checkVariableExistence(args[i],buffer,static_cast<size_t>(bytesRead))) {
            cerr << "smash error: unsetenv: " << args[i]<<" does not exist" <<endl;
            return;
        }
        //cout << "TEST| variable exists" <<endl;
        for (char **env = __environ; *env != nullptr; ++env) {
            char* equalSign = strchr(*env, '=');
            size_t keyLen = equalSign - *env;

            if (strncmp(*env, args[i], keyLen) == 0 && strlen(args[i]) == keyLen) {
                char **shift = env;
                do {
                    *shift = *(shift + 1);
                    ++shift;
                } while (*shift != nullptr);

                break;
            }
        }

        i++;
    }
    //cout << "TEST| left while 1" <<endl;
}

SysInfoCommand::SysInfoCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}


static string readLine(const string& path) {
    ifstream file(path);
    string out;
    getline(file, out);
    return out;
}

static time_t getBootTime() {
    ifstream file("/proc/stat");
    string key;
    long value;
    while (file >> key) {
        if (key == "btime") {
            file >> value;
            return static_cast<time_t>(value);
        }
    }
    return 0;
}

static string getSystemName() {
    ifstream file("/proc/version");
    string word;
    file >> word; // first word should be Linux
    return word;
}

void SysInfoCommand::execute() {
    // system name
    cout << "System: " << getSystemName() << endl;

    // Hostname
    string hostname = readLine("/proc/sys/kernel/hostname");
    cout << "Hostname: " << hostname << endl;

    // Kernel
    string kernel = readLine("/proc/sys/kernel/osrelease");
    cout << "Kernel: " << kernel << endl;

    // Architecture we assume always x86_64 according to the SEGEL
    cout << "Architecture: x86_64" << endl;

    // Boot time
    time_t boot_time = getBootTime();
    struct tm* info = localtime(&boot_time);
    char time_buffer[64];
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", info);
    cout << "Boot Time: " << time_buffer << endl;
}

RedirectionCommand::RedirectionCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
    string cmd = this -> cmd_str;
    size_t signPos = cmd.find('>');
    cmdToRun = _trim(cmd.substr(0, signPos));
    overrideFlag = true;;
    if (cmd[signPos + 1] == '>') {
        overrideFlag = false;;
        signPos++;
    }
    outputFile = _trim(cmd.substr(signPos + 1));
}

void RedirectionCommand::execute() {
    int ogStdout = dup(STDOUT_FILENO);
    if (ogStdout == -1) {
        perror("smash error: dup failed");
        return;
    }

    int newFileFD = open(outputFile.c_str(), O_WRONLY | O_CREAT | (overrideFlag ? O_TRUNC : O_APPEND), 0644);
    if (newFileFD == -1) {
        perror("smash error: open failed");
        return;
    }

    if (dup2(newFileFD, STDOUT_FILENO) == -1) {
        perror("smash error: dup2 failed");
        if(close(newFileFD) == -1){
            perror("smash error: close failed");
        }
        return;
    }

    SmallShell::getInstance().executeCommand(cmdToRun.c_str());

    if (dup2(ogStdout, STDOUT_FILENO) == -1) {
        perror("smash error: dup2 failed");
    }

    if(close(newFileFD) == -1 || close(ogStdout) == -1){
        perror("smash error: close failed");
    }
}


PipeCommand::PipeCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
    size_t pipePos = cmd_str.find('|');
    // consider checking if | actually exists

    stderrPipe = (cmd_str[pipePos + 1] == '&');

    command1 = _trim(cmd_str.substr(0, pipePos));
    command2 = stderrPipe ? _trim(cmd_str.substr(pipePos + 2)) : _trim(cmd_str.substr(pipePos + 1));
}

void PipeCommand::execute() {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("smash error: pipe failed");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("smash error: fork failed");
        return;
    }

    if (pid1 == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], stderrPipe ? STDERR_FILENO : STDOUT_FILENO);
        close(pipefd[1]);
        SmallShell::getInstance().executeCommand(command1.c_str());
        exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("smash error: fork failed");
        return;
    }

    if (pid2 == 0) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        SmallShell::getInstance().executeCommand(command2.c_str());
        exit(0);
    }
    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, nullptr, 0);
    waitpid(pid2, nullptr, 0);

}



unsigned long getDirSize(const std::string& path) {
    unsigned long totalAllocatedBytes = 0;

    int fd = open(path.c_str(), O_RDONLY | O_DIRECTORY);
    if (fd == -1) {
        perror("smash error: open failed");
        return 0;
    }

    char buf[MAX_BUFF_SIZE];

    while (true) {
        int nread = syscall(SYS_getdents64, fd, buf, MAX_BUFF_SIZE);
        if (nread == -1) {
            perror("smash error: getdents64 failed");
            break;
        }
        if (nread == 0) {
            break; // end of directory
        }

        int bpos = 0;
        while (bpos < nread) {
            struct linux_dirent64* d =
                    (struct linux_dirent64*)(buf + bpos);
            string name = d->d_name;
            if (name != "." && name != "..") {
                string fullPath = path + "/" + name;
                struct stat st{};
                if (lstat(fullPath.c_str(), &st) == -1) {
                    perror("smash error: lstat failed");
                } else {
                    if (S_ISDIR(st.st_mode)) {
                        // Recursing into directory
                        totalAllocatedBytes += getDirSize(fullPath);
                    } else {
                        // Using filesystem disk allocation: st_blocks = # of 512B blocks
                        totalAllocatedBytes += (unsigned long)st.st_blocks * 512UL;
                    }
                }
            }
            bpos += d->d_reclen;
        }
    }
    close(fd);
    return totalAllocatedBytes;
}

DiskUsageCommand::DiskUsageCommand(const char* cmd_line)
        : BuiltInCommand(cmd_line) {}

void DiskUsageCommand::execute() {
    // No argument → use "."
    if (args[1] == nullptr) {
        args[1] = (char*)".";
        args[2] = nullptr;
    }
    // Too many arguments
    if (args[2] != nullptr) {
        std::cerr << "smash error: du: too many arguments" << std::endl;
        return;
    }
    string path = args[1];
    unsigned long totalBytes = getDirSize(path);
    //converting to Kb
    unsigned long totalKB = (totalBytes + 1023) / 1024;
    std::cout << "Total disk usage: " << totalKB << " KB" << std::endl;
}



WhoAmICommand::WhoAmICommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}


bool get_uid(uid_t& uid) {
    const char* path = "/proc/self/status";
    char buf[MAX_BUFF_SIZE];

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("smash error: open failed");
        return false;
    }

    ssize_t n = read(fd, buf, MAX_BUFF_SIZE - 1);
    if (n == -1) {
        perror("smash error: read failed");
        close(fd);
        return false;
    }
    if (close(fd) == -1) {
        perror("smash error: close failed");
        return false;
    }

    buf[n] = '\0';

    char* line = strtok(buf, "\n");
    while (line != nullptr) {

        if (strncmp(line, "Uid:", 4) == 0) {
            char* p = line + 4;

            while (*p == ' ' || *p == '\t') {
                p++;
            }

            unsigned long val = strtoul(p, nullptr, 10);
            uid = (uid_t)val;

            return true;
        }

        line = strtok(nullptr, "\n");
    }

    return false;
}



static bool get_name_gid_and_dir(uid_t uid, string& pw_name, gid_t& gid, string& pw_dir) {
    const char* path = "/etc/passwd";
    char buf[MAX_BUFF_SIZE];

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("smash error: open failed");
        return false;
    }

    ssize_t n = read(fd, buf, MAX_BUFF_SIZE - 1);
    if (n == -1) {
        perror("smash error: read failed");
        close(fd);
        return false;
    }
    if (close(fd) == -1) {
        perror("smash error: close failed");
        return false;
    }

    buf[n] = '\0';

    char* line = strtok(buf, "\n");
    while (line != nullptr) {
        char* fields[7] = { nullptr };
        size_t fcount = 0;

        char* p = line;
        fields[fcount++] = p;

        while (fcount < 7) {
            char* colon = strchr(p, ':');
            if (!colon)
                break;

            *colon = '\0';      
            p = colon + 1;      
            fields[fcount++] = p;
        }

        if (fcount >= 6) {
            // fields: 0=name, 2=uid, 3=gid, 5=home
            unsigned long file_uid = strtoul(fields[2], nullptr, 10);

            if ((uid_t)file_uid == uid) {
                pw_name = fields[0];
                gid     = (gid_t)strtoul(fields[3], nullptr, 10);
                pw_dir  = fields[5];
                return true;
            }
        }

        line = strtok(nullptr, "\n");
    }

    return false;
}




void WhoAmICommand::execute() {
    uid_t uid;
    if (!get_uid(uid)) {
        return;
    }

    string pw_name, pw_dir;
    gid_t gid;
    if (!get_name_gid_and_dir(uid, pw_name, gid, pw_dir)) {
        return;
    }

    cout << pw_name << endl << uid << endl << gid << endl << pw_dir << endl;
}


/// usbinfo bonus 10 points
static bool readFile(const std::string& path, std::string& out)
{
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
        return false;

    char buffer[256];
    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (n <= 0)
        return false;

    buffer[n] = '\0';
    out = buffer;

    // remove trailing newline if exists
    if (!out.empty() && out.back() == '\n')
        out.pop_back();

    return true;
}

USBInfoCommand::USBInfoCommand(const char* cmd_line)
        : BuiltInCommand(cmd_line)
{
}

void USBInfoCommand::execute()
{
    const char* base = "/sys/bus/usb/devices";
    DIR* dir = opendir(base);
    if (!dir) {
        std::cerr << "smash error: usbinfo: no USB devices found" << endl;
        return;
    }

    struct DeviceEntry {
        int devnum;
        string name;
        string vendor;
        string product;
        string manufacturer;
        string prodName;
        string maxpower;
    };

    std::vector <DeviceEntry> list;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        // We only want directories like "1-1", "2-3.1".. to find devices
        string devPath = string(base) + "/" + entry->d_name;

        struct stat st;
        if (stat(devPath.c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
            continue;

        // Read devnum
        string devnumStr;
        if (!readFile(devPath + "/devnum", devnumStr))
            continue; // this is not a device, we only want devices

        int devnum = atoi(devnumStr.c_str());

        DeviceEntry d;
        d.devnum = devnum;
        d.name = entry->d_name;

        //read fields and failing means N/A
        if (!readFile(devPath + "/idVendor", d.vendor))
            d.vendor = "N/A";

        if (!readFile(devPath + "/idProduct", d.product))
            d.product = "N/A";

        if (!readFile(devPath + "/manufacturer", d.manufacturer))
            d.manufacturer = "N/A";

        if (!readFile(devPath + "/product", d.prodName))
            d.prodName = "N/A";

        if (!readFile(devPath + "/bMaxPower", d.maxpower))
            d.maxpower = "N/A";
        else {
            // Some formats have "200mA" already. Others are like "200"
            if (d.maxpower.find("mA") == std::string::npos)
                d.maxpower += "mA";
        }

        list.push_back(d);
    }
    closedir(dir);
    if (list.empty()) {
        std::cerr << "smash error: usbinfo: no USB devices found" << endl;
        return;
    }

    //sort by devnum
    std::sort(list.begin(), list.end(),
              [](const DeviceEntry& a, const DeviceEntry& b) {
                  return a.devnum < b.devnum;
              });
    //result
    for (const auto& d : list) {
        std::cout << "Device " << d.devnum << ": ";
        std::cout << "ID " << d.vendor << ":" << d.product << " ";
        std::cout << d.manufacturer << " " << d.prodName << " ";
        std::cout << "MaxPower: " << d.maxpower << "\n";
    }
}





