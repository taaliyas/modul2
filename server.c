#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

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
	int scount = 0;
	int p1 = 0, p2 = 0;
	printf("[SERVER] Waiting for sensors...\n");

	while(scount < 2){
		msgrcv(q1, &msg, sizeof(msg) - sizeof(long), 1, 0);
		scount++;

		if(a_id == 1){
			p1 = msg.s_id;
		} else {
			p2 = msg.s_id;
		}

		printf("[SERVER] Sensor %d connected\n", scount);

		struct msg_buffer rep;
		rep.msg_type = msg.s_id;
		rep.s_id = scount;
		strcpy(rep.msg_text, "REGISTERED");

		msgsnd(q2, &rep, sizeof(rep) - sizeof(long), 0);
	}
	printf("[SERVER] System ready!\n");

	while(1){
		char sA = '-', sB = '-', sC = '-', sD = '-';
		int r1 = 0, r2 = 0;

		while(!(r1 && r2)){
			msgrcv(q1, &msg, sizeof(msg) - sizeof(long), 0, 0);

			if(msg.msg_type == 3){
				printf("\n[SERVER] Exit signal received\n");

				struct msg_buffer sd_msg;
				strcpy(sd_msg.msg_text, "SHUTDOWN");

				sd_msg.msg_type = p1;
				msgsnd(q2, &sd_msg, sizeof(sd_msg) - sizeof(long), 0);

				sd_msg.msg_type = p2;
	            msgsnd(q2, &sd_msg, sizeof(sd_msg) - sizeof(long), 0);

				printf("[SERVER] Shutting down system...\n");

				msgctl(q1, IPC_RMID, NULL);
				msgctl(q2, IPC_RMID, NULL);

				printf("[SERVER] Cleaning up message queue...\n");
				printf("[SERVER] Done.\n");
				return 0;
	  		}

			if(msg.msg_type != 2){
				continue;
			}

			if(msg.s_id == p1){
				if(r1) continue:
				r1 = 1;
			} else if(msg.s_id == p2){
				if(r2) continue;
				r2 = 1;
			}

			char c[200];
			strcpy(c, msg.msg_text);
			char *t = strtok(c, "|");

			while(t){
				int id;
				char loc, stat;
				sscanf(t, "%d %c %c", &id, &loc, &stat);

				if(loc == 'A') sA = stat;
				else if(loc == 'B') sB = stat;
				else if(loc == 'C') sC = stat;
				else if(loc == 'D') sD = stat;
				t = strtok(NULL, "|");
			}
		}

		printf("\n[SERVER] Received data:\n");
		printf("A: %c\n", sA);
		printf("B: %c\n", sB);
		printf("C: %c\n", sC);
		printf("D: %c\n", sD);

		int hi = 0, lo = 0;
		char arr[4] = {sA, sB, sC, sD};

		for(int i = 0; i < 4; i++){
			if(arr[i] == 'H'){
				hi++;
			} else if(arr[i] == 'L'){
				lo++;
			}
		}

		printf("\n[SERVER] Summary:\n");
		printf("High Traffic: %d\n", hi);
		printf("Low Traffic: %d\n", lo);

		char status[30];
		if(hi >= 3){
			strcpy(status, "MACET TOTAL");
		} else if(hi == 2){
			strcpy(status, "PADAT");
		} else if(hi <= 1){
			strcpy(status, "LANCAR");
		}

		printf("\n[SERVER] City Status : %s\n", status);

		struct msg_buffer rep;
		rep.msg_type = p1;
		strcpy(rep.msg_text, status);
		msgsnd(q2, &rep, sizeof(rep) - sizeof(long), 0);

		rep.msg_type = p2;
        msgsnd(q2, &rep, sizeof(rep) - sizeof(long), 0);
	}
}
