/*---------------------------------------------------------------------------------------
--	SOURCE FILE:	main.c -   A simple program to demonstrate forking processes and
--                              inter-process communication via pipes
--
--	PROGRAM:		assignment_1.exe
--
--	FUNCTIONS:		
--
--	DATE:			January 22, 2019
--
--	REVISIONS:		(NONE)
--
--	DESIGNERS:		Jacky Li
--
--	PROGRAMMER:		Jacky Li
--
--	NOTES:
--     This simple program will create 2 pipes, then create 3 processes from 2 fork 
--     operations
--          Fork 1 (Parent): will be the INPUT process in which reads the user's input
--          Fork 1 (Child): 
--              Will be the TRANSLATE process which will translate the msg
--              obtained from a pipe that the input process wrote to
--              
--              Will fork another subprocess (OUTPUT)
--
--          Fork 2 (Child): 
--              will be the OUTPUT process that reads from a pipe and output
--              it to the console.
--      This program will disable all standard keyboard inputs, and restore it on end
---------------------------------------------------------------------------------------*/
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MSGSIZE 128
#define PIPE_READ 0
#define PIPE_WRITE 1
#define CTRL_K 11

#define ERR_PIPE 300
#define ERR_MASK_RESTORE 301

#define ERR_FORK_TRANSL 400
#define ERR_FORK_OUTPUT 401

#define ERR_BLOCK 500

/* Fnc proto */
void fInput(int *pipe_to_trans, int *pipe_to_out);
void fTranslate(int *pipe_from_in, int *pipe_to_out);
void fOutput(int *pipe_in);

/*------------------------------------------------------------------------------------
--	FUNCTION:		main
--
--	DATE:			Jan 23, 2019
--
--	REVISIONS:      (NONE)
--
--	DESIGNER:		Jacky Li
--
--	PROGRAMMER:		Jacky Li
--
--	INTERFACE:		int main(void)
--
--	RETURNS:		
--					0       on successful exit;
--					300     on pipe creation fail;
--                  500     on signal masking for creating forks
--                  400     on forking the translate process fail
--                  401     on forking the output process fail
--                  301     on signal masking restore fail
--
--	NOTES:
--		This function inits all pipes and processes for this program
------------------------------------------------------------------------------------*/
int main(void)
{
    // Init signals + Signal masks
    sigset_t mask;
    sigset_t oldMask;

    // Init pipe variables
    int pInTrans[2];
    int pTransOut[2];

    // Catch error if either pipe fails
    if ((pipe(pInTrans) < 0) || (pipe(pTransOut) < 0))
    {
        perror("pipes failed to init");
        return ERR_PIPE;
    }

    // Block all signals until forking completes
    if ((sigfillset(&mask) == -1) || (sigprocmask(SIG_SETMASK, &mask, &oldMask) == -1))
    {
        perror("Failed to block the signals");
        return ERR_BLOCK;
    }

    /* ----FORK---- */
    switch (fork())
    {
    case -1:
        perror("first fork");
        return ERR_FORK_TRANSL;
    case 0:
        /* ----FORK AGAIN---- */
        switch (fork())
        {
        case -1:
            perror("second fork");
            return ERR_FORK_OUTPUT;
        case 0:
            // ==========Child, (OUTPUT) proc
            // Should restore signals
            if (sigprocmask(SIG_SETMASK, &oldMask, NULL) == -1)
            {
                perror("Parent failed to restore signal mask");
                return ERR_MASK_RESTORE;
            }
            close(pInTrans[PIPE_READ]);
            close(pInTrans[PIPE_WRITE]);
            close(pTransOut[PIPE_WRITE]);
            fOutput(&pTransOut[PIPE_READ]);
            break;
        default:
            // ==========Parent, (TRANSLATE) proc
            // Should restore signals
            if (sigprocmask(SIG_SETMASK, &oldMask, NULL) == -1)
            {
                perror("Parent failed to restore signal mask");
                return ERR_MASK_RESTORE;
            }
            close(pInTrans[PIPE_WRITE]);
            close(pTransOut[PIPE_READ]);
            fTranslate(&pInTrans[PIPE_READ], &pTransOut[PIPE_WRITE]);
        }
        break;
    default:
        // ==========Parent, (INPUT) proc
        // Should restore signals
        if (sigprocmask(SIG_SETMASK, &oldMask, NULL) == -1)
        {
            perror("Parent failed to restore signal mask");
            return ERR_MASK_RESTORE;
        }
        close(pInTrans[PIPE_READ]);
        close(pTransOut[PIPE_READ]);
        fInput(&pInTrans[PIPE_WRITE], &pTransOut[PIPE_WRITE]);
        break;
    }

    return 0;
}

/*------------------------------------------------------------------------------------
--	FUNCTION:		fInput
--
--	DATE:			Jan 23, 2019
--
--	REVISIONS:      (NONE)
--
--	DESIGNER:		Jacky Li
--
--	PROGRAMMER:		Jacky Li
--
--	INTERFACE:		void fInput(int *pipe_to_trans, int *pipe_to_out)
--					int * pipe_to_trans: int pointer to write to the pipe that 
--                      the translate process will read from
--					int * pipe_to_out: int pointer to write to the pipe that 
--                      the output process will read from
--
--	RETURNS:		void
--
--	NOTES:
--		This function starts a loop with a blocking read to stdin from the user
--      All input from the read will be piped to the Translate and Output process
--          
--          Each input char will be piped to Output to be echoed out to console, as 
--            well as being stored in a limited local buffer, to be sent to translate
--            later
--
--          On 'E' press, the local buffer will be sent to the Translate process via
--            pipe
--      If the local buffer fills up, it will be sent to Translate immediately.
--      Turns terminal into 'raw' mode, and disables echos from stdin
------------------------------------------------------------------------------------*/
void fInput(int *pipe_to_trans, int *pipe_to_out)
{
    char buf[MSGSIZE];
    memset(buf, '\0', MSGSIZE);
    int msg_curr_size = 0;
    int stop_flag = 0;
    char curr_char = '\0';

    // Kill all default terminal
    system("/bin/stty raw igncr -echo");

    // Keep reading from input
    while (!stop_flag)
    {
        curr_char = getchar();

        if (curr_char == CTRL_K)
        {
            system("stty -raw -igncr echo");
            kill(0, 9);
        }
        // Echo to output regardless first, ignore all but chars
        if (curr_char >= 'A' && curr_char <= 'z')
        {
            write(*pipe_to_out, &curr_char, 1);
        }
        else
        {
            continue;
        }

        switch (curr_char)
        {
        // case 'X':
        //     // printf("[Trans] X read\r\n");
        //     buf[msg_curr_size - 1] = '\0';
        //     msg_curr_size--;
        //     break;
        case 'T':
            system("stty -raw -igncr echo");
            return;
        case 'E':
            write(*pipe_to_trans, &buf, MSGSIZE);
            memset(buf, '\0', MSGSIZE);
            msg_curr_size = 0;
            break;
        // case 'K':
        //     memset(buf, '\0', MSGSIZE);
        //     msg_curr_size = 0;
        //     continue;
        default:
            if (curr_char >= 'A' && curr_char <= 'z')
            {
                // Save to buffer, if buffer fills, send regardless
                if (msg_curr_size + 2 == MSGSIZE)
                {
                    buf[msg_curr_size] = curr_char;
                    // Limit reached, send msg and reset buffer
                    write(*pipe_to_trans, &buf, MSGSIZE);
                    memset(buf, '\0', MSGSIZE);
                    msg_curr_size = 0;
                    break;
                }
                else
                {
                    buf[msg_curr_size] = curr_char;
                    msg_curr_size++;
                    break;
                }
            }
            break;
        }
    }
}

/*------------------------------------------------------------------------------------
--	FUNCTION:		fTranslate
--
--	DATE:			Jan 23, 2019
--
--	REVISIONS:      (NONE)
--
--	DESIGNER:		Jacky Li
--
--	PROGRAMMER:		Jacky Li
--
--	INTERFACE:		void fTranslate(int *pipe_from_in, int *pipe_to_out)
--					    int * pipe_from_in: int pointer to a pipe that the input process
--                          writes to, and this process will read fron
--					    int * pipe_to_out: int pointer to write to the pipe that 
--                          the output process will read from
--	RETURNS:		void
--
--	NOTES:
--      This function will translate 'a' to 'z', and processes all special chars 
--
------------------------------------------------------------------------------------*/
void fTranslate(int *pipe_from_in, int *pipe_to_out)
{
    char inbuf[MSGSIZE];
    char outbuf[MSGSIZE];
    memset(outbuf, '\0', MSGSIZE);
    memset(inbuf, '\0', MSGSIZE);
    int num_read = 0;
    int char_count;
    int write_pointer = 0;
    while (1)
    {
        switch (num_read = read(*pipe_from_in, inbuf, MSGSIZE))
        {
        case -1:
            return;
        case 0:
            return;
        default:
            for (char_count = 0; inbuf[char_count] != '\0'; ++char_count, ++write_pointer)
            {
                switch (inbuf[char_count])
                {
                case 'a':
                    outbuf[write_pointer] = 'z';
                    break;
                case 'X':
                    // printf("[Trans] X read\r\n");
                    outbuf[write_pointer - 1] = '\0';
                    write_pointer -= 2;
                    break;
                case 'K':
                    memset(outbuf, '\0', MSGSIZE);
                    write_pointer = 0;
                    break;
                default:
                    outbuf[write_pointer] = inbuf[char_count];
                    break;
                }
            }
            write(*pipe_to_out, &outbuf, write_pointer);
            memset(outbuf, '\0', MSGSIZE);
            memset(inbuf, '\0', MSGSIZE);
            break;
        }
    }
}

/*------------------------------------------------------------------------------------
--	FUNCTION:		fOutput
--
--	DATE:			Jan 23, 2019
--
--	REVISIONS:      (NONE)
--
--	DESIGNER:		Jacky Li
--
--	PROGRAMMER:		Jacky Li
--
--	INTERFACE:		void fOutput(int *input)
--					    int * input: pointer to a pipe to read from
--
--	RETURNS:		void
--
--	NOTES:
--      This function will print to console what it reads from the input pipe
--
------------------------------------------------------------------------------------*/
void fOutput(int *input)
{
    // Simply output things from pipe
    char inc_c = '\0';
    int t_count = 0;
    int n_read = 0;
    while (1)
    {
        // Blocking read from pipe
        if ((n_read = read(*input, &inc_c, 1)) > 0)
        {
            printf("%c", inc_c);
        }

        // If for E and T
        if (inc_c == 'T' || inc_c == 'E')
        {
            printf("\r\n");
            if (inc_c == 'T' && ++t_count == 2)
            {
                exit(0);
            }
        }
        // Char rec'd flush stdout, print all input first
        fflush(stdout);
    }
}
