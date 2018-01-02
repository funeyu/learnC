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
    AST_LITERAL,
    AST_VAR,
    AST_STR,
    AST_FUNCALL,
    AST_DECL,
    AST_ADDR,
    AST_DEREF,
    AST_STRING,
    AST_LVAR,
    AST_LREF,
    AST_GVAR,
    AST_GREF,
    AST_ARRAY_INIT,
};

enum {
    CTYPE_VOID,
    CTYPE_INT,
    CTYPE_CHAR,
    CTYPE_STR,
    CTYPE_PTR,
};

typedef struct Ctype {
    int type;
    struct Ctype *ptr;
    int size;
} Ctype;

typedef struct Ast Ast;
struct Ast {
    char type;
    Ctype *ctype;
    Ast *next;
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
        // local variable
        struct {
            char *lname;
            int loff;
        };  

        // global variable
        struct {
            char *gname;
            char *glabel;
        };

        // local reference
        struct {
            struct Ast *lref;
            int lrefoff;
        };
        // global reference
        struct {
            struct Ast *gref;
            int goff;
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
        struct {
            struct Ast *decl_var;
            struct Ast *decl_init;
        };

        // Array init
        struct {
            int size;
            struct Ast **array_init;
        };
        struct {
            struct Ast *operand;
        };
    };
};

static Ast *vars = NULL;
static Ast *strings = NULL;
static Ast *globals = NULL;
static Ast *locals = NULL;

static int labelseq = 0;

void emit_intexpr(Ast *ast);
static Ast *read_symbol(char c);
static Ast *read_string(void);
static Ast *read_prim(void);
static Ast *read_ident_or_func(char *c);
static Ast *read_expr(void);
static Ast *make_ast_up(Ast *init);
static char result_type(char op, Ast *left, Ast *right);
static Ctype *make_ptr_type(Ctype* ctype);
static Ctype *make_array_type(Ctype *ctype, int size);
static int ctype_size(Ctype *ctype);

static Ctype *ctype_int = &(Ctype){CTYPE_INT, NULL};
static Ctype *ctype_char = &(Ctype){CTYPE_CHAR, NULL};
static Ctype *ctype_str = &(Ctype){CTYPE_STR, NULL};

static Ast *make_ast_op(char type, Ast *left, Ast *right) {
    Ast *r = malloc(sizeof(Ast));
    r->type = type;
    r->ctype = result_type(type, left, right);
    r->left = left;
    r->right = right;

    return r;
}

static *make_next_label(void) {
    String *s = make_string();
    string_appendf(s, ".L%d", labelseq++);
    return get_cstring(s);
}

static Ast *ast_lvar(Ctype *ctype, char *name) {
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_LVAR;
    r->lname = name;
    r->next = NULL;

    if(locals) {
        Ast *p;
        for(p = locals; p->next; p = p->next);
        p->next = r;
    } else {
        locals = r;
    }

    return r;
}

static Ast *ast_lref(Ctype *ctype, Ast *lvar, int off) {
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_LREF;
    r->ctype = ctype;
    r->lref = lvar;
    r->lrefoff = off;
    return r;
}

static Ast *ast_gvar(Ctype *ctype, char *name, bool filelocal) {
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_GVAR;
    r->ctype = ctype;
    r->gname = name;
    r->glabel = filelocal ? make_next_label() : name;
    r->next = NULL;
    if(globals) {
        Ast *p;
        for(p = locals; p->next; p = p->next);
        p->next = r;
    } else {
        globals = r;
    }
    return r;
}

static Ast *ast_gref(Ctype *ctype, Ast *gvar, int off) {
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_GREF;
    r->ctype = ctype;
    r->gref = gvar;
    r->goff = off;
    return r;
}

static Ast *ast_string(char *str) {
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_STRING;
    r->ctype = make_array_type(ctype_char, strlen(str) +1);
    r->sval = str;
    r->slabel = make_next_label();
    r->next = globals;
    globals = r;
    return r;
}

static Ast *ast_array_init(int size, Ast **array_init) {
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_ARRAY_INIT;
    r->ctype = NULL;
    r->size = size;
    r->array_init = array_init;

    return r;
}

static Ast *make_ast_uop(char type Ctype *ctype, Ast *operand) {
    Ast *r = malloc(sizeof(Ast));
    r->type = type;
    r->ctype = ctype;
    r->ctype = ctype;
    r->operand = operand;
    return r;
}

static Ast *make_ast_int(int val) {
    Ast *r = malloc(sizeof(Ast));
    r-> type = AST_LITERAL;
    r-> ctype = ctype_int;
    r->ival = val;
    return r;
}

static Ast *make_ast_var(Ctype *ctype, char *vname) {
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_VAR;
    r->vname = vname;
    r->ctype = ctype;
    r->vpos = vars ? vars->vpos + 1 : 1;
    r->vnext = vars;
    vars = r;

    return r;
}

static Ast *make_ast_char(char c) {
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_LITERAL;
    r->ctype = ctype_char;
    r->c = c;
    return r;
}

static Ast *make_ptr_type(Ctype *ctype) {
    Ctype *r = malloc(sizeof(Ctype));
    r->type = CTYPE_PTR;
    r->ptr = ctype;
    return r;
}

static Ast *find_var(char *name) {
    for(Ast *v = vars; v; v = v->vnext) {
        if(!strcmp(name, v->vname)) {
            return v;
        }
    }

    return NULL;
}

static Ast * make_arg() {
    Token *name = read_token();
    // todo: 函数参数得需要有类型的
    return make_ast_var(CTYPE_VOID, name->sval);
}

static Ast *make_ast_funcall(char *fname, int nargs, Ast **args) {
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_FUNCALL;
    r->ctype = ctype_int;
    r->fname = fname;
    r->nargs = nargs;
    r->args = args;

    return r;
}

static Ast *make_ast_string(char *str) {
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_LITERAL;
    r->ctype = ctype_str;
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

static Ast *make_ast_decl(Ast *var, Ast *init) {
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_DECL;
    r->ctype = NULL;
    r->decl_var = var;
    r->decl_init = init ? init : NULL;
    return r;
}

static Ast *read_func_args(char *fname) {
    Ast **args = malloc(sizeof(Ast*) * (MAX_ARGS + 1));
    int i = 0, nargs = 0;
    for (; i < MAX_ARGS; i ++) {
        Token *tok = read_token();
        if(is_punct(tok, ')')) break;
        unget_token(tok);
        args[i] = make_arg();
        nargs++;
        tok = read_token();
        if(is_punct(tok, ')')) break;
        if(!is_punct(tok, ','))
            perror("unexcepted character");
    }

    return make_ast_funcall(fname, nargs, args);
}

static Ast *read_ident_or_func(char* c) {
    Token *token = read_token();
    if(is_punct(token, '(')) { // 这里形如：'(a,b,c,d)', 说明是function
        return read_func_args(c);
    }

    unget_token(token);
    Ast *v = find_var(c);
    if(!v) v = make_ast_var(CTYPE_VOID, c);
    return v;
}

static void ensure_lvalue(Ast *ast) {
    if(ast->type != AST_VAR)
        perror("lvalue expected");
}

static Ast *read_unary_expr(void) {
    Token *token = read_token();
    if(is_punct(token, '&')) {
        Ast *operand = read_unary_expr();
        ensure_lvalue(operand);
        return make_ast_uop(AST_ADDR, make_ptr_type(operand->ctype, operand), operand);
    }
    if(is_punct(token, '*')) {
        Ast *operand = read_unary_expr();
        if(operand->ctype->type != CTYPE_PTR)
            perror("pointer type excepted!!!");

        return make_ast_uop(AST_DEREF, operand->ctype->ptr, operand);
    }
    unget_token(token);
    return read_prim();
}

static Ast *read_prim(void) {
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
        case TTYPE_PUNCT:
            printf("unexpected character:%c\n", token->punct);
            return NULL;
        default:
            printf("internal error token\n");
            return NULL;
    }
}

static int get_priority(char op) {
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

static Ctype *get_ctype(Token *token) {
    if(token->type != TTYPE_IDENT) 
        return NULL;
    if(!strcmp(token->sval, "int")) 
        return ctype_int;
    if(!strcmp(token->sval, "char"))
        return ctype_char;
    if(!strcmp(token->sval, "string"))
        return ctype_str;

    return -1;
}

static bool is_type_keyword(Token *token) {
    return get_ctype(token) != -1;
}


static void expect(char punct) {
    Token *token = read_token();
    if(!is_punct(token, punct)) 
        printf("'%c' expected", punct);
}

static char next_punct() {
    Token *token = read_token();
    if(token->type == TTYPE_PUNCT)
        return token->punct;
    else {
        unget_token(token);
        return (char)0;
    }
}

static Ast *read_decl(void) {
    Ast *init;

    int ctype = get_ctype(read_token());
    Token *name = read_token();
    if(name->type != TTYPE_IDENT) 
        printf("Identifier expected");
    Ast *var = make_ast_var(ctype, name->sval);

    char next_p = next_punct();
    if(next_p) {
        if(next_p == '=') {
            Ast *left = read_prim();
            init = make_ast_up(left);

            return make_ast_decl(var, init);
        } else if(next_p == ';') {// 没有初始值的申明
            return make_ast_decl(var, init);
        }
        return NULL;
    } else {
        perror("expected punct!!!!!");
        return NULL;
    }

    
}

static Ast *read_decl_array_initializer(Ctype *ctype) {
    
}

static char result_type(char op, Ast *left, Ast *right) {
    Ast *var_str;

    switch(left->ctype) {
        case CTYPE_VOID:
            goto err;
        case CTYPE_INT:
            switch(right->ctype) {
                case CTYPE_INT:
                case CTYPE_CHAR:
                    return CTYPE_INT;
                case CTYPE_STR:
                    goto err;
            }
            break;
        case CTYPE_CHAR:
            switch(right->ctype) {
                case CTYPE_CHAR:
                    return CTYPE_CHAR;
                case CTYPE_STR:
                case CTYPE_INT:
                    goto err;
            }
            break;
        case CTYPE_STR:
            var_str = find_var(left->sval);
            printf("var_str:%s", left->sval);
            if(var_str) {
                printf("sssssss");
            }
            return CTYPE_INT;
            // goto err;
        default:
            perror("internal error!");

    }

    err:
        perror("incompatible operands");
        return (char)0;
}

static Ast *make_ast_up(Ast *ast) {

    Token *type = read_token();
    if (type == NULL || is_punct(type, ';')) {
        return ast;
    }
    int c = type->punct;

    Ast *right = read_prim();
    Token *next_op = read_token();

    if (next_op == NULL || get_priority(c) >= get_priority(next_op->punct)) {
        if(next_op)
            unget_token(next_op);    
        return make_ast_up(make_ast_op(c, ast, right));
    } else {
        unget_token(next_op);
        return make_ast_up(make_ast_op(c, ast, make_ast_up(right)));
    }
}

static char *ctype_to_string(int ctype) {
    switch(ctype) {
        case CTYPE_VOID: 
            return "void";
        case CTYPE_INT:
            return "int";
        case CTYPE_CHAR:
            return "char";
        case CTYPE_STR:
            return "string";
        default:
            printf("Unknown ctype: %d", ctype);
            return NULL;
    }
}
static void print_quote(char *p) {
    while(*p) {
        if(*p == '\"' || *p == '\\') 
            printf("\\");
        printf("%c", *p);
        p++;
    }
}

static void print_ast(Ast *ast) {
    
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
            for (int i = 0; i < ast->nargs; i ++) {
                if(ast->args[i]) {
                    print_ast(ast->args[i]);
                }
                
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
        case AST_DECL:
            printf("(decl %s %s ",
                ctype_to_string(ast->decl_var->ctype),
                ast->decl_var->vname);
            if(ast->decl_init)
                print_ast(ast->decl_init);
            printf(")");
            break;
		default:
		  printf("should not reach here!");

	}
}

int main(int argc, char **arg) {

    Ast *f;
    Ast *r;
    Ast *expressions[EXPR_LEN];
    int nexpr = 0;
    for(;;) {
        Token *begin = read_token();
        if(!begin)
            break;
        unget_token(begin);

        if(is_type_keyword(begin)) {
            r = read_decl();
        } else {
            Ast *left = read_prim();
            r = make_ast_up(left);
        }
        
        expressions[nexpr++] = r;
    }

    for(int v = 0; v < nexpr; v ++) {
        print_ast(expressions[v]);
    }

    // 下面是链表的写法
    // print_ast(f);
    // if(f->next == NULL) {
    //     printf("\n%s\n", "next");
    // } else {
    //     printf("\n%s\n", "else");
    // }
    // while(f) {
    //     // print_ast(f);
    //     printf("%s\n", "fff");
    //     f = f->next ? f->next : NULL;
    // }

    return 0;
}
