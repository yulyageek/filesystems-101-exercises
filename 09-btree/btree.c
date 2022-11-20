#include "solution.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct node
{
	int num; //количество ключей
	int *keys; //ключи, упорядоченные (n)
	bool is_leaf; //является ли вершиной 
	struct node **children; //Указатели на дочерние узлы (n+1)
};

struct btree{
	struct node *root;
	int L;
	int count;
};

struct node* node_alloc(unsigned int L)
{
	struct node *node = (struct node *)malloc(sizeof(struct node));
	node->keys = (int*)calloc(2 * L, sizeof(int));
	node->children = (struct node**)calloc( 2*L+1, sizeof(struct node*));
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
	tree->count = 0;
	return tree;
}

void btree_free(struct btree *t)
{
	void node_free(struct node *n){
		if(!n->is_leaf){
			for(int i=0; i<=n->num; i++){
				node_free(n->children[i]);
			}
		}
		free(n->keys);
		free(n->children);
		free(n);
	}
	node_free(t->root);
	free(t);
}

void btree_insert(struct btree *t, int x)
{
	//mother_node->children[id] == full_node
	void btree_split_children(struct node *mother_node, int id, struct node *full_node){
		int L = t->L;
		struct node *new_node = node_alloc(L);
		new_node->is_leaf = full_node->is_leaf;
		new_node->num = L - 1;
		for(int i = 0; i < L-1; i++){
			new_node->keys[i] = full_node->keys[i+L];
		}
		if(!full_node->is_leaf){
			for(int i = 0; i < L; i++){
				new_node->children[i] = full_node->children[i+L];
			}
		}
		full_node->num = L - 1;
		for(int i = mother_node->num; i > id; i--){
			mother_node->children[i+1] = mother_node->children[i];
		}
 		mother_node->children[id+1] = new_node;
		for(int i = mother_node->num-1; i >= id; i--){
			mother_node->keys[i+1] = mother_node->keys[i];
		}
		mother_node->keys[id] = full_node->keys[L-1];
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
			if (node->children[i]->num == 2 * t->L - 1){
				btree_split_children(node, i, node->children[i]);
				if(x > node->keys[i]){
					i += 1;
				}
			}
			btree_insert_nonfull(node->children[i], x);
		}
	}
	if(btree_contains(t, x)){
		return;
	}
	struct node *root = t->root;
	if(root->num == 2 * t->L - 1){
		struct node *new_root = node_alloc(t->L);
		t->root = new_root;
		new_root->is_leaf = false;
		new_root->num = 0;
		new_root->children[0] = root;
		btree_split_children(new_root, 0, root);
		btree_insert_nonfull(new_root, x);
	} else{
		btree_insert_nonfull(root, x);
	}
	t->count += 1;
}

void print_tree(struct btree *t){
	void print_node(struct node *n, char *layers){
		printf("%s", layers);
		for(int i=0; i<n->num; i++){
			printf("%d ", n->keys[i]);
		}
		printf("\n");
		if(!n->is_leaf){
			size_t len = strlen(layers);
			layers[len] = '\t';
			layers[len+1] = '\0';
			for(int i=0; i<=n->num; i++){
				print_node(n->children[i], layers);
			}
			layers[len] = '\0';
		}
	}
	char layers[100];
	layers[0] = '\0';
	print_node(t->root, layers);
}

void btree_delete(struct btree *t, int x)
{
	void btree_delete_from_node(struct node *node, int x){
		int L = t->L;
		int i = 0;
		while(i < node->num && x > node->keys[i]){
			i += 1;
		}
		if(i < node->num && x == node->keys[i]){ //ключ x в узле node
			if (node->is_leaf){
				for(int j=i; j<node->num-1;j++){
					node->keys[j] = node->keys[j+1];
				}
				node->num -= 1;
				return;
			}
			//внутрений узел
			struct node *prev_dougther = node->children[i];
			struct node *next_dougther = node->children[i+1];
			struct node *current_node = node;

			if(prev_dougther->num >= L){
				//Находим предщественника x, рекурсивно удаляем
				current_node = prev_dougther;
				while(!current_node->is_leaf){
					current_node = current_node->children[current_node->num];
				}
				int prev_x = current_node->keys[current_node->num-1];
				node->keys[i] = prev_x;
				btree_delete_from_node(prev_dougther, prev_x);
			}
			else if(next_dougther->num >= L){
				current_node = next_dougther;
				while(!current_node->is_leaf){
					current_node = current_node->children[0];
				}
				int next_x = current_node->keys[0];
				node->keys[i] = next_x;
				btree_delete_from_node(next_dougther, next_x);
			}
			else{
				//Вносим k и все ключи следующей дочери в предыдущую
				prev_dougther->keys[L-1] = x;
				for(int j = 0; j < L - 1; j++){
					prev_dougther->keys[L+j] = next_dougther->keys[j];
				}
				if(!prev_dougther->is_leaf){
					for(int j = 0; j < L; j++){
						prev_dougther->children[L+j] = next_dougther->children[j];
					}
				}
				prev_dougther->num = 2* L - 1;
				//x удаляем из node и следующую дочь
				for(int j=i; j<node->num-1; j++){
					node->keys[j] = node->keys[j+1];
				}
				for(int j=i+1; j<=node->num-1; j++){
					node->children[j] = node->children[j+1];
				}
				node->num -= 1;
				free(next_dougther->keys);
				free(next_dougther->children);
				free(next_dougther);
				btree_delete_from_node(prev_dougther, x);
			}
		}
		else{ //в узле нет ключа
			//ключ содержится в i дочке
			//struct node *dougther = node->children[i];
			if(node->is_leaf){
				return;
			}
			if(node->children[i]->num == L-1){
				if((i > 0 && node->children[i-1]->num >= L) || (i < node->num && node->children[i+1]->num >= L)){
					if (i < node->num && node->children[i+1]->num >= L) {
					//if (i < node->num - 1 && node->children[i+1]->num >= L) {
						//передать ключ разделитель и детей
						node->children[i]->keys[node->children[i]->num] = node->keys[i];
						node->children[i]->num += 1;
						if(!node->children[i]->is_leaf){
							node->children[i]->children[node->children[i]->num] = node->children[i+1]->children[0];
						}
						//на его место поместить крайний ключ из соседнего дерева
						node->keys[i] = node->children[i+1]->keys[0];
						for(int j = 0; j < node->children[i+1]->num - 1; j++){
							node->children[i+1]->keys[j] = node->children[i+1]->keys[j+1];
						}
						if(!node->children[i+1]->is_leaf){
							for(int j = 0; j < node->children[i+1]->num; j++){
								node->children[i+1]->children[j] = node->children[i+1]->children[j+1];
							}
						}
						node->children[i+1]->num -= 1;
					}
					else{
						//передать ключ разделитель и детей
						// for(int j=0; j < node->children[i]->num; j++){
						// 	node->children[i]->keys[j+1] = node->children[i]->keys[j];
						// 	if(!node->children[i]->is_leaf){
						// 		node->children[i]->children[j+1] = node->children[i]->children[j];
						// 	}
						// }
						for(int j=node->children[i]->num - 1; j >= 0; j--){
							node->children[i]->keys[j+1] = node->children[i]->keys[j];
						}
						if(!node->children[i]->is_leaf){
							for(int j=node->children[i]->num; j >= 0; j--){
								node->children[i]->children[j+1] = node->children[i]->children[j];
							}
						}
						node->children[i]->num += 1;
						//node->children[i]->keys[0] = node->keys[i];
						node->children[i]->keys[0] = node->keys[i-1];
						if(!node->children[i]->is_leaf){
							node->children[i]->children[0] = node->children[i-1]->children[node->children[i-1]->num];
						}

						//на его место поместить крайний ключ из соседнего дерева
						//node->keys[i] = node->children[i-1]->keys[node->children[i-1]->num - 1];
						node->keys[i-1] = node->children[i-1]->keys[node->children[i-1]->num - 1];
						//node->children[i+1]->num -= 1;
						node->children[i-1]->num -= 1;
					}
					//btree_delete_from_node(node->children[i], x);
				}
				else{ //объединим узлы
					if(i > 0){
						for(int j = 0; j < L - 1; j++){
							node->children[i]->keys[L+j] = node->children[i]->keys[j];
						}
						if(!(node->children[i]->is_leaf)){
							for(int j = 0; j < L; j++){
								node->children[i]->children[L+j] = node->children[i]->children[j];
							}
						}						
						node->children[i]->keys[L-1] = node->keys[i-1];
						for(int j = 0; j < L - 1; j++){
							node->children[i]->keys[j] = node->children[i-1]->keys[j];
						}
						if(!node->children[i]->is_leaf){
							for(int j = 0; j < L; j++){
								node->children[i]->children[j] = node->children[i-1]->children[j];
							}
						}
						node->children[i]->num = 2 * L - 1;

						free(node->children[i-1]->keys);
						free(node->children[i-1]->children);
						free(node->children[i-1]);
						
						for(int j = i-1; j < node->num - 1; j++){
							node->keys[j] = node->keys[j+1];
						}
						for(int j = i-1; j < node->num; j++){
							node->children[j] = node->children[j+1];
						}
						node->num -= 1;
						i -= 1;
					} else {						
						node->children[i]->keys[L-1] = node->keys[i];
						for(int j = 0; j < L - 1; j++){
							node->children[i]->keys[L+j] = node->children[i+1]->keys[j];
						}
						if(!node->children[i]->is_leaf){
							for(int j = 0; j < L; j++){
								node->children[i]->children[L+j] = node->children[i+1]->children[j];
							}
						}
						node->children[i]->num = 2 * L - 1;

						free(node->children[i+1]->keys);
						free(node->children[i+1]->children);
						free(node->children[i+1]);
						
						for(int j = i; j < node->num - 1; j++){
							node->keys[j] = node->keys[j+1];
						}
						for(int j = i + 1; j < node->num; j++){
							node->children[j] = node->children[j+1];
						}
						node->num -= 1;
					}
				}
			}
			btree_delete_from_node(node->children[i], x);
			if(node->num == 0){
			 	struct node *old_value = node;
				node = old_value->children[i];
				//free(old_value);
			}
		}
	}
	if(!btree_contains(t, x)){
		return;
	}
	btree_delete_from_node(t->root, x);
	if(!t->root->is_leaf && t->root->num == 0){
		struct node *new_node = t->root->children[0];
		free(t->root->keys);
		free(t->root->children);
		free(t->root);
		t->root = new_node;
	}
	t->count -= 1;
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
		return btree_contains_in_node(n->children[i], x);
	}
	return btree_contains_in_node(t->root, x);
}

struct btree_iter
{
	struct btree *t;
	struct node **path;
	int *indexes;
	int current_depth;
};

struct btree_iter* btree_iter_start(struct btree *t)
{
	struct btree_iter *it = (struct btree_iter *) malloc(sizeof(struct btree_iter));
	it->t = t;
	int max_depth = 0;
	struct node *node = t->root;
	while(!node->is_leaf){
		node = node->children[0];
		max_depth += 1;
	}
	max_depth += 1;
	//printf("depth %d\n", max_depth);
	it->path = (struct node **) malloc(sizeof(struct node *) * max_depth);
	it->indexes = (int*) malloc(sizeof(int *) * max_depth);

	it->current_depth = 0;
	it->path[0] = t->root;
	it->indexes[0] = -1;
	return it;
}

void btree_iter_end(struct btree_iter *i)
{
free(i->path);
	free(i->indexes);
	free(i);
}

bool btree_iter_next(struct btree_iter *i, int *x)
{
	// if(!i->path[i->current_depth]->is_leaf){
	// 	i->path[i->current_depth + 1] = i->path[i->current_depth]->children[i->indexes[i->current_depth]+1];
	// 	i->indexes[i->current_depth + 1] = 0;
	// 	i->current_depth += 1;
	// }
	if (i->path[i->current_depth]->is_leaf){
		if(i->indexes[i->current_depth] < i->path[i->current_depth]->num - 1){
			i->indexes[i->current_depth] += 1;
		} else {
			i->current_depth -= 1;
			//если самый правый сын
			while(i->indexes[i->current_depth] == i->path[i->current_depth]->num){
				if(i->current_depth == 0){
					return false;
				}
				else{
					i->current_depth -= 1;
				}
			}
		}
	} else{
		i->indexes[i->current_depth] += 1;
		while(!i->path[i->current_depth]->is_leaf){
			i->path[i->current_depth+1] = i->path[i->current_depth]->children[i->indexes[i->current_depth]];
			i->indexes[i->current_depth+1] = 0;
			i->current_depth += 1;
		}
	}

	*x = i->path[i->current_depth]->keys[i->indexes[i->current_depth]];
	return true;
}