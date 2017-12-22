#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

#define BUFLEN 256
enum {
    AST_INT,
    AST_STR
};

typedef struct Ast {
    char type;
    union {
        int ival;
        char *sval;
        struct {
            struct Ast *left;
            struct Ast *right;
        };
    };
} Ast;

void emit_intexpr(Ast *ast);
Ast *read_string(void);
Ast *read_expr(void);

Ast *make_ast_op(char type, Ast *left, Ast *right) {
    Ast *r = malloc(sizeof(Ast));
    r->type = type;
    r->left = left;
    r->right = right;

    return r;
}

Ast *make_ast_int(int val) {
    Ast *r = malloc(sizeof(Ast));
    r-> type = AST_INT;
    r->ival = val;
    return r;
}

Ast *make_ast_str(char *str) {
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_STR;
    r->sval = str;
    return r;
}

void skip_space(void) {
    int c;
    while((c=getc(stdin)) != EOF) {
    	if(isspace(c))
    		continue;
    	ungetc(c, stdin);
    	return;
    }
}

Ast *read_number(int n) {
    for(;;) {
        int c = getc(stdin);
        if(!isdigit(c)) {
            ungetc(c, stdin);
            return make_ast_int(n);
        }

        n = n * 10 + (c - '0');
    }
}

Ast *read_string(void) { 
    char *buf = malloc(BUFLEN);
    int i = 0;
    for(;;) {
        int c = getc(stdin);
        if(c == EOF)
           printf("unterminated string");
        if(c == '"')
            break;
        buf[i++] = c;
        if(i == BUFLEN -1) {
           printf("string too long");
        }
    }

    return make_ast_str(buf);
}

Ast *read_prim(void) {
    int c = getc(stdin);
    if (isdigit(c)) {
        return read_number(c - '0');
    } else if (c == '"') {
        return read_string();
    } else if (c == EOF) {
       printf("Error printed byprintf");
    }

    printf("exit(1);");
    exit(1);
}

int get_priority(int operator) {
    if(operator == '+' || operator == '-')
        return 1;
    else if(operator == '/' || operator == '*') {
        return 2;
    } 
    return 0;
}

Ast *make_ast_up(Ast *ast) {
    skip_space();
    int c = getc(stdin);
    if (c == EOF)
      return ast;
    int op;
    if (c == '+') {
        op = '+';
    }
    else if (c == '-') {
        op = '-';
    }
    else if (c == '*') {
        op = '*';
    }
    else if (c == '/') {
        op = '/';
    }
    Ast *right = read_prim();
    int next_op = getc(stdin);
    if (next_op == EOF || get_priority(op) >= get_priority(next_op)) {
        ungetc(next_op, stdin);
        return make_ast_up(make_ast_op(op, ast, right));
    } else {
        printf("here\n");
        ungetc(next_op, stdin);
        return make_ast_up(make_ast_op(op, ast, make_ast_up(right)));
    }
}

void print_ast(Ast *ast) {             
	switch(ast->type) {
		case '+':
			printf("(+ ");
			goto printf_op;
		case '-':
			printf("(- ");
        case '*':
            printf("(* ");
            goto printf_op;
        case '/':
            printf("(/ ");
		printf_op:
			print_ast(ast->left);
			printf(" ");
			print_ast(ast->right);
			printf(")");
			break;
		case AST_INT:
			printf("%d", ast->ival);
			break;
		case AST_STR:
			printf("%s", ast->sval);
			break;
		default:
		printf("should not reach here!");

	}
}

int main(int argc, char **arg) {

    Ast *left = read_prim();
    Ast *r = make_ast_up(left);

    print_ast(r);

    return 0;
}
