/*---------------------------------------------------------------------------------------
  --	SOURCE FILE:	server.c -   A Server/Client program that sends/receives data via IPC
  --                             message queues. 
  --
  --	PROGRAM:		server.exe
  --
  --	FUNCTIONS:		
  --      int open_queue(key_t keyval);
  --      int read_message(int qid, long mtype, struct Mesg *imsg);
  --      int send_message(int qid, Mesg *omsg);
  --      void *clientThread(void *msg_qid);
  --      int client(int msg_qid, char *fname, int priority);
  --      int server_transfer_proc(int msg_qid, Mesg imsg);
  --      int server(int msg_qid);
  --      int main(int argc, char *argv[]);
  --
  --	DATE:			    Mar 27, 2019
  --
  --	REVISIONS:		Mar 27, 2019
  --
  --	DESIGNERS:		Jacky Li
  --
  --	PROGRAMMER:		Jacky Li
  --
  --	NOTES:
  --     This simple program is composed of two parts: Server mode and Client mode
  --     Server Mode:
  --         (1) Open/Create a message queue on the OS (shared config on both Srv + Client)
  --         (2) Continuously waits for message queue to have mtype MAXPID + 500
  --              - Forks child process to send data to destination specified in message
  --              - Once file to be read is finished reading, signals to client
  --     Client Mode:
  --         (1) CMD line will specify filename and priority to be sent to server
  --         (2) Client will send message into message queue 
  --              - Filename
  --              - Priority
  --              - Client PID
  --         (3) Waits for server to put messages into queue
  --         (4) Reads queue for messsage mtype = to its own PID
  --         (5) Keeps reading until server sends end message
---------------------------------------------------------------------------------------*/
#define MAX_PID 32768
#define LISTEN_MSG MAX_PID + 500
#define INC_MSG_SIZE 512
#define FILENAME_SIZE 128
#define ERR_FORK_INIT_LISTEN 403
#define PRIORITY_MAX 1
#define MSG_KEY 1337
#define OPTIONS "?t:f:p:"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include "mesg.h"

// Function prototypes
int open_queue(key_t keyval);
int read_message(int qid, long mtype, struct Mesg *imsg);
int send_message(int qid, Mesg *omsg);
void *clientThread(void *msg_qid);
int client(int msg_qid, char *fname, int priority);
int server_transfer_proc(int msg_qid, Mesg imsg);
int server(int msg_qid);
int main(int argc, char *argv[]);

/*------------------------------------------------------------------------------------
  --	FUNCTION:		open_queue
  --
  --	DATE:			    Mar 27, 2019
  --
  --	REVISIONS:		Mar 27, 2019
  --
  --	DESIGNERS:		Jacky Li
  --
  --	PROGRAMMER:		Jacky Li
  --
  --	INTERFACE:		int open_queue(key_t keyval)
  --
  --	RETURNS:		
  --					qid   on successful creation of IPC message queue
  --          -1    on failure       
  --	NOTES:
  --		Wrapper function for creating a system message queue
------------------------------------------------------------------------------------*/
int open_queue(key_t keyval)
{
  int qid;
  if ((qid = msgget(keyval, IPC_CREAT | 0660)) == -1)
  {
    return (-1);
  }
  return qid;
}

/*------------------------------------------------------------------------------------
  --	FUNCTION:		read_message
  --
  --	DATE:			    Mar 27, 2019
  --
  --	REVISIONS:		Mar 27, 2019
  --
  --	DESIGNERS:		Jacky Li
  --
  --	PROGRAMMER:		Jacky Li
  --
  --	INTERFACE:		int read_message(int qid, long mtype, struct Mesg *imsg)
  --                  int qid:      message queue id
  --               long mtype:      message mtype for the queue to identify
  --               Mseg *imsg:      The message struct to be populated from the queue
  --
  --	RETURNS:		
  --					n     number of bytes written to mtext[] array in message struct
  --          -1    on failure       
  --	NOTES:
  --		Wrapper function for reading from a message queue
------------------------------------------------------------------------------------*/
int read_message(int qid, long mtype, struct Mesg *imsg)
{
  int result;
  int length;
  length = sizeof(struct Mesg) - sizeof(long);
  if ((result = msgrcv(qid, imsg, length, mtype, IPC_NOWAIT)) == -1)
  {
    return (-1);
  }
  return (result);
}

/*------------------------------------------------------------------------------------
  --	FUNCTION:		send_message
  --
  --	DATE:			    Mar 27, 2019
  --
  --	REVISIONS:		Mar 27, 2019
  --
  --	DESIGNERS:		Jacky Li
  --
  --	PROGRAMMER:		Jacky Li
  --
  --	INTERFACE:		int send_message(int qid, Mesg *omsg)
  --                  int qid:      message queue id
  --               Mseg *omsg:      The message struct to be sent over to the queue
  --
  --	RETURNS:		
  --					0     on success
  --          -1    on failure       
  --	NOTES:
  --		Wrapper function for reading from a message queue
------------------------------------------------------------------------------------*/
int send_message(int qid, Mesg *omsg)
{
  int result;
  int length = sizeof(Mesg) - sizeof(long);
  if ((result = msgsnd(qid, omsg, length, 0)) < 0)
  {
    return -1;
  }
  return (result);
}

/*------------------------------------------------------------------------------------
  --	FUNCTION:		clientThread
  --
  --	DATE:			    Mar 27, 2019
  --
  --	REVISIONS:		Mar 27, 2019
  --
  --	DESIGNERS:		Jacky Li
  --
  --	PROGRAMMER:		Jacky Li
  --
  --	INTERFACE:		void *clientThread(void *msg_qid)
  --                  void *msg_qid:      void pointer to message queue id, to be used
  --
  --	RETURNS:		
  --     
  --	NOTES:
  --		Client thread function
------------------------------------------------------------------------------------*/
void *clientThread(void *msg_qid)
{
  printf("client thread initiated, passed in qid: %d\n", *(int *)msg_qid);
  while (1)
  {
  }
  return 0;
}

/*------------------------------------------------------------------------------------
  --	FUNCTION:		client
  --
  --	DATE:			    Mar 27, 2019
  --
  --	REVISIONS:		Mar 27, 2019
  --
  --	DESIGNERS:		Jacky Li
  --
  --	PROGRAMMER:		Jacky Li
  --
  --	INTERFACE:		int client(int msg_qid, char *fname, int priority)
  --                     int msg_qid:      message queue id
  --                     char *fname:      Filename to query from server
  --                    int priority:      The priority of this request to server
  --
  --	RETURNS:		
  --					 0    on success
  --          -1    on failure of thread creation
  --          -2    on failure to send over initial connect message to server       
  --	NOTES:
  --		Client function to be run by this program when specified to be in client mode
  --      (1) Will send to a server process with pre-defined IPC channel
  --          - Init message, with 
  --              (1) this process ID
  --              (2) Priority for the server to allocate resources to this request
  --              (3) a filename that the server will send over
  --      (2) Wait for the message queue to be populated with mtype = this process ID
  --      (3) Once the incoming buffer is filled, write to console
  --      (4) Will keep reading for the same mtype until server sends message with 
  --          Priority == -1, then this process will die
------------------------------------------------------------------------------------*/
int client(int msg_qid, char *fname, int priority)
{
  // Req: Create thread
  pthread_t th_clnt;
  if (pthread_create(&th_clnt, NULL, clientThread, &msg_qid) != 0)
  {
    perror("thread creation error");
    return -1;
  }

  // Reads filename from stdin
  Mesg omsg;
  // Prep First init message to be sent
  omsg.mtype = LISTEN_MSG;
  strcpy(omsg.mesg_data, fname);
  omsg.mesg_priority = priority;
  omsg.mesg_len = strlen(fname);
  omsg.pid = getpid();

  // Writes filename to IPC channel
  printf("string to be sent to %d, length: %ld\n", msg_qid, strlen(fname));
  if (send_message(msg_qid, &omsg) == -1)
  {
    return -2;
  }
  printf("Client has sent: pid:%d\n", omsg.pid);

  // Starts reading from server
  Mesg imsg;
  unsigned long num_msg = 0;
  unsigned long complete_msg = 0;
  unsigned long total_bytes_recv = 0;
  unsigned long curr_bytes_recv = 0;
  while (1)
  {
    if (read_message(msg_qid, (long)getpid(), &imsg) > 0)
    {
      ++num_msg;
      // total_bytes_recv += imsg.mesg_len;
      total_bytes_recv += strlen(imsg.mesg_data);
      curr_bytes_recv += imsg.mesg_len; //Because null term.
      if (curr_bytes_recv >= MAXMESSAGEDATA)
      {
        ++complete_msg;
        printf("inc buffer filled: %lu\n", complete_msg);
        curr_bytes_recv = MAXMESSAGEDATA - curr_bytes_recv;
      }
      if (imsg.mesg_priority < 0)
      {
        printf("Srv end msg, totalbrecv: %ld totalmsg: %ld\n", total_bytes_recv, num_msg);
        return 0;
      }
    }
  }

  return 0;
}

/*------------------------------------------------------------------------------------
  --	FUNCTION:		server_transfer_proc
  --
  --	DATE:			    Mar 27, 2019
  --
  --	REVISIONS:		Mar 27, 2019
  --
  --	DESIGNERS:		Jacky Li
  --
  --	PROGRAMMER:		Jacky Li
  --
  --	INTERFACE:		int server_transfer_proc(int msg_qid, Mesg imsg)
  --                     int msg_qid:      message queue id
  --                       Mesg imsg:      Msg struct data from initial message
  --
  --	RETURNS:		
  --					 0    on success
  --          -1    on failure of message send
  --          -2    on failure to open file     
  --	NOTES:
  --		Server transfer process function. This is run when this application is set to 
  --    be run in server mode, and an incoming message from a client specifies a file
  --    request
  --    This function will
  --      (1) Attempt to open a file
  --      (2) Read the content of a file
  --      (3) Fill up buffer size according to imsg structure passed in
  --      (4) Send message into msg_qid whenever
  --          (4.1) Buffer size according to priority is filled
  --          (4.2) File to be read is finished reading
  --      (5) Send a final message with priority -1 to the client to signal EOT
------------------------------------------------------------------------------------*/
int server_transfer_proc(int msg_qid, Mesg imsg)
{
  struct Mesg smsg;
  printf("srv transfer proc %d called for client proc: %d\n", getpid(), imsg.pid);
  // Opens file to read
  FILE *fp;
  if ((fp = fopen(imsg.mesg_data, "r")) == NULL)
  {
    printf("file open failed: %s\n", imsg.mesg_data);
    // Fail: Send ASCII error msg
    smsg.mtype = imsg.mtype;
    const char *file_io_err = "File Open error";
    strcpy(smsg.mesg_data, file_io_err);
    smsg.mesg_len = strlen(file_io_err);
    smsg.mesg_priority = -1;
    // Send
    if (send_message(msg_qid, &smsg) > 0)
    {
      printf("svr sent failed\n");
      return -1;
    }
    return -2;
  }
  else
  {
    printf("file open success\n");
    // Success: Write file to IPC channel
    printf("Transfer Requested: prior:%d, type:%lu, pid:%d, incLen:%d\nmsg:%s\n",
           imsg.mesg_priority,
           imsg.mtype,
           imsg.pid,
           imsg.mesg_len,
           imsg.mesg_data);
    int packetSize = MAXMESSAGEDATA / imsg.mesg_priority;
    printf("Transfer packet size will be: %d\n", packetSize);
    char c;
    int count = 0;
    while ((c = fgetc(fp)) != EOF && (count < packetSize))
    {
      // Fill buffer
      smsg.mesg_data[count] = c;
      ++count;
      if (count == packetSize - 1) // Account for null-terminated
      {
        smsg.mesg_data[count] = '\0';
        smsg.mesg_len = count;
        smsg.mtype = imsg.pid;
        // Send it!
        if (send_message(msg_qid, &smsg) == -1)
        {
          printf("svr sent failed\n");
          return -1;
        }
        else
        {
          // Cleanup for next packet
          count = 0;
          memset(smsg.mesg_data, '\0', MAXMESSAGEDATA);
        }
      }
    }
    // Attach last null terminated
    printf("read file terminated\n");
    smsg.mesg_data[count] = '\0';
    smsg.mesg_len = count;
    smsg.mtype = imsg.pid;
    smsg.mesg_priority = -1;

    // Send message
    if (send_message(msg_qid, &smsg) > 0)
    {
      printf("svr sent failed\n");
      return -1;
    }
    else
    {
      printf("sending over last msg:%s\n", smsg.mesg_data);
    }
    fclose(fp);
    return 0;
  }
  return 0;
}

/*------------------------------------------------------------------------------------
  --	FUNCTION:		server
  --
  --	DATE:			    Mar 27, 2019
  --
  --	REVISIONS:		Mar 27, 2019
  --
  --	DESIGNERS:		Jacky Li
  --
  --	PROGRAMMER:		Jacky Li
  --
  --	INTERFACE:		int server(int msg_qid)
  --                     int msg_qid:      message queue id
  --
  --	RETURNS:		
  --					 0    on success
  --    
  --	NOTES:
  --     Server Mode:
  --         (1) Open/Create a message queue on the OS (shared config on both Srv + Client)
  --         (2) Continuously waits for message queue to have mtype MAXPID + 500
  --              - Forks child process to send data to destination specified in message
------------------------------------------------------------------------------------*/
int server(int msg_qid)
{
  printf("server function running %d\n", getpid());
  struct Mesg imsg;
  int recv_len;
  // Listen for incoming
  while (1)
  {
    recv_len = read_message(msg_qid, LISTEN_MSG, &imsg);
    // If message rec'd
    if (recv_len != -1)
    {
      printf("Init msg got size: %d\n", recv_len);
      // Reads filename from IPC channel
      printf("Incoming: prior:%d, type:%lu, pid:%d, incLen:%d\nmsg:%s\n",
             imsg.mesg_priority,
             imsg.mtype,
             imsg.pid,
             imsg.mesg_len,
             imsg.mesg_data);
      // Should fork here
      switch (fork())
      {
      case -1:
        printf("fork failed");
        exit(666);
      case 0:
        // Child
        server_transfer_proc(msg_qid, imsg);
        printf("proc function finished\n");
        return 0;
      default:
        // Parent - do nothing
        break;
      }
    }
  }
  return 0;
}

/*------------------------------------------------------------------------------------
  --	FUNCTION:		main
  --
  --	DATE:			    Mar 27, 2019
  --
  --	REVISIONS:		Mar 27, 2019
  --
  --	DESIGNERS:		Jacky Li
  --
  --	PROGRAMMER:		Jacky Li
  --
  --	INTERFACE:		int main(int argc, char *argv[])
  --
  --	RETURNS:		
  --					 0    on success
  --    
  --	NOTES:
  --      Main executable for this program
  --        [OPTIONS]
  --          [SERVER]
  --          -t : "server" or "client" - specifies the behaviour of this program
  --          [CLIENT]
  --          -f : Specifies which file the server should send
  --          -p : Priority 
  --
------------------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
  // Optargs
  int opt;
  char srv_cln[FILENAME_SIZE];
  char fname[FILENAME_SIZE];
  int priority;
  // Determine key
  int msg_qid;
  key_t msgq_key = MSG_KEY;

  // Create IPC: Msg Queue
  if ((msg_qid = open_queue(msgq_key)) == -1)
  {
    printf("open queue failed\n");
  }
  printf("open queue ok, qid: %d\n", msg_qid);

  // User read args for this function
  while ((opt = getopt(argc, argv, OPTIONS)) != -1)
  {
    switch (opt)
    {
    case 't':
      strncpy(srv_cln, optarg, FILENAME_SIZE);
      break;
    case 'f':
      strncpy(fname, optarg, FILENAME_SIZE);
      break;
    case 'p':
      priority = atoi(optarg);
      break;
    default:
    case '?':
      printf("wat");
      break;
    }
  }

  if (strcmp(srv_cln, "server") == 0)
  {
    printf("%s Mode\n", srv_cln);
    server(msg_qid);
    printf("server proc %d finished\n", getpid());
    return 0;
  }

  if (strcmp(srv_cln, "client") == 0)
  {
    printf("%s Mode\n", srv_cln);
    client(msg_qid, fname, priority);
    exit(0);
  }
  return 0;
}
