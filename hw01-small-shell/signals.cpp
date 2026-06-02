#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {

    //we get the fg pid (if exists)
    pid_t fgPid = SmallShell::getInstance().getForegroundPid();

    std::cout << "smash: got ctrl-C" << std::endl;

    if (fgPid > 0) {
        if (kill(fgPid, SIGINT) == 0) {
            cout << "smash: process " << fgPid << " was killed" << endl;
        } else {
            perror("smash error: kill failed");
        }

        //when fg process killed return to default value
        SmallShell::getInstance().setForegroundPid(-1);

    }
}
