/*
 * Adapted from 
 * http://www-theorie.physik.unizh.ch/~dpotter/howto/daemonize
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "daemonize.h"

namespace isaword {

/**
 * Forks off the current process as a daemon.
 * @param log_file file to log stdout and stderr to.
 * May be NULL to indicate no logging.
 * @return result of fork(): positive PID in for the
 * parent process, 0 for the child process, a negative
 * PID in case of failure.
 */
pid_t daemonize(const char* log_file) {
    pid_t sid;

    /* already a daemon */
    if ( getppid() == 1 ) return -1;

    /* Fork off the parent process */
    pid_t pid = fork();
    
    if (pid < 0) {
        return pid;
    }
    
    /* If we got a good PID, then we can exit the parent process. */
    if (pid > 0) {
        return pid;
    }

    /* At this point we are executing as the child process */

    /* Change the file mode mask */
    umask(0);

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        return sid;
    }

    /* Change the current working directory.  This prevents the current
       directory from being locked; hence not being able to remove it. */
    if ((chdir("/")) < 0) {
        return -1;
    }

    /* Redirect standard files to proper loging output. */
    const char* outfile = log_file;
    
    if (outfile == NULL) {
        outfile = "/dev/null";
    }
    
    freopen( "/dev/null", "r", stdin);
    freopen( outfile, "w", stdout);
    freopen( outfile, "w", stderr);
    
    return 0;
}

} /* namespace isaword */
