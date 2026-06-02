// Ver: 04-11-2025
#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <set>
#include <sys/wait.h>
#include <algorithm>
#include <map>
#include <vector>


#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

using namespace std;

class Command {
protected:

    string cmd_str;
    const char* cmd_line;
    char** args;
    bool isBackgroundCmd;
    bool isForegroundCmd;

public:
    Command(const char *cmd_line);

    virtual ~Command() = default;

    virtual void execute() = 0;

    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed

    virtual string getCmdStr();

    bool getIsBackgroundCmd() const {
        return isBackgroundCmd;
    }
    bool getIsForegroundCmd() const {
        return isForegroundCmd;
    }

    void setBackgroundCmd() {
        isBackgroundCmd = true;
        isForegroundCmd = false;
    }
    void setForegroundCmd() {
        isForegroundCmd = true;
        isBackgroundCmd = false;
    }



};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand() {
    }
};

class ExternalCommand : public Command {
protected:
    string unchanged_str;
public:

    ExternalCommand(const char *cmd_line, const char *unchanged_str);

    virtual string getCmdStr(){
        return unchanged_str;
    }
    virtual ~ExternalCommand() {
    }

    void execute() override;

    bool isComplex() const;
};

class RedirectionCommand : public BuiltInCommand {
    string outputFile;
    bool overrideFlag;
    string cmdToRun;
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() = default;

    void execute() override;
};

class PipeCommand : public BuiltInCommand {
    string command1;
    string command2;
    bool stderrPipe;
public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {
    }

    void execute() override;
};

class DiskUsageCommand : public BuiltInCommand {
public:
    DiskUsageCommand(const char *cmd_line);

    virtual ~DiskUsageCommand() {
    }

    void execute() override;
};

class WhoAmICommand : public BuiltInCommand {
public:
    WhoAmICommand(const char *cmd_line);

    virtual ~WhoAmICommand() {
    }

    void execute() override;
};

class USBInfoCommand : public BuiltInCommand {
    // TODO: Add your data members **BONUS: 10 Points**
public:
    USBInfoCommand(const char *cmd_line);

    virtual ~USBInfoCommand() {
    }

    void execute() override;
};

class ChpromptCommand : public BuiltInCommand{
private:
    string prompt;
public:
    ChpromptCommand(const char* cmd_line);

    virtual ~ChpromptCommand(){
    }

    void execute() override;

    void setPrompt(string prompt){
        this->prompt = prompt;
    }
};

class ChangeDirCommand : public BuiltInCommand {

public:

    ChangeDirCommand(const char *cmd_line);

    virtual ~ChangeDirCommand() {
    }

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() {
    }

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {
    }

    void execute() override;
};


//real down
class JobsList {
public:
    class JobEntry {
        int jobId;
        pid_t pid;
        string cmdStr;

    public:
        JobEntry(int jobId, pid_t pid,const string& cmdStr) : jobId(jobId), pid(pid), cmdStr(cmdStr) {
        }
        int getJobId() const {return jobId;}
        pid_t getPid() const {return pid;}
        string getCommandLine() const {return cmdStr;}
        bool operator<(const JobEntry& other) const {
            return jobId < other.getJobId();
        }
        bool isJobFinished() const;
    };
private:
    vector<JobEntry> jobList;
    int maxJobId;
public:

    JobsList() : maxJobId(0){}

    ~JobsList() = default;

    void addJob(Command *cmd, pid_t pid);

    void printJobsList();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);


    int getJobsListSize() const {
        return jobList.size();
    }

    vector<JobEntry>& getJobEntries() {
        return jobList;
    }

    void updateMaxId() {
        if(jobList.empty()) {
            maxJobId = 0;
        }else {
            sort(jobList.begin(), jobList.end());
            maxJobId = jobList.end()->getJobId();
        }
    }


};

class JobsCommand : public BuiltInCommand {
    JobsList* jobsList;
public:
    JobsCommand(const char *cmd_line,JobsList *jobs):BuiltInCommand(cmd_line),jobsList(jobs){}

    virtual ~JobsCommand()=default;

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobsList* jobsList;
public:
    KillCommand(const char *cmd_line, JobsList *jobs) :BuiltInCommand(cmd_line),jobsList(jobs){}

    virtual ~KillCommand() {
    }

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList* jobsList;
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs):BuiltInCommand(cmd_line),jobsList(jobs){}

    virtual ~ForegroundCommand() = default;

    void execute() override;
};

class QuitCommand : public BuiltInCommand {
    JobsList *jobsList;
public:
    QuitCommand(const char *cmd_line, JobsList *jobs):BuiltInCommand(cmd_line),jobsList(jobs){}

    virtual ~QuitCommand()=default;

    void execute() override;
};


class AliasHelper{
private:
    vector <pair<string, string>> addedAliasList;
    vector <string> reservedWords = {
            "chprompt","chprompt&", "showpid","showpid&","pwd","pwd&", "cd",
            "jobs","jobs&", "fg","fg&","quit","quit&", "kill","alias",
            "alias&","unalias","unsetenv","sysinfo","sysinfo&","du","whoami","whoami&","usbinfo","usbinfo&",
            ">", "<", "|", "|&", ">>", "<<"

    };

public:
    bool addAliasName(const string& aliasName,const string& cmd_line);
    bool removeAliasName(const string& aliasName);
    void printAllAliasNames() const;
    string convertAliasToOriginal(const string& cmd_line);
};



class AliasCommand : public BuiltInCommand {
public:
    AliasCommand(const char *cmd_line);

    virtual ~AliasCommand() {
    }

    void execute() override;
};

class UnAliasCommand : public BuiltInCommand {
public:
    UnAliasCommand(const char *cmd_line);

    virtual ~UnAliasCommand() {
    }

    void execute() override;
};

class UnSetEnvCommand : public BuiltInCommand {
public:
    UnSetEnvCommand(const char *cmd_line) : BuiltInCommand(cmd_line){}

    virtual ~UnSetEnvCommand() = default;

    bool checkVariableExistence(const char*, const char*, size_t);

    void execute() override;

};

class SysInfoCommand : public BuiltInCommand {
public:
    SysInfoCommand(const char *cmd_line);

    virtual ~SysInfoCommand() {
    }

    void execute() override;
};

class SmallShell {
private:
    // TODO: Add your data members
    string shellName;
    string lastPwd;
    JobsList* jobsList;
    AliasHelper* aliasHelper;
    pid_t smash_pid;

    pid_t fgPid;

    SmallShell();

public:
    Command *CreateCommand(const char *cmd_line,const char *unchanged_cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell();

    void executeCommand(const char *cmd_line);

    // TODO: add extra methods as needed
    pid_t getSmashPid() const;
    string getName() const;
    void setName(const string& name);
    string getLastPwd() const;
    void setLastPwd(const string& currPwd);
    AliasHelper* getAliasHelper();
    pid_t getForegroundPid() const;
    void setForegroundPid(pid_t pid);


};

#endif //SMASH_COMMAND_H_

