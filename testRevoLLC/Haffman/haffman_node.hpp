#pragma once

struct Node
{
    Node(int data, unsigned freq);
    Node *left = nullptr;
    Node *right = nullptr;

    int data;
    unsigned freq;
};

/* TODO: lamda */
struct compare
{
    bool operator()(Node *left, Node *right);
};


void print_tree(Node* root);

void free_tree(Node* root);