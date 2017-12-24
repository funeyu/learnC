#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#define BUFLEN 256
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
Ast *read_ident_or_func(char c);
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


void skip_space(void) {
    int c;
    while((c=getc(stdin)) != EOF) {
    	if(isspace(c))
    		continue;
    	ungetc(c, stdin);
    	return;
    }
}

Ast *find_var(char *name) {
    for(Ast *v = vars; v; v = v->vnext) {
        if(!strcmp(name, v->vname)) {
            return v;
        }
    }

    return NULL;
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

Ast *read_char(void) {
    char c = getc(stdin);
    if(c == EOF) goto err;

    char c1 = getc(stdin);
    if(c1 != '\'') {
        perror("wrong char formart");
    }
    if(c1 == EOF) {
        goto err;
    }

    return make_ast_char(c);
err: 
    perror("unterminated char");
    return NULL;
}

Ast *read_prim(void) {

    char c = getc(stdin);
    if (isdigit(c)) {
        return read_number(c - '0');
    } else if(c == '"') {
        return read_string();
    } else if(c == '\'') {
        return read_char();
    } else if (isalpha(c)) {
        return read_ident_or_func(c);
    } else if (c == EOF) {
        return NULL;
    }

    printf("exit(1);");
    exit(1);
}

char *read_ident(char c) {
    char *buf = malloc(BUFLEN);
    buf[0] = c;
    int i = 1;
    for(;;) {
        int c = getc(stdin);
        if(!isalnum(c)) {
            ungetc(c, stdin);
            break;
        }
        buf[i++] = c;
        if(i == BUFLEN -1)
            perror("Identifier too long");
    }
    buf[i] = '\0';
    return buf;
}

Ast * make_arg(char c) {
    char *name = read_ident(c);
    return make_ast_var(name);
}

Ast *make_ast_funcall(char *fname, int nargs, Ast **args) {
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_FUNCALL;
    r->fname = fname;
    r->nargs = nargs;
    r->args = args;

    return r;
}

Ast *make_ast_str(char *str) {
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
        skip_space();
        char c = getc(stdin);
        if(c == ')') break;
        args[i] = make_arg(c);
        nargs++;
        c = getc(stdin);
        if(c == ',') skip_space();
        else if(c == ')') break;
        else perror("unexcepted character");
    }

    return make_ast_funcall(fname, nargs, args);
}

Ast *read_ident_or_func(char c) {
    char *name = read_ident(c);
    skip_space();
    char c2 = getc(stdin);
    if(c2 == '(') { // 这里形如：'(a,b,c,d)', 说明是function
        return read_func_args(name);
    }

    ungetc(c2, stdin);
    Ast *v = find_var(name);
    if(!v) v = make_ast_var(name);
    return v;
}

Ast *read_string(void) {
    char *buf = malloc(BUFLEN);
    int c = getc(stdin);
    for(int i=0; ;i++) {
        if(c == '"') break;
        buf[i] = c;
        c = getc(stdin);
    }

    return make_ast_str(buf);
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

    print_ast(r);

    return 0;
}
