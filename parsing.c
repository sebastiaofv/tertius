#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"

/* Declare new tval struct */
typedef struct tval{
    int type;
    long num;

    char* err;
    char* sym;

    int count;
    struct tval** cell;
} tval;

/* Possible tval types */
enum { TVAL_NUM, TVAL_ERR, TVAL_SYM, TVAL_SEXPR };

/* Possible error types */
enum { TERR_DIV_ZERO, TERR_BAD_OP, TERR_BAD_NUM };

/* Create a pointer to a new Number tval */
tval* tval_num(long x){
    tval* v = malloc(sizeof(tval));
    v->type = TVAL_NUM;
    v->num = x;
    return v;
}

/* Create a new pointer to error type */
tval* tval_err(char* m){
    tval* v = malloc(sizeof(tval));
    v->type = TVAL_ERR;
    v->err = malloc(strlen(m) + 1);
    strcpy(v->err, m);
    return v;
}

/* Construct a pointer to a new Symbol tval */
tval* tval_sym(char* s) {
    tval* v = malloc(sizeof(tval));
    v->type = TVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;          
}

/* A pointer to a new empty Sexpr tval */
tval* tval_sexpr(void) {
    tval* v = malloc(sizeof(tval));
    v->type = TVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;          
}

void tval_del(tval* v){
    switch(v->type){
        /* Do nothing for number type */
        case TVAL_NUM:
            break;

        /* For error or Sym free the string data */
        case TVAL_ERR:
            free(v->err);
            break;
        case TVAL_SYM:
            free(v->sym);
            break;

        /* If Sexpr then delete all elements inside */
        case TVAL_SEXPR:
            for (int i = 0; i < v->count; i++) {
                tval_del(v->cell[i]);
            }
            free(v->cell);
            break;
    }

    /* Free the memory allocated for the tval struc */
    free(v);
}

tval* tval_add(tval* v, tval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(tval*) * v->count);
    v->cell[v->count-1] = x;
    return v; 
}

tval* tval_pop(tval* v, int i) {
    /* Find the item at "i" */
    tval* x = v->cell[i];
        
    /* Shift memory after the item at "i" over the top */
    memmove(&v->cell[i], &v->cell[i+1], sizeof(tval*) * (v->count-i-1));

    /* Decrease the count of items in the list */
    v->count--;
            
    /* Reallocate the memory used */
    v->cell = realloc(v->cell, sizeof(tval*) * v->count);
    return x;
              
}

tval* tval_take(tval* v, int i) {
    tval* x = tval_pop(v, i);
    tval_del(v);
    return x;      
}

void tval_print(tval* v);

void tval_expr_print(tval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {
        /* Print Value contained within */
        tval_print(v->cell[i]);

        /* Don't print trailing space if last element */
        if (i != (v->count-1)) {
            putchar(' ');         
        }

    }
    
    putchar(close);     
}

/* Print tval */
void tval_print(tval* v){
    switch(v->type){
        case TVAL_NUM:
            printf("%li", v->num);
            break;
        case TVAL_ERR:
           printf("Error: %s", v->err);
           break;
        case TVAL_SYM:
           printf("%s", v->sym);
           break;
        case TVAL_SEXPR:
           tval_expr_print(v, '(', ')');
           break;
    }
}


void tval_println(tval* v) { tval_print(v); putchar('\n');  }

tval* builtin_op(tval* a, char* op) {
    /* Ensure all arguments are numbers */
    for (int i = 0; i < a->count; i++) {
        if (a->cell[i]->type != TVAL_NUM) {
            tval_del(a);
            return tval_err("Cannot operate on non-number!");                
        }
    }
      
    /* Pop the first element */
    tval* x = tval_pop(a, 0);
        
    /* If no arguments and sub then perform unary negation */
    if ((strcmp(op, "-") == 0) && a->count == 0) {
        x->num = -x->num;       
    }
        
    /* While there are still elements remaining */
    while (a->count > 0) {
          
    /* Pop the next element */
    tval* y = tval_pop(a, 0);
                  
    /* Perform operation */
    if (strcmp(op, "+") == 0) { x->num += y->num;  }
    if (strcmp(op, "-") == 0) { x->num -= y->num;  }
    if (strcmp(op, "*") == 0) { x->num *= y->num;  }
    if (strcmp(op, "/") == 0) {
    if (y->num == 0) {
        tval_del(x); tval_del(y);
        x = tval_err("Division By Zero.");
        break;
    }
    x->num /= y->num;
                                        
    }
                              
    /* Delete element now finished with */
    tval_del(y);
                                
    }
        
    /* Delete input expression and return result */
    tval_del(a);
    return x;
          
}

tval* tval_eval(tval* v);

tval* tval_eval_sexpr(tval* v) {
      
    /* Evaluate Children */
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = tval_eval(v->cell[i]);      
    }
      
    /* Error Checking */
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == TVAL_ERR) { return tval_take(v, i);  }
    }
      
    /* Empty Expression */
    if (v->count == 0) { return v;  }
        
    /* Single Expression */
    if (v->count == 1) { return tval_take(v, 0);  }      
          
    /* Ensure First Element is Symbol */
    tval* f = tval_pop(v, 0);
    if (f->type != TVAL_SYM) {
        tval_del(f); tval_del(v);
        return tval_err("S-expression Does not start with symbol.");
    }
            
    /* Call builtin with operator */
    tval* result = builtin_op(v, f->sym);
    tval_del(f);
    return result;
                
}

tval* tval_eval(tval* v) {
    /* Evaluate Sexpressions */
    if (v->type == TVAL_SEXPR) { return tval_eval_sexpr(v);  }
    /* All other tval types remain the same */
    return v;    
}

tval* tval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ?
    tval_num(x) : tval_err("invalid number");      
}


tval* tval_read(mpc_ast_t* t) {
      
    /* If Symbol or Number return conversion to that type */
    if (strstr(t->tag, "number")) { return tval_read_num(t);  }
    if (strstr(t->tag, "symbol")) { return tval_sym(t->contents);  }
          
    /* If root (>) or sexpr then create empty list */
    tval* x = NULL;
    if (strcmp(t->tag, ">") == 0) { x = tval_sexpr();  } 
    if (strstr(t->tag, "sexpr"))  { x = tval_sexpr();  }
                
    /* Fill this list with any valid expression contained within */
    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue;  }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue;  }
        if (strcmp(t->children[i]->tag,  "regex") == 0) { continue;  }
        x = tval_add(x, tval_read(t->children[i]));
    }
          
    return x;
              
}

int main(int argc, char** argv){

    /* Create some parsers */
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Tertius = mpc_new("tertius");

    /* Definition */

    mpca_lang(MPCA_LANG_DEFAULT,
            "                                                   \
            number   : /-?[0-9]+/ ;                             \
            symbol   : '+' | '-' | '*' | '/' | '%' | '^' ;      \
            sexpr    : '(' <expr>* ')'  ;                       \
            expr     : <number> | <symbol> | <sexpr> ;          \
            tertius  : /^/ <expr>* /$/ ;                        \
            ",
            Number, Symbol, Sexpr, Expr, Tertius);

    

    /* Version and exit info */
    puts("Tertius v0.0.1");
    puts("Press ctrl+c to exit\n");


    while(1){

        /* Output prompt */
        char* input = readline("tertius> ");
        
        /* Add input to history */
        add_history(input);

        /* Parse the user input */
        mpc_result_t r;
        if(mpc_parse("<stdin>", input, Tertius, &r)) {
            tval* x = tval_eval(tval_read(r.output));
            tval_println(x);
            tval_del(x);
            mpc_ast_delete(r.output);
        } else {
            /* Print error */
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        /* Free the retreived input */
        free(input);

    }
    
    /* Cleanup Parsers*/
    mpc_cleanup(4, Number, Symbol, Sexpr, Expr, Tertius);

    return 0;
}
