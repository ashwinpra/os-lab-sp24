/*
    Operating Systems Laboratory Assignment 1
    Name: Ashwin Prasanth
    Roll Number: 21CS30009
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#define MAXLINE 100 // max size of line read in file
#define MAX_CITIES 100 // max number of cities
#define MAX_CHILDREN 10 // max number of children for a city

// making a node for storing the cities and its children
typedef struct _node {
    char* city_name;
    struct _node* children[MAX_CHILDREN];
    int num_children;
} node;

// utility function to make a city node
node* make_node(char* city_name, int num_children) {
    node* new_node = malloc(sizeof(node));
    new_node->city_name = malloc(strlen(city_name) + 1);
    strcpy(new_node->city_name, city_name);
    new_node->num_children = num_children;
    return new_node;
}

int main(int argc, char const *argv[])
{   
    node* cities[MAX_CITIES]; // adjacency list for cities

    node* root = malloc(sizeof(node)); // root node - given as user input

    int indentation = 0; // indentation level - 0 if unspecified

    // throwing error if number of arguments are not right
    if(argc < 2) {
        printf("Run with a node name");
        exit(1);
    }
    
    // else, getting city name and indentation
    else {
        root->city_name = argv[1];
        if(argc > 2) indentation = atoi(argv[2]);
    }


    char word[MAXLINE]; // word that will be read from file
    int i_children = 0; // to iterate through the children of parent node
    int total_children = 0; // total children of parent node
    int parent_index = 0; // index of parent node
    int i_cities = 0; // to iterate through adjacency list

    // reading the treeinfo file
    FILE* fp = fopen("treeinfo.txt", "r");
    while(fscanf(fp, "%s", word) != EOF) {
        // "word" is either the parent node (first word of the line) , no. of children, or the children themselves

        if(word[0] >= '0' && word[0] <= '9') {
            // it is denoting number of children
            i_children = atoi(word);
            total_children = i_children;
        }

        else {
            // if i_children is 0, then its parent, else children

            if(i_children == 0) {
                node* parent = make_node(word, 0);
                cities[i_cities++] = parent;
                parent_index = i_cities-1;
            }

            else{
                // it is a child
                node* child = make_node(word, 0);

                if(total_children == i_children) {
                    // first child
                    cities[parent_index]->num_children = total_children;
                }
                // add this node to list of children
                cities[parent_index]->children[total_children - i_children] = child;
                i_children--;
            }
        }
    }

    // looking for root node
    int root_index = -1;
    for(int i = 0; i < i_cities; i++) {
        if(strcmp(cities[i]->city_name, root->city_name) == 0) {
            root_index = i;
            break;
        }
    }

    // throwing error if city name is invalid
    if(root_index == -1) {
        printf("City %s not found\n", root->city_name);
        exit(1);
    }

    // first print the parent, then fork children
    printf("%*s%s (%d)\n", indentation*4, "", cities[root_index]->city_name, getpid());

    for(int i = 0; i < cities[root_index]->num_children; i++) {
        if(fork() == 0) {
            char indentation_str[MAXLINE];
            sprintf(indentation_str, "%d", indentation+1); // convert indentation to string

            execlp("./proctree", "./proctree", cities[root_index]->children[i]->city_name, indentation_str, NULL);
        }
        else {
            wait(NULL);
        }
    }

    return 0;
}
