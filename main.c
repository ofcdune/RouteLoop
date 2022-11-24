#include "stack.h"
#include "queue.h"
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
    char isLast: 1;
    char isConcat: 1;
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
    if (CONCAT == letter) {
        return 1;
    } else if ('*' == letter || '+' == letter) {
        return 3;
    } else if ('|' == letter) {
        return 2;
    } else {
        return 0;
    }
}

char get_next() {
    return input_string[pos++];
}

char peek_next() {
    return input_string[pos+1];
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

void shunting_yard() {
    char c, top;
    char *token, *stack_out;

    struct InputFlags flags;
    flags.isEscaped = 0;
    flags.isLast = 0;
    flags.isConcat = 0;

    while (0 != (c = get_next())) {

        /* Allocate the token which we put into the stack/queue */
        token = (char *) calloc(1, sizeof(*token));
        if (NULL == token) {
            fputs("Failed to allocate memory to <<char *token>>", stderr);
            exit(-1);
        }

        /* If the flag 'isConcat' is set, we set the current char to the concat byte and reverse the reading position */
        if (flags.isConcat) {
            c = CONCAT;
            pos--;
            flags.isConcat = 0;
        }
        token[0] = c;

        /* If the flag 'isEscaped' is set, we enqueue directly, turn off the 'isEscaped' flag and enable the 'isConcat' flag */
        if (flags.isEscaped) {
            enqueue(token, input_queue);
            flags.isEscaped = 0;
            flags.isConcat = 1;
        }

        /* main body of the shunting yard algorithm */
        if ('(' == c) {
            /* if the character is an open bracket, we push it on the token stack */
            push(token, token_stack);
            printf("Pushing %c on the token stack\n", c);
        } else if (')' == c) {
            /* we assume we have not found an opening bracket, so we set the 'openingFound' to 0 */
            flags.openingPFound = 0;
            while (!is_empty_stack(token_stack)) {
                /* we pop from the stack until we get the left bracket */
                stack_out = (char *) pop(token_stack);
                if ('(' != *stack_out) {
                    enqueue(stack_out, input_queue);
                    printf("Enqueuing %c into the input queue\n", *stack_out);
                } else {
                    flags.openingPFound = 1;
                    break;
                }
            }

            if (!flags.openingPFound) {
                fputs("Syntax Error: Missing '('", stderr);
                exit(-1);
            }

            /* after the right bracket, we assume that it concatenates to another character */
            flags.isConcat = 1;

        } else if ('|' == c || '*' == c) {

            /* we get the precedence if the current letter is an operator */
            unsigned char prec = get_precedence(c);
            while (!is_empty_stack(token_stack)) {
                top = * (char *) peek_stack(token_stack);
                int top_prec = get_precedence(top);
                if ('(' != top && (prec < top_prec)) {
                    stack_out = (char *) pop(token_stack);
                    printf("Higher Precedence: Popping %c out of the token stack\n", *stack_out);
                    enqueue(stack_out, input_queue);
                    printf("Higher Precedence: Enqueuing %c into the input queue\n", *stack_out);
                } else {
                    break;
                }
            }
            push(token, token_stack);
            printf("Pushing %c into token_stack\n", c);
        } else if (CONCAT == c) {

            /* all left associative operands go here */
            unsigned char prec = get_precedence(c);
            while (!is_empty_stack(token_stack)) {
                top = * (char *) peek_stack(token_stack);
                int top_prec = get_precedence(top);
                if ('(' != top && (prec <= top_prec)) {
                    stack_out = (char *) pop(token_stack);
                    printf("): Popping %c out of the token stack\n", *stack_out);
                    enqueue(stack_out, input_queue);
                    printf("): Enqueuing %c into the input queue\n", *stack_out);
                } else {
                    break;
                }
            }
            push(token, token_stack);
            printf("Pushing %c on the token stack\n", c);
        } else if ('\\' == c) {
            flags.isEscaped = 1;
            continue;
        } else {
            enqueue(token, input_queue);
            printf("Enqueuing %c into the input queue\n", c);
            flags.isConcat = 1;
        }
    }

    puts("Putting the rest of the stack into the input stack");
    void *to_enqueue;
    while (!is_empty_stack(token_stack)) {
        to_enqueue = (char *) pop(token_stack);
        printf("Popping %c from the token stack\n", * (char *) to_enqueue);
        enqueue(to_enqueue, input_queue);
        printf("Enqueuing %c into the input queue\n", * (char *) to_enqueue);
    }
    from_rpn();
}

int main() {
    input_string = (char *) calloc(9, sizeof(*input_string));
    if (NULL == input_string) {
        fputs("Failed to allocate memory to <<char *input_string>>", stderr);
        exit(-1);
    }
    memcpy(input_string, "p(ai|e)n", 8);

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

    return 0;
}
