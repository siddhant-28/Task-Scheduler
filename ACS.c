/*--------------------------------------------
 * Siddhant Kumar
 *
 * REFERENCES
 * ----------
 * LinkedList Implementation adapted from:
 * [1] https://www.tutorialspoint.com/data_structures_algorithms/linked_list_program_in_c.htm
 * [2] https://www.geeksforgeeks.org/linked-list-set-1-introduction/
 *
 *
 * DESCRIPTION
 * -----------
 * ACS - Airline Check-in System using threads
 *
 *---------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <sys/types.h>
#include <sys/wait.h>   
#include <signal.h>     
#include <errno.h> 
#include <stdlib.h>  
#include <readline/readline.h>
#include <readline/history.h>
#include <pthread.h>
#include <stdbool.h> 
#include <sys/time.h>


void *customer_entry(void *cus_info);
void *clerk_entry(void *clerkNum);

//Customer struct to store information read from the input file
typedef struct customer_info{
    	int user_id;
	int class_type;
	int service_time;
	int arrival_time;
} customer_info;

//Clerk struct to store clerk ID
struct clerk_info {
	int clerk_id;
};

//LinkedList node for both the economy and business queues
typedef struct node {
	customer_info info;
	struct node* next;
} node;

//Mutex Variables
pthread_mutex_t econ_queue_lock;
pthread_mutex_t busi_queue_lock;

pthread_mutex_t clerk_lock_0;
pthread_mutex_t clerk_lock_1;
pthread_mutex_t clerk_lock_2;
pthread_mutex_t clerk_lock_3;
pthread_mutex_t clerk_lock_4;
pthread_mutex_t clerk_lock_wait;
pthread_mutex_t overall_time_lock;

//Condition Variables
pthread_cond_t clerk_convar_0;
pthread_cond_t clerk_convar_1;
pthread_cond_t clerk_convar_2;
pthread_cond_t clerk_convar_3;
pthread_cond_t clerk_convar_4;
pthread_cond_t clerk_convar_wait;

pthread_cond_t econ_queue_convar;
pthread_cond_t busi_queue_convar;

//LinkedList heads for both the queues
node *head_econ = NULL;
node *head_busi = NULL;

/* global variables */

struct timeval init_time; //Simulation start time

double overall_waiting_time;
double econ_waiting_time;
double busi_waiting_time;

int queue_length[2]; //real-time queue length information

int queue_status[2] = {-1,-1}; //Used to the clerk currently using the queue. Initialized to -1 . Clerks are numbered from 0 - 4

int winner_selected[2] = {false, false}; //Used to check if the first person in the queue has been successfully dequeued

int temp_econ;
int temp_busi;

/*Adds a customer to the economy/business queue
 * Uses the class_type attribute to assign the queue
 */
void enqueue(customer_info info) {

	node *new_node = (node*) malloc(sizeof(node));
	new_node->info = info;
	new_node->next = NULL;

	if(info.class_type == 0) {
		node *node_econ = head_econ;

		if(head_econ == NULL) {
			head_econ = new_node;
			return;
		}
		while(node_econ->next != NULL) {
			node_econ = node_econ->next;
		}
		node_econ->next = new_node;
	}
	else if(info.class_type == 1) {
		node *node_busi = head_busi;

		if(head_busi == NULL) {
			head_busi = new_node;
			return;
		}

                while(node_busi->next != NULL) {
                        node_busi = node_busi->next;
                }
                node_busi->next = new_node;
	}
}

/*Removes the first customer from the economy/business queue
 * Uses the class_type attribute to remove from the respective queue
 */
void dequeue(customer_info info) {

	if(info.class_type == 0) {
		if(head_econ == NULL) {
			return;
		}

		node *temp = head_econ;
		head_econ = head_econ->next;


		free(temp);
	}
	else {
		if(head_busi == NULL) {
                        return;
                }

                node *temp = head_busi;
                head_busi = head_busi->next;


                free(temp);
	}
}


/*Check if the customer is at the head of the queue
 */
int check_head(customer_info info) {
	if(info.class_type == 0) {
		node* temp = head_econ;
		if(temp->info.user_id == info.user_id) {
			return 1;
		}
		return 0;
	}
	else if(info.class_type == 1)  {
		node* temp = head_busi;
		if(temp->info.user_id == info.user_id) {
			return 1;
		}
		return 0;
	}
	return -1;
}

/*Select the queue based upon priority
 * If there is customer in the business class, select the business queue
 * Otherwise select the economy queue
 */
int select_queue() {
	if(queue_length[1] > 0) {
		return 1;
	}
	else if(queue_length[0] > 0) {
		return 0;
	}
	else {
		return -1;
	}
}

/*Used to get the current system time
 *Adapted from the provided sample_gettimeofday.c
 */
double getCurSystemTime() {
	struct timeval cur_time;
	double cur_secs, init_secs;
	
	init_secs = (init_time.tv_sec + (double) init_time.tv_usec / 1000000);
	
	gettimeofday(&cur_time, NULL);
	cur_secs = (cur_time.tv_sec + (double) cur_time.tv_usec / 1000000);
	
	return cur_secs - init_secs;
}




int main(int argc, char const *argv[]) {

	//Initialize all the mutex and condition variables
	
	if(pthread_mutex_init(&econ_queue_lock, NULL) != 0) {
		printf("\n Mutex init failed \n");
		exit(0);
	}

	if(pthread_mutex_init(&busi_queue_lock, NULL) != 0) {
                printf("\n Mutex init failed \n");
                exit(0);
        }
	if(pthread_mutex_init(&clerk_lock_0, NULL) != 0) {
                printf("\n Mutex init failed \n");
                exit(0);
        }
	if(pthread_mutex_init(&clerk_lock_1, NULL) != 0) {
                printf("\n Mutex init failed \n");
                exit(0);
        }
	if(pthread_mutex_init(&clerk_lock_2, NULL) != 0) {
                printf("\n Mutex init failed \n");
                exit(0);
        }
	if(pthread_mutex_init(&clerk_lock_3, NULL) != 0) {
                printf("\n Mutex init failed \n");
                exit(0);
        }
	if(pthread_mutex_init(&clerk_lock_4, NULL) != 0) {
                printf("\n Mutex init failed \n");
                exit(0);
        }

        if(pthread_mutex_init(&clerk_lock_wait, NULL) != 0) {
                printf("\n Mutex init failed \n");
                exit(0);
        }
 	if(pthread_mutex_init(&overall_time_lock, NULL) != 0) {
                printf("\n Mutex init failed \n");
                exit(0);
        }


	if(pthread_cond_init(&econ_queue_convar, NULL) != 0) {
                printf("\n Convar  init failed \n");
                exit(0);
        }
	 if(pthread_cond_init(&busi_queue_convar, NULL) != 0) {
                printf("\n Convar init failed \n");
                exit(0);
        }
	if(pthread_cond_init(&clerk_convar_0, NULL) != 0) {
                printf("\n Convar init failed \n");
                exit(0);
        }
	if(pthread_cond_init(&clerk_convar_1, NULL) != 0) {
                printf("\n Convar init failed \n");
                exit(0);
        }
	if(pthread_cond_init(&clerk_convar_2, NULL) != 0) {
                printf("\n Convar init failed \n");
                exit(0);
        }
	if(pthread_cond_init(&clerk_convar_3, NULL) != 0) {
                printf("\n Convar init failed \n");
                exit(0);
        }
	if(pthread_cond_init(&clerk_convar_4, NULL) != 0) {
                printf("\n Convar init failed \n");
                exit(0);
        }

        if(pthread_cond_init(&clerk_convar_wait, NULL) != 0) {
                printf("\n Convar init failed \n");
                exit(0);
        }

	gettimeofday(&init_time, NULL);



	//Reading from the input file provided and storing in the customer_info struct

	FILE * f;

	f = fopen(argv[1], "r");

	char line[256];
	char first_line[5];

	char* token_array[4];

	char *token;

	fgets(first_line, sizeof(first_line), f);
	
	
	int num_customers = atoi(first_line);

	//Assigning structs
	struct customer_info customer_array[num_customers];
	struct clerk_info clerk_array[5];

	int i = 0;
	int array_count = 0;

	while(fgets(line, sizeof(line), f)) {
		token = strtok(line, ":");

		while(token != NULL) {
			token_array[i] = token;
			token = strtok(NULL, ",");
			i++;
		}

		i = 0;

		for(int j = 0; j < 4; j++) {
			if(j == 0) {
				customer_array[array_count].user_id = atoi(token_array[j]);
			}
			else if(j == 1) {
				customer_array[array_count].class_type = atoi(token_array[j]);
			}
			else if(j == 2) {
				customer_array[array_count].arrival_time = atoi(token_array[j]);
			}
			else {
				customer_array[array_count].service_time = atoi(token_array[j]);
			}
		}

		array_count++;
	}

	//Thread arrays
	pthread_t clerk_thread[5];
	pthread_t customer_thread[num_customers];

	for(int j = 0; j < 5; j++) {
		clerk_array[j].clerk_id = j;
	}


	//Creating Clerk and Customer threads
	for(int j = 0; j < 5; j++) {
		if(pthread_create(&clerk_thread[j], NULL, clerk_entry, (void *)&clerk_array[j]) != 0) {
			printf("Cannot create thread: %d\n", j);
		}

	}

	for(int j = 0; j < num_customers; j++) {
		if(pthread_create(&customer_thread[j], NULL, customer_entry, (void *)&customer_array[j]) != 0) {
			printf("Cannot create thread: %d\n", j);
		}
	}

	//Waiting for customer threads to terminate
	for(int j = 0; j < num_customers; j++) {
		if(pthread_join(customer_thread[j], NULL) != 0) {
			printf("Error on pthread_join\n");
		}
	}

	//Waiting Times

	printf("\n All jobs finished... Calculating wait times...\n");
	fprintf(stdout, "The average waiting time for all customers in the system is: %.2f seconds. \n", overall_waiting_time/num_customers);
	fprintf(stdout, "The average waiting time for all business-class  customers is: %.2f seconds. \n", busi_waiting_time/num_customers);
	fprintf(stdout, "The average waiting time for all economy-class customers is: %.2f seconds. \n", econ_waiting_time/num_customers);



	return 0;
}


/*Customer Thread
 * 8 customer threads run in parallel
 */
void * customer_entry(void * cus_info) {
	
	struct customer_info * p_info = (struct customer_info *) cus_info;

	//Sleep until the arrival time of the customer

	usleep(p_info->arrival_time * 100000);

	fprintf(stdout, "A customer arrives: customer ID %2d. \n", p_info->user_id);

	//Lock the queue based upon the class of the customer

	int class = p_info->class_type;
	if(class == 0) {
		if(pthread_mutex_lock(&econ_queue_lock) != 0) {
			printf("Economy Mutex lock failed.\n");
		}
	}
	else if(class == 1) {
		if(pthread_mutex_lock(&busi_queue_lock) != 0) {
			printf("Business Mutex lock failed.\n");
		}
	}

	//Add customer to the queue
	
	enqueue(*p_info);


 	double queue_enter_time = getCurSystemTime();

	
	//Update queue length based upon class
	
	if (class == 0) {
		queue_length[0]+= 1;
	}
	else if (class == 1) {
		queue_length[1] += 1;
	}
	

	pthread_mutex_lock(&clerk_lock_wait);



        if(class == 0) {
                fprintf(stdout, "A customer enters a queue: the queue ID %1d, and the length of the queue %2d. \n", class, queue_length[0]);
        }

        else if(class == 1) {
                fprintf(stdout, "A customer enters a queue: the queue ID %1d, and the length of the queue %2d. \n", class, queue_length[1]);
        }


	while(1) {
		if(class == 0) {
			if(pthread_cond_signal(&clerk_convar_wait) != 0) {
				printf("Clerk wait cond signal failed.\n"); //Signal to the clerks that there is a customer in the queue
			}
		
			pthread_mutex_unlock(&clerk_lock_wait);

			if(pthread_cond_wait(&econ_queue_convar, &econ_queue_lock) != 0) {
				printf("Economy wait condition failed.\n"); //Wait for clerk to signal the economy customer
			}

			//If customer is at head, then remove it from the queue and break
			if(check_head(*p_info) == 1 && winner_selected[0] == false) {
				dequeue(*p_info);
				queue_length[0] -= 1;
				winner_selected[0] = true;
				break;
			}
		}
		else if(class == 1) {
			if(pthread_cond_signal(&clerk_convar_wait) != 0) {
				printf("Clerk wait cond signal failed.\n"); //Signal to the clerks that there is a customer in the queue
			}
		
			pthread_mutex_unlock(&clerk_lock_wait);
		
			if(pthread_cond_wait(&busi_queue_convar, &busi_queue_lock) != 0) { //Wait for the clerk to signal the business customer
				printf("Business wait condition failed.\n");
			}

			//If customer is at head, then remove it from the queue and break
			if(check_head(*p_info) == 1 && winner_selected[1] == false) {
				dequeue(*p_info);
				queue_length[1] -= 1;
				winner_selected[1] = true;
				break;
			}
		}
	}

	//Check to see which clerk signaled the customer
	int clerk_num;
	if(class == 0) {
                clerk_num = queue_status[0];
		temp_econ = clerk_num;
                queue_status[0] = -1;
		if(pthread_mutex_unlock(&econ_queue_lock) != 0) {
			printf("Economy queue unlock failed.\n");
		}
        }
        else if(class == 1) {
                clerk_num = queue_status[1];
		temp_busi = clerk_num;
                queue_status[1] = -1;
		if(pthread_mutex_unlock(&busi_queue_lock) != 0) {
			printf("Business queue unlock failed.\n");
		}
        }



	usleep(10);



	double queue_exit_time = getCurSystemTime();

	//Update the waiting times

	pthread_mutex_lock(&overall_time_lock);
	overall_waiting_time += queue_exit_time - queue_enter_time;
	if(class == 0) {
		econ_waiting_time += queue_exit_time - queue_enter_time;
	}
	else if(class == 1) {
		busi_waiting_time += queue_exit_time - queue_enter_time;
	}
	pthread_mutex_unlock(&overall_time_lock);

	fprintf(stdout, "A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", queue_exit_time, p_info->user_id, clerk_num);

	//Sleep until the end of serivce time
	usleep(p_info->service_time * 100000);

	double service_end_time = getCurSystemTime();

	fprintf(stdout, "A clerk finished serving a customer: end time %.2f, the customer ID %2d, the clerk ID: %1d. \n", service_end_time, p_info->user_id, clerk_num);

	//Signal to the clerk that the customer has finished service
                if(clerk_num == 0) {
			pthread_mutex_lock(&clerk_lock_0);
                        pthread_cond_signal(&clerk_convar_0);
			pthread_mutex_unlock(&clerk_lock_0);
              }
                else if(clerk_num == 1) {
                        pthread_mutex_lock(&clerk_lock_1);
                        pthread_cond_signal(&clerk_convar_1);
                        pthread_mutex_unlock(&clerk_lock_1);			
                }
                else if(clerk_num == 2) {
                        pthread_mutex_lock(&clerk_lock_2);
                        pthread_cond_signal(&clerk_convar_2);
                        pthread_mutex_unlock(&clerk_lock_2);                
		}
                else if(clerk_num == 3) {
                        pthread_mutex_lock(&clerk_lock_3);
                        pthread_cond_signal(&clerk_convar_3);
                        pthread_mutex_unlock(&clerk_lock_3);
                }
                else if(clerk_num == 4) {
                        pthread_mutex_lock(&clerk_lock_4);
                        pthread_cond_signal(&clerk_convar_4);
                        pthread_mutex_unlock(&clerk_lock_4);			
                }



	pthread_exit(NULL);
}

/*Clerk Function
 * 5 clerk threads running in parallel.
 */
void * clerk_entry(void * clerkNum) {

	struct clerk_info * c_info = (struct clerk_info *) clerkNum;
	int clerk_num = c_info->clerk_id;

	while(1) {

		//Wait for the queues to be non empty

		pthread_mutex_lock(&clerk_lock_wait);
		while(queue_length[0] == 0 && queue_length[1] == 0) {
			pthread_cond_wait(&clerk_convar_wait, &clerk_lock_wait);
		}
		pthread_mutex_unlock(&clerk_lock_wait);

		//Select queue based on priority

		int selected_queue = select_queue();


		//Lock the selected queue

		if(selected_queue == 0) {
			if(pthread_mutex_lock(&econ_queue_lock) != 0) {
				printf("Economy queue lock failed.\n");
			}
		}
		else if(selected_queue == 1) {
			if(pthread_mutex_lock(&busi_queue_lock) != 0) {
				printf("Business queue lock failed.\n");
			}
		}

		//Signal to the queue, that the clerk is ready to serve
		if(selected_queue == 0 && queue_status[0] < 0) {
			queue_status[0] = clerk_num;
			
			winner_selected[0] = false;
			if(pthread_cond_signal(&econ_queue_convar) != 0) {
				printf("Economy condition failed\n");
			}
		}
		else if(selected_queue == 1 && queue_status[1] < 0) {
			queue_status[1] = clerk_num; 
			winner_selected[1] = false;
                        if(pthread_cond_signal(&busi_queue_convar) != 0) {
				printf("Business condition failed\n");
			}
		}

		//Unlock the selected queue
                if(selected_queue == 0) {
                       if(pthread_mutex_unlock(&econ_queue_lock) != 0) {
			       printf("Economy unlock failed.\n");
		       }
                }
                else if(selected_queue == 1) {
                        if(pthread_mutex_unlock(&busi_queue_lock) != 0) {
				printf("Business unlock failed.\n");
			}
                }


		//Wait for the signal from customer after is has finished service
		if(clerk_num == 0 && (queue_status[0] == 0 || queue_status[1] == 0)) {
			pthread_mutex_lock(&clerk_lock_0);
			pthread_cond_wait(&clerk_convar_0, &clerk_lock_0);
			pthread_mutex_unlock(&clerk_lock_0);
		}
		else if(clerk_num == 1  && (queue_status[0] == 1 || queue_status[1] == 1)) {
			pthread_mutex_lock(&clerk_lock_1);
                        pthread_cond_wait(&clerk_convar_1, &clerk_lock_1);
			pthread_mutex_unlock(&clerk_lock_1);
                }
		else if(clerk_num == 2 && (queue_status[0] == 2 || queue_status[1] == 2)) {
			pthread_mutex_lock(&clerk_lock_2);
                        pthread_cond_wait(&clerk_convar_2, &clerk_lock_2);
			pthread_mutex_unlock(&clerk_lock_2);
                }
		else if(clerk_num == 3  && (queue_status[0] == 3 || queue_status[1] == 3)) {
			pthread_mutex_lock(&clerk_lock_3);
                        pthread_cond_wait(&clerk_convar_3, &clerk_lock_3);
			pthread_mutex_unlock(&clerk_lock_3);
                }
		else if(clerk_num == 4 && (queue_status[0] == 4 || queue_status[1] == 4)) {
			pthread_mutex_lock(&clerk_lock_4);
                        pthread_cond_wait(&clerk_convar_4, &clerk_lock_4);
			pthread_mutex_unlock(&clerk_lock_4);
                }

		
	}
	


	pthread_exit(NULL);
}

