// Queue to represent scheduling
// If SJF, queue will be sorted by shortest time
// If FCFS, queue will be sorted by order of arrival

#define NO_PID -1
#define FCFS 0
#define SJF 1

// Node represents a process queue
struct node {
    // PID of the process, will be -1 if not running
    int pid;

    // Name is the name of the program - not the process
    char *name;
    // Including args inside of the node makes things easier
    // to keep track of. Especially when switching to queued process
    char **args;

    // Next process in the queue
    struct node *next;
};

// Queue to represent FCFS scheduling
struct queue {
    struct node *head;
    struct node *tail;

    // Type of scheduling to use
    int sched_type;
};


// Initialize the queue
void initqueue(struct queue *q, int sched_type) {
    q->head = NULL;
    q->tail = NULL;

    q->sched_type = sched_type;
}

// Evaluate exec time of node
int evaltime(struct node *n) {
    // Given by arg1 * arg2
    // Specic to our p-shell.c program, so this will can't be generalized
    // We would need a comprehensive list of the average times for each program
    // But beyond the scope of this project
    return atoi(n->args[1]) * atoi(n->args[2]);
}


// Free a specific node when deleting or dequeueing it from the queue
void freenode(struct node *n) {
    // Free the name
    free(n->name);
    // Free all arguments
    for (int i = 0; n->args[i] != NULL; i++) {
        free(n->args[i]);
    }
    // Free the array of arguments (2D array)
    free(n->args);

    // Finall free the node itself
    free(n);
}

// Standard insert for FCFS scheduling
void standardinsert(struct node *curr_node, struct queue *q) {
    // If the queue is empty, set the head and tail to the new node
    if (q->head == NULL && q->tail == NULL) {
        q->head = curr_node;
        q->tail = curr_node;
    }
    // Add the new node to the end of the queue
    else {
        // Set the previous tail to point to the new node
        q->tail->next = curr_node;
        // Set the tail to the new node
        q->tail = curr_node;
    }
}

// Sorted insert for SJF scheduling
// Sorting on insert ensures queue is always sorted
// Less efficient than min head - but easier to implement with our current code
void sortedinsert(struct node *node_to_insert, struct queue *q) {
    // If queue is empty - speical case
    if (q->head == NULL && q->tail == NULL) {
        q->head = node_to_insert;
        q->tail = node_to_insert;
        return;
    }

    // If insert is new head - special case
    if (evaltime(q->head) > evaltime(node_to_insert)) {
        node_to_insert->next = q->head;
        q->head = node_to_insert;
    }
    // If insert is new tail - special case
    else if (evaltime(q->tail) <= evaltime(node_to_insert)) {
        q->tail->next = node_to_insert;
        q->tail = node_to_insert;
    }
    // Otherwise, insert in the proper sorted order
    else {
        // Locate the node before the point of insertion
        struct node *curr_node = q->head;
        while (curr_node->next != NULL && evaltime(curr_node->next) < evaltime(node_to_insert)) {
            curr_node = curr_node->next;
        }

        // Insert the node
        node_to_insert->next = curr_node->next;
        curr_node->next = node_to_insert;
    }
}


// Adds a node to the end of the queue
void enqueue(char *name, char **args, struct queue *q) {
    // Create a new node, with pid and name as passed arguments
    struct node *curr_node = (struct node *)malloc(sizeof(struct node));
    // When we enqueue, we do not run the process
    curr_node->pid = NO_PID;

    // Allocate memory for the name and copy it
    curr_node->name = (char *)malloc(sizeof(char) * strlen(name));
    curr_node->name = strcpy(curr_node->name, name);

    // Allocate memory for the arguments and copy them
    // 10 can be any number, it's just a placeholder (should be more than 4)
    curr_node->args = (char **)malloc(sizeof(char *) * 10);
    for (int i = 0; args[i] != NULL; i++) {
        curr_node->args[i] = (char *)malloc(sizeof(char) * strlen(args[i]));
        curr_node->args[i] = strcpy(curr_node->args[i], args[i]);
    }

    // Determine where to insert the node based on the scheduling type
    if (q->sched_type == FCFS) { standardinsert(curr_node, q); }
    else if (q->sched_type == SJF) { sortedinsert(curr_node, q); }
    else { standardinsert(curr_node, q); }
}


// Removes the head of the queue and returns its pid
int dequeue(struct queue *q) {
    // If the queue is empty, let caller know
    if (q->head == NULL || q->tail == NULL) { return -1; }

    // Otherwise, remove the head of the queue and return its pid
    struct node *curr_node = q->head;
    int curr_pid = curr_node->pid;
    q->head = q->head->next;

    // If the queue is now empty, set the tail to NULL
    if (q->head == NULL) { q->tail = NULL; }

    // Free the memory allocated for the removed node
    freenode(curr_node);

    return curr_pid;
}


// Deletes the node with the given pid from the queue
void delete(int key, struct queue *q) {
    struct node *curr_node = q->head;
    struct node *prev_node = NULL;

    // Search for the node with the given pid, deleting it if found
    while (curr_node != NULL && curr_node->pid != key) {
        prev_node = curr_node;
        curr_node = curr_node->next;
    }

    // If the node was not found, return
    if (curr_node == NULL) { return; }

    // If the node is the head of the queue, set the head to the next node
    if (curr_node == q->head) { q->head = q->head->next; }
    // If the node is the tail of the queue, set the tail to the previous node
    if (curr_node == q->tail) { q->tail = prev_node; }

    // Set the previous node to point to the next node
    if (prev_node != NULL) { prev_node->next = curr_node->next; }

    // Free the memory allocated for the removed node
    freenode(curr_node);
}