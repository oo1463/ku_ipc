#include <stdio.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "ku_ipc.h"
#include <string.h>

struct params{
    int msqid, msgsz, msgflg;
    void *msgp;
    long msgtyp;
};

struct params user_param;

int ku_msgget(int key, int msgflg){
    int dev;
    int ret;

    user_param.msqid = key;
    user_param.msgflg = msgflg;

    dev = open("/dev/ku_ipc_dev", O_RDWR);
    
    ret = ioctl(dev, KU_MSGGET, &user_param);

    close(dev);
    return ret;

}

int ku_msgclose(int msqid){
    int dev;
    int ret;

    user_param.msqid = msqid;

    dev = open("/dev/ku_ipc_dev", O_RDWR);

    ret = ioctl(dev, KU_MSGCLOSE, &user_param);
    
    close(dev);
    
    return ret;

}

int ku_msgsnd(int msqid, void *msgp, int msgsz, int msgflg){
    int dev;
    int ret;

    user_param.msqid = msqid;
    user_param.msgp = msgp;
    user_param.msgsz = msgsz;
    user_param.msgflg = msgflg;

    dev = open("/dev/ku_ipc_dev", O_RDWR);
    ret = ioctl(dev, KU_MSGSND, &user_param);
    
    close(dev);

    return ret;

}

int ku_msgrcv(int msqid, void *msgp, int msgsz, long msgtyp, int msgflg){
    int dev;
    int ret;

    user_param.msqid = msqid;
    user_param.msgp = msgp;
    user_param.msgsz = msgsz;
    user_param.msgflg = msgflg;
    user_param.msgtyp = msgtyp;

    dev = open("/dev/ku_ipc_dev", O_RDWR);
    ret = ioctl(dev, KU_MSGRCV, &user_param);
    

    close(dev);

    return ret;

}

    