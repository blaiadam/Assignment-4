#include "token.h"
#include "data.h"
#include "symbol.h"
#include <string.h>
#include <stdlib.h>

/**
 * This pointer is set when by codeGenerator() func and used by printEmittedCode() func.
 * 
 * You are not required to use it anywhere. The implemented part of the skeleton
 * handles the printing. Instead, you are required to fill the vmCode properly by making
 * use of emit() func.
 * */
FILE* _out;

/**
 * Token list iterator used by the code generator. It will be set once entered to
 * codeGenerator() and reset before exiting codeGenerator().
 * 
 * It is better to use the given helper functions to make use of token list iterator.
 * */
TokenListIterator _token_list_it;

/**
 * Current level. Use this to keep track of the current level for the symbol table entries.
 * */
unsigned int currentLevel;

/**
 * Current scope. Use this to keep track of the current scope for the symbol table entries.
 * NULL means global scope.
 * */
Symbol* currentScope;

/**
 * Symbol table.
 * */
SymbolTable symbolTable;

/**
 * The array of instructions that the generated(emitted) code will be held.
 * */
Instruction vmCode[MAX_CODE_LENGTH];

/**
 * The next index in the array of instructions (vmCode) to be filled.
 * */
int nextCodeIndex;

/**
 * The id of the register currently being used.
 * */
int currentReg;

/**
 * Emits the instruction whose fields are given as parameters.
 * Internally, writes the instruction to vmCode[nextCodeIndex] and returns the
 * nextCodeIndex by post-incrementing it.
 * If MAX_CODE_LENGTH is reached, prints an error message on stderr and exits.
 * */
int emit(int OP, int R, int L, int M);

/**
 * Prints the emitted code array (vmCode) to output file.
 * 
 * This func is called in the given codeGenerator() function. You are not required
 * to have another call to this function in your code.
 * */
void printEmittedCodes();

/**
 * Returns the current token using the token list iterator.
 * If it is the end of tokens, returns token with id nulsym.
 * */
Token getCurrentToken();

/**
 * Returns the type of the current token. Returns nulsym if it is the end of tokens.
 * */
int getCurrentTokenType();

/**
 * Advances the position of TokenListIterator by incrementing the current token
 * index by one.
 * */
void nextToken();

/**
 * Functions used for non-terminals of the grammar
 * 
 * rel-op func is removed on purpose. For code generation, it is easier to parse
 * rel-op as a part of condition.
 * */
int program();
int block();
int const_declaration();
int var_declaration();
int proc_declaration();
int statement();
int condition();
int expression();
int term();
int factor();

/******************************************************************************/
/* Definitions of helper functions starts *************************************/
/******************************************************************************/

Token getCurrentToken()
{
    return getCurrentTokenFromIterator(_token_list_it);
}

int getCurrentTokenType()
{
    return getCurrentToken().id;
}

void nextToken()
{
    _token_list_it.currentTokenInd++;
}

/**
 * Given the code generator error code, prints error message on file by applying
 * required formatting.
 * */
void printCGErr(int errCode, FILE* fp)
{
    if(!fp || !errCode) return;

    fprintf(fp, "CODE GENERATOR ERROR[%d]: %s.\n", errCode, codeGeneratorErrMsg[errCode]);
}

int emit(int OP, int R, int L, int M)
{
    if(nextCodeIndex == MAX_CODE_LENGTH)
    {
        fprintf(stderr, "MAX_CODE_LENGTH(%d) reached. Emit is unsuccessful: terminating code generator..\n", MAX_CODE_LENGTH);
        exit(0);
    }
    
    vmCode[nextCodeIndex] = (Instruction){ .op = OP, .r = R, .l = L, .m = M};    

    return nextCodeIndex++;
}

void printEmittedCodes()
{
    for(int i = 0; i < nextCodeIndex; i++)
    {
        Instruction c = vmCode[i];
        fprintf(_out, "%d %d %d %d\n", c.op, c.r, c.l, c.m);
    }
}

/******************************************************************************/
/* Definitions of helper functions ends ***************************************/
/******************************************************************************/

/**
 * Advertised codeGenerator function. Given token list, which is possibly the
 * output of the lexer, parses a program out of tokens and generates code. 
 * If encountered, returns the error code.
 * 
 * Returning 0 signals successful code generation.
 * Otherwise, returns a non-zero code generator error code.
 * */
int codeGenerator(TokenList tokenList, FILE* out)
{
    // Set output file pointer
    _out = out;

    /**
     * Create a token list iterator, which helps to keep track of the current
     * token being parsed.
     * */
    _token_list_it = getTokenListIterator(&tokenList);

    // Initialize current level to 0, which is the global level
    currentLevel = 0;

    // Initialize current scope to NULL, which is the global scope
    currentScope = NULL;

    // The index on the vmCode array that the next emitted code will be written
    nextCodeIndex = 0;

    // The id of the register currently being used
    currentReg = 0;

    // Initialize symbol table
    initSymbolTable(&symbolTable);

    // Start parsing by parsing program as the grammar suggests.
    int err = program();

    // Print symbol table - if no error occured
    if(!err)
    {
        // Print the emitted codes to the file
        printEmittedCodes();
    }

    // Reset output file pointer
    _out = NULL;

    // Reset the global TokenListIterator
    _token_list_it.currentTokenInd = 0;
    _token_list_it.tokenList = NULL;

    // Delete symbol table
    deleteSymbolTable(&symbolTable);

    // Return err code - which is 0 if parsing was successful
    return err;
}

// Already implemented.
int program()
{
	// Generate code for block
    int err = block();
    if(err) return err;

    // After parsing block, periodsym should show up
    if( getCurrentTokenType() == periodsym )
    {
        // Consume token
        nextToken();

        // End of program, emit halt code
        emit(SIO_HALT, 0, 0, 3);

        return 0;
    }
    else
    {
        // Periodsym was expected. Return error code 6.
        return 6;
    }
}

int block()
{
    int err = 0;
	// Setup the jump address.
	int jmpAddr = emit(JMP, 0, 0, 0);
	
	// Check current token for constant, variable, or procedure type. Pass to
	// necessary functions and perform error check.
	if(getCurrentTokenType() == constsym && err == 0)
		err = const_declaration();
	if(err != 0)
		return err;
	
	if(getCurrentTokenType() == varsym && err == 0)
		err = var_declaration();
	if(err != 0)
		return err;
	
	if(getCurrentTokenType() == procsym && err == 0)
		err = proc_declaration();
	if(err != 0)
		return err;
	
	// Set procedure jump address.
	vmCode[jmpAddr].m = nextCodeIndex;
	emit(INC, 0, 0, 4);
	
	err = statement();
	if(err != 0)
		return err;
	
	emit(RTN, 0, 0, 0);
	
    return 0;
}

int const_declaration()
{
    // Do while loop parses constant declaration. Go until a comma isn't found.
    do
	{
		// Declare a new Symbol and set its initial values.
		Symbol *newSym = malloc(sizeof(Symbol));
		newSym->type = CONST;
		newSym->level = currentLevel;
		newSym->scope = currentScope;
		
		// Get next token and check that it is an identifier.
		nextToken();
		if(getCurrentTokenType() != identsym)
			return 3;
		// Update the symbol's name.
		strcpy(newSym->name, getCurrentToken().lexeme);
		
		// Get next token and check that it is an equal sign.
		nextToken();
		if(getCurrentTokenType() != eqsym)
			return 2;
		
		// Get the next token and check that it is a number.
		nextToken();
		if(getCurrentTokenType() != numbersym)
			return 1;
		// Update the symbol's value.
		newSym->value = atoi(getCurrentToken().lexeme);
		
		// Add the new symbol to the table.
		addSymbol(&symbolTable, *newSym);
		
		// Get next token.
		nextToken();
	} while(getCurrentTokenType() == commasym);
	
	// Check for semicolon and get the next token.
	if(getCurrentTokenType() != semicolonsym)
		return 10;
	nextToken();

    // Successful parsing.
    return 0;
}

int var_declaration()
{
	// Do while loop parses variable declaration. Go until a comma isn't found.
    do
	{
		// Declare a new Symbol and set its initial values.
		Symbol* newSym = malloc(sizeof(Symbol));
		newSym->type = VAR;
		newSym->level = currentLevel;
		newSym->scope = currentScope;
		newSym->address = nextCodeIndex;
		
		// Get next token and check that it is an identifier.
		nextToken();
		if(getCurrentTokenType() != identsym)
			return 3;
		// Update the symbol's name.
		strcpy(newSym->name, getCurrentToken().lexeme);
		
		// Add the new symbol to the table and INC for the variable.
		addSymbol(&symbolTable, *newSym);
		emit(INC, 0, 0, 1);
		
		// Get the next token.
		nextToken();
	} while(getCurrentTokenType() == commasym);
	
	// Check for semicolon and get the next token.
	if(getCurrentTokenType() != semicolonsym)
		return 4;
	nextToken();

    return 0;
}

int proc_declaration()
{
    // Error variable for tracking error codes.
	int err = 0;
	
	// While loop parses procedure declaration.
    while(getCurrentTokenType() == procsym)
	{
		// Declare a new Symbol and set its initial values.
		Symbol* tempSym = currentScope;
		Symbol* newSym = malloc(sizeof(Symbol));
		newSym->type = PROC;
		newSym->level = currentLevel;
		newSym->scope = currentScope;
		newSym->address = nextCodeIndex;
		
		// Get next token and check that it is an identifier.
		nextToken();
		if(getCurrentTokenType() != identsym)
			return 3;
		// Update the symbol's name.
		strcpy(newSym->name, getCurrentToken().lexeme);
		
		// Add the new symbol to the table.
		addSymbol(&symbolTable, *newSym);
		
		// Get next token and check that it is a semicolon.
		nextToken();
		if(getCurrentTokenType() != semicolonsym)
			return 5;
		
		// Get next token.
		nextToken();
		
		
		// Increment the current level for the next block and decrement it after 
		// the block is finished. Also set the scope before and after block.
		currentScope = newSym;
		currentLevel++;
		
		err = block();
		if(err != 0)
			return err;
		
		currentLevel--;
		currentScope = tempSym;
		
		// Check for semicolon after new block.
		if(getCurrentTokenType() != semicolonsym)
			return 5;
		
		// Get next token.
		nextToken();
	}

    return 0;
}

int statement()
{
    // Error variable for tracking error codes.
	int err = 0, jmp, jmp2;
	Symbol* currSym;
	
	// Statement that begins with an identifier symbol.
    if(getCurrentTokenType() == identsym)
	{	
		currSym = findSymbol(&symbolTable, currentScope, getCurrentToken().lexeme);
		
		// Check the scope and type of current symbol.
		if(currSym->scope != currentScope)
			return 15;
		if(currSym->type != VAR)
			return 16;
		
		// Get next token and check if it is a become symbol.
		nextToken();
		if(getCurrentTokenType() != becomessym)
			return 7;
		
		// Get next token and pass to expression.
		nextToken();
		err = expression();
		if(err != 0)
			return err;
	}
	// Statement that begins with a call symbol.
	else if(getCurrentTokenType() == callsym)
	{
		// Get next token and check if it is an identifier.
		nextToken();
		if(getCurrentTokenType() != identsym)
			return 8;
		
		currSym = findSymbol(&symbolTable, currentScope, getCurrentToken().lexeme);
		
		// Check scope and type of current symbol.
		if(currSym->scope != currentScope)
			return 15;
		if(currSym->type == PROC)
			emit(CAL, 0, currentLevel - currSym->level, currSym->address);
		else
			return 17;
		
		// Get next token.
		nextToken();
	}
	// Statement that begins with begin symbol.
	else if(getCurrentTokenType() == beginsym)
	{
		// Get next token and pass to statement.
		nextToken();
		err = statement();
		if(err != 0)
			return err;
		
		while (getCurrentTokenType() == semicolonsym)
		{
			// Get next token and pass to statement.
			nextToken();
			err = statement();
			if(err != 0)
				return err;
		}
		
		// Check for end symbol and get the next token.
		if(getCurrentTokenType() != endsym)
			return 10;
		nextToken();
	}
	// Statement that begins with if symbol.
	else if(getCurrentTokenType() == ifsym)
	{
		// Get next token and pass to condition.
		nextToken();
		err = condition();
		if(err != 0)
			return err;
		
		// Check the token is a then symbol.
		if(getCurrentTokenType() != thensym)
			return 9;
		
		// Get next token.
		nextToken();
		
		// Set jump address.
		jmp = nextCodeIndex;
		emit(JPC, 0, 0, 0);
		
		// Run statement and check for error.
		err = statement();
		if(err != 0)
			return err;
		
		// Update jump address.
		vmCode[jmp].m = nextCodeIndex;
		
		// Check for else statement. Get the next token and pass
		// to statement if an else token is the current token.
		if(getCurrentTokenType() == elsesym)
		{
			// Set else jump address.
			jmp2 = nextCodeIndex;
			emit(JMP, 0, 0, 0);
			
			// Get the next token and update else jump address.
			nextToken();
			vmCode[jmp2].m = nextCodeIndex;
			
			// Run statement and check for error.
			err = statement();
			if(err != 0)
				return err;
			
			vmCode[jmp].m = nextCodeIndex;
		}
	}
	// Statement that begins with while symbol.
	else if(getCurrentTokenType() == whilesym)
	{
		jmp = nextCodeIndex;
		
		// Get next token and pass to condition.
		nextToken();
		err = condition();
		if(err != 0)
			return err;
		
		jmp2 = nextCodeIndex;
		emit(JPC, 0, 0, 0);
		
		// Check the token is a do symbol.
		if(getCurrentTokenType() != dosym)
			return 11;
		
		// Get next token and pass to statement.
		nextToken();
		err = statement();
		if(err != 0)
			return err;
		
		// Jump back to while.
		emit(JMP, 0, 0, jmp);
		vmCode[jmp2].m = nextCodeIndex;
	}
	// Statement that begins with write symbol.
	else if(getCurrentTokenType() == writesym)
	{
		// Get next token and check if its an identifier.
		nextToken();
		if(getCurrentTokenType() != identsym)
			return 3;
		
		// Get current symbol and check its scope and type.
		currSym = findSymbol(&symbolTable, currentScope, getCurrentToken().lexeme);
		if(currSym->scope != currentScope)
			return 15;
		if(currSym->type == PROC)
			return 18;
		
		emit(LOD, 0, currentLevel - currSym->level, currSym->address);
		emit(SIO_WRITE, 0, 0, 0);
		
		// Get next token.
		nextToken();
	}
	// Statement that begins with read symbol.
	else if(getCurrentTokenType() == readsym)
	{
		emit(SIO_READ, 0, 0, 0);
		
		// Get next token and check if its an identifier.
		nextToken();
		if(getCurrentTokenType() != identsym)
			return 3;
		
		// Get current symbol and check its scope and type.
		currSym = findSymbol(&symbolTable, currentScope, getCurrentToken().lexeme);
		if(currSym->scope != currentScope)
			return 15;
		if(currSym->type != VAR)
			return 19;
		
		// Get next token.
		nextToken();
		emit(STO, 0, currentLevel - currSym->level, currSym->address);
	}

    return 0;
}

int condition()
{
	int err = 0;
	
	// Check for odd symbols.
	if(getCurrentTokenType() == oddsym)
	{
		nextToken();
		
		// Run expression and then emit the ODD.
		err = expression();
		if(err != 0)
			return err;
		
		emit(ODD, 0, 0, 0);
	}
	else
	{
		// Run expression then check for relational operations.
		err = expression();
		if(err != 0)
			return err;
		
		if(getCurrentTokenType() == eqsym)
			emit(EQL, 0, 0, 0);
		else if(getCurrentTokenType() == neqsym)
			emit(NEQ, 0, 0, 0);
		else if(getCurrentTokenType() == leqsym)
			emit(LEQ, 0, 0, 0);
		else if(getCurrentTokenType() == geqsym)
			emit(GEQ, 0, 0, 0);
		else if(getCurrentTokenType() == lessym)
			emit(LSS, 0, 0, 0);
		else if(getCurrentTokenType() == gtrsym)
			emit(GTR, 0, 0, 0);
		else
			return 12;
		
		nextToken();
	}
	
	err = expression();
	if(err != 0)
		return err;
	
    return 0;
}

int expression()
{
	// Error variable for tracking error codes.
	int err = 0;
	int op = getCurrentTokenType();
	
	// Get the next token if the current is a plus or minus sign.
    if(op == plussym || op == minussym)
	{
		nextToken();
		
		err = term();
		if(err != 0)
			return err;
		
		if(op == minussym)
			emit(NEG, 0, 0, 0);
	} 
	
	err = term();
	if(err != 0)
		return err;
	
	// Continue parsing until the end of the expression.
	while(op == plussym || op == minussym)
	{
		nextToken();
		
		err = term();
		if(err != 0)
			return err;
		
		if(op == plussym)
			emit(ADD, 0, 0, 0);
		else
			emit(SUB, 0, 0, 0);
	}

    return 0;
}

int term()
{
    // Error variable for tracking errors.
	int err = 0;
	int op = getCurrentTokenType();
	
    err = factor();
	if(err != 0)
		return err;
	
	// Continue parsing until the end of the term expression.
	while(op == multsym || op == slashsym)
	{
		nextToken();
		
		err = factor();
		if(err != 0)
			return err;
		
		if(op == multsym)
			emit(MUL, 0, 0, 0);
		else
			emit(DIV, 0, 0, 0);
	}

    return 0;
}

int factor()
{
	// Create current symbol and check for symbol scope.
	Symbol* currSym = findSymbol(&symbolTable, currentScope, getCurrentToken().lexeme);
	if(currSym == NULL)
		return 15;
	
    // Is the current token a identsym?
    if(getCurrentTokenType() == identsym)
    {	
		// Check current symbol type.
		if(currSym->type == PROC)
			return 14;
		else if(currSym->type == CONST)
			emit(LIT, 0, 0, currSym->value);
		else
			emit(LOD, 0, currentLevel - currSym->level, currSym->address);
		
        // Consume identsym
        nextToken(); // Go to the next token..

        // Success
        return 0;
    }
    // Is that a numbersym?
    else if(getCurrentTokenType() == numbersym)
    {	
		int value = atoi(getCurrentToken().lexeme);
		emit(LIT, 0, 0, value);
		
        // Consume numbersym
        nextToken(); // Go to the next token..

        // Success
        return 0;
    }
    // Is that a lparentsym?
    else if(getCurrentTokenType() == lparentsym)
    {
        // Consume lparentsym
        nextToken(); // Go to the next token..

        // Continue by parsing expression.
        int err = expression();

        /**
         * If parsing of expression was not successful, immediately stop parsing
         * and propagate the same error code by returning it.
         * */
        
        if(err) return err;

        // After expression, right-parenthesis should come
        if(getCurrentTokenType() != rparentsym)
        {
            /**
             * Error code 13: Right parenthesis missing.
             * Stop parsing and return error code 13.
             * */
            return 13;
        }
		
        // It was a rparentsym. Consume rparentsym.
        nextToken(); // Go to the next token..
    }
    else
    {
        /**
          * Error code 24: The preceding factor cannot begin with this symbol.
          * Stop parsing and return error code 24.
          * */
        return 14;
    }

    return 0;
}