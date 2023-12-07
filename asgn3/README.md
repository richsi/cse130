I start off by initializing a struct queue and the variables inside of it.
The variables include the size of the queue, the number of items in the queue, the mutext, and thread conditions, and a pointer to the head node of a linked list.

queue_new initializes the queue and sets the values for all the variables inside my struct. It initializes the queue size to the size, the number of items to 0, the head of the linked list to NULL, and all of the thread variables to NULL as well.

queue_delete frees all the memory of the queue. It also destroys the thread mutext and conditions

queue_push pushes to my queue in a FIFO order. Unless the arguments passed in are invalid, the thread locks until the function finishes running where then a signal will be passed and the thread will unlock. The queue works by assigning the node's value to elem and appending it to the list.

queue_pop operates similarly to queue_push except you remove the first node of the linked list and move the head pointer to the next node. It also frees the first node. The thread lock, signal, and unlock logic is the same as queue_push. It locks, executes the function, then signals to unlock.

