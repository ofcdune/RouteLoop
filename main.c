#include "regex.h"
#include <string.h>

#define CONCAT '~'

/* TODO: HINT: Please do not use this code for production, this is a simple test setup */

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
	char lookbehind: 1;
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
		// printf("Type: %c\n", to_print->type);
		if ('i' == to_print->type) {
			// printf("Identifier: %c\n", to_print->identifier);
		}
		print_tree(to_print->left);
		print_tree(to_print->right);
	}
}

unsigned char get_precedence(char letter) {
	if (CONCAT == letter) {
		return 3;
	} else if ('*' == letter || '+' == letter) {
		return 4;
	} else if ('|' == letter) {
		return 1;
	} else {
		return 0;
	}
}

char get_next() {
	return input_string[pos++];
}

void from_rpn() {
	char *from_queue;
	node *right, *left, *tree_walker;

	while (!is_empty_queue(input_queue)) {
		tree_walker = (node *) calloc(1, sizeof(*tree_walker));
		if (NULL == tree_walker) {
			fputs("Failed to allocate memory to <<node *tree_walker>>", stderr);
			exit(-1);
		}
		from_queue = (char *) dequeue(input_queue);
		printf("%s ", *from_queue != CONCAT ? from_queue : "CONCAT");
		if ('|' == *from_queue) {
			tree_walker->type = '|';
			tree_walker->identifier = 1;

			right = (node *) pop(rpn_stack);
			left = (node *) pop(rpn_stack);

			if (NULL == left || NULL == right) {
				fputs("Syntax Error: Missing symbol around the | operator", stderr);
				// exit(-1);
			}

			tree_walker->left = left;
			tree_walker->right = right;
		} else if ('*' == *from_queue) {
			tree_walker->type = '*';
			tree_walker->identifier = 1;

			left = (node *) pop(rpn_stack);
			if (NULL == left) {
				fputs("Syntax Error: Missing symbol before the * operator", stderr);
				// exit(-1);
			}

			left->left = tree_walker;
			push(left, rpn_stack);
			continue;
		} else if (CONCAT == *from_queue) {
			tree_walker->type = 'c';
			tree_walker->identifier = 1;

			right = (node *) pop(rpn_stack);
			left = (node *) pop(rpn_stack);

			if (NULL == left || NULL == right) {
				fputs("Syntax Error: Missing symbol around the concatenation operator", stderr);
				// exit(-1);
			}

			tree_walker->left = left;
			tree_walker->right = right;
		} else {
			tree_walker->type = 'i';
			tree_walker->identifier = *from_queue;
		}
		push(tree_walker, rpn_stack);
	}
	putchar('\n');
	tree = pop(rpn_stack);

	// if (!is_empty_stack(rpn_stack)) {
	//     puts("Stack is not empty");
	// }
}

void into_tokenstack_left(const char *token_left) {
	char top;
	unsigned char top_precedence;
	void *from_stack;

	unsigned char precedence = get_precedence(*token_left);
	while (!is_empty_stack(token_stack)) {
		top = * (char *) peek_stack(token_stack);
		top_precedence = get_precedence(top);

		printf("\t===============> top: %c\n", top);

		if ('(' != top && precedence <= top_precedence && top != *token_left) {
			from_stack = pop(token_stack);
			printf("\tHigher Precedence: Popping %c out of the token stack\n", * (char *) from_stack);
			enqueue(from_stack, input_queue);
			printf("\tHigher Precedence: Enqueuing %c into the input queue\n", * (char *) from_stack);
		} else {
			break;
		}
	}
	push((void *) token_left, token_stack);
	printf("\tPushing %c into token_stack\n", *token_left);
}

void into_tokenstack(const char *token) {
	char top;
	unsigned char top_precedence;
	void *from_stack;

	unsigned char precedence = get_precedence(*token);
	while (!is_empty_stack(token_stack)) {
		top = * (char *) peek_stack(token_stack);
		top_precedence = get_precedence(top);

		printf("\t===============> top: %c\n", top);

		if ('(' != top && precedence < top_precedence) {
			from_stack = pop(token_stack);
			printf("\tHigher Precedence: Popping %c out of the token stack\n", * (char *) from_stack);
			enqueue(from_stack, input_queue);
			printf("\tHigher Precedence: Enqueuing %c into the input queue\n", * (char *) from_stack);
		} else {
			break;
		}
	}
	push((void *) token, token_stack);
	printf("\tPushing %c into token_stack\n", *token);
}

void pop_until_bracket() {
	struct InputFlags flags;
	char *stack_out;
	/* we assume we have not found an opening bracket, so we set the 'openingFound' to 0 */
	flags.openingPFound = 0;
	while (!is_empty_stack(token_stack)) {
		/* we pop from the stack until we get the left bracket */
		stack_out = (char *) pop(token_stack);
		printf("CLOSE_PARENS: Popping %c from token stack\n", *stack_out);
		if ('(' != *stack_out) {
			enqueue(stack_out, input_queue);
			printf("CLOSE_PARENS: Enqueuing %c into the input queue\n", *stack_out);
		} else {
			flags.openingPFound = 1;
			break;
		}
	}

	if (!flags.openingPFound) {
		fputs("Syntax Error: Missing '('", stderr);
		exit(-1);
	}
}

void shunting_yard() {
	char c;
	char *token;

	struct InputFlags flags;
	flags.isEscaped = 0;
	flags.lookbehind = 0;

	while (0 != (c = get_next())) {

		/* Allocate the token which we put into the stack/queue */
		token = (char *) calloc(1, sizeof(*token));
		if (NULL == token) {
			fputs("Failed to allocate memory to <<char *token>>", stderr);
			exit(-1);
		}
		token[0] = c;

		/* If the flag 'isEscaped' is set, we enqueue the token directly */
		if (flags.isEscaped) {
			enqueue(token, input_queue);
			printf("(Escaped): Enqueuing %c into the input queue\n", c);
			flags.isEscaped = 0;
		}

		/* main body of the shunting yard algorithm */
		switch (c) {
			case '(':
				/* if the character is an open bracket, we just push it on the token stack */
				if (flags.lookbehind) {
					token[0] = CONCAT;
					into_tokenstack_left(token);
					flags.lookbehind = 0;

					token = (char *) calloc(1, sizeof(*token));
					if (NULL == token) {
						fputs("Failed to allocate memory to <<char *token>>", stderr);
						exit(-1);
					}

					token[0] = '(';
				}
				push(token, token_stack);
				puts("Pushing ( into the token stack");
				break;
			case ')':
				flags.lookbehind = 1;
				pop_until_bracket();
				break;
			case '|':
				flags.lookbehind = 0;
				into_tokenstack(token);
				break;
			case '*':
				flags.lookbehind = 1;
				into_tokenstack(token);
				break;
			case CONCAT:
				/* generally, all left associative tokens go here */
				into_tokenstack_left(token);
				break;
			case '\\':
				if (flags.lookbehind) {
					flags.lookbehind = 0;
					token[0] = CONCAT;
					into_tokenstack_left(token);
				}
				flags.isEscaped = 1;
				break;
			default:
				if (flags.lookbehind) {
					token[0] = CONCAT;
					into_tokenstack_left(token);

					token = (char *) calloc(1, sizeof(*token));
					if (NULL == token) {
						fputs("Failed to allocate memory to <<char *token>>", stderr);
						exit(-1);
					}

					token[0] = c;
				}
				enqueue(token, input_queue);
				printf("Enqueuing %c into the input queue\n", c);
				flags.lookbehind = 1;
		}
	}

	puts("===> Putting the rest of the stack into the input stack <===");
	void *to_enqueue;
	while (!is_empty_stack(token_stack)) {
		to_enqueue = (char *) pop(token_stack);
		printf("Popping %c from the token stack\n", * (char *) to_enqueue);
		enqueue(to_enqueue, input_queue);
		printf("Enqueuing %c into the input queue\n", * (char *) to_enqueue);
	}
	from_rpn();
}

void parse_pattern(const char *new_pattern, int size) {
	input_string = (char *) calloc(size+1, sizeof(*input_string));
	if (NULL == input_string) {
		fputs("Failed to allocate memory to <<char *input_string>>", stderr);
		exit(-1);
	}
	memcpy(input_string, new_pattern, size);

	tree = calloc(1, sizeof(*tree));
	if (NULL == tree) {
		fputs("Failed to allocate memory to <<node *tree>>", stderr);
		exit(-1);
	}

	token_stack = initialize_stack();
	rpn_stack = initialize_stack();
	input_queue = initialize_queue();

	shunting_yard();

	// print_tree(tree);
}

#define PATTERN_L 7

int main() {

	/*
	  * Examples:
	  * i(ce*)d = ice*~d~~
	  * p(ai|e)n = pai~e|n~~
	  * (hi*|e) = hi*~e|
	 */

	char new_pattern[PATTERN_L] = "i(ce*)d";
	// set_pattern(new_pattern, PATTERN_L);

	parse_pattern(new_pattern, PATTERN_L);

	return 0;
}
