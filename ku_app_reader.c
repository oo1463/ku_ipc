
#include "ku_ipc_lib.c"

void main(){

    int ret;
    int msqid;
    int key=0;
    int msgsize=10;
    long msgtype = 1;

    struct msgbuf rcved_msg = {0, "Receiving failed"};

    printf("받을 msg type을 입력하세요\n: ");
    scanf("%ld",&msgtype);

    msqid = ku_msgget(key, KU_IPC_CREAT);
    ret = ku_msgrcv(msqid, &rcved_msg, msgsize, msgtype ,KU_MSG_NOERROR);

    ku_msgclose(msqid);

    printf("%s\n", rcved_msg.text);
}
