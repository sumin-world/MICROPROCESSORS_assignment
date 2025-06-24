/* 
 * tsh - A tiny shell program with job control
 * 
 * <김수민 2022122051>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline)
{
    sigset_t mask_all, prev_one; 
    char *argv[MAXARGS];
    sigfillset(&mask_all);
    
    int bg = parseline(cmdline, argv);

    // Check if the command is a built-in command: quit, bg, fg, or jobs
    if (!strncmp(cmdline, "quit", 4) || 
        !strncmp(cmdline, "bg", 2) || 
        !strncmp(cmdline, "fg", 2) || 
        !strncmp(cmdline, "jobs", 4)) 
    {
        if (!builtin_cmd(argv))
            printf("Not a builtin command.\n");
    }
    else
    {
        // Run foreground or background
        if (bg == 0 || (argv[0] != NULL && bg == 1))
        {
            int pid = fork();
            
            // Block SIGCHLD signals
            if (sigprocmask(SIG_BLOCK, &mask_all, &prev_one))
                printf("sigprocmask function failed.\n");
            
            // Child process
            if (pid == 0)
            {
                // Set process group ID for the child
                if (setpgid(0, 0))
                    printf("setpgid function failed.\n");
                
                // Unblock signals
                if (sigprocmask(SIG_SETMASK, &prev_one, NULL))
                    printf("sigprocmask function failed.\n");
                
                // Execute the command
                if (execve(argv[0], argv, NULL) == -1)
                {
                    fprintf(stdout, "%s: Command not found\n", argv[0]);
                    exit(0);
                }
            }
            // Parent process
            else if (pid > 0)
            {
                // Block signals
                if (sigprocmask(SIG_BLOCK, &mask_all, NULL))
                    printf("sigprocmask function failed.\n");
                
                // Determine if the job is in the foreground or background
                int state = (bg == 0) ? FG : BG;
                addjob(jobs, pid, state, cmdline);
                
                // Unblock signals
                if (sigprocmask(SIG_SETMASK, &prev_one, NULL))
                    printf("sigprocmask function failed.\n");
                
                // If the job is in the foreground, wait for it to finish
                if (bg == 0)
                    waitfg(pid);
                // If the job is in the background, print information about it
                else
                {
                    struct job_t *job = getjobpid(jobs, pid);
                    printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
                }
            }
            else
            {
                printf("fork function failed.\n");
            }
        }
    }

    return;
}
/*
 * parseline - Parse the command line and build the argv array.
 *
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.
 */
int parseline(const char *cmdline, char **argv)
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;

    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv)
{
	// Check for the quit command
	if (!strncmp(argv[0], "quit", 4))
	{
		exit(0);
	}
	// Check for bg and fg commands
	else if (!strncmp(argv[0], "bg", 2) || !strncmp(argv[0], "fg", 2))
	{
		do_bgfg(argv);
		return 1;
	}
	// Check for the jobs command
	else if (!strncmp(argv[0], "jobs", 4))
	{
		listjobs(jobs);
		return 1;
	}

    return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the built-in bg and fg commands
 */
void do_bgfg(char **argv) 
{	
	if (argv[1] == NULL)
	{
		// Check if the command requires a PID or %%jobid argument
		if (!strncmp(argv[0], "bg", 2) || !strncmp(argv[0], "fg", 2))
		{
			printf("%s command requires PID or %%jobid argument\n", argv[0]);
		}
		return;
	}

	// background command
	if (!strncmp(argv[0], "bg", 2))
	{
		struct job_t *target_job;
		int jid = 0;
		pid_t pid = 0;
		
		// job id
		if (*argv[1] == '%')
		{
			argv[1]++;
			jid = atoi(argv[1]);
			target_job = getjobjid(jobs, jid);
			
			// error handling for non-existent job
			if (target_job == NULL)
			{
				printf("%%%d: No such job\n", jid);
				return;
			}

			// Change the job state to background
			target_job->state = BG;
			printf("[%d] (%d) %s", target_job->jid, target_job->pid, target_job->cmdline);
			
			// Send a continue signal to the process group to resume execution
			if (kill(-(target_job->pid), SIGCONT))
				printf("kill function failed.\n");
		}
		// Parse the process id
		else if (('0' <= *argv[1]) && ('9' >= *argv[1]))
		{
			pid = (pid_t)atoi(argv[1]);
			target_job = getjobpid(jobs, pid);

			// error handling for non-existent process
			if (target_job == NULL)
			{
				printf("(%d): No such process\n", pid);
				return;
			}
			
			// Change the job state to background
			target_job->state = BG;
			printf("[%d] (%d) %s", target_job->jid, target_job->pid, target_job->cmdline);
			
			// Send a continue signal to the process group to resume execution 
			if (kill(-pid, SIGCONT))
				printf("kill function failed.\n");
		}
		// error for invalid argument
		else
		{
			printf("bg: argument must be a pid or %%jobid\n");
		}
	}
	//foreground command
	else
	{
		struct job_t *target_job;
		int jid = 0;
		pid_t pid = 0;

		// job id
		if (*argv[1] == '%')
		{
			argv[1]++;
			jid = atoi(argv[1]);
			target_job = getjobjid(jobs, jid);

			// error handling for non-existent job
			if (target_job == NULL)
			{
				printf("%%%d: No such job\n", jid);
				return;
			}

			// Change the job state to foreground

			target_job->state = FG;

			// send a continue signal to the process group to resume execution
			if (kill(-(target_job->pid), SIGCONT))
				printf("kill function failed.\n");
			
			// wait for the foreground process to finish
			waitfg(target_job->pid);
		}
		// pid
		else if (('0' <= *argv[1]) && ('9' >= *argv[1]))
		{
			pid = (pid_t)atoi(argv[1]);
			target_job = getjobpid(jobs, pid);

			// error handling
			if (target_job == NULL)
			{
				printf("(%d): No such process\n", pid);
				return;
			}

			target_job->state = FG;
			if (kill(-pid, SIGCONT))
				printf("kill function failed.\n");
			
			waitfg(pid);
		}
		// error
		else
		{
			printf("fg: argument must be a pid or %%jobid\n");
		}
	}
    return;
}
/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
	struct job_t *fg_job = getjobpid(jobs, pid);

	// Use a busy loop to wait until the specified process is no longer in the foreground
	while(fg_job->state == FG)
	{
		// Sleep for a short duration to reduce CPU usage during the waiting period
		sleep(0.1);
	}
    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig)
{
    // Save and restore errno to avoid interfering with system calls
    int olderrno = errno;
    sigset_t mask_all, prev_all;
    int status = 0;
    pid_t pid = 0;

    // Block all signals in the handler
    sigfillset(&mask_all);

    // Reap all terminated or stopped children using non-blocking waitpid
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0)
    {
        // Process terminated.
        if (WIFEXITED(status) || WIFSIGNALED(status))
        {
            struct job_t *delete_job = getjobpid(jobs, pid);

            // Print termination message for SIGINT
            if (WIFSIGNALED(status))
                printf("Job [%d] (%d) terminated by signal 2\n", delete_job->jid, delete_job->pid);

            // Safely delete the job from the job list
            if (sigprocmask(SIG_BLOCK, &mask_all, &prev_all))
                printf("sigprocmask function failed.\n");
            deletejob(jobs, pid);
            if (sigprocmask(SIG_SETMASK, &prev_all, NULL))
                printf("sigprocmask function failed.\n");
        }
        // Process stopped.
        else if (WIFSTOPPED(status))
        {
            struct job_t *stopped_job = getjobpid(jobs, pid);

            // Print stop message for SIGTSTP
            printf("Job [%d] (%d) stopped by signal 20\n", stopped_job->jid, stopped_job->pid);

            // Safely update the state of the stopped job
            if (sigprocmask(SIG_BLOCK, &mask_all, &prev_all))
                printf("sigprocmask function failed.\n");
            stopped_job->state = ST;
            if (sigprocmask(SIG_SETMASK, &prev_all, NULL))
                printf("sigprocmask function failed.\n");
        }
    }
    // Restore original errno
    errno = olderrno;
    return;
}

/*
 * sigint_handler - The kernel sends a SIGINT to the shell whenever the
 * user types ctrl-c at the keyboard. Catch it and send it along
 * to the foreground job.
 */
void sigint_handler(int sig)
{
    sigset_t mask, prev_mask;

    // Initialize an empty set and add SIGINT to the set
    if (sigemptyset(&mask) || sigaddset(&mask, SIGINT))
        printf("sigemptyset/sigaddset function failed.\n");

    int olderrno = errno;
    pid_t curr_pid = -fgpid(jobs);

    // If foreground job exists.
    if (curr_pid != 0)
    {
        // Block SIGINT temporarily
        if (sigprocmask(SIG_BLOCK, &mask, &prev_mask))
            printf("sigprocmask function failed.\n");

        // Send SIGINT signal to the foreground job
        if (kill(curr_pid, SIGINT))
            printf("kill function failed.\n");

        // Unblock SIGINT
        if (sigprocmask(SIG_SETMASK, &prev_mask, NULL))
            printf("sigprocmask function failed.\n");
    }
    // Restore original errno
    errno = olderrno;
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 * the user types ctrl-z at the keyboard. Catch it and suspend the
 * foreground job by sending it a SIGTSTP.
 */
void sigtstp_handler(int sig)
{
    sigset_t mask, prev_mask;

    // Initialize an empty set and add SIGTSTP to the set
    if (sigemptyset(&mask) || sigaddset(&mask, SIGTSTP))
        printf("sigemptyset/sigaddset function failed.\n");

    int olderrno = errno;
    pid_t curr_pid = -fgpid(jobs);

    // If foreground job exists.
    if (curr_pid != 0)
    {
        // Block SIGTSTP temporarily
        if (sigprocmask(SIG_BLOCK, &mask, &prev_mask))
            printf("sigprocmask function failed.\n");

        // Send SIGTSTP signal to the foreground job
        if (kill(curr_pid, SIGTSTP))
            printf("kill function failed.\n");

        // Unblock SIGTSTP
        if (sigprocmask(SIG_SETMASK, &prev_mask, NULL))
            printf("sigprocmask function failed.\n");
    }
    // Restore original errno
    errno = olderrno;
    return;
}
/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}




