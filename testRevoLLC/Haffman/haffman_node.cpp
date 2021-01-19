#include <Haffman/haffman_node.hpp>

Node::Node(int data, unsigned freq) : data(data), freq(freq) {}

bool compare::operator()(Node *left, Node *right)
{
    return left->freq > right->freq;
}
