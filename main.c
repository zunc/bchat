/* 
 * File:   main.c
 * Author: khoai
 *
 * Created on April 3, 2014, 2:49 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include "server.h"

#include "connmap.h"

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("Enter your port in parameter\nEx: bchat 8888\n");
		return 1;
	}
	server_start(atoi(argv[1]));
	return (EXIT_SUCCESS);
}

