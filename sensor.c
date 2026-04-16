#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>

struct msg_buffer{
	long msg_type;
	int s_id;
	char msg_text[200];
};

int main(){
	key_t k1 = ftok("progfile1", 65);
	key_t k2 = ftok("progfile2", 75);

	int q1 = msgget(k1, 0666 | IPC_CREAT);
	int q2 = msgget(k2, 0666 | IPC_CREAT);

	struct msg_buffer msg;

	msg.msg_type = 1;
	msg.s_id = getpid();
	strcpy(msg.msg_text, "REGISTER");

	msgsnd(q1, &msg, sizeof(msg) - sizeof(long), 0);
	msgrcv(q2, &msg, sizeof(msg) - sizeof(long), getpid(), 0);
	int id = msg.s_id;

	while(1){
		char d1[50], d2[50];
		int vcount = 0;

		printf("Masukkan data :\n");

		while(vcount < 2){
			int input;
			char in[50], loc[10], stat[10];
			scanf("%[^\n]", in);

			if(strcmp(in, "exit") == 0){
				printf("\n[SENSOR] Sending exit signal...\n");

				msg.msg_type = 3;
				msg.s_id = getpid();
				strcpy(msg.msg_text, "EXIT");

				msgsnd(q1, &msg, sizeof(msg) - sizeof(long), 0);

				printf("[SENSOR] Shutting down...\n");
				return 0;
			}

			if(sscanf(in, "%d %s %s", &input, loc, stat) != 3){
				printf("[ERROR]\n");
				continue;
			}

			if(input != id){
				printf("[ERROR]\n");
				continue;
			}

			if(!(strcmp(loc, "A") == 0 || strcmp(loc, "B") == 0 ||
				strcmp(loc, "C") == 0 || strcmp(loc, "D") == 0) ||
				!(strcmp(stat, "L") == 0 || strcmp(stat, "H") == 0)) {

				printf("[ERROR]\n");
				continue;
			}

			if(vcount == 0){
				sprintf(d1, "%d %s %s", input, loc, stat);
			} else {
				sprintf(d2, "%d %s %s", input, loc, stat);
			}
			vcount++;
		}

		sprintf(msg.msg_text, "%s | %s", d1, d2);
		msg.msg_type = 2;
		msg.s_id = getpid();
		msgsnd(q1, &msg, sizeof(msg) - sizeof(long), 0);

		if(msgrcv(q2, &msg, sizeof(msg) - sizeof(long), getpid(), 0)){
			if(strcmp(msg.msg_text, "SHUTDOWN") == 0){
				printf("\n[INFO] Another sensor has exited\n");
			        printf("[INFO] Cancelling current input...\n");
			        printf("[INFO] System shutting down...\n)");
			        return 0;
			}
			printf("[INFO] Current city status : %s\n", msg.msg_text);
		}
	}
	return 0;
}
