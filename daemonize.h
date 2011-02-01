/*
 * Adapted from 
 * http://www-theorie.physik.unizh.ch/~dpotter/howto/daemonize
 */

#ifndef ISAWORD_DAEMONIZE_H
#define ISAWORD_DAEMONIZE_H

#include <sys/types.h>

namespace isaword {
    
/**
 * Forks off the current process as a daemon.
 * @param log_file file to log stdout and stderr to.
 * May be NULL to indicate no logging.
 * @return result of fork(): positive PID in for the
 * parent process, 0 for the child process, a negative
 * PID in case of failure.
 */
pid_t daemonize(const char* log_file = NULL);

} /* namespace isaword */

#endif
