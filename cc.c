#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#define BUFLEN 256
enum {
    AST_INT,
    AST_SYM,
};

typedef struct Var {
    char *name;
    int pos;
    struct Var *next;
} Var;

typedef struct Ast {
    char type;
    union {
        int ival;
        Var *var;
        struct {
            struct Ast *left;
            struct Ast *right;
        };
    };
} Ast;

Var *vars = NULL;

void emit_intexpr(Ast *ast);
Ast *read_symbol(char c);
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

Ast *make_ast_sym(Var *var) {
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_SYM;
    r->var = var;
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

Var *find_var(char *name) {
    Var *v = vars;
    for(; v; v = v->next) {
        if(!strcmp(name, v->name)) {
            return v;
        }
    }

    return NULL;
}

Var *make_var(char *name) {
    Var *v = malloc(sizeof(Var));
    v->name = name;
    v->pos = vars ? vars->pos + 1 : 1;
    v->next = vars;
    vars = v;
    return v;
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


Ast *read_prim(void) {
    int c = getc(stdin);
    if (isdigit(c)) {
        return read_number(c - '0');
    } else if (isalpha(c)) {
        return read_symbol(c);
    } else if (c == EOF) {
        return NULL;
    }

    printf("exit(1);");
    exit(1);
}

Ast *read_symbol(char c) {
    char *buf = malloc(BUFLEN);
    buf[0] = c;
    int i = 1;
    for(;;) {
        int c = getc(stdin);
        if(!isalpha(c)) {
            ungetc(c, stdin);
            break;
        }
        buf[i++] = c;
        if(i == BUFLEN -1)
            perror("Symbol too long");
    }
    buf[i] = '\0';
    Var *v = find_var(buf);
    if(!v) v = make_var(buf);
    return make_ast_sym(v);
}

int get_priority(char op) {
    switch(op) {
        case '=':
            return 1;
        case '+': case '-':
            return 2;
        case '*': case '/':
            return 3;
        default:
            return -1;
    }
   
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
    else if(c == '=') {
        op = '=';
    }

    Ast *right = read_prim();
    int next_op = getc(stdin);
    if (next_op == EOF || get_priority(op) >= get_priority(next_op)) {
        ungetc(next_op, stdin);
        return make_ast_up(make_ast_op(op, ast, right));
    } else {
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
        case '=':
            printf("(=");
		printf_op:
			print_ast(ast->left);
			printf(" ");
			print_ast(ast->right);
			printf(")");
			break;
		case AST_INT:
			printf("%d", ast->ival);
			break;
		case AST_SYM:
			printf("%s", ast->var->name);
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
