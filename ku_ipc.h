#define IOCTL_START_NUM 0x80
#define IOCTL_NUM1 IOCTL_START_NUM+1
#define IOCTL_NUM2 IOCTL_START_NUM+2
#define IOCTL_NUM3 IOCTL_START_NUM+3
#define IOCTL_NUM4 IOCTL_START_NUM+4

#define KU_IOCTL_NUM 'z'
#define KU_MSGGET _IOWR(KU_IOCTL_NUM, IOCTL_NUM1, unsigned long *)
#define KU_MSGCLOSE _IOWR(KU_IOCTL_NUM, IOCTL_NUM2, unsigned long *)
#define KU_MSGSND _IOWR(KU_IOCTL_NUM, IOCTL_NUM3, unsigned long *)
#define KU_MSGRCV _IOWR(KU_IOCTL_NUM, IOCTL_NUM4, unsigned long *)

#define KUIPC_MAXMSG 5
#define KUIPC_MAXVOL 1000

#define KU_IPC_CREAT 1
#define KU_IPC_EXCL 2
#define KU_IPC_NOWAIT 3
#define KU_MSG_NOERROR 4


struct msgbuf{
    long type;
    char text[128];
};

