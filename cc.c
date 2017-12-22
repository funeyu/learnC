#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

#define BUFLEN 256
enum {
    AST_OP_PLUS,
    AST_OP_MINUS,
    AST_INT,
    AST_STR
};

typedef struct Ast {
    int type;
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

Ast *make_ast_op(int type, Ast *left, Ast *right) {
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
        printf("ccc");
        return read_number(c - '0');
    } else if (c == '"') {
        return read_string();
    } else if (c == EOF) {
       printf("Error printed byprintf");
    }

    printf("exit(1);");
    exit(1);
}


Ast *make_ast_up(Ast *ast) {
    skip_space();
    int c = getc(stdin);
    if (c == EOF)
      return ast;
    int op;
    if (c == '+') {
        op = AST_OP_PLUS;
    }
    else if (c == '-') {
        op = AST_OP_MINUS;
    }
    Ast *right = read_prim();
    return make_ast_up(make_ast_op(op, ast, right));
}

void print_ast(Ast *ast) {             
	switch(ast->type) {
		case AST_OP_PLUS:
			printf("(+ ");
			goto printf_op;
		case AST_OP_MINUS:
			printf("(- ");
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
