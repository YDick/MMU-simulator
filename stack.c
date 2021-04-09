// Reference: GeeksForGeeks.org "implement a stack using singly linked list"

#include "stack.h"

struct Node* head;


void push(int data) {
    struct Node* temp;
    temp = new Node();

    temp->data = data;
    temp->next = head;

    top = temp;

}

int pop() {
    struct Node* temp;

    if (top == NULL) {
        cout << "\n Stack Underflow" << endl;
        exit(1);
    }
    else {
        temp = top;
        top = top -> next;
        temp -> next = NULL;

        int ret = temp->data;
        free(temp);
        return ret;
    }
}

int moveToTop(int data) {


}