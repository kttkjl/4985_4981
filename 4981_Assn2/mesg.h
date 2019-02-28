#define MAXMESSAGEDATA 4096 /* don't want sizeof(Mesg) > 4096 */
#define MESGHDRSIZE (sizeof(Mesg) - MAXMESGDATA)
/* length of mesg_len and mesg_type */
typedef struct Mesg
{
  long mtype;   /* message type */
  int mesg_len; /* #bytes in mesg_data */
  int pid;
  int mesg_priority;
  char mesg_data[MAXMESSAGEDATA];
} Mesg; //Alias for struct Mesg = Mesg

// Expected message structure
// struct inc_msg
// {
//   long mtype;
//   int priority;
//   char msg[INC_MSG_SIZE];
// } imsg;
