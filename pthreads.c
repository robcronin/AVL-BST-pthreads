#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <pthread.h>

// set up node structure
typedef struct node{
	int val;		// tree's value
	struct node *left;	// child pointers
	struct node *right;
	pthread_mutex_t lock;	// and individual lock
}NODE;


// Global Args
int max=1000;									// set as max number possible in tree
int gap=3;									// set as digits for max number (max-1)
char* empty="~~~";								// set as empty node print symbol (use gap number of characters) 
int quiet=0;									// variable to choose if add/del info is printed or not

// tree root and a root_lock
NODE *tree_root;
pthread_mutex_t root_lock;

// Various Counters
int add_counter=0, del_counter=0, bal_counter=0;
int add_attempts=0, del_attempts=0;
int p_finish=0;




// functions used
void parse_args(int argc, char *argv[], int *no_adds, int *seed, int *quiet);	//takes in command line arguments

void add_value(int new_val);							// adds a specified value to the tree (-1 for random)
void find_gap(NODE **start, NODE **new, int dir);				// finds a place to put new in the direction of dir from start
void delete_value(int del_val);							// deletes a specified value from the tree (-1 for random)

int find_height(NODE **tree);							// finds the height of the tree
int rebalance(NODE **tree, NODE **parent, int direction);			// recursive function to rebalance the tree at a given node with a given parent
void rebalance_tree();								// calls the rebalance function with the correct arguments for a given tree

void delete_tree(NODE **tree);							// deletes a tree and all its allocated memory is freed

void *p_add(void *arg);								// pthreads function to add a specified number of values in poisson intervals
void *p_del();									// pthreads function to delete a specified number of values in poisson intervals
void *p_bal();									// pthreads function to rebalance the tree periodically

int poisson_gen(double lambda);							// function to generate poisson random variables 

// Printing was implemented for debugging purposes(tree will most likely be too large to print by current default)
int find_height_print(NODE *tree);						// find height function without locks
void print_gap(int a);								// function to print "a" number of gaps
void print_line(NODE *tree, int start, int inc, int num);			// prints a line in the tree with specified gaps
void print_tree(NODE *tree);							// prints out the tree (if under a certain height)



int main(int argc, char *argv[]){


	//set default arguments
	int no_adds=1000;
	int seed=time(NULL);
	parse_args(argc, argv, &no_adds, &seed, &quiet);

	// seeds program
	printf("Seed is %d\n",seed);
	srand(seed);
	srand48(seed-101);

	// pthreads arguments
	pthread_t *handles;
	pthread_mutex_init(&root_lock,NULL);
	int num_threads=3;
		
	handles=malloc(num_threads*sizeof(pthread_t));


	// sets up tree
	tree_root=NULL;

	// runs 3 threads for adding, deleting and balancing
	pthread_create(&handles[0],NULL,p_add, (void *)&no_adds);
	pthread_create(&handles[1],NULL,p_del, NULL);
	pthread_create(&handles[2],NULL,p_bal, NULL);
	
	// waits for all threads to finish
	int i;
	for(i=0;i<num_threads;i++){
		pthread_join(handles[i],NULL);
	}
	free(handles);

	print_tree(tree_root);		// prints tree
	delete_tree(&tree_root);	// deletes from memory

	
	// prints out some stats
	printf("\n\nAdds:\t\t%d (%d attempts)\nDeletes:\t%d (%d attempts)\nBalances:\t%d\n",add_counter,add_attempts,del_counter,del_attempts,bal_counter);
	return 0;
}


void parse_args(int argc, char *argv[], int *no_adds, int *seed, int *quiet){
	//parse command line arguments
	int opt;
	while((opt=getopt(argc,argv,"n:s:q"))!=-1){
		switch(opt){
			case 'n':
				*no_adds=atoi(optarg);
				break;
			case 's':
				*seed=atoi(optarg);
				break;
			case 'q':
				*quiet=1;
				break;
			default:
				fprintf(stderr,"Usage: %s [-nsq]\n",argv[0]);
				exit(EXIT_FAILURE);
		}
	}
	return;
}


// adds a specified value to the tree (-1 for random)
void add_value(int new_val){
	if(new_val==-1){
		new_val=rand()%max;
	}
	// allocates memory for a new node and sets up values
	NODE *new_node;
	new_node=(NODE *)malloc(sizeof(NODE));
	new_node->val=new_val;
	new_node->left=NULL;
	new_node->right=NULL;
	pthread_mutex_init(&(new_node->lock),NULL);


	NODE *parent, *child;
	int stop=0;

	// locks the root lock (to find current root)
	pthread_mutex_lock(&root_lock);	

	// if its the root node it sets the pointer to our new node and sets break condition
	if(tree_root==NULL){
		tree_root=new_node;
		pthread_mutex_unlock(&root_lock);
		stop=1;
	}
	// otherwise sets parent as the current root and locks it, and unlocks root lock
	else{
		parent=tree_root;
		pthread_mutex_lock(&(parent->lock));
		pthread_mutex_unlock(&root_lock);
	}



	// loops through until stop is set to 1
	while(stop==0){

		// if the new value is less than the current value at parent go left
		if(new_val<parent->val){
			// checks if the node to the left is set (sets it as new node if so and breaks out)
			if(parent->left==NULL){
				parent->left=new_node;
				pthread_mutex_unlock(&(parent->lock));
				stop=1;
			}
			// otherwise moves pointer forwards (unlocks parent and locks next node)
			else{
				child=parent->left;
				pthread_mutex_lock(&(child->lock));
				pthread_mutex_unlock(&(parent->lock));
				parent=child;
			}
		}
		// if the new value is less than the current value at parent go right
		else if(new_val>parent->val){
			// checks if the node to the right is set (sets it as new node if so and breaks out)
			if(parent->right==NULL){
				parent->right=new_node;
				pthread_mutex_unlock(&(parent->lock));
				stop=1;
			}
			// otherwise moves pointer forwards (unlocks parent and locks next node)
			else{
				child=parent->right;
				pthread_mutex_lock(&(child->lock));
				pthread_mutex_unlock(&(parent->lock));
				parent=child;
			}
		}
		// else the value is already in the tree so free up the node and break out
		else{
			pthread_mutex_unlock(&(parent->lock));
			free(new_node);
			return;
		}
	}
	// prints out info unless quiet and updates add counter
	if(quiet==0){printf("Added %0*d\n",gap,new_val);}
	add_counter++;
	return;
}

// finds a place to put new in the direction of dir from start
void find_gap(NODE **start, NODE **new, int dir){
	NODE *parent=*start;
	NODE *child;

	// if we're going in left direction
	if(dir==0){
		// loops through looking for an empty spot to place new
		while(1){
			// if the leftside is empty it places new and unlocks the nodes
			if(parent->left==NULL){
				parent->left=*new;
				pthread_mutex_unlock(&(parent->lock));
				pthread_mutex_unlock(&((*new)->lock));
				return;
			}
			// else it moves forwards and locks/unlocks
			else{
				child=parent->left;
				pthread_mutex_lock(&(child->lock));
				pthread_mutex_unlock(&(parent->lock));
				parent=child;
			}
		}
	}
	if(dir==1){
		// loops through looking for an empty spot to place new
		while(1){
			// if the rightside is empty it places new and unlocks the nodes
			if(parent->right==NULL){
				parent->right=*new;
				pthread_mutex_unlock(&(parent->lock));
				pthread_mutex_unlock(&((*new)->lock));
				return;
			}
			// else it moves forwards and locks/unlocks
			else{
				child=parent->right;
				pthread_mutex_lock(&(child->lock));
				pthread_mutex_unlock(&(parent->lock));
				parent=child;
			}
		}
	}
	return;
}


// deletes a specified value from the tree (-1 for random)
void delete_value(int del_val){
	// randomises delete value if requested
	if(del_val==-1){
		del_val=rand()%max;
	}

	NODE *parent, *deletee;

	// locks the root lock
	pthread_mutex_lock(&root_lock);
	// if the tree is empty, there's nothing to delete so return
	if(tree_root==NULL){
		pthread_mutex_unlock(&root_lock);
		return;
	}
	// else set parent as current root and lock it
	else{
		parent=tree_root;
		pthread_mutex_lock(&(parent->lock));
	}


	int del_l=0, del_r=0, del_root=0;
	int stop=0,counter=0;

	// loops through looking for the value
	while(stop==0){
		// goes left if the value is less than the current value
		if(del_val<parent->val){
			// if the node on the left isn't set, then break
			if(parent->left==NULL){
				pthread_mutex_unlock(&(parent->lock));
				stop=1;
			}	
			// if the node on the left is the delete value, lock the deletee and break
			else if(parent->left->val==del_val){
				deletee=parent->left;
				pthread_mutex_lock(&(deletee->lock));
				del_l=1;
				stop=1;
			}	
			// else move the pointer forward and lock the next node and unlock previous
			else{
				deletee=parent->left;
				pthread_mutex_lock(&(deletee->lock));
				pthread_mutex_unlock(&(parent->lock));
				parent=deletee;
			}
				
		}
		// otherwise goes right
		else if(del_val>parent->val){
			// if the node on the right isn't set, then break
			if(parent->right==NULL){
				pthread_mutex_unlock(&(parent->lock));
				stop=1;
			}	
			// if the node on the right is the delete value, lock the deletee and break
			else if(parent->right->val==del_val){
				deletee=parent->right;
				pthread_mutex_lock(&(deletee->lock));
				del_r=1;
				stop=1;
			}	
			// else move the pointer forward and lock the next node and unlock previous
			else{
				deletee=parent->right;
				pthread_mutex_lock(&(deletee->lock));
				pthread_mutex_unlock(&(parent->lock));
				parent=deletee;
			}
		}
		// else (will only be called if the root node is the deleted value)
		else{
			del_root=1;
			stop=1;
		}

		// if we aren't deleting the root then we can unlock root_lock
		if(counter==0 && del_root==0){
			pthread_mutex_unlock(&root_lock);
		}
		counter++;	// counter ensures we only unlock root lock once
	}
			


	// if the value to be deleted is to the left of the parent
	if(del_l==1){
		// if the value to the left of the deletee is not empty
		if(deletee->left!=NULL){
			pthread_mutex_lock(&(deletee->left->lock));		// lock deletee's left
			parent->left=deletee->left;				// set the parent's left to point to the deletee's left

			// if deletee's right is also not empty
			if(deletee->right!=NULL){
				pthread_mutex_lock(&(deletee->right->lock));	// lock deletee's right
				find_gap(&(parent->left),&(deletee->right),1);	// and try and place it to the right of deletee's left (unlocks both)
			}
			else{
				pthread_mutex_unlock(&(deletee->left->lock));	// else unlock deletee's left
			}
		}
		// else if the value to the right is not empty (but the left is)
		else if(deletee->right!=NULL){
			parent->left=deletee->right;	// can set parent's left to deletee's right as deletee's left is empty
		}
		// else if both children are empty
		else{
			parent->left=NULL;		// set parent's left to NULL 
		}

		// unlock the nodes and free deletee
		pthread_mutex_unlock(&(deletee->lock));
		pthread_mutex_unlock(&(parent->lock));
		free(deletee);			
	}
	// else if it is to the right of the parent (SIMILAR TO del_l)
	else if(del_r==1){
		if(deletee->left!=NULL){
			pthread_mutex_lock(&(deletee->left->lock));
			parent->right=deletee->left;
			if(deletee->right!=NULL){
				pthread_mutex_lock(&(deletee->right->lock));
				find_gap(&(parent->right),&(deletee->right),1);
			}
			else{
				pthread_mutex_unlock(&(deletee->left->lock));
			}
		}
		else if(deletee->right!=NULL){
			parent->right=deletee->right;
		}
		else{
			parent->right=NULL;
		}
		pthread_mutex_unlock(&(deletee->lock));
		pthread_mutex_unlock(&(parent->lock));
		free(deletee);
	}
	// else if the root is to be deleted
	else if(del_root==1){
		// checks if the root's left is set
		if(parent->left!=NULL){		
			pthread_mutex_lock(&(parent->left->lock));		// locks roots left
			tree_root=parent->left;					// points the root the old root's left
			if(parent->right!=NULL){				// checks if old root's right is also non empty
				pthread_mutex_lock(&(parent->right->lock));	// locks old root's right if so
				find_gap(&(parent->left),&(parent->right),1);	// finds a spot for it to the right of the new root
			}
			else{
				pthread_mutex_unlock(&(parent->left->lock));	// else unlocks parent's left
			}
		}
		// else checks if the root's right is set (left isn't)
		else if(parent->right!=NULL){		
			tree_root=parent->right;			// points the root to the old root's right (as root's left is empty)
		}
		else{
			tree_root=NULL;					// else points the root to NULL
		}

		pthread_mutex_unlock(&(parent->lock));		// unlocks the node
		pthread_mutex_unlock(&root_lock);		// unlocks the root lock
		free(parent);					// frees the old root
	}

	// If a node was deleted then update counter and print info if requested
	if(del_l+del_r+del_root>0){
		del_counter++;
		if(quiet==0){printf("Deleted %0*d\n",gap,del_val);}
	}
	return;
}



// recursive function to find the height of the tree
// input's parent should be locked
int find_height(NODE **tree){
	if(*tree==NULL){return 0;}		// if the input is NULL return 0
	int left, right;
	
	pthread_mutex_lock(&((*tree)->lock));	// locks the input

	left=find_height(&((*tree)->left));	// check left node's height
	right=find_height(&((*tree)->right));	// and rights

	pthread_mutex_unlock(&((*tree)->lock));	// unlocks

	if(left>right){return left+1;}		// if left is bigger return left's height plus one for the current node
	return right+1;				// else return right's height plus one
}


// function to recursively rebalance a tree from a given parent node and direction from which the tree comes from 
int rebalance(NODE **tree, NODE **parent, int direction){
	int counter=0;					// sets up a counter for amount of rebalances required
	int l=0, r=0;

	// if the tree isn't empty find the heights at each side
	if((*tree)!=NULL){
		l=find_height(&(*tree)->left);		// finds the height to the left
		r=find_height(&(*tree)->right);		// and to the right
	}
	// otherwise it doesn't need rebalancing 
	else{
		return 0;
	}

	NODE *ptemp=*parent, *ctemp=*tree;		// sets up temps for parent and child

	// parent and child equal it means we are at the root
	if(ptemp==ctemp){
		// keeps rebalancing while the difference in sizes is bigger than 1 (AVL tree)
		while(abs(l-r)>1){
			// loops while the right side is bigger
			while(r-l>1){
				tree_root=ptemp->right;					// sets the root to the old root's right
				ptemp->right=NULL;					// sets the old roots right to NULL
				find_gap(&tree_root,&ptemp,0);				// slots old root to left of new root

		
				ptemp=tree_root;					// updates ptemp to new parent
				pthread_mutex_lock(&(ptemp->lock));			// relocks it

				// updates l, r and counter
				l=find_height(&(ptemp->left));
				r=find_height(&(ptemp->right));
				counter++;	
			}
			// loops while the left side is bigger
			while(l-r>1){
				tree_root=ptemp->left;					// sets the root to the old root's left
				ptemp->left=NULL;					// sets the old roots left to NULL
				find_gap(&tree_root,&ptemp,1);				// slots old root to left of new root

		
				ptemp=tree_root;					// updates ptemp to new parent
				pthread_mutex_lock(&(ptemp->lock));			// relocks

				l=find_height(&(ptemp->left));
				r=find_height(&(ptemp->right));
				counter++;						// increments counter
			}
		}
		// checks if both sides are non empty and rebalances from there if not
		if(ptemp->left!=NULL){
			pthread_mutex_lock(&(ptemp->left->lock));
			counter+=rebalance(&(ptemp->left),&ptemp,0);
		}
		if(ptemp->right!=NULL){
			pthread_mutex_lock(&(ptemp->right->lock));
			counter+=rebalance(&(ptemp->right),&ptemp,1);
		}
		
		// unlocks the root and root lock
		pthread_mutex_unlock(&(ptemp->lock));
		pthread_mutex_unlock(&root_lock);
	}

	// otherwise we are at a non root node
	else{
		// loops through while the gap is bigger than one
		while(abs(l-r)>1){
			// loops while the right side is bigger
			while((r-l)>1){
				// if the direction from the parent is left
				if(direction==0){
					pthread_mutex_lock(&(ctemp->right->lock));
					ptemp->left=ctemp->right;		// sets parent's left to child's right
					ctemp->right=NULL;			// set child's right to NULL
					find_gap(&(ptemp->left),&ctemp,0);	// slots in child to left of parent's left

					ctemp=ptemp->left;			// update ctemp
					pthread_mutex_lock(&(ctemp->lock));	// relock

				}	

				// else if the direction from parent is right
				else{
					pthread_mutex_lock(&(ctemp->right->lock));
					ptemp->right=ctemp->right;		// sets parent's right to child's right
					ctemp->right=NULL;			// set child's right to NULL
					find_gap(&(ptemp->right),&ctemp,0);	// slots in child to left of parent's right

					ctemp=ptemp->right;			// update ctemp
					pthread_mutex_lock(&(ctemp->lock));	// relock

				}

				// updates l,r and counter
				l=find_height(&(ctemp->left));
				r=find_height(&(ctemp->right));

				counter++;	
			}

				
			// loops while the left side is bigger
			while((l-r)>1){						// SIMILAR to ABOVE
				// if the direction from the parent is left
				if(direction==0){
					pthread_mutex_lock(&(ctemp->left->lock));
					ptemp->left=ctemp->left;		// sets parent's left to child's left
					ctemp->left=NULL;			// set child's right to NULL
					find_gap(&(ptemp->left),&ctemp,1);	// slots in child to right of parent's left

					ctemp=ptemp->left;			// update ctemp
					pthread_mutex_lock(&(ctemp->lock));	// relock

				}	

				// else if the direction from parent is right
				else{
					pthread_mutex_lock(&(ctemp->left->lock));
					ptemp->right=ctemp->left;		// sets parent's right to child's left
					ctemp->left=NULL;			// set child's right to NULL
					find_gap(&(ptemp->right),&ctemp,1);	// slots in child to right of parent's right

					ctemp=ptemp->right;			// update ctemp
					pthread_mutex_lock(&(ctemp->lock));	// relock

				}

				l=find_height(&(ctemp->left));
				r=find_height(&(ctemp->right));

				counter++;			// increments counter
			}
		}
		
		// checks if left and right are non empty and rebalances from them if not
		if(ctemp->left!=NULL){
			pthread_mutex_lock(&(ctemp->left->lock));
			counter+=rebalance(&(ctemp->left),&ctemp,0);
		}
		if(ctemp->right!=NULL){
			pthread_mutex_lock(&(ctemp->right->lock));
			counter+=rebalance(&(ctemp->right),&ctemp,1);
		}
		
		// unlocks child node once it is passed
		pthread_mutex_unlock(&(ctemp->lock));
	}

	return counter;		// return how many rebalances have taken place
}

// calls the rebalance function until no rebalances necessary
void rebalance_tree(){
	int n=1;
	// loops through with the root arguments until zero rebalances were necessary
	while(n>0){
		pthread_mutex_lock(&root_lock);
		if(tree_root!=NULL){
			pthread_mutex_lock(&(tree_root->lock));
			n=rebalance(&tree_root,&tree_root,0);
		}
		else{
			pthread_mutex_unlock(&root_lock);
			n=0;
		}
	}
}

// deletes a tree and all its allocated memory is freed
void delete_tree(NODE **tree){
	// if the input isn't NULL
	if((*tree)!=NULL){
		delete_tree(&((*tree)->left));	// free from the left
		delete_tree(&((*tree)->right));	// and right
		free(*tree);			// then free the node
	}
}

 // pthreads function to add a specified number of values in poisson intervals
void *p_add(void *arg){
	int *no_adds = (int *)arg;
	int i;
	// loops a specified number of times
	for(i=0;i<(*no_adds);i++){
		usleep(50*poisson_gen(2));
		add_value(-1);
		add_attempts++;
	}
	p_finish++;	// updates p_finish to tell other threads to finish
	return NULL;
}
// pthreads function to delete values in poisson intervals
void *p_del(){
	// loops until p_add is finished
	while(p_finish==0){
		usleep(50*poisson_gen(2));
		delete_value(-1);
		del_attempts++;
	}
	return NULL;
}

// pthreads function to rebalance the tree periodically
void *p_bal(){
	// loops until p_add is finished
	while(p_finish==0){
		usleep(100*poisson_gen(20));
		rebalance_tree();
		if(quiet==0){printf("\t\tBalanced\n");}
		bal_counter++;
	}
	rebalance_tree();
	return NULL;
}

// function to generate poisson random variables 
int poisson_gen(double lambda){
	int k=0;
	double p=exp(-lambda);		//initialises p
	double F=p;			// sets F to p
	double rnum=drand48();		// generates a random number

	// loops until it finds the right value of k
	while(rnum>F){			
		p*=lambda/(k+1);	// calculates p
		F+=p;			// bumps up F
		k++;			// increments k counter
	}
	return k;
}

// finds height without locks
int find_height_print(NODE *tree){
	if(tree==NULL){return 0;}		// if the input is NULL return 0
	int left, right;

	left=find_height_print(tree->left);	// check left node's height
	right=find_height_print(tree->right);	// and rights

	if(left>right){return left+1;}		// if left is bigger return left's height plus one for the current node
	return right+1;				// else return right's height plus one
}
	
// function to print "a" number of gaps
void print_gap(int a){
	int i;
	for(i=0;i<a*gap;i++){printf(" ");}
}

// prints a line in the tree with specified gaps
void print_line(NODE *tree, int start, int inc, int num){
	int j,k;
	NODE *temp;
	print_gap(start);	// prints start number of gaps

	int val;

	// loops through how many values are on the line
	for(j=0;j<num;j++){
		val=1;		// resets val
		temp=tree;	// sets temp to start of tree
		
		// it takes num/2 steps through the tree to get to the level with num values (won't enter loop if root)
		for(k=num/2;k>=1;k/=2){
			// uses j as binary representation of directions it travels (0 for left, 1 for right)
			// if direction is left
			if((k&j)==0){
				if(temp->left==NULL){val=0;k=0;}	// sets val to 0 if left is an empty NODE and breaks by setting k=0
				else{temp=temp->left;}			// else moves along
			}
			// else direction is right
			else{
				if(temp->right==NULL){val=0;k=0;}	// SIMILAR to LEFT
				else{temp=temp->right;}
			}
		}
		if(val==1){printf("%0*d",gap,temp->val);}			// if val is still 1 then it prints the value
		else{printf("%s",empty);}					// else prints ~~
		print_gap(inc);						// prints the incremental gap
	}
	printf("\n\n");							// and two line breaks for readability
}
			
// prints out the tree (if under a certain height)
void print_tree(NODE *tree){
	if( tree == NULL){
		printf("Tree is empty, can't print\n");
		return;
	}

	int height=find_height_print(tree);	// finds height of tree
	// only prints if 6 or lower height
	if(height>6){
		printf("Tree too large to print\n");
		return;
	}
	int i;
	// loops through the layers (setting values to print_line accordingly)
	for(i=height;i>0;i--){
		print_line(tree,pow(2,i-1),pow(2,i)-1,pow(2,height-i));
	}		
		return;
	
}
