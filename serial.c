#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

// set up node structure
typedef struct node{
	int val;		// node's value
	struct node *left;	// child pointers
	struct node *right;
}NODE;


// Global Args
int max=1000000;									// set as max number possible in tree
int gap=3;									// set as digits for max number (max-1)
char* empty="~~~";								// set as empty node print symbol (use gap number of characters) 
int quiet=0;									// variable to choose if add/del info is printed or not

// tree
NODE *tree_root;

// Various Counters
int add_counter=0, del_counter=0, bal_counter=0;


// functions used
void parse_args(int argc, char *argv[], int *no_adds, int *seed, int *quiet);	//takes in command line arguments

void add_value(int new_val);							// adds a specified value to the tree (-1 for random)
void find_gap(NODE **start, NODE **new, int dir);				// finds a place to put new in the direction of dir from start
void delete_value(int del_val);							// deletes a specified value from the tree (-1 for random)

int find_height(NODE *tree);							// finds the height of the tree
int rebalance(NODE **tree, NODE **parent, int direction);			// recursive function to rebalance the tree at a given node with a given parent
void rebalance_tree();								// calls the rebalance function with the correct arguments for a given tree

void delete_tree(NODE **tree);							// deletes a tree and all its allocated memory is freed

// Printing was implemented for debugging purposes(tree will most likely be too large to print by current default)
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

	// sets up tree
	tree_root=NULL;

	int i;
	for(i=0;i<no_adds;i++){
		add_value(-1);
	//	delete_value(-1);
		rebalance_tree();
		if(quiet==0){printf("\t\tBalanced\n");}
	}


	print_tree(tree_root);		// prints tree
	delete_tree(&tree_root);	// deletes from memory

	
	// prints out some stats
	printf("\n\nAdds:\t\t%d (%d attempts)\nDeletes:\t%d (%d attempts)\nBalances:\t%d\n",add_counter,no_adds,del_counter,no_adds,no_adds);
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


	NODE *parent;
	int stop=0;

	// if its the root node it sets the pointer to our new node and sets break condition
	if(tree_root==NULL){
		tree_root=new_node;
		stop=1;
	}
	// otherwise sets parent as the root
	else{
		parent=tree_root;
	}



	// loops through until stop is set to 1
	while(stop==0){

		// if the new value is less than the current value at parent go left
		if(new_val<parent->val){
			// checks if the node to the left is set (sets it as new node if so and breaks out)
			if(parent->left==NULL){
				parent->left=new_node;
				stop=1;
			}
			// otherwise moves pointer forwards
			else{
				parent=parent->left;
			}
		}
		// if the new value is less than the current value at parent go right
		else if(new_val>parent->val){
			// checks if the node to the right is set (sets it as new node if so and breaks out)
			if(parent->right==NULL){
				parent->right=new_node;
				stop=1;
			}
			// otherwise moves pointer forwards
			else{
				parent=parent->right;
			}
		}
		// else the value is already in the tree so free up the node and break out
		else{
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

	// if we're going in left direction
	if(dir==0){
		// loops through looking for an empty spot to place new
		while(1){
			// if the leftside is empty it places new
			if(parent->left==NULL){
				parent->left=*new;
				return;
			}
			// else it moves forwards
			else{
				parent=parent->left;
			}
		}
	}
	if(dir==1){
		// loops through looking for an empty spot to place new
		while(1){
			// if the rightside is empty it places new
			if(parent->right==NULL){
				parent->right=*new;
				return;
			}
			// else it moves forwards
			else{
				parent=parent->right;
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

	// if the tree is empty, there's nothing to delete so return
	if(tree_root==NULL){
		return;
	}
	// else set parent as root
	else{
		parent=tree_root;
	}


	int del_l=0, del_r=0, del_root=0;
	int stop=0;

	// loops through looking for the value
	while(stop==0){
		// goes left if the value is less than the current value
		if(del_val<parent->val){
			// if the node on the left isn't set, then break
			if(parent->left==NULL){
				stop=1;
			}	
			// if the node on the left is the delete value, break
			else if(parent->left->val==del_val){
				deletee=parent->left;
				del_l=1;
				stop=1;
			}	
			// else move the pointer forward
			else{
				parent=parent->left;
			}
				
		}
		// otherwise goes right
		else if(del_val>parent->val){
			// if the node on the right isn't set, then break
			if(parent->right==NULL){
				stop=1;
			}	
			// if the node on the right is the delete value, break
			else if(parent->right->val==del_val){
				deletee=parent->right;
				del_r=1;
				stop=1;
			}	
			// else move the pointer forward 
			else{
				parent=parent->right;
			}
		}
		// else (will only be called if the root node is the deleted value)
		else{
			del_root=1;
			stop=1;
		}
	}
			


	// if the value to be deleted is to the left of the parent
	if(del_l==1){
		// if the value to the left of the deletee is not empty
		if(deletee->left!=NULL){
			parent->left=deletee->left;				// set the parent's left to point to the deletee's left
			// if deletee's right is also not empty
			if(deletee->right!=NULL){
				find_gap(&(parent->left),&(deletee->right),1);	// and try and place it to the right of deletee's left 
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

		// frees deletee
		free(deletee);			
	}
	// else if it is to the right of the parent (SIMILAR TO del_l)
	else if(del_r==1){
		if(deletee->left!=NULL){
			parent->right=deletee->left;
			if(deletee->right!=NULL){
				find_gap(&(parent->right),&(deletee->right),1);
			}
		}
		else if(deletee->right!=NULL){
			parent->right=deletee->right;
		}
		else{
			parent->right=NULL;
		}
		free(deletee);
	}
	// else if the root is to be deleted
	else if(del_root==1){
		// checks if the root's left is set
		if(parent->left!=NULL){		
			tree_root=parent->left;					// points the root the old root's left
			if(parent->right!=NULL){				// checks if old root's right is also non empty
				find_gap(&(parent->left),&(parent->right),1);	// finds a spot for it to the right of the new root
			}
		}
		// else checks if the root's right is set (left isn't)
		else if(parent->right!=NULL){		
			tree_root=parent->right;			// points the root to the old root's right (as root's left is empty)
		}
		else{
			tree_root=NULL;					// else points the root to NULL
		}

		free(parent);					// frees the old root
	}

	// If a node was deleted then update counter and print info if requested
	if(del_l+del_r+del_root>0){
		del_counter++;
		if(quiet==0){printf("Deleted %0*d\n",gap,del_val);}
	}
	return;
}


// finds height of tree recursively
int find_height(NODE *tree){
	if(tree==NULL){return 0;}		// if the input is NULL return 0
	int left, right;

	left=find_height(tree->left);	// check left node's height
	right=find_height(tree->right);	// and rights

	if(left>right){return left+1;}		// if left is bigger return left's height plus one for the current node
	return right+1;				// else return right's height plus one
}


// function to recursively rebalance a tree from a given parent node and direction from which the tree comes from 
int rebalance(NODE **tree, NODE **parent, int direction){
	int counter=0;					// sets up a counter for amount of rebalances required
	int l=0, r=0;

	// if the tree isn't empty find the heights at each side
	if((*tree)!=NULL){
		l=find_height((*tree)->left);		// finds the height to the left
		r=find_height((*tree)->right);		// and to the right
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

				// updates l, r and counter
				l=find_height(ptemp->left);
				r=find_height(ptemp->right);
				counter++;	
			}
			// loops while the left side is bigger
			while(l-r>1){
				tree_root=ptemp->left;					// sets the root to the old root's left
				ptemp->left=NULL;					// sets the old roots left to NULL
				find_gap(&tree_root,&ptemp,1);				// slots old root to left of new root

		
				ptemp=tree_root;					// updates ptemp to new parent

				l=find_height(ptemp->left);
				r=find_height(ptemp->right);
				counter++;						// increments counter
			}
		}
		ctemp=ptemp; 	// updates ctemp for recursive call at end of function
	}

	// otherwise we are at a non root node
	else{
		// loops through while the gap is bigger than one
		while(abs(l-r)>1){
			// loops while the right side is bigger
			while((r-l)>1){
				// if the direction from the parent is left
				if(direction==0){
					ptemp->left=ctemp->right;		// sets parent's left to child's right
					ctemp->right=NULL;			// set child's right to NULL
					find_gap(&(ptemp->left),&ctemp,0);	// slots in child to left of parent's left

					ctemp=ptemp->left;			// update ctemp

				}	

				// else if the direction from parent is right
				else{
					ptemp->right=ctemp->right;		// sets parent's right to child's right
					ctemp->right=NULL;			// set child's right to NULL
					find_gap(&(ptemp->right),&ctemp,0);	// slots in child to left of parent's right

					ctemp=ptemp->right;			// update ctemp

				}

				// updates l,r and counter
				l=find_height(ctemp->left);
				r=find_height(ctemp->right);

				counter++;	
			}

				
			// loops while the left side is bigger
			while((l-r)>1){						// SIMILAR to ABOVE
				// if the direction from the parent is left
				if(direction==0){
					ptemp->left=ctemp->left;		// sets parent's left to child's left
					ctemp->left=NULL;			// set child's right to NULL
					find_gap(&(ptemp->left),&ctemp,1);	// slots in child to right of parent's left

					ctemp=ptemp->left;			// update ctemp

				}	

				// else if the direction from parent is right
				else{
					ptemp->right=ctemp->left;		// sets parent's right to child's left
					ctemp->left=NULL;			// set child's right to NULL
					find_gap(&(ptemp->right),&ctemp,1);	// slots in child to right of parent's right

					ctemp=ptemp->right;			// update ctemp

				}

				l=find_height(ctemp->left);
				r=find_height(ctemp->right);

				counter++;			// increments counter
			}
		}
		
	}
	// checks if left and right are non empty and rebalances from them if not
	if(ctemp->left!=NULL){
		counter+=rebalance(&(ctemp->left),&ctemp,0);
	}
	if(ctemp->right!=NULL){
		counter+=rebalance(&(ctemp->right),&ctemp,1);
	}

	return counter;		// return how many rebalances have taken place
}

// calls the rebalance function until no rebalances necessary
void rebalance_tree(){
	// loops through with the root arguments until zero rebalances were necessary
	while(rebalance(&tree_root,&tree_root,0)>0){}
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

	int height=find_height(tree);	// finds height of tree
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
