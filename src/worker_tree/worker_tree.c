#include "worker_tree.h"
#include <stdlib.h>
#include <stdio.h>


/* --- NODE METHODS --- */

void node_init(Node* node, WorkerId id)
{
    node->id = id;
    node->left = NULL;
    node->right = NULL;
}

bool node_full(Node* node)
{
    return node->left != NULL && node->right != NULL;
}


void node_add(Node* parent, Node* child)
{
    if(parent->left == NULL) parent->left = child;
    else if(parent->right == NULL) parent->right = child;
}

/* --- TREE METHODS --- */

void tree_init(WorkerTree* tree)
{
    tree->root = NULL;
}

Result tree_create_node(WorkerTree* tree, WorkerId worker_id)
{
    Result res = {.parent = -1, .child = worker_id};

    if(tree->root == NULL)
    {
        tree->root = (Node*) malloc(sizeof(Node));
        node_init(tree->root, worker_id);
        return res;
    }

    Queue q;
    queue_init(&q);
    queue_push(&q, (void*) tree->root, sizeof(tree->root));

    while(!queue_empty(&q))
    {
        Node* node;
        queue_pop(&q, (void**) &node, NULL);

        if(!node_full(node))
        {
            Node* c = (Node*) malloc(sizeof(Node));
            node_init(c, worker_id);
            node_add(node, c);
            queue_delete(&q);

            res.parent = node->id;
            return res;
        }

        queue_push(&q, node->left, sizeof(node->left));
        queue_push(&q, node->right, sizeof(node->right));
    }

    queue_delete(&q);
}

bool __find_node(Node* node, WorkerId worker_id)
{
    if(node == NULL) return false;
    if(node->id == worker_id) return true;

    return __find_node(node->left, worker_id) || __find_node(node->right, worker_id);
}

bool tree_exists(WorkerTree* tree, WorkerId worker_id)
{
    return __find_node(tree->root, worker_id);
}

void __print_node(Node* node, int layer)
{
    if(node == NULL) return;

    __print_node(node->right, layer + 1);
    for(int i = 0; i < layer; i++) printf("\t");
    printf("[Worker #%d]\n", node->id);
    __print_node(node->left, layer + 1);
}

void print_tree(WorkerTree* tree)
{
    __print_node(tree->root, 0);
}

void __delete_node(Node* node)
{
    if(node == NULL) return;

    __delete_node(node->left);
    __delete_node(node->right);
    free(node);
}

void tree_delete(WorkerTree* tree)
{
    __delete_node(tree->root);
}


Node* __remove_node(Node* node, WorkerId worker_id)
{
    if(node == NULL) return NULL;
    if(node->id == worker_id)
    {
        __delete_node(node);
        return NULL;
    }
    node->left = __remove_node(node->left, worker_id);
    node->right = __remove_node(node->right, worker_id);
    return node;
}


void tree_remove_node(WorkerTree* tree, WorkerId worker_id)
{
    tree->root = __remove_node(tree->root, worker_id);
}


void tree_get_nodes(WorkerTree* tree, Queue* nodes)
{
    Queue q;
    queue_init(&q);
    queue_push(&q, (void*) tree->root, sizeof(tree->root));

    while(!queue_empty(&q))
    {
        Node* node;
        queue_pop(&q, (void**) &node, NULL);
        
        if(node == NULL) continue;
        
        queue_push(nodes, (void*) &node->id, sizeof(node->id));
        queue_push(&q, (void*) node->left, sizeof(node->left));
        queue_push(&q, (void*) node->right, sizeof(node->right));
    }

    queue_delete(&q);
}