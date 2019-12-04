// Transpiler for language D Flat (learning purposes)
//
// ---------
// --LEXER--
// ---------
// Heavily based on Sean T. Barrett's C lexer which can be found at:
// https://github.com/nothings/stb/blob/master/stb_c_lexer.h
//

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum token
{
    TOKEN_eof = 256,
    TOKEN_parse_error,

    TOKEN_id,
    TOKEN_int_number,
    TOKEN_real_number,
    TOKEN_char_number,
    TOKEN_string_text,

    // Types
    TOKEN_char,
    TOKEN_int,
    TOKEN_float,
    TOKEN_string,

    // Control
    TOKEN_if,
    TOKEN_else,
    TOKEN_for,
    TOKEN_return,

    // Operators
    TOKEN_double_colon,
    TOKEN_coloneq,
    TOKEN_pluseq,
    TOKEN_minuseq,
    TOKEN_muleq,
    TOKEN_diveq,
    TOKEN_modeq,
    TOKEN_plusplus,
    TOKEN_minusminus,
    TOKEN_arrow,

    TOKEN_eq,
    TOKEN_noteq,
    TOKEN_lesseq,
    TOKEN_moreeq,
    TOKEN_andand,
    TOKEN_oror,

    TOKEN_inline,
};

struct lexer
{
    // Lexer variables
    char* InputStream;
    char* EndOfFile;
    char* ParsePoint;
    char* StringStorage;
    int32_t StringStorageLength;

    // Lexer parse location for error messages
    char* FirstChar;
    char* LastChar;

    // Lexer token variables
    int32_t Token;
    double RealNumber;
    uint64_t IntNumber;
    char* String;
    int32_t StringLength;
};

struct location
{
    int32_t LineNumber;
    int32_t LineOffset;
};

static void InitLexer(lexer* Lexer, const char* InputStream, const char* InputStreamEnd, char* StringStorage, int32_t StringStorageLength)
{
    Lexer->InputStream = (char*)InputStream;
    Lexer->EndOfFile = (char*)InputStreamEnd;
    Lexer->ParsePoint = Lexer->InputStream;
    Lexer->StringStorage = StringStorage;
    Lexer->StringStorageLength = StringStorageLength;
}

static void GetLocation(location* Location, lexer* Lexer, char* CurrentPoint)
{
    char* ParsePoint = Lexer->InputStream;
    int32_t LineNumber = 1;
    int32_t CharOffset = 0;
    while((*ParsePoint) && (ParsePoint <= CurrentPoint))
    {
        if((*ParsePoint == '\n') || (*ParsePoint == '\r'))
        {
            ParsePoint += (ParsePoint[0] + ParsePoint[1] == '\r' + '\n' ? 2 : 1);
            ++LineNumber;
            CharOffset = 0;
        }
        else
        {
            ++ParsePoint;
            ++CharOffset;
        }
    }
    Location->LineNumber = LineNumber;
    Location->LineOffset = CharOffset;
}

static int32_t Tokenize(lexer* Lexer, int32_t Token, char* Start, char* End)
{
    Lexer->Token = Token;
    Lexer->FirstChar = Start;
    Lexer->LastChar = End;
    Lexer->ParsePoint = End + 1;
    return 1;
}

static int32_t TokenizeEOF(lexer* Lexer)
{
    Lexer->Token = TOKEN_eof;
    return 0;
}

static int32_t ParseChar(char* ParsePoint, char** NextPoint)
{
    if(*ParsePoint == '\\')
    {
        *NextPoint = ParsePoint + 2;
        switch(ParsePoint[1])
        {
            case '\\':
            {
                return '\\';
            }
            case '\'':
            {
                return '\'';
            }
            case '"':
            {
                return '"';
            }
            case 't':
            {
                return '\t';
            }
            case 'r':
            {
                return '\r';
            }
            case 'n':
            {
                return '\n';
            }
            case 'f':
            {
                return '\f';
            }
        }
    }
    *NextPoint = ParsePoint + 1;
    return *ParsePoint;
}

static int32_t ParseString(lexer* Lexer, char* ParsePoint)
{
    char* Start = ParsePoint;
    char* Output = Lexer->StringStorage;
    char* OutputEnd = Lexer->StringStorage + Lexer->StringStorageLength;
    while(*ParsePoint != '"')
    {
        int Character;
        if(*ParsePoint == '\\')
        {
            char* NextPoint;
            Character = ParseChar(ParsePoint, &NextPoint);

            if(Character < 0)
            {
                return Tokenize(Lexer, TOKEN_parse_error, Start, NextPoint);
            }
            ParsePoint = NextPoint;
        }
        else
        {
            Character = *ParsePoint++;
        }
        
        if(Output + 1 > OutputEnd)
        {
            return Tokenize(Lexer, TOKEN_parse_error, Start, ParsePoint);
        }
        *Output++ = (char)Character;
    }
    *Output++ = '\0';
    Lexer->String = Lexer->StringStorage;
    Lexer->StringLength = (int32_t)(Output - Lexer->StringStorage);
    return Tokenize(Lexer, TOKEN_string_text, Start, ParsePoint);
}

static token MatchToken(lexer* Lexer)
{
    if(strcmp(Lexer->String, "char") == 0)
    {
        return TOKEN_char;
    }
    if(strcmp(Lexer->String, "int") == 0)
    {
        return TOKEN_int;
    }
    if(strcmp(Lexer->String, "float") == 0)
    {
        return TOKEN_float;
    }
    if(strcmp(Lexer->String, "string") == 0)
    {
        return TOKEN_string;
    }
    if(strcmp(Lexer->String, "if") == 0)
    {
        return TOKEN_if;
    }
    if(strcmp(Lexer->String, "else") == 0)
    {
        return TOKEN_else;
    }
    if(strcmp(Lexer->String, "for") == 0)
    {
        return TOKEN_for;
    }
    if(strcmp(Lexer->String, "return") == 0)
    {
        return TOKEN_return;
    }
    return TOKEN_id;
}

static bool IsWhitespace(char Character)
{
    return Character == ' ' || Character == '\t' || Character == '\r' || Character == '\n' || Character == '\f';
}

static bool IsUppercase(char Character)
{
    return (Character >= 'A') && (Character <= 'Z');
}

static bool IsLowercase(char Character)
{
    return (Character >= 'a') && (Character <= 'z');
}

static bool IsLetter(char Character)
{
    return IsUppercase(Character) || IsLowercase(Character) || Character == '_';
}

static bool IsDigit(char Character)
{
    return (Character >= '0') && (Character <= '9');
}

static bool IsSymbol(char Character)
{
    return IsLetter(Character) || IsDigit(Character);
}

static bool IsType(int32_t Token)
{
    return (Token == TOKEN_char) || (Token == TOKEN_int) || (Token == TOKEN_float) || (Token == TOKEN_string);
}

static bool IsBinaryOperator(int32_t Token)
{
    return (Token == '+') || (Token == '-') || (Token == '*') || (Token == '/') || (Token == '%') || (Token == '<') || (Token == '>') || (Token == TOKEN_eq) ||
            (Token == TOKEN_noteq) || (Token == TOKEN_lesseq) || (Token == TOKEN_moreeq) || (Token == TOKEN_andand) || (Token == TOKEN_oror);
}

static int32_t GetToken(lexer* Lexer)
{
    char* ParsePoint = Lexer->ParsePoint;

    // Skipping whitespace and comments
    for(;;)
    {
        while((ParsePoint < Lexer->EndOfFile) && IsWhitespace(*ParsePoint))
        {
            ++ParsePoint;
        }

        if((ParsePoint < Lexer->EndOfFile) && (ParsePoint[0] == '/') && (ParsePoint[1] == '/'))
        {
            while((ParsePoint < Lexer->EndOfFile) && (*ParsePoint != '\r') && (*ParsePoint != '\n'))
            {
                ++ParsePoint;
            }
            continue;
        }

        if((ParsePoint < Lexer->EndOfFile) && (ParsePoint[0] == '/') && (ParsePoint[1] == '*'))
        {
            char* Start = ParsePoint;
            ParsePoint += 2;

            while((ParsePoint < Lexer->EndOfFile) && ((ParsePoint[0] != '*') || (ParsePoint[1] != '/')))
            {
                ++ParsePoint;
            }
            
            if(ParsePoint == Lexer->EndOfFile)
            {
                return Tokenize(Lexer, TOKEN_parse_error, Start, ParsePoint - 1);
            }

            ParsePoint += 2;
            continue;
        }

        break;
    }

    if(ParsePoint >= Lexer->EndOfFile)
    {
        return TokenizeEOF(Lexer);
    }

    switch(*ParsePoint)
    {
        default:
        {
            if(IsLetter(*ParsePoint))
            {
                int32_t Length = 0;
                Lexer->String = Lexer->StringStorage;
                Lexer->StringLength = Length;

                do
                {
                    if(Length + 1 >= Lexer->StringStorageLength)
                    {
                        return Tokenize(Lexer, TOKEN_parse_error, ParsePoint, ParsePoint + Length);
                    }
                    Lexer->String[Length] = ParsePoint[Length];
                    ++Length;
                } while(IsSymbol(ParsePoint[Length]));
                Lexer->StringLength = Length;
                Lexer->String[Lexer->StringLength++] = '\0';
                return Tokenize(Lexer, MatchToken(Lexer), ParsePoint, ParsePoint + Length - 1);
            }
            else if (IsDigit(*ParsePoint))
            {
                char* NextPoint = ParsePoint;

                while((NextPoint < Lexer->EndOfFile) && IsDigit(*NextPoint))
                {
                    ++NextPoint;
                }
                if(NextPoint < Lexer->EndOfFile)
                {
                    if(*NextPoint == '.')
                    {
                        Lexer->RealNumber = strtod((char*)ParsePoint, (char**)&NextPoint);
                        return Tokenize(Lexer, TOKEN_real_number, ParsePoint, NextPoint - 1);
                    }
                    else
                    {
                        Lexer->IntNumber = strtoul((char*)ParsePoint, (char**)&NextPoint, 10);
                        return Tokenize(Lexer, TOKEN_int_number, ParsePoint, NextPoint - 1);
                    }
                }
            }
        }

single_char:
        return Tokenize(Lexer, *ParsePoint, ParsePoint, ParsePoint);

        case ':':
        {
            if(ParsePoint + 1 < Lexer->EndOfFile)
            {
                if(ParsePoint[1] == ':')
                {
                    return Tokenize(Lexer, TOKEN_double_colon, ParsePoint, ParsePoint + 1);
                }
                if(ParsePoint[1] == '=')
                {
                    return Tokenize(Lexer, TOKEN_coloneq, ParsePoint, ParsePoint + 1);
                }
            }
        }
        goto single_char;

        case '+':
        {
            if(ParsePoint + 1 < Lexer->EndOfFile)
            {
                if(ParsePoint[1] == '+')
                {
                    return Tokenize(Lexer, TOKEN_plusplus, ParsePoint, ParsePoint + 1);
                }
                if(ParsePoint[1] == '=')
                {
                    return Tokenize(Lexer, TOKEN_pluseq, ParsePoint, ParsePoint + 1);
                }
            }
        }
        goto single_char;

        case '-':
        {
            if(ParsePoint + 1 < Lexer->EndOfFile)
            {
                if(ParsePoint[1] == '-')
                {
                    return Tokenize(Lexer, TOKEN_minusminus, ParsePoint, ParsePoint + 1);
                }
                if(ParsePoint[1] == '=')
                {
                    return Tokenize(Lexer, TOKEN_minuseq, ParsePoint, ParsePoint + 1);
                }
                if(ParsePoint[1] == '>')
                {
                    return Tokenize(Lexer, TOKEN_arrow, ParsePoint, ParsePoint + 1);
                }
            }
        }
        goto single_char;

        case '*':
        {
            if(ParsePoint + 1 < Lexer->EndOfFile)
            {
                if(ParsePoint[1] == '=')
                {
                    return Tokenize(Lexer, TOKEN_muleq, ParsePoint, ParsePoint + 1);
                }
            }
        }
        goto single_char;

        case '/':
        {
            if(ParsePoint + 1 < Lexer->EndOfFile)
            {
                if(ParsePoint[1] == '=')
                {
                    return Tokenize(Lexer, TOKEN_diveq, ParsePoint, ParsePoint + 1);
                }
            }
        }
        goto single_char;

        case '%':
        {
            if(ParsePoint + 1 < Lexer->EndOfFile)
            {
                if(ParsePoint[1] == '=')
                {
                    return Tokenize(Lexer, TOKEN_modeq, ParsePoint, ParsePoint + 1);
                }
            }
        }
        goto single_char;

        case '=':
        {
            if(ParsePoint + 1 < Lexer->EndOfFile)
            {
                if(ParsePoint[1] == '=')
                {
                    return Tokenize(Lexer, TOKEN_eq, ParsePoint, ParsePoint + 1);
                }
            }
        }
        goto single_char;

        case '!':
        {
            if(ParsePoint + 1 < Lexer->EndOfFile)
            {
                if(ParsePoint[1] == '=')
                {
                    return Tokenize(Lexer, TOKEN_noteq, ParsePoint, ParsePoint + 1);
                }
            }
        }
        goto single_char;

        case '<':
        {
            if(ParsePoint + 1 < Lexer->EndOfFile)
            {
                if(ParsePoint[1] == '=')
                {
                    return Tokenize(Lexer, TOKEN_lesseq, ParsePoint, ParsePoint + 1);
                }
                if(ParsePoint[1] == '>')
                {
                    return Tokenize(Lexer, TOKEN_inline, ParsePoint, ParsePoint + 1);
                }
            }
        }
        goto single_char;

        case '>':
        {
            if(ParsePoint + 1 < Lexer->EndOfFile)
            {
                if(ParsePoint[1] == '=')
                {
                    return Tokenize(Lexer, TOKEN_moreeq, ParsePoint, ParsePoint + 1);
                }
            }
        }
        goto single_char;

        case '|':
        {
            if(ParsePoint + 1 < Lexer->EndOfFile)
            {
                if(ParsePoint[1] == '|')
                {
                    return Tokenize(Lexer, TOKEN_oror, ParsePoint, ParsePoint + 1);
                }
            }
        }
        goto single_char;

        case '&':
        {
            if(ParsePoint + 1 < Lexer->EndOfFile)
            {
                if(ParsePoint[1] == '&')
                {
                    return Tokenize(Lexer, TOKEN_andand, ParsePoint, ParsePoint + 1);
                }
            }
        }
        goto single_char;

        case '"':
        {
            return ParseString(Lexer, ParsePoint + 1);
        }
        goto single_char;

        case '\'':
        {
            char* Start = ParsePoint;

            Lexer->IntNumber = ParseChar(ParsePoint + 1, &ParsePoint);

            if(Lexer->IntNumber < 0)
            {
                return Tokenize(Lexer, TOKEN_parse_error, Start, Start);
            }
            if((ParsePoint == Lexer->EndOfFile) || (*ParsePoint != '\''))
            {
                return Tokenize(Lexer, TOKEN_parse_error, Start, ParsePoint);
            }
            return Tokenize(Lexer, TOKEN_char_number, Start, ParsePoint);
        }
        goto single_char;
    }
}

static void PrintToken(lexer* Lexer)
{
    switch(Lexer->Token)
    {
        case TOKEN_id:
        {
            printf("%s", Lexer->String);
        } break;
        case TOKEN_char:
        {
            printf("char");
        } break;
        case TOKEN_int:
        {
            printf("int");
        } break;
        case TOKEN_float:
        {
            printf("float");
        } break;
        case TOKEN_string:
        {
            printf("string");
        } break;
        case TOKEN_if:
        {
            printf("if");
        } break;
        case TOKEN_else:
        {
            printf("else");
        } break;
        case TOKEN_for:
        {
            printf("for");
        } break;
        case TOKEN_return:
        {
            printf("return");
        } break;
        case TOKEN_double_colon:
        {
            printf("::");
        } break;
        case TOKEN_coloneq:
        {
            printf(":=");
        } break;
        case TOKEN_pluseq:
        {
            printf("+=");
        } break;
        case TOKEN_minuseq:
        {
            printf("-=");
        } break;
        case TOKEN_muleq:
        {
            printf("*=");
        } break;
        case TOKEN_diveq:
        {
            printf("/=");
        } break;
        case TOKEN_modeq:
        {
            printf("%%=");
        } break;
        case TOKEN_plusplus:
        {
            printf("++");
        } break;
        case TOKEN_minusminus:
        {
            printf("--");
        } break;
        case TOKEN_arrow:
        {
            printf("->");
        } break;
        case TOKEN_eq:
        {
            printf("==");
        } break;
        case TOKEN_noteq:
        {
            printf("!=");
        } break;
        case TOKEN_lesseq:
        {
            printf("<=");
        } break;
        case TOKEN_moreeq:
        {
            printf(">=");
        } break;
        case TOKEN_andand:
        {
            printf("&&");
        } break;
        case TOKEN_oror:
        {
            printf("||");
        } break;
        case TOKEN_inline:
        {
            printf("<>");
        } break;
        case TOKEN_string_text:
        {
            printf("\"%s\"", Lexer->String);
        } break;
        case TOKEN_int_number:
        {
            printf("%llu", Lexer->IntNumber);
        } break;
        case TOKEN_real_number:
        {
            printf("%g", Lexer->RealNumber);
        } break;
        case TOKEN_char_number:
        {
            printf("'%c'", (char)Lexer->IntNumber);
        } break;
        default:
        {
            if((Lexer->Token >= 0) && (Lexer->Token < 256))
            {
                printf("%c", (int32_t)Lexer->Token);
            }
            else
            {
                printf("UNKNOWN TOKEN: %ld\n", Lexer->Token);
            }
        } break;
    }
}

// ----------
// --PARSER--
// ----------
// Heavily based on LLVM parser tutorial at:
// https://llvm.org/docs/tutorial/index.html
// Bits of code based on open-source Jai compiler/transpiler at:
// https://github.com/machinamentum/jai
//
// TODO(rytis): Check for memory leaks. Even better - implement dynamic allocator myself for guaranteed memory management process-wise.

#define MAX_STRING_COUNT 10000

struct string_storage
{
    int32_t Length;
    int32_t Capacity;
    char* Strings;

    uint32_t StringCount;
    char* StringArray[MAX_STRING_COUNT];
};

static void InitStringStorage(string_storage* StringStorage, const char* AllocatedSpace, int32_t StringStorageCapacity)
{
    StringStorage->Length = 0;
    StringStorage->Capacity = StringStorageCapacity;
    StringStorage->Strings = (char*)AllocatedSpace;

    StringStorage->StringCount = 0;
    StringStorage->StringArray[0] = StringStorage->Strings;
}

static int32_t AddStringToStorage(string_storage* Storage, char* String, uint32_t StringLength)
{
    Storage->StringArray[Storage->StringCount++] = &Storage->Strings[Storage->Length];

    for(uint32_t i = 0; i < StringLength; ++i)
    {
        if(Storage->Length <= Storage->Capacity)
        {
            Storage->Strings[Storage->Length++] = String[i];
        }
        else
        {
            printf("Error: string storage capacity exceeded!\n");
            return -1;
        }
    }

    return Storage->StringCount - 1;
}

#define MAX_PARAMETER_COUNT 10
#define MAX_EXPRESSION_COUNT 30

enum ast_type
{
    AST_expr,
    AST_func,
};

enum expr_type
{
    EXPR_char,
    EXPR_int,
    EXPR_real,
    EXPR_string,
    EXPR_id,
    EXPR_var,
    EXPR_paren,
    EXPR_binary,
    EXPR_call,
    EXPR_if,
    EXPR_for,
    EXPR_return,
    EXPR_inline,
};

struct char_expr
{
    char CharValue;
};

struct int_number_expr
{
    uint64_t IntValue;
};

struct real_number_expr
{
    double RealValue;
};

struct string_expr
{
    char* String;
};

struct id_expr
{
    char* String;
};

struct inline_expr
{
    char* Text;
};

struct expr
{
    expr_type ExprType;
    union
    {
        char_expr CharExpr;
        int_number_expr IntExpr;
        real_number_expr RealExpr;
        string_expr StringExpr;
        id_expr IdExpr;
        inline_expr InlineExpr;

        struct var_expr
        {
            int32_t Type;
            char* Name;
            expr* Expr;
        } VarExpr;

        struct paren_expr
        {
            expr* InnerExpr;
        } ParenExpr;

        struct binary_expr
        {
            int32_t Operator;
            expr* LHS;
            expr* RHS;
        } BinaryExpr;

        struct call_expr
        {
            char* Name;
            uint32_t ArgumentCount;
            expr* Arguments[MAX_PARAMETER_COUNT];
        } CallExpr;

        struct if_expr
        {
            expr* Statement;
            uint32_t TrueExpressionCount;
            expr* TrueExpressions[MAX_EXPRESSION_COUNT];
            uint32_t FalseExpressionCount;
            expr* FalseExpressions[MAX_EXPRESSION_COUNT];
        } IfExpr;

        struct for_expr
        {
            expr* Definition;
            expr* Condition;
            expr* Action;
            uint32_t ExpressionCount;
            expr* Expressions[MAX_EXPRESSION_COUNT];
        } ForExpr;
        
        struct return_expr
        {
            expr* Expression;
        } ReturnExpr;
    };
};

struct func
{
    int32_t Type;
    char* Name;
    uint32_t ParameterCount;
    expr* Parameters[MAX_PARAMETER_COUNT];
    uint32_t ExpressionCount;
    expr* Expressions[MAX_EXPRESSION_COUNT];
};

struct ast
{
    ast_type AstType;
    union
    {
        expr* Expr;
        func* Func;
    };
};

static void FreeExpression(expr* Expression)
{
    if(!Expression)
    {
        return;
    }

    switch(Expression->ExprType)
    {
        default:
        {
            fprintf(stderr, "Error: Unknown expression type.\n");
        } // Fall through
        case EXPR_char:
        {
        } // Fall through
        case EXPR_int:
        {
        } // Fall through
        case EXPR_real:
        {
        } // Fall through
        case EXPR_string:
        {
        } // Fall through
        case EXPR_id:
        {
        }
        case EXPR_inline:
        {
            free(Expression);
        } break;
        case EXPR_var:
        {
            FreeExpression(Expression->VarExpr.Expr);
            free(Expression);
        } break;
        case EXPR_paren:
        {
            FreeExpression(Expression->ParenExpr.InnerExpr);
            free(Expression);
        } break;
        case EXPR_binary:
        {
            expr* LHS = Expression->BinaryExpr.LHS;
            expr* RHS = Expression->BinaryExpr.RHS;
            FreeExpression(LHS);
            FreeExpression(RHS);
            free(Expression);
        } break;
        case EXPR_call:
        {
            uint32_t ArgumentCount = Expression->CallExpr.ArgumentCount;
            for(uint32_t i = 0; i < ArgumentCount; ++i)
            {
                FreeExpression(Expression->CallExpr.Arguments[i]);
            }
            free(Expression);
        } break;
        case EXPR_if:
        {
            uint32_t TrueCount = Expression->IfExpr.TrueExpressionCount;
            uint32_t FalseCount = Expression->IfExpr.FalseExpressionCount;
            for(uint32_t i = 0; i < TrueCount; ++i)
            {
                FreeExpression(Expression->IfExpr.TrueExpressions[i]);
            }
            for(uint32_t i = 0; i < FalseCount; ++i)
            {
                FreeExpression(Expression->IfExpr.FalseExpressions[i]);
            }
            FreeExpression(Expression->IfExpr.Statement);
            free(Expression);
        } break;
        case EXPR_for:
        {
            FreeExpression(Expression->ForExpr.Definition);
            FreeExpression(Expression->ForExpr.Condition);
            FreeExpression(Expression->ForExpr.Action);
            uint32_t ExprCount = Expression->ForExpr.ExpressionCount;
            for(uint32_t i = 0; i < ExprCount; ++i)
            {
                FreeExpression(Expression->ForExpr.Expressions[i]);
            }
            free(Expression);
        } break;
        case EXPR_return:
        {
            FreeExpression(Expression->ReturnExpr.Expression);
            free(Expression);
        } break;
    }
}

static void FreeFunction(func* Function)
{
    if(!Function)
    {
        // printf("Error: NULL pointer received when freeing statement.\n");
        return;
    }

    for(uint32_t i = 0; i < Function->ParameterCount; ++i)
    {
        FreeExpression(Function->Parameters[i]);
    }
    for(uint32_t i = 0; i < Function->ExpressionCount; ++i)
    {
        FreeExpression(Function->Expressions[i]);
    }
    free(Function);
}

static void FreeAst(ast* Ast)
{
    if(!Ast)
    {
        printf("Error: NULL pointer received when freeing AST.\n");
        return;
    }

    switch(Ast->AstType)
    {
        default:
        {
        } break;
        case AST_expr:
        {
            expr* Expression = Ast->Expr;
            // free(Ast);
            FreeExpression(Expression);
        } break;
        case AST_func:
        {
            func* Function = Ast->Func;
            // free(Ast);
            FreeFunction(Function);
        } break;
    }
}

static int32_t GetTokenPrecedence(int32_t Token)
{
    switch(Token)
    {
        default:
        {
            return -1;
        } break;
        case '=':
        {
            return 10;
        } break;
        case '+':
        {
            return 30;
        } break;
        case '-':
        {
            return 30;
        } break;
        case '*':
        {
            return 40;
        } break;
        case '/':
        {
            return 40;
        } break;
        case '%':
        {
            return 40;
        } break;
        case '<':
        {
            return 20;
        } break;
        case '>':
        {
            return 20;
        } break;
        case TOKEN_eq:
        {
            return 10;
        } break;
        case TOKEN_noteq:
        {
            return 10;
        } break;
        case TOKEN_lesseq:
        {
            return 20;
        } break;
        case TOKEN_moreeq:
        {
            return 20;
        } break;
        case TOKEN_andand:
        {
            return 40;
        } break;
        case TOKEN_oror:
        {
            return 30;
        } break;
    }
}

static void PrintLocationError(location* Location, char* String)
{
    fprintf(stderr, "|%d:%d| error: %s\n", Location->LineNumber, Location->LineOffset, String);
}

static expr* ExpressionExpectedError(lexer* Lexer, expr* Expression, char* String)
{
    location Location;
    GetLocation(&Location, Lexer, Lexer->FirstChar);
    PrintLocationError(&Location, strcat("expected ", String));
    FreeExpression(Expression);
    return NULL;
}

static int32_t PeekToken(lexer* Lexer)
{
    char* ParsePoint = Lexer->ParsePoint;
    int32_t OldToken = Lexer->Token;
    GetToken(Lexer);
    int32_t Token = Lexer->Token;
    Lexer->ParsePoint = ParsePoint;
    Lexer->Token = OldToken;
    return Token;
}

static expr* ParseCharExpr(lexer* Lexer)
{
    expr* Result = (expr*)malloc(sizeof(expr));
    Result->ExprType = EXPR_char;
    Result->CharExpr.CharValue = (char)Lexer->IntNumber;
    GetToken(Lexer);
    return Result;
}

static expr* ParseIntExpr(lexer* Lexer)
{
    expr* Result = (expr*)malloc(sizeof(expr));
    Result->ExprType = EXPR_int;
    Result->IntExpr.IntValue = Lexer->IntNumber;
    GetToken(Lexer);
    return Result;
}

static expr* ParseRealExpr(lexer* Lexer)
{
    expr* Result = (expr*)malloc(sizeof(expr));
    Result->ExprType = EXPR_real;
    Result->RealExpr.RealValue = Lexer->RealNumber;
    GetToken(Lexer);
    return Result;
}

static expr* ParseStringExpr(lexer* Lexer, string_storage* Storage)
{
    expr* Result = (expr*)malloc(sizeof(expr));
    Result->ExprType = EXPR_string;
    int32_t StringIndex = AddStringToStorage(Storage, Lexer->String, Lexer->StringLength);
    Result->StringExpr.String = Storage->StringArray[StringIndex];
    GetToken(Lexer);
    return Result;
}

static expr* ParseExpression(lexer* Lexer, string_storage* Storage);

static expr* ParseIdExpr(lexer* Lexer, string_storage* Storage)
{
    int32_t StringIndex = AddStringToStorage(Storage, Lexer->String, Lexer->StringLength);

    GetToken(Lexer);

    expr* Result = (expr*)malloc(sizeof(expr));

    if((Lexer->Token != '(') && (Lexer->Token != ':'))
    {
        Result->ExprType = EXPR_id;
        Result->IdExpr.String = Storage->StringArray[StringIndex];
        return Result;
    }

    switch(Lexer->Token)
    {
        default:
        {
            free(Result);
            return NULL;
        } break;
        case ':':
        {
            Result->ExprType = EXPR_var;
            Result->VarExpr.Name = Storage->StringArray[StringIndex];

            GetToken(Lexer);
            if(!IsType(Lexer->Token))
            {
                return ExpressionExpectedError(Lexer, Result, "type");
            }

            Result->VarExpr.Type = Lexer->Token;

            GetToken(Lexer);
            if(Lexer->Token == ';')
            {
                Result->VarExpr.Expr = NULL;
                return Result;
            }
            if(Lexer->Token != '=')
            {
                return ExpressionExpectedError(Lexer, Result, "=");
            }
            GetToken(Lexer);
            Result->VarExpr.Expr = ParseExpression(Lexer, Storage);
        } break;
        case '(':
        {
            Result->ExprType = EXPR_call;
            Result->CallExpr.Name = Storage->StringArray[StringIndex];

            Result->CallExpr.ArgumentCount = 0;
            GetToken(Lexer);

            for(;;)
            {
                uint32_t ArgumentCount = Result->CallExpr.ArgumentCount;
                Result->CallExpr.Arguments[ArgumentCount] = ParseExpression(Lexer, Storage);
                if(!Result->CallExpr.Arguments[ArgumentCount])
                {
                    FreeExpression(Result);
                    return NULL;
                }
                ++Result->CallExpr.ArgumentCount;

                if(Lexer->Token == ')')
                {
                    break;
                }

                if(Lexer->Token != ',')
                {
                    return ExpressionExpectedError(Lexer, Result, ", or )");
                }
                GetToken(Lexer);
            }
            GetToken(Lexer);
        } break;
    }
    return Result;
}

static expr* ParseParenExpr(lexer* Lexer, string_storage* Storage)
{
    assert(Lexer->Token == '(');
    GetToken(Lexer);
    expr* Result = (expr*)malloc(sizeof(expr));
    Result->ExprType = EXPR_paren;
    Result->ParenExpr.InnerExpr = ParseExpression(Lexer, Storage);
    if(!Result->ParenExpr.InnerExpr)
    {
        free(Result);
        return NULL;
    }

    if(Lexer->Token != ')')
    {
        return ExpressionExpectedError(Lexer, Result, ")");
    }
    GetToken(Lexer);
    return Result;
}

static expr* ParseIfExpr(lexer* Lexer, string_storage* Storage)
{
    GetToken(Lexer);

    expr* Result = (expr*)malloc(sizeof(expr));
    Result->ExprType = EXPR_if;
    Result->IfExpr.TrueExpressionCount = 0;
    Result->IfExpr.FalseExpressionCount = 0;

    Result->IfExpr.Statement = ParseExpression(Lexer, Storage);

    if(!Result->IfExpr.Statement)
    {
        return ExpressionExpectedError(Lexer, Result, "statement");
    }

    if(Lexer->Token != '{')
    {
        return ExpressionExpectedError(Lexer, Result, "{ after statement");
    }

    GetToken(Lexer);
    for(;;)
    {
        if(Lexer->Token == '}')
        {
            break;
        }

        uint32_t TrueCount = Result->IfExpr.TrueExpressionCount;

        Result->IfExpr.TrueExpressions[TrueCount] = ParseExpression(Lexer, Storage);
        if(!Result->IfExpr.TrueExpressions[TrueCount])
        {
            FreeExpression(Result);
            return NULL;
        }
        int32_t ExprType = Result->IfExpr.TrueExpressions[TrueCount]->ExprType;
        ++Result->IfExpr.TrueExpressionCount;

        if((ExprType == EXPR_if) || (ExprType == EXPR_for))
        {
            if(Lexer->Token != '}')
            {
                return ExpressionExpectedError(Lexer, Result, "}");
            }
        }
        else
        {
            if(Lexer->Token != ';')
            {
                return ExpressionExpectedError(Lexer, Result, ";");
            }
        }
        GetToken(Lexer);
    }

    int32_t Token = PeekToken(Lexer);
    if(Token != TOKEN_else)
    {
        return Result;
    }

    GetToken(Lexer);
    GetToken(Lexer);
    if(Lexer->Token != '{')
    {
        return ExpressionExpectedError(Lexer, Result, "{ after else statement");
    }

    GetToken(Lexer);

    for(;;)
    {
        if(Lexer->Token == '}')
        {
            break;
        }

        uint32_t FalseCount = Result->IfExpr.FalseExpressionCount;

        Result->IfExpr.FalseExpressions[FalseCount] = ParseExpression(Lexer, Storage);
        if(!Result->IfExpr.FalseExpressions[FalseCount])
        {
            FreeExpression(Result);
            return NULL;
        }
        int32_t ExprType = Result->IfExpr.FalseExpressions[FalseCount]->ExprType;
        ++Result->IfExpr.FalseExpressionCount;

        if((ExprType == EXPR_if) || (ExprType == EXPR_for))
        {
            if(Lexer->Token != '}')
            {
                return ExpressionExpectedError(Lexer, Result, "}");
            }
        }
        else
        {
            if(Lexer->Token != ';')
            {
                return ExpressionExpectedError(Lexer, Result, ";");
            }
        }
        GetToken(Lexer);
    }

    return Result;
}

static expr* ParseForExpr(lexer* Lexer, string_storage* Storage)
{
    GetToken(Lexer);

    expr* Result = (expr*)malloc(sizeof(expr));
    Result->ExprType = EXPR_for;
    Result->ForExpr.Definition = NULL;
    Result->ForExpr.Condition = NULL;
    Result->ForExpr.Action = NULL;
    Result->ForExpr.ExpressionCount = 0;

    expr* Definition = ParseExpression(Lexer, Storage);
    if(!Definition)
    {
        FreeExpression(Result);
        return NULL;
    }

    if(Lexer->Token == ';')
    {
        Result->ForExpr.Definition = Definition;

        GetToken(Lexer);

        Result->ForExpr.Condition = ParseExpression(Lexer, Storage);
        if(!Result->ForExpr.Condition)
        {
            FreeExpression(Result);
            return NULL;
        }

        if(Lexer->Token != ';')
        {
            return ExpressionExpectedError(Lexer, Result, ";");
        }
        GetToken(Lexer);

        Result->ForExpr.Action = ParseExpression(Lexer, Storage);
        if(!Result->ForExpr.Action)
        {
            FreeExpression(Result);
            return NULL;
        }

        if(Lexer->Token != '{')
        {
            return ExpressionExpectedError(Lexer, Result, "{");
        }
    }
    else if(Lexer->Token == '{')
    {
        Result->ForExpr.Condition = Definition;
    }
    else
    {
        FreeExpression(Definition);
        return ExpressionExpectedError(Lexer, Result, "; or {");
    }

    GetToken(Lexer);

    for(;;)
    {
        if(Lexer->Token == '}')
        {
            break;
        }

        uint32_t ExprCount = Result->ForExpr.ExpressionCount;

        Result->ForExpr.Expressions[ExprCount] = ParseExpression(Lexer, Storage);
        if(!Result->ForExpr.Expressions[ExprCount])
        {
            FreeExpression(Result);
            return NULL;
        }
        int32_t ExprType = Result->ForExpr.Expressions[ExprCount]->ExprType;
        ++Result->ForExpr.ExpressionCount;

        if((ExprType == EXPR_if) || (ExprType == EXPR_for))
        {
            if(Lexer->Token != '}')
            {
                return ExpressionExpectedError(Lexer, Result, "}");
            }
        }
        else
        {
            if(Lexer->Token != ';')
            {
                return ExpressionExpectedError(Lexer, Result, ";");
            }
        }
        GetToken(Lexer);
    }

    return Result;
}

static expr* ParseReturnExpr(lexer* Lexer, string_storage* Storage)
{
    GetToken(Lexer);
    expr* Result = (expr*)malloc(sizeof(expr));
    Result->ExprType = EXPR_return;
    Result->ReturnExpr.Expression = ParseExpression(Lexer, Storage);
    if(!Result->ReturnExpr.Expression)
    {
        FreeExpression(Result);
        return NULL;
    }
    if(Lexer->Token != ';')
    {
        return ExpressionExpectedError(Lexer, Result, ";");
    }
    return Result;
}

static expr* ParseInlineExpr(lexer* Lexer, string_storage* Storage)
{
    location ErrorLocation;

    GetToken(Lexer);

    if(Lexer->Token != TOKEN_string_text)
    {
        GetLocation(&ErrorLocation, Lexer, Lexer->FirstChar);
        PrintLocationError(&ErrorLocation, "expected \"");
        return NULL;
    }

    int32_t StringIndex = AddStringToStorage(Storage, Lexer->String, Lexer->StringLength);

    GetToken(Lexer);

    expr* Result = (expr*)malloc(sizeof(expr));
    Result->ExprType = EXPR_inline;
    Result->InlineExpr.Text = Storage->StringArray[StringIndex];
    return Result;
}

static expr* ParsePrimaryExpression(lexer* Lexer, string_storage* Storage)
{
    switch(Lexer->Token)
    {
        default:
        {
            location ErrorLocation;
            GetLocation(&ErrorLocation, Lexer, Lexer->FirstChar);
            PrintLocationError(&ErrorLocation, "unknown token when expecting an expression");
            return NULL;
        } break;
        case TOKEN_char_number:
        {
            return ParseCharExpr(Lexer);
        } break;
        case TOKEN_int_number:
        {
            return ParseIntExpr(Lexer);
        } break;
        case TOKEN_real_number:
        {
            return ParseRealExpr(Lexer);
        } break;
        case TOKEN_string_text:
        {
            return ParseStringExpr(Lexer, Storage);
        } break;
        case TOKEN_id:
        {
            return ParseIdExpr(Lexer, Storage);
        } break;
        case '(':
        {
            return ParseParenExpr(Lexer, Storage);
        } break;
        case TOKEN_if:
        {
            return ParseIfExpr(Lexer, Storage);
        } break;
        case TOKEN_for:
        {
            return ParseForExpr(Lexer, Storage);
        } break;
        case TOKEN_return:
        {
            return ParseReturnExpr(Lexer, Storage);
        } break;
        case TOKEN_inline:
        {
            return ParseInlineExpr(Lexer, Storage);
        } break;
    }
}

static expr* ParseBinaryExpressionRHS(int32_t ExprPrecedence, lexer* Lexer, string_storage* Storage, expr* LHS)
{
    // TODO(rytis): Likely a memory leak here. CHECK!
    for(;;)
    {
        int32_t TokenPrecedence = GetTokenPrecedence(Lexer->Token);
        if(TokenPrecedence < ExprPrecedence)
        {
            return LHS;
        }

        int32_t Operator = Lexer->Token;
        GetToken(Lexer);

        expr* RHS = ParsePrimaryExpression(Lexer, Storage);
        if(!RHS)
        {
            FreeExpression(LHS);
            FreeExpression(RHS);
            return NULL;
        }

        int32_t NextPrecedence = GetTokenPrecedence(Lexer->Token);
        if(TokenPrecedence < NextPrecedence)
        {
            expr* OldRHS = RHS;
            RHS = ParseBinaryExpressionRHS(TokenPrecedence + 1, Lexer, Storage, OldRHS);
            if(!RHS)
            {
                FreeExpression(OldRHS);
                FreeExpression(LHS);
                FreeExpression(RHS);
                return NULL;
            }
        }

        expr* OldLHS = LHS;
        LHS = (expr*)malloc(sizeof(expr));
        LHS->ExprType = EXPR_binary;
        LHS->BinaryExpr.Operator = Operator;
        LHS->BinaryExpr.LHS = OldLHS;
        LHS->BinaryExpr.RHS = RHS;
    }
}

static expr* ParseExpression(lexer* Lexer, string_storage* Storage)
{
    expr* LHS = ParsePrimaryExpression(Lexer, Storage);
    if(!LHS)
    {
        return NULL;
    }

    return ParseBinaryExpressionRHS(0, Lexer, Storage, LHS);
}

static expr* ParseFunctionDeclarationVariable(lexer* Lexer, string_storage* Storage)
{
    location ErrorLocation;

    if(Lexer->Token != TOKEN_id)
    {
        GetLocation(&ErrorLocation, Lexer, Lexer->FirstChar);
        PrintLocationError(&ErrorLocation, "expected variable name");
        return NULL;
    }

    uint32_t StringIndex = AddStringToStorage(Storage, Lexer->String, Lexer->StringLength);
    
    GetToken(Lexer);
    if(Lexer->Token != ':')
    {
        GetLocation(&ErrorLocation, Lexer, Lexer->FirstChar);
        PrintLocationError(&ErrorLocation, "expected :");
        return NULL;
    }

    GetToken(Lexer);
    if(!IsType(Lexer->Token))
    {
        GetLocation(&ErrorLocation, Lexer, Lexer->FirstChar);
        PrintLocationError(&ErrorLocation, "expected type");
        return NULL;
    }

    expr* Result = (expr*)malloc(sizeof(expr));
    Result->ExprType = EXPR_var;
    Result->VarExpr.Type = Lexer->Token;
    Result->VarExpr.Name = Storage->StringArray[StringIndex];
    Result->VarExpr.Expr = NULL;

    GetToken(Lexer);

    return Result;
}

static func* ParseFunctionDeclaration(lexer* Lexer, string_storage* Storage)
{
    location ErrorLocation;

    func* Result = (func*)malloc(sizeof(func));
    Result->ParameterCount = 0;
    Result->ExpressionCount = 0;

    uint32_t StringIndex = AddStringToStorage(Storage, Lexer->String, Lexer->StringLength);
    Result->Name = Storage->StringArray[StringIndex];

    GetToken(Lexer); // Eat the name.
    GetToken(Lexer); // Eat the double colon.

    if(Lexer->Token != '(')
    {
        GetLocation(&ErrorLocation, Lexer, Lexer->FirstChar);
        PrintLocationError(&ErrorLocation, "expected (");
        free(Result);
        return NULL;
    }

    GetToken(Lexer);
    if(Lexer->Token != ')')
    {
        for(;;)
        {
            uint32_t ParamCount = Result->ParameterCount;
            Result->Parameters[ParamCount] = ParseFunctionDeclarationVariable(Lexer, Storage);
            if(!Result->Parameters[ParamCount])
            {
                FreeFunction(Result);
                return NULL;
            }
            ++Result->ParameterCount;

            if(Lexer->Token == ')')
            {
                break;
            }

            if(Lexer->Token != ',')
            {
                GetLocation(&ErrorLocation, Lexer, Lexer->FirstChar);
                PrintLocationError(&ErrorLocation, "expected , or )");
                FreeFunction(Result);
                return NULL;
            }
            GetToken(Lexer);
        }
    }

    GetToken(Lexer);
    if(Lexer->Token != TOKEN_arrow)
    {
        GetLocation(&ErrorLocation, Lexer, Lexer->FirstChar);
        PrintLocationError(&ErrorLocation, "expected ->");
        FreeFunction(Result);
        return NULL;
    }

    GetToken(Lexer);
    if(!IsType(Lexer->Token))
    {
        GetLocation(&ErrorLocation, Lexer, Lexer->FirstChar);
        PrintLocationError(&ErrorLocation, "expected type in function declaration");
        FreeFunction(Result);
        return NULL;
    }
    Result->Type = Lexer->Token;
    GetToken(Lexer);

    if(Lexer->Token == ';')
    {
        GetToken(Lexer);
        Result->ExpressionCount = 0;
        return Result;
    }

    if(Lexer->Token != '{')
    {
        GetLocation(&ErrorLocation, Lexer, Lexer->FirstChar);
        PrintLocationError(&ErrorLocation, "expected { or ; after function declaration");
        FreeFunction(Result);
        return NULL;
    }

    GetToken(Lexer);

    for(;;)
    {
        if(Lexer->Token == '}')
        {
            break;
        }

        uint32_t ExprCount = Result->ExpressionCount;

        Result->Expressions[ExprCount] = ParseExpression(Lexer, Storage);
        if(!Result->Expressions[ExprCount])
        {
            FreeFunction(Result);
            return NULL;
        }
        int32_t ExprType = Result->Expressions[ExprCount]->ExprType;
        ++Result->ExpressionCount;

        if((ExprType == EXPR_if) || (ExprType == EXPR_for))
        {
            if(Lexer->Token != '}')
            {
                GetLocation(&ErrorLocation, Lexer, Lexer->FirstChar);
                PrintLocationError(&ErrorLocation, "expected }");
                FreeFunction(Result);
                return NULL;
            }
        }
        else
        {
            if(Lexer->Token != ';')
            {
                GetLocation(&ErrorLocation, Lexer, Lexer->FirstChar);
                PrintLocationError(&ErrorLocation, "expected ; in function declaration");
                FreeFunction(Result);
                return NULL;
            }
        }
        GetToken(Lexer);
    }

    return Result;
}

static int32_t Parse(ast* Ast, lexer* Lexer, string_storage* Storage)
{
    if((Lexer->Token != TOKEN_id) && (Lexer->Token != TOKEN_inline))
    {
        return 0;
    }

    int32_t Token = PeekToken(Lexer);
    switch(Token)
    {
        case TOKEN_double_colon:
        {
            Ast->AstType = AST_func;
            Ast->Func = ParseFunctionDeclaration(Lexer, Storage);
        } break;
        default:
        {
            Ast->AstType = AST_expr;
            
            Ast->Expr = ParseExpression(Lexer, Storage);
            if(Lexer->Token != ';')
            {
                location ErrorLocation;
                GetLocation(&ErrorLocation, Lexer, Lexer->FirstChar);
                PrintLocationError(&ErrorLocation, "expected ;");
            }
        } break;
    }
    return 1;
}

// --------------
// --TRANSLATOR--
// --------------

static void TranslateString(FILE* FileHandle, char* String)
{
    char* CurrentChar = String;
    while(*CurrentChar != '\0')
    {
        switch(*CurrentChar)
        {
            default:
            {
                fprintf(FileHandle, "%c", *CurrentChar);
            } break;
            case '\n':
            {
                fprintf(FileHandle, "\\n");
            } break;
            case '\r':
            {
                fprintf(FileHandle, "\\r");
            } break;
            case '\t':
            {
                fprintf(FileHandle, "\\t");
            } break;
            case '\f':
            {
                fprintf(FileHandle, "\\f");
            } break;
        }
        ++CurrentChar;
    }
}

static int32_t TranslateType(FILE* FileHandle, int32_t Type)
{
    switch(Type)
    {
        default:
        {
            return 0;
        } break;
        case TOKEN_char:
        {
            fprintf(FileHandle, "char ");
        } break;
        case TOKEN_int:
        {
            fprintf(FileHandle, "int ");
        } break;
        case TOKEN_float:
        {
            fprintf(FileHandle, "float ");
        } break;
        case TOKEN_string:
        {
            fprintf(FileHandle, "char* ");
        } break;
    }
    return 1;
}

static int32_t TranslateOperator(FILE* FileHandle, int32_t Operator)
{
    if(Operator < TOKEN_eof)
    {
        fprintf(FileHandle, "%c", (char)Operator);
    }
    else
    {
        switch(Operator)
        {
            default:
            {
                return 0;
            } break;
            case TOKEN_pluseq:
            {
                fprintf(FileHandle, "+=");
            } break;
            case TOKEN_minuseq:
            {
                fprintf(FileHandle, "-=");
            } break;
            case TOKEN_muleq:
            {
                fprintf(FileHandle, "*=");
            } break;
            case TOKEN_diveq:
            {
                fprintf(FileHandle, "/=");
            } break;
            case TOKEN_modeq:
            {
                fprintf(FileHandle, "%%=");
            } break;
            case TOKEN_eq:
            {
                fprintf(FileHandle, "==");
            } break;
            case TOKEN_noteq:
            {
                fprintf(FileHandle, "!=");
            } break;
            case TOKEN_lesseq:
            {
                fprintf(FileHandle, "<=");
            } break;
            case TOKEN_moreeq:
            {
                fprintf(FileHandle, ">=");
            } break;
            case TOKEN_andand:
            {
                fprintf(FileHandle, "&&");
            } break;
            case TOKEN_oror:
            {
                fprintf(FileHandle, "||");
            } break;
        }
    }
    return 1;
}

static int32_t TranslateExpression(FILE* FileHandle, expr* Expression, bool IsParent)
{
    if(!Expression)
    {
        return 0;
    }

    switch(Expression->ExprType)
    {
        default:
        {
            return 0;
        } break;
        case EXPR_char:
        {
            fprintf(FileHandle, "'%c'", Expression->CharExpr.CharValue);
        } break;
        case EXPR_int:
        {
            fprintf(FileHandle, "%llu", Expression->IntExpr.IntValue);
        } break;
        case EXPR_real:
        {
            fprintf(FileHandle, "%ff", Expression->RealExpr.RealValue);
        } break;
        case EXPR_string:
        {
            fprintf(FileHandle, "\"");
            TranslateString(FileHandle, Expression->StringExpr.String);
            fprintf(FileHandle, "\"");
        } break;
        case EXPR_id:
        {
            fprintf(FileHandle, "%s", Expression->IdExpr.String);
        } break;
        case EXPR_var:
        {
            TranslateType(FileHandle, Expression->VarExpr.Type);
            fprintf(FileHandle, "%s", Expression->VarExpr.Name);
            if(Expression->VarExpr.Expr != NULL)
            {
                fprintf(FileHandle, "=");
                if(!TranslateExpression(FileHandle, Expression->VarExpr.Expr, false))
                {
                    return 0;
                }
            }
            if(IsParent)
            {
                fprintf(FileHandle, ";\n");
            }
        } break;
        case EXPR_paren:
        {
            fprintf(FileHandle, "(");
            if(!TranslateExpression(FileHandle, Expression->ParenExpr.InnerExpr, false))
            {
                return 0;
            }
            fprintf(FileHandle, ")");
        } break;
        case EXPR_binary:
        {
            if(!TranslateExpression(FileHandle, Expression->BinaryExpr.LHS, false))
            {
                return 0;
            }
            if(!TranslateOperator(FileHandle, Expression->BinaryExpr.Operator))
            {
                return 0;
            }
            if(!TranslateExpression(FileHandle, Expression->BinaryExpr.RHS, false))
            {
                return 0;
            }
            if(IsParent)
            {
                fprintf(FileHandle, ";\n");
            }
        } break;
        case EXPR_call:
        {
            fprintf(FileHandle, "%s(", Expression->CallExpr.Name);
            for(uint32_t i = 0; i < Expression->CallExpr.ArgumentCount; ++i)
            {
                if(!TranslateExpression(FileHandle, Expression->CallExpr.Arguments[i], false))
                {
                    return 0;
                }
                if(i != Expression->CallExpr.ArgumentCount - 1)
                {
                    fprintf(FileHandle, ", ");
                }
            }
            if(IsParent)
            {
                fprintf(FileHandle, ");\n");
            }
            else
            {
                fprintf(FileHandle, ")");
            }
        } break;
        case EXPR_if:
        {
            fprintf(FileHandle, "if(");
            if(!TranslateExpression(FileHandle, Expression->IfExpr.Statement, false))
            {
                return 0;
            }
            fprintf(FileHandle, ")\n{\n");
            for(uint32_t i = 0; i < Expression->IfExpr.TrueExpressionCount; ++i)
            {
                if(!TranslateExpression(FileHandle, Expression->IfExpr.TrueExpressions[i], true))
                {
                    return 0;
                }
            }
            fprintf(FileHandle, "}\n");
            if(Expression->IfExpr.FalseExpressionCount > 0)
            {
                fprintf(FileHandle, "else\n{\n");
                for(uint32_t i = 0; i < Expression->IfExpr.FalseExpressionCount; ++i)
                {
                    if(!TranslateExpression(FileHandle, Expression->IfExpr.FalseExpressions[i], true))
                    {
                        return 0;
                    }
                }
                fprintf(FileHandle, "}\n");
            }
        } break;
        case EXPR_for:
        {
            if(Expression->ForExpr.Condition && !Expression->ForExpr.Definition && !Expression->ForExpr.Action)
            {
                fprintf(FileHandle, "while(");
                if(!TranslateExpression(FileHandle, Expression->ForExpr.Condition, false))
                {
                    return 0;
                }
            }
            else
            {
                fprintf(FileHandle, "for(");
                if(Expression->ForExpr.Definition)
                {
                    if(!TranslateExpression(FileHandle, Expression->ForExpr.Definition, false))
                    {
                        return 0;
                    }
                }
                fprintf(FileHandle, ";");

                if(Expression->ForExpr.Condition)
                {
                    if(!TranslateExpression(FileHandle, Expression->ForExpr.Condition, false))
                    {
                        return 0;
                    }
                }
                fprintf(FileHandle, ";");

                if(Expression->ForExpr.Action)
                {
                    if(!TranslateExpression(FileHandle, Expression->ForExpr.Action, false))
                    {
                        return 0;
                    }
                }
            }
            fprintf(FileHandle, ")\n{\n");
            for(uint32_t i = 0; i < Expression->ForExpr.ExpressionCount; ++i)
            {
                if(!TranslateExpression(FileHandle, Expression->ForExpr.Expressions[i], true))
                {
                    return 0;
                }
            }
            fprintf(FileHandle, "}\n");
        } break;
        case EXPR_return:
        {
            fprintf(FileHandle, "return ");
            if(!TranslateExpression(FileHandle, Expression->ReturnExpr.Expression, false))
            {
                return 0;
            }
            fprintf(FileHandle, ";\n");
        } break;
        case EXPR_inline:
        {
            char* CurrentChar = Expression->InlineExpr.Text;
            while(*CurrentChar != '\0')
            {
                if((*CurrentChar != '\r') && (*CurrentChar != '\t') && (*CurrentChar != '\f'))
                {
                    fprintf(FileHandle, "%c", *CurrentChar);
                }
                ++CurrentChar;
            }
            fprintf(FileHandle, "\n");
        } break;
    }
    return 1;
}

static int32_t TranslateFunction(FILE* FileHandle, func* Function)
{
    if(!Function)
    {
        return 0;
    }

    TranslateType(FileHandle, Function->Type);
    fprintf(FileHandle, "%s(", Function->Name);
    for(uint32_t i = 0; i < Function->ParameterCount; ++i)
    {
        if(!TranslateExpression(FileHandle, Function->Parameters[i], false))
        {
            return 0;
        }
        if(i != (Function->ParameterCount - 1))
        {
            fprintf(FileHandle, ",");
        }
    }

    if(Function->ExpressionCount <= 0)
    {
        fprintf(FileHandle, ");");
    }
    else
    {
        fprintf(FileHandle, ")\n{\n");
        for(uint32_t i = 0; i < Function->ExpressionCount; ++i)
        {
            if(!TranslateExpression(FileHandle, Function->Expressions[i], true))
            {
                return 0;
            }
        }
        fprintf(FileHandle, "}");
    }
    fprintf(FileHandle, "\n");
    return 1;
}

static int32_t Translate(FILE* FileHandle, ast* Ast)
{
    switch(Ast->AstType)
    {
        default:
        {
            return 0;
        } break;
        case AST_expr:
        {
            expr* Expression = Ast->Expr;
            return TranslateExpression(FileHandle, Expression, true);
        } break;
        case AST_func:
        {
            func* Function = Ast->Func;
            return TranslateFunction(FileHandle, Function);
        } break;
    }
}

int main(int ArgCount, char** ArgValues)
{
    if(ArgCount != 2)
    {
        fprintf(stderr, "Error: Expected file name.\n");
        return 0;
    }
    char* FileName = ArgValues[1];

    FILE* FileHandle = fopen(FileName, "rb");
    char* Text = (char*)malloc(1 << 20);
    int32_t Length = FileHandle ? (int32_t)fread(Text, 1, 1 << 20, FileHandle): -1;
    if(Length < 0)
    {
        printf("Error reading file!\n");
        free(Text);
        fclose(FileHandle);
        return 1;
    }
    fclose(FileHandle);

    lexer Lexer;
    InitLexer(&Lexer, Text, Text + Length, (char*)malloc(0x10000), 0x10000);

    string_storage StringStorage;
    InitStringStorage(&StringStorage, (char*)malloc(1 << 20), 1 << 20);

    uint32_t ResultCount = 0;
    ast Results[100]; // Arbitrary number for now

    while(GetToken(&Lexer))
    {
#if 0
        // Testing out the lexer
        if(Lexer.Token == TOKEN_parse_error)
        {
            printf("\nPARSE ERROR\n");
            break;
        }
        PrintToken(&Lexer);
        printf(" ");
#else
        Results[ResultCount] = {};
        if(Parse(&Results[ResultCount], &Lexer, &StringStorage))
        {
            ++ResultCount;
        }
#endif
    }

    FILE* ResultFileHandle = fopen("result.c", "w");

    for(uint32_t i = 0; i < ResultCount; ++i)
    {
        if(!Translate(ResultFileHandle, &Results[i]))
        {
            fprintf(stderr, "Error: translation of AST[%d] failed.\n", i);
        }
    }
    fclose(FileHandle);

    // Freeing all the expressions
    // printf("\nResultCount = %d\n", ResultCount);
    for(uint32_t i = 0; i < ResultCount; ++i)
    {
        FreeAst(&Results[i]);
    }
    free(Lexer.InputStream);
    free(Lexer.StringStorage);
    free(StringStorage.Strings);
    return 0;
}
