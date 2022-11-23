#include "stack.h"
#include "queue.h"
#include <string.h>

typedef struct InputNode {
	/*
	  * (c) concatenation
	  * (|) union
	  * (*) kleene closure
	  * (+)
	  * (i) identifier
	  * (\) escaped input
	 */
	char type;
	char identifier;
    struct InputNode *left, *right;
} node;

struct InputFlags {
	char isEscaped: 1;
	char openingPFound: 1;
};

int pos = 0;
char *input_string;
node *tree;
stack *token_stack, *rpn_stack;
queue *input_queue;

void print_tree(node *to_print) {
	if (NULL != to_print) {
		printf("Type: %c\n", to_print->type);
		if ('i' == to_print->type) {
			printf("Identifier: %c\n", to_print->identifier);
		}
		print_tree(to_print->left);
		print_tree(to_print->right);
	}
}

unsigned char get_precedence(char letter) {
	if ('c' == letter) {
		return 1;
	} else if ('*' == letter || '+' == letter) {
		return 2;
	} else if ('|' == letter) {
		return 3;
	} else {
		return 4;
	}
}

char get_next() {
	return input_string[pos++];
}

void from_rpn() {
	char *from_queue;
	node *right, *left;
	node *tree_walker = tree;

	while (!is_empty_queue(input_queue)) {
		from_queue = (char *) dequeue(input_queue);
		if ('|' == *from_queue) {
			tree_walker->type = '|';
			tree_walker->identifier = 1;

			right = (node *) pop(rpn_stack);
			left = (node *) pop(rpn_stack);

			tree_walker->left = left;
			tree_walker->right = right;
			push(tree_walker, rpn_stack);
		} else if ('*' == *from_queue) {
			tree_walker->type = '*';
			tree_walker->identifier = 1;

			left = (node *) pop(rpn_stack);
			left->left = tree_walker;
			push(left, rpn_stack);
		} else if (10 == *from_queue) {
			tree_walker->type = 'c';
			tree_walker->identifier = 1;

			right = (node *) pop(rpn_stack);
			left = (node *) pop(rpn_stack);

			tree_walker->left = left;
			tree_walker->right = right;
			push(tree_walker, rpn_stack);
		} else {
			tree_walker->type = 'i';
			tree_walker->identifier = *from_queue;
			push(tree_walker, rpn_stack);
		}
		tree_walker = (node *) calloc(1, sizeof(*tree_walker));
		if (NULL == tree_walker) {
			fputs("Failed to allocate memory to <<node *tree_walker>>", stderr);
			exit(-1);
		}
	}
	free(right);
	free(left);
	tree = (node *) pop(rpn_stack);
}

void shunting_yard() {
    char c, top;
    char *token, *stack_out;

	struct InputFlags flags;
	flags.isEscaped = 0;

    while (0 != (c = get_next())) {
        token = calloc(1, sizeof(*token));

        if (NULL == token) {
            fputs("Failed to allocate memory to <<char *token>>", stderr);
			exit(-1);
        }
        token[0] = c;
		if ('(' == c) {
			enqueue((void *) 10, input_queue);
			push(token, token_stack);
		} else if (')' == c) {
			flags.openingPFound = 0;
			while (!is_empty_stack(token_stack)) {
				stack_out = (char *) pop(token_stack);
				printf("%c\n", *stack_out);
				if ('(' != *stack_out) {
					/* concatenation byte */
					enqueue(stack_out, input_queue);
				} else {
					flags.openingPFound = 1;
					break;
				}
			}

			if (!flags.openingPFound) {
				fputs("Syntax Error: Missing '('", stderr);
				exit(-1);
			}

		} else if ('|' == c || '*' == c) {
			unsigned char prec = get_precedence(c);

			while (!is_empty_stack(token_stack)) {
				top = * (char *) peek_stack(token_stack);
				if (top != '(' && ((prec < get_precedence(top)) || (get_precedence(top) == prec && '|' == c))) {
					stack_out = (char *) pop(token_stack);
					enqueue(stack_out, input_queue);
				} else {
					push(token, token_stack);
					break;
				}
			}
		} else {
			/* concatenation byte */
			enqueue((void *) 10, input_queue);
			enqueue(token, input_queue);
		}
    }

	while (!is_empty_stack(token_stack)) {
		void *to_enqueue = pop(token_stack);
		enqueue(to_enqueue, input_queue);
	}
	from_rpn();
}

int main() {
	input_string = (char *) calloc(9, sizeof(*input_string));
	if (NULL == input_string) {
		fputs("Failed to allocate memory to <<char *input_string>>", stderr);
		exit(-1);
	}
	memcpy(input_string, "p(ai|e)n", 9);
	tree = calloc(1, sizeof(*tree));
	if (NULL == tree) {
		fputs("Failed to allocate memory to <<node *tree>>", stderr);
		exit(-1);
	}

	token_stack = initialize_stack();
	rpn_stack = initialize_stack();
	input_queue = initialize_queue();

	shunting_yard();

	print_tree(tree);

	return 0;
}
