
#ifndef MENU_CLI_H
#define MENU_CLI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "WWVB_Arduino.h"
#include "embedded_cli.h"

extern EmbeddedCli *cli;


//void init_cli();
void init_menu_cli();

void cli_loop();

// some utility functions for handling common user values
bool try_parse_hex_uint16t(const char* tok, uint16_t * val);
bool try_parse_hex_uint8t(const char* tok, uint8_t * val);
bool try_parse_hex_uint32t(const char* tok, uint32_t * val);


// want to create what looks like a menu based system using embedded_cli which doesn't natively support it
// need to create some primitives
// 1. cd 
// 2. ls
//
// 3. need to create a structure to allow for branching filesystem

#define MAX_NAME_LEN 32

typedef enum {
    MY_DIRECTORY,
    MY_FILE
} NodeType;

// a basic node, either directory or file
// 

typedef struct Node {
    //char name[MAX_NAME_LEN];
    char * name;
    NodeType type;
    CliCommandBinding cliBinding;
    struct Node *parent;
    struct Node **children;      // Points to array of children nodes
    int num_children;
} Node;


void change_to_node(Node * n);
void add_root_filesystem(Node * child);
void addMenuCLI_current_directory(); 


#endif