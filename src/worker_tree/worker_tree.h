#ifndef __WORKER_TREE_H__
#define __WORKER_TREE_H__


#include "../queue/queue.h"
#include <stdbool.h>
#include <stdlib.h>

typedef int WorkerId;

typedef struct __Node {
    WorkerId id;
    struct __Node* left;
    struct __Node* right;
} Node;

typedef struct {
    Node* root;
} WorkerTree;

typedef struct {
    WorkerId parent;
    WorkerId child;
} Result;


void tree_init(WorkerTree* tree);
Result tree_create_node(WorkerTree* tree, WorkerId worker_id);
void tree_remove_node(WorkerTree* tree, WorkerId worker_id);
bool tree_exists(WorkerTree* tree, WorkerId worker_id);
void tree_get_nodes(WorkerTree* tree, Queue* nodes);
void print_tree(WorkerTree* tree);
void tree_delete(WorkerTree* tree);


#endif