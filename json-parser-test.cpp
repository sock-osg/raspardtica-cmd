#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdlib.h>
#include <RF24/RF24.h>

RF24 radio(RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);

const uint64_t MASTER_PIPE = 0xF0F0F0F0E1LL;

// hack to avoid SEG FAULT, issue #46 on RF24 github https://github.com/TMRh20/RF24.git
unsigned long got_message;

/* Flag set by ‘--verbose’. */
static int verbose_flag;

void setup(uint64_t* device) {
	//Prepare the radio module
	//printf("\nPreparing interface\n");
	radio.begin();
	radio.setRetries(15, 15);
	//	radio.setChannel(0x4c);
	//	radio.setPALevel(RF24_PA_MAX);
	//	radio.setPALevel(RF24_PA_MAX);

	radio.printDetails();
	radio.openWritingPipe(MASTER_PIPE);
	radio.openReadingPipe(1, *device);
	//	radio.startListening();
}

bool sendMessage(int action) {
	//This function send a message, the 'action', to the arduino and wait for answer
	//Returns true if ACK package is received
	//Stop listening
	radio.stopListening();
	unsigned long message = action;
	printf("Now sending  %lu...", message);

	//Send the message
	bool ok = radio.write(&message, sizeof(unsigned long));
	if(!ok) {
		printf("failed...\n\r");
	} else {
		printf("ok!\n\r");
	}

	//Listen for ACK
	radio.startListening();
	//Let's take the time while we listen
	unsigned long started_waiting_at = millis();
	bool timeout = false;
	while (!radio.available() && !timeout) {
		//printf("%d", !radio.available());
		if (millis() - started_waiting_at > 1000 ){
			timeout = true;
		}
	}

	if (timeout) {
		//If we waited too long the transmission failed
		printf("Oh gosh, it's not giving me any response...\n\r");
		return false;
	} else {
		//If we received the message in time, let's read it and print it
		radio.read( &got_message, sizeof(unsigned long) );
		printf("Yay! Got this response %lu.\n\r",got_message);
		return true;
	}

}

int sendCommand(char* action, uint64_t* device) {
	char choice =

	setup();
	bool switched = false;
	int counter = 0;

	// Define the options
	while ((choice = getopt(argc, argv, "m:")) != -1) {
		if (choice == 'm') {
			printf("\n Talking with my NRF24l01+ friends out there....\n");

			while (switched == false && counter < 5) {
				switched = sendMessage(atoi(optarg));
				counter ++;
				sleep(1);
			}
		} else {
			// A little help:
			printf("\n\rIt's time to make some choices...\n");
			printf("\n\rTIP: Use -m idAction for the message to send. ");
			printf("\n\rExample (id number 12, action number 1): ");
			printf("\nsudo ./remote -m 121\n");
		}

		//return 0 if everything went good, 2 otherwise
		if (counter < 5) {
			return 0;
		} else {
			return 2;
		}
	}
}

int main (int argc, char *argv[]) {
  int opt;

  char* action;
	uint64_t device;

  while (true) {
  	static struct option long_options[] = {
  		{"verbose", no_argument, &verbose_flag, 1},
  		{"brief",   no_argument, &verbose_flag, 0},
  		// These options don’t set a flag. We distinguish them by their indices.
  		{"action", required_argument, 0, 'a'},
  		{"device", required_argument, 0, 'd'},
  		{0, 0, 0, 0}
  	};

  	// getopt_long stores the option index here.
		int option_index = 0;

		opt = getopt_long (argc, argv, "a:d:", long_options, &option_index);
		//printf ("Opt %d with value %s\n", opt, optarg);

		if (opt == -1) {
			break;
		}

		switch (opt) {
			case 0:
      	/* If this option set a flag, do nothing else now. */
        if (long_options[option_index].flag != 0)
        	break;
				printf ("option %s\n", long_options[option_index].name);
        	if (optarg)
          	printf (" with arg %s\n", optarg);
            break;
			case 'a':
				action = optarg;
				break;
			case 'd':
				device = strtoull(optarg, NULL, 0);
      	break;
		}
  }

  return sendCommand(action, &device);
}
