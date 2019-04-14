#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <pthread.h>

typedef struct paystub_s{
	double amount;
	int days_since_last_payment;
	struct tm date;
}paystub_t;

typedef struct node_s node;

struct node_s{
	paystub_t *data;
	node *next;
};

void debug(int verbose, int line);
node *new_node(paystub_t t);
void add_to_list(node *list, paystub_t t, int count);
paystub_t *new_stub(paystub_t t);


#define RECORD_FILENAME "budget_record"
int verbose_flag = 0;
int main(int argc, char **argv){
	//getopt stuff
	char c;
	int add_flag = 0;

	//the big boys
	int record_fd = 0;
	node *head = NULL;
	int count = 0;
	paystub_t new_entry;
	paystub_t buffer;
	struct tm newtime;
   	time_t ltime;
	ssize_t bytes_read;


	while((c = getopt(argc, argv, "hva:d:")) != -1){
		switch(c){
			case 'h':
				//help message
				fprintf(stderr, "usage:\n./budget <-a: $$.$$> <-v> <-h>\n-a: add a new payment, enter amount in USD\n-v: run in verbose mode\n-h: display this help message\n");
				exit(EXIT_SUCCESS);
			break;
			case 'v':
				verbose_flag = 1;
			break;
			case 'a':
				add_flag = 1;
				//add a new entry into the thing
				//this will just build a new paystub_t and store the command line info, what happens with it depends on the results
				//after attempting to read the record file
				new_entry.amount = atof(optarg);
			break;
		}
	}
	head = (node *)calloc(1, sizeof(node));


	record_fd = open(RECORD_FILENAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	while((bytes_read = read(record_fd, &buffer, sizeof(paystub_t))) > 0){
		add_to_list(head, buffer, count);
		count++;
	}
	//fprintf(stderr, "bytes read: %lu - count: %d\n", bytes_read, count);
	close(record_fd);


	//put the new entry in the place

	if(add_flag > 0){
		ltime=time(&ltime);
		localtime_r(&ltime, &newtime);
		new_entry.date = newtime;

		if(count > 0){
			//the record already exists, do stuff
			//iterate through the linked list to find the last item
			{
				node *tmp = head;
				while(tmp->next != NULL){
					tmp = tmp->next;
				}
				if(verbose_flag > 0){
					fprintf(stderr, "last payment date:%d\n", tmp->data->date.tm_yday);
					fprintf(stderr, "todays payment date:%d\n", newtime.tm_yday);
				}
				//calculate days since last payment for the new entry
				new_entry.days_since_last_payment = newtime.tm_yday - tmp->data->date.tm_yday;	
			}
		} else {
			//no previous record, new record boys
			//add the new entry to the list
			new_entry.days_since_last_payment = 0;
		}
		
		if(verbose_flag > 0){
			fprintf(stderr, "number of entries in record (prior to adding new entry):%d\n", count);
		}

		//fprintf(stderr, "new entry info: ammount %f days %d date %s\n", new_entry.amount, new_entry.days_since_last_payment, asctime(&new_entry.date) );
		add_to_list(head, new_entry, count);
		count++;
	}

	//some error handling
	//if no entry has been added and there is nothing in the list

	if(add_flag == 0 && count == 0){
		fprintf(stderr, "bro, my man, you gotta enter some infor dawg, this list is deadass EMPTY man, for real\n");
		exit(EXIT_FAILURE);
	}


	//iterate through the list and get the info
	{
		double sum = 0.0;
		int time_interval = 1;
		node *tmp = head;
		if(tmp == NULL){
			fprintf(stderr, "uh oh.....\n");
		}
		
		//since this is the first entry, the time interval is gonna be zero
		//but you can't divide by zero so we set the time_interval variable we will actually be using for calculations to 1
		//but the first entry in the list is gonna have a 0 time_interval so we don't need to do anything with it

		sum += tmp->data->amount;

		while(tmp->next != NULL){
			tmp = tmp->next;
			sum += tmp->data->amount;
			time_interval += tmp->data->days_since_last_payment;
			if(verbose_flag > 0){
				fprintf(stderr, "entry %s:%2.2f\n", asctime(&tmp->data->date), tmp->data->amount);
			}
		}
		if(verbose_flag > 0){
			fprintf(stderr, "number of entries in record (including new entry):%d\n", count);
			fprintf(stderr, "total payment amount in record:%2.2f\n", sum);
			fprintf(stderr, "total days elapsed in record:%d\n", time_interval );
		}

		fprintf(stderr, "Budget Info:\nAverage Income:%2.2f\tAverage Time Between Payments:%f\nDaily Spending:%2.2f\n",(double)(sum / count), (double)(time_interval/count), ((double)(sum / count))/((double)(time_interval/count)));
	}

	//update the record
	record_fd = open(RECORD_FILENAME, O_TRUNC | O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	{
		node *tmp = head;
		ssize_t bytes_written = 0;
		bytes_written = write(record_fd, tmp->data, sizeof(paystub_t));
		debug(verbose_flag, __LINE__);
		while(tmp->next != NULL){
			debug(verbose_flag, __LINE__);
			tmp = tmp->next;
			bytes_written = write(record_fd, tmp->data, sizeof(paystub_t));
		}
	}
	close(record_fd);
	debug(verbose_flag, __LINE__);

	return 0;
}

void debug(int verbose, int line){
	if(verbose > 0){
		fprintf(stderr, "reached line %d\n", line);
	}
}

node *new_node(paystub_t t){
	node *nt = (node *)calloc(1, sizeof(node));
	nt->data = new_stub(t);
	return nt;
}

paystub_t *new_stub(paystub_t t){
	paystub_t *newguy = (paystub_t *)calloc(1, sizeof(paystub_t));

	memcpy(&newguy->amount, &t.amount, sizeof(double));
	memcpy(&newguy->date, &t.date, sizeof(struct tm));
	memcpy(&newguy->days_since_last_payment, &t.days_since_last_payment, sizeof(int));
	//fprintf(stderr, "allocated boy:\namount:%f date:%s days:%d\n",newguy->amount, asctime(&newguy->date), newguy->days_since_last_payment );
	return newguy;
}

void add_to_list(node *list, paystub_t t, int count){
	node *tmp = list;

	if(count == 0){
		//fprintf(stderr, "adding first entry to list....%f\n", t.amount);
		tmp->data = new_stub(t);
	
	} else {

		while(tmp->next != NULL){
			debug(verbose_flag, __LINE__);
			tmp = tmp->next;
		}
		//tmp points to end of list
		//copy

		//fprintf(stderr, "adding (%f) at position %d\n", t.amount, count);
		tmp->next = new_node(t);
	}

}