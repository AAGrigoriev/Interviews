#pragma once

struct Node
{
    Node(int data, unsigned freq);
    Node *left = nullptr;
    Node *right = nullptr;

    int data;
    unsigned freq;
};

struct compare
{
    bool operator()(Node *left, Node *right);
};