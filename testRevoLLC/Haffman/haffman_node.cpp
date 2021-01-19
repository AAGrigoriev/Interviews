#include <Haffman/haffman_node.hpp>
#include <iostream>

Node::Node(int data, unsigned freq) : data(data), freq(freq) {}

bool compare::operator()(Node *left, Node *right)
{
    return left->freq > right->freq;
}

void print_tree(Node *root)
{
    if (root == nullptr)
        return;

    if (root->data != '*')
        std::cout << char(root->data) << std::endl;

    print_tree(root->left);
    print_tree(root->right);
}

void free_tree(Node *root)
{
    if (!root)
        return;

    if (root->left)
        free_tree(root->left);
    if (root->right)
        free_tree(root->right);
    delete root;
}