#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "cc.h"

#define MAX_ARGS 6
#define EXPR_LEN 100

enum {
    AST_INT,
    AST_CHAR,
    AST_VAR,
    AST_STR,
    AST_FUNCALL,
};

typedef struct Ast {
    char type;
    union {
        int ival;
        char c;
        struct {
            char *sval;
            int sid;
            struct Ast *snext;
        };
        struct {
            char *vname;
            int vpos;
            struct Ast *vnext;
        };
        struct {
            struct Ast *left;
            struct Ast *right;
        };
        struct {
            char *fname;
            int nargs;
            struct Ast **args;
        };
    };
} Ast;

Ast *vars = NULL;
Ast *strings = NULL;

void emit_intexpr(Ast *ast);
Ast *read_symbol(char c);
Ast *read_string(void);
Ast *read_prim(void);
Ast *read_ident_or_func(char *c);
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

Ast *make_ast_var(char *vname) {
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_VAR;
    r->vname = vname;
    r->vpos = vars ? vars->vpos + 1 : 1;
    r->vnext = vars;
    vars = r;

    return r;
}

Ast *make_ast_char(char c) {
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_CHAR;
    r->c = c;
    return r;
}

Ast *find_var(char *name) {
    for(Ast *v = vars; v; v = v->vnext) {
        if(!strcmp(name, v->vname)) {
            return v;
        }
    }

    return NULL;
}

Ast * make_arg(char c) {
    Token *name = read_token();
    return make_ast_var(name->sval);
}

Ast *make_ast_funcall(char *fname, int nargs, Ast **args) {
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_FUNCALL;
    r->fname = fname;
    r->nargs = nargs;
    r->args = args;

    return r;
}

Ast *make_ast_string(char *str) {
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_STR;
    r->sval = str;
    if(strings == NULL) {
        r->sid = 0;
        r->snext = NULL;
    } else {
        r->sid = strings->sid + 1;
        r->snext = strings;
    }

    strings = r;
    return r;
}

Ast *read_func_args(char *fname) {
    Ast **args = malloc(sizeof(Ast*) * (MAX_ARGS + 1));
    int i = 0, nargs = 0;
    for (; i < MAX_ARGS; i ++) {
        char c = getc(stdin);
        if(c == ')') break;
        args[i] = make_arg(c);
        nargs++;
        c = getc(stdin);
        if(c == ',') ;
        else if(c == ')') break;
        else perror("unexcepted characterhhhh");
    }

    return make_ast_funcall(fname, nargs, args);
}

Ast *read_ident_or_func(char* c) {
    Token *token = read_token();
    if(is_punct(token, '(')) { // 这里形如：'(a,b,c,d)', 说明是function
        return read_func_args(c);
    }

    Ast *v = find_var(c);
    if(!v) v = make_ast_var(c);
    return v;
}

Ast *read_prim(void) {
    Token *token = read_token();
    switch(token->type) {
        case TTYPE_IDENT:
            return read_ident_or_func(token->sval);
        case TTYPE_INT:
            return make_ast_int(token->ival);
        case TTYPE_CHAR:
            return make_ast_char(token->c);
        case TTYPE_STRING:
            return make_ast_string(token->sval);
        default:
            printf("internal error token");
            return NULL;
    }
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
    int c = getc(stdin);
    if (c == EOF || c == 10) {
        return ast;
    }
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

void print_quote(char *p) {
    while(*p) {
        if(*p == '\"' || *p == '\\') 
            printf("\\");
        printf("%c", *p);
        p++;
    }
}

void print_ast(Ast *ast) {          
	switch(ast->type) {
		case '+':
			printf("(+ ");
			goto printf_op;
		case '-':
			printf("(- ");
            goto printf_op;
        case '*':
            printf("(* ");
            goto printf_op;
        case '/':
            printf("(/ ");
            goto printf_op;
        case '=':
            printf("(=");
            goto printf_op;
        case AST_CHAR:
            printf("'%c'", ast->c);
            break;
        case AST_FUNCALL:
            printf("%s(", ast->fname);
            for (int i = 0; ast->args[i]; i ++) {
                print_ast(ast->args[i]);
                if(ast->args[i + 1])
                    printf(",");
            }
            printf(")");
            break;
        case AST_STR:
            printf("\"");
            print_quote(ast-> sval);
            printf("\"");
            break;
		printf_op:
			print_ast(ast->left);
			printf(" ");
			print_ast(ast->right);
			printf(")");
			break;
		case AST_INT:
			printf("%d", ast->ival);
			break;
		case AST_VAR:
			printf("%s", ast->vname);
			break;
		default:
		  printf("should not reach here!");

	}
}

int main(int argc, char **arg) {

    Ast *left = read_prim();
    Ast *r = make_ast_up(left);

    print_ast(left);

    return 0;
}
