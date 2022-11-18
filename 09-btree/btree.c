#include <solution.h>
#include <stdlib.h>

struct node
{
	int num; //количество ключей
	int *keys; //ключи, упорядоченные (n)
	bool is_leaf; //является ли вершиной 
	struct node **doughters; //Указатели на дочерние узлы (n+1)
};

struct btree{
	struct node *root;
	int L;
};

struct node* node_alloc(unsigned int L)
{
	struct node *node = (struct node *)malloc(sizeof(struct node));
	node->keys = (int*)malloc(2 * L * sizeof(int));
	node->doughters = (struct node**)malloc((2*L+1) * sizeof(struct node*));
	return node;
}

struct btree* btree_alloc(unsigned int L)
{
	struct btree *tree = (struct btree *)malloc(sizeof(struct btree));
	struct node *root = node_alloc(L);
	root->is_leaf = true;
	root->num = 0;

	tree->root = root;
	tree->L = L;
	return tree;
}

void btree_free(struct btree *t)
{
	(void) t;
}

void btree_insert(struct btree *t, int x)
{
	//mother_node->doughters[id] == full_node
	void btree_split_child(struct node *mother_node, int id, struct node *full_node){
		int L = t->L;
		struct node *new_node = node_alloc(L);
		new_node->is_leaf = full_node->is_leaf;
		new_node->num = L - 1;
		for(int i = 0; i < L-1; i++){
			new_node->keys[i] = full_node->keys[i+L];
		}
		if(!full_node->is_leaf){
			for(int i = 0; i < L; i++){
				new_node->doughters[i] = full_node->doughters[i+L];
			}
		}
		full_node->num = L - 1;
		for(int i = mother_node->num + 1; i < id + 1; i--){
			mother_node->doughters[i+1] = mother_node->doughters[i];
		}
		mother_node->doughters[id+1] = new_node;
		for(int i = mother_node->num; i < id; i--){
			mother_node->keys[i+1] = mother_node->keys[i];
		}
		mother_node->keys[id] = full_node->keys[L];
		mother_node->num += 1;
	}

	void btree_insert_nonfull(struct node *node, int x){
		int i = node->num - 1;
		if(node->is_leaf){
			while(i >= 0 && x < node->keys[i]){
				node->keys[i+1] = node->keys[i];
				i -= 1;
			}
			node->keys[i+1] = x;
			node->num += 1;
		} else {
			while(i >= 0 && x < node->keys[i]){
				i -= 1;
			}
			i += 1;
			if (node->doughters[i]->num == 2 * t->L - 1){
				btree_split_child(node, i, node->doughters[i]);
				if(x > node->keys[i]){
					i += 1;
				}
			}
			btree_insert_nonfull(node->doughters[i], x);
		}
	}
	struct node *root = t->root;
	if(root->num == 2 * t->L - 1){
		struct node *new_root = node_alloc(t->L);
		t->root = new_root;
		new_root->is_leaf = false;
		new_root->num = 0;
		new_root->doughters[0] = root;
		btree_split_child(new_root, 0, root);
		btree_insert_nonfull(new_root, x);
	} else{
		btree_insert_nonfull(root, x);
	}
}

void btree_delete(struct btree *t, int x)
{
	// btree_delete_from_node(struct node *node, int x){
	// 	int i = 0;
	// 	while(i < node->num && x > n->keys[i]){
	// 		i += 1;
	// 	}
	// 	if(i < node->num && x == node->keys[i]){
	// 		if (node->is_leaf){
	// 			remove(node, x);
	// 		}
	// 		else{
				
	// 		}
	// 	}
	// }
}

bool btree_contains(struct btree *t, int x)
{
	bool btree_contains_in_node(struct node *n, int x){
		int i = 0;
		while(i < n->num && x > n->keys[i]){
			i += 1;
		}
		if(i < n->num && x == n->keys[i]){
			return true;
		}
		if(n->is_leaf){
			return false;
		}
		return btree_contains_in_node(n->doughters[i], x);
	}
	return btree_contains_in_node(t->root, x);
}

struct btree_iter
{

};

struct btree_iter* btree_iter_start(struct btree *t)
{
}

void btree_iter_end(struct btree_iter *i)
{
	
}

bool btree_iter_next(struct btree_iter *i, int *x)
{
	(void) i;
	(void) x;

	return false;
}
