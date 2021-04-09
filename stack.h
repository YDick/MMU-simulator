// Reference: GeeksForGeeks.org "implement a stack using singly linked list"

#include <stdlib.h>

struct Node {
    int data;
    struct Node *next;
};

void push(int data);
void pop();