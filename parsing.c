#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"

/* Declare new tval struct */
typedef struct {
    int type;
    long num;
    int err;
} tval;

/* Possible tval types */
enum { TVAL_NUM, TVAL_ERR };

/* Possible error types */
enum { TERR_DIV_ZERO, TERR_BAD_OP, TERR_BAD_NUM };

/* Create new number type */
tval tval_num(long x){
    tval v;
    v.type = TVAL_NUM;
    v.num = x;
    return v;
}

/* Create a new error type */
tval tval_err(int x){
    tval v;
    v.type = TVAL_ERR;
    v.err = x;
    return v;
}

/* Print tval */
void tval_print(tval v){
    switch(v.type){
        case TVAL_NUM:
            printf("%li", v.num);
            break;
        case TVAL_ERR:
            if (v.err == TERR_DIV_ZERO) {
                printf("Error: Division By Zero!");              
            }
            if (v.err == TERR_BAD_OP) {
                printf("Error: Invalid Operator!");              
            }
            if (v.err == TERR_BAD_NUM)  {
                printf("Error: Invalid Number!");              
            }
            break;
    }
}

/* Print the tval with new line*/
void tval_println(tval v){
    tval_print(v);
    putchar('\n');
}

/* Use the operator so see what to perform */
tval evaluate_op(tval x, char* op, tval y){
    
    /* If any value is an error return it */
    if (x.type == TVAL_ERR){ return x; }
    if (y.type == TVAL_ERR){ return y; }
    
    /* Addition */
    if(strcmp(op, "+") == 0){return tval_num(x.num + y.num); }
    /* Subtraction */
    if(strcmp(op, "-") == 0){return tval_num(x.num - y.num); }
    /* Multiplication */
    if(strcmp(op, "*") == 0){return tval_num(x.num * y.num); }
    /* Division */
    if(strcmp(op, "/") == 0){
        return y.num == 0
            ? tval_err(TERR_DIV_ZERO)
            : tval_num(x.num / y.num);
    }
    /* Remainder of a division */
    if(strcmp(op, "%") == 0){
        return y.num == 0
            ? tval_err(TERR_DIV_ZERO)
            : tval_num(x.num % y.num);
    }
    /* Power of */
    if(strcmp(op, "^") == 0){return tval_num(pow(x.num, y.num)); }


    return tval_err(TERR_BAD_OP);
}

tval eval(mpc_ast_t* t){
    /* If number - Return the number*/
    if(strstr(t->tag, "number")) {
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? tval_num(x) : tval_err(TERR_BAD_NUM);
    }
   
    /* The operator is always the second child */
    char* op = t->children[1]->contents;
    tval x = eval(t->children[2]);
    
    /* Iterate the remaining children */
    int i = 3;
    while(strstr(t->children[i]->tag, "expr")){
        x = evaluate_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

int main(int argc, char** argv){

    /* Create some parsers */
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Tertius = mpc_new("tertius");

    /* Definition */

    mpca_lang(MPCA_LANG_DEFAULT,
            "                                                   \
            number   : /-?[0-9]+/ ;                             \
            operator : '+' | '-' | '*' | '/' | '%' | '^' ;      \
            expr     : <number> | '(' <operator> <expr>+ ')' ;  \
            tertius  : /^/ <operator> <expr>+ /$/ ;             \
            ",
            Number, Operator, Expr, Tertius);

    

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
            tval result = eval(r.output);
            tval_println(result);
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
    mpc_cleanup(4, Number, Operator, Expr, Tertius);

    return 0;
}
