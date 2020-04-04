#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"

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
            operator : '+' | '-' | '*' | '/' ;                  \
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
            /* On success print the AST */
            mpc_ast_print(r.output);
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
