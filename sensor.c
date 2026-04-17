#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

struct msg_buffer {
    long msg_type;
    int s_id;
    char msg_text[200];
};

int id, pid, q1, q2;
int running = 1, ex = 0;

pthread_mutex_t lock;

void* thread(void* arg) {
    struct msg_buffer msg;

    while (1) {
		msgrcv(q2, &msg, sizeof(msg) - sizeof(long), pid, 0);
        pthread_mutex_lock(&lock);

        if (strcmp(msg.msg_text, "SHUTDOWN") == 0) {
			if(ex){
				pthread_mutex_unlock(&lock);
				break;
			}
			
            printf("\n[INFO] Another sensor has exited\n");
			printf("[INFO] Cancelling current input...\n");
            printf("[INFO] System shutting down...\n");

            running = 0;
            pthread_mutex_unlock(&lock);
			break;
        }

        printf("\n[INFO] Current city status: %s\n\n", msg.msg_text);
        printf("Masukkan data:\n\n");

        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main() {
    key_t k1 = ftok("progfile1", 65);
    key_t k2 = ftok("progfile2", 75);

    q1 = msgget(k1, 0666 | IPC_CREAT);
    q2 = msgget(k2, 0666 | IPC_CREAT);

    struct msg_buffer msg;

    pthread_mutex_init(&lock, NULL);

    msg.msg_type = 1;
    msg.s_id = getpid();
	pid = msg.s_id;
    strcpy(msg.msg_text, "REGISTER");
	
    msgsnd(q1, &msg, sizeof(msg) - sizeof(long), 0);
    msgrcv(q2, &msg, sizeof(msg) - sizeof(long), getpid(), 0);
    id = msg.s_id;

    pthread_t tid;
    pthread_create(&tid, NULL, thread, NULL);

	printf("Masukkan data:\n");
	
    while (running) {
		int vcount = 0;
        char d1[50], d2[50];
		
        while (vcount < 2 && running) {
            int input;
            char in[50], loc, stat;

            scanf(" %[^\n]", in);

            if (!running){
				break;
			}
			
            if (strcmp(in, "exit") == 0) {
                printf("[SENSOR] Sending exit signal...\n");

                msg.msg_type = 3;
                msg.s_id = pid;
                strcpy(msg.msg_text, "EXIT");
                msgsnd(q1, &msg, sizeof(msg) - sizeof(long), 0);

				ex = 1;
				running = 0;
                break;
            }

            if (sscanf(line, "%d %c %c", &input, loc, stat) != 3) {
                printf("[ERROR]\n");
                continue;
            }

            if (input != id) {
                printf("[ERROR]\n");
                continue;
            }

            if (!(loc == 'A' || loc == 'B' || loc == 'C' || loc == 'D') || !(stat == 'H' || stat == 'L')){
                printf("[ERROR]\n");
                continue;
            }

            if (vcount == 0)
                sprintf(d1, "%d %c %c", input, loc, stat);
            else
                sprintf(d2, "%d %c %c", input, loc, stat);

            vcount++;
        }

        if (!running) break;

        if (vcount == 2) {
            sprintf(msg.msg_text, "%s | %s", d1, d2);
            msg.msg_type = 2;
            msg.s_id = pid;
            msgsnd(q1, &msg, sizeof(msg) - sizeof(long), 0);
        }
    }
	
    pthread_join(tid, NULL);
    pthread_mutex_destroy(&lock);

    printf("[SENSOR] Shutting down...\n");
    return 0;
}
