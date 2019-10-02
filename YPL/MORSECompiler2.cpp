//-----------------------------------------------------------
// Dr. Art Hanna
// SPL1 Parser
// BUGFIX  8-29-2018 (Bug found by Dhvani Patel)
//            Added TokenDescription() function to "help" GetNextToken() instrumentation 
//            work correctly even when the entries in TOKENTYPE enumeration are *NOT* in
//            1-to-1 correspondence with entries in TOKENTABLE[] array.
// SPL1Parser.cpp
//-----------------------------------------------------------
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <vector>

using namespace std;

//#define TRACEREADER
//#define TRACESCANNER
//#define TRACEPARSER
#define TRACECOMPILER

#include "YPL.h"

//-----------------------------------------------------------
typedef enum
//-----------------------------------------------------------
{
// pseudo-terminals
   IDENTIFIER,
   STRING,
   INTEGER,
   EOPTOKEN,
   UNKTOKEN,
// reserved words
   TRANSMISSION,
   END,
   PRINT,
   ENDL,
   FUNCTION,
   ENDFUNC,
   ENDLINE,
   QUOTE,
   TRUE,
   FALSE,
   OR,
   AND,
   NOT,
// punctuation
   COMMA,
   PERIOD,
   OPARENTHESIS,
   CPARENTHESIS,
// operators
   LESSTHAN,
   LESSTHANEQUAL,
   GREATERTHAN,
   GREATERTHANEQUAL,
   EQUAL,
   NOTEQUAL,
   PLUS,
   MINUS,
   MULIPLY,
   DIVIDE,
// ***NONE***
} TOKENTYPE;

//-----------------------------------------------------------
struct TOKENTABLERECORD
//-----------------------------------------------------------
{
   TOKENTYPE type;
   char description[30+1];
   bool isReservedWord;
};

//-----------------------------------------------------------
const TOKENTABLERECORD TOKENTABLE[] =
//-----------------------------------------------------------
{
   { IDENTIFIER       ,"IDENTIFIER"         ,false},
   { STRING           ,"STRING"             ,false},
   { EOPTOKEN         ,"EOPTOKEN"           ,false},
   { UNKTOKEN         ,"UNKTOKEN"           ,false},
   { TRANSMISSION     ,"-... . --. .. -."   ,true},
   { END              ,"END"                ,true},
   { ENDL             ,"ENDL"               ,true},
   { COMMA            ,"COMMA"              ,false},
   { FUNCTION         ,"..-. ..- -. -.-."   ,true}, //FUNC
   { PRINT            ,".--. .-. .. -. -"   ,true}, //PRINT
   { ENDLINE          ,"-.-"                ,true}, //k
   { QUOTE            ,".-..-."             ,true}, //Qoute marks ""
   { COMMA            ,"--..--"             ,true}, //Comma ,
   { OR               ,"--- .-."            ,true},
   { AND              ,".- -. -.."          ,true},
   { NOT              ,"-. --- -"           ,true},
   { OPARENTHESIS     ,"-.--."              ,true}, // Parenthesis (
   { CPARENTHESIS     ,"-.--.-"             ,true}, // Parenthesis )
   { TRUE             ,"- .-. ..- ."        ,true}, // TRUE
   { FALSE            ,"..-. .- .-.. ... ." ,true}, //FALSE
   { LESSTHAN         ,".-.. -"             ,true},
   { LESSTHANEQUAL    ,".-.. - -...-"       ,true},
   { GREATERTHAN      ,"--. -"              ,true},
   { GREATERTHANEQUAL ,"--. - -...-"        ,true},
   { EQUAL            ,"-...-"              ,true},
   { NOTEQUAL         ,"-. -...-"           ,true},
   { PLUS             ,".-.-."              ,true}, // +
   { MINUS            ,"-....- "            ,true}, // -
   { MULIPLY          ,"--"                 ,true}, // M
   { DIVIDE           ,"-.."                ,true}, // D
   { ENDFUNC          ,".-.-.-"             ,true} //Full Stop
};

//-----------------------------------------------------------
struct TOKEN
//-----------------------------------------------------------
{
   TOKENTYPE type;
   char lexeme[SOURCELINELENGTH+1];
   int sourceLineNumber;
   int sourceLineIndex;
};

//--------------------------------------------------
// Global variables
//--------------------------------------------------
READER<CALLBACKSUSED> reader(SOURCELINELENGTH,LOOKAHEAD);
LISTER lister(LINESPERPAGE);
CODE code;

#ifdef TRACEPARSER
int level;
#endif

//-----------------------------------------------------------
void EnterModule(const char module[])
//-----------------------------------------------------------
{
#ifdef TRACEPARSER
   char information[SOURCELINELENGTH+1];

   level++;
   sprintf(information,"   %*s>%s",level*2," ",module);
   lister.ListInformationLine(information);
#endif
}

//-----------------------------------------------------------
void ExitModule(const char module[])
//-----------------------------------------------------------
{
#ifdef TRACEPARSER
   char information[SOURCELINELENGTH+1];

   sprintf(information,"   %*s<%s",level*2," ",module);
   lister.ListInformationLine(information);
   level--;
#endif
}

//--------------------------------------------------
void ProcessCompilerError(int sourceLineNumber,int sourceLineIndex,const char errorMessage[])
//--------------------------------------------------
{
   char information[SOURCELINELENGTH+1];

// Use "panic mode" error recovery technique: report error message and terminate compilation!
   sprintf(information,"     At (%4d:%3d) %s",sourceLineNumber,sourceLineIndex,errorMessage);
   lister.ListInformationLine(information);
   lister.ListInformationLine("SPL compiler ending with compiler error!\n");
   throw( SPLEXCEPTION("SPL compiler ending with compiler error!") );
}

//-----------------------------------------------------------
int main()
//-----------------------------------------------------------
{
   void Callback1(int sourceLineNumber,const char sourceLine[]);
   void Callback2(int sourceLineNumber,const char sourceLine[]);
   void GetNextToken(TOKEN tokens[]);
   void ParseSPLProgram(TOKEN tokens[]);

   char sourceFileName[80+1];
   TOKEN tokens[LOOKAHEAD+1];
   
   cout << "Source filename? ";
   cin >> sourceFileName;

   try
   {
      lister.OpenFile(sourceFileName);
      code.OpenFile(sourceFileName);

      // CODEGENERATION
      code.EmitBeginningCode(sourceFileName);
      // ENDCODEGENERATION

      reader.SetLister(&lister);
      reader.AddCallbackFunction(Callback1);
      reader.AddCallbackFunction(Callback2);
      reader.OpenFile(sourceFileName);

   // Fill tokens[] and lexemes[][] for look-ahead
      for (int i = 0; i <= LOOKAHEAD; i++)
         GetNextToken(tokens);

#ifdef TRACEPARSER
      level = 0;
#endif
   
      ParseSPLProgram(tokens);
      // CODEGENERATION
      code.EmitEndingCode();
      // ENDCODEGENERATION
   }
   catch (SPLEXCEPTION splException)
   {
      cout << "SPL exception: " << splException.GetDescription() << endl;
   }
   lister.ListInformationLine("******* SPL1 parser ending");
   cout << "SPL1 parser ending\n";

   system("PAUSE");
   return( 0 );
   
}

//-----------------------------------------------------------
void ParseSPLProgram(TOKEN tokens[])
//-----------------------------------------------------------
{
   void GetNextToken(TOKEN tokens[]);
   void ParsePROGRAMDefinition(TOKEN tokens[]);

   EnterModule("MORSEProgram");

   if ( tokens[0].type == TRANSMISSION )
      ParsePROGRAMDefinition(tokens);
   else
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                           "Expecting -... . –-. .. -.");

   if ( tokens[0].type != EOPTOKEN ) 
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                           "Expecting end-of-program");

   ExitModule("MORSEProgram");
}

void ParsePROGRAMDefinition(TOKEN tokens[]) {
   void GetNextToken(TOKEN tokens[]);
   void ParseStatement(TOKEN tokens[]);
   void ParseFUNCTIONDefinition(TOKEN tokens[]);

   char line[SOURCELINELENGTH+1];
   char label[SOURCELINELENGTH+1];
   char reference[SOURCELINELENGTH+1];

   EnterModule("PROGRAMDefinition");

   // CODEGENERATION
   code.EmitUnformattedLine("; **** =========");
   sprintf(line,"; **** PROGRAM module (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);
   code.EmitUnformattedLine("; **** =========");
   code.EmitFormattedLine("PROGRAMMAIN","EQU"  ,"*");

   code.EmitFormattedLine("","PUSH" ,"#RUNTIMESTACK","set SP");
   code.EmitFormattedLine("","POPSP");
   code.EmitFormattedLine("","PUSHA","STATICDATA","set SB");
   code.EmitFormattedLine("","POPSB");
   code.EmitFormattedLine("","PUSH","#HEAPBASE","initialize heap");
   code.EmitFormattedLine("","PUSH","#HEAPSIZE");
   code.EmitFormattedLine("","SVC","#SVC_INITIALIZE_HEAP");
   sprintf(label,"PROGRAMBODY%04d",code.LabelSuffix());
   code.EmitFormattedLine("","CALL",label);
   code.AddDSToStaticData("Normal program termination","",reference);
   code.EmitFormattedLine("","PUSHA",reference);
   code.EmitFormattedLine("","SVC","#SVC_WRITE_STRING");
   code.EmitFormattedLine("","SVC","#SVC_WRITE_ENDL");
   code.EmitFormattedLine("","PUSH","#0D0","terminate with status = 0");
   code.EmitFormattedLine("","SVC" ,"#SVC_TERMINATE");
   code.EmitUnformattedLine("");
   code.EmitFormattedLine(label,"EQU","*");
   // ENDCODEGENERATION

   GetNextToken(tokens);

   if(tokens[0].type == FUNCTION) {
      ParseFUNCTIONDefinition(tokens);
   } else {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex, "Expecting Function");
   }
   /*while ( tokens[0].type != END )
      ParseStatement(tokens);*/

   if(tokens[0].type != EOPTOKEN) {
      GetNextToken(tokens);
   }

   // CODEGENERATION
   code.EmitFormattedLine("","RETURN");
   code.EmitUnformattedLine("; **** =========");
   sprintf(line,"; **** END (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);
   code.EmitUnformattedLine("; **** =========");
   // ENDCODEGENERATION

   ExitModule("PROGRAMDefinition");
}

void ParseFUNCTIONDefinition(TOKEN tokens[]) {
   void GetNextToken(TOKEN tokens[]);
   void ParseStatement(TOKEN tokens[]);

   EnterModule("FUNCTIONDefinition");

   GetNextToken(tokens);
   if(tokens[0].type != IDENTIFIER) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex, "Expecting Identifier");
   }
   GetNextToken(tokens);
   while(tokens[0].type != ENDFUNC) {
      ParseStatement(tokens);
   }
   GetNextToken(tokens);

   ExitModule("FUNCTIONDefinition");
}

void ParseStatement(TOKEN tokens[]) {
   void GetNextToken(TOKEN tokens[]);
   void ParsePRINTStatement(TOKEN tokens[]);

   EnterModule("Statement");

   switch ( tokens[0].type )
   {
      case PRINT:
         ParsePRINTStatement(tokens);
         break;
      default:
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                              "Expecting beginning-of-statement");
         break;
   }

   ExitModule("Statement");
}

void ParsePRINTStatement(TOKEN tokens[]) {
   void ParseExpression(TOKEN tokens[], DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   char line[SOURCELINELENGTH+1];
   DATATYPE datatype;

   EnterModule("PRINTStatement");

   sprintf(line,"; **** PRINT statement (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);

   do
   {
      GetNextToken(tokens);

      switch ( tokens[0].type )
      {
         case QUOTE: //TODO: Make quote into a morse
            GetNextToken(tokens);
            break;
         case STRING:
            // CODEGENERATION
            char reference[SOURCELINELENGTH+1];

            code.AddDSToStaticData(tokens[0].lexeme,"",reference);
            code.EmitFormattedLine("","PUSHA",reference);
            code.EmitFormattedLine("","SVC","#SVC_WRITE_STRING");
            // ENDCODEGENERATION
            GetNextToken(tokens);
            break;
         /*case ENDL:
            GetNextToken(tokens);
            break;*/
         default:
         {
            //ParseExpression(tokens,datatype);
         }
      }
   } while ( tokens[0].type == COMMA );
   if ( tokens[0].type != ENDLINE )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                           "Expecting '-.-'");

   GetNextToken(tokens);

   ExitModule("PRINTStatement");
}

void ParseExpression(TOKEN tokens[], DATATYPE &datatype)
{
   void ParseConjuction(TOKEN tokens[], DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   DATATYPE datatypeLHS,datatypeRHS;

   EnterModule("Expression");

   ParseConjuction(tokens, datatypeLHS);

   if( (tokens[0].type == OR) ) {
      while( (tokens[0]. type == OR) ) {
         TOKENTYPE operation = tokens[0].type;

         GetNextToken(tokens);
         ParseConjuction(tokens,datatypeRHS);

         switch(operation) {
            case OR:
               if( !((datatypeLHS == BOOLEANTYPE) && (datatypeRHS == BOOLEANTYPE)) ) {
                  ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting boolean operands");
               }

               code.EmitFormattedLine("","OR");
               datatype = BOOLEANTYPE;
               break;
         }
      }
   } else {
      datatype = datatypeLHS;
   }
   ExitModule("Expression");
}

void ParseConjuction(TOKEN tokens[], DATATYPE &datatype)
{
   void ParseNegation(TOKEN tokens[],DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   DATATYPE datatypeLHS,datatypeRHS;

   EnterModule("Conjuction");

   ParseNegation(tokens,datatypeLHS);

   if( (tokens[0].type == AND) ) {
      while( (tokens[0].type == AND) ) {
         TOKENTYPE operation = tokens[0].type;

         GetNextToken(tokens);
         ParseNegation(tokens, datatypeRHS);

         switch(operation) {
            case AND:
            if( !((datatypeLHS == BOOLEANTYPE) && (datatypeRHS == BOOLEANTYPE)) ) {
               ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting boolean operands");
            }
            code.EmitFormattedLine("","AND");
            datatype = BOOLEANTYPE;
            break;
         }
      }
   } else {
      datatype = datatypeLHS;
   }
   ExitModule("Conjuction");
}

void ParseNegation(TOKEN tokens[], DATATYPE &datatype)
{
   void ParseComparison(TOKEN tokens[], DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   DATATYPE datatypeRHS;

   EnterModule("Negation");

   if(tokens[0].type == NOT) {
      GetNextToken(tokens);
      ParseComparison(tokens,datatypeRHS);

      if( !(datatypeRHS == BOOLEANTYPE)) {
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting boolean operand");
      }
      code.EmitFormattedLine("","NOT");
      datatype = BOOLEANTYPE;
   } else {
      ParseComparison(tokens,datatype);
   }
   ExitModule("Negation");
}

void ParseComparison(TOKEN tokens[], DATATYPE &datatype)
{
   void ParseComparator(TOKEN tokens[], DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   DATATYPE datatypeLHS, datatypeRHS;

   EnterModule("Comparison");

   ParseComparator(tokens, datatypeLHS);
   if( (tokens[0].type == LESSTHAN) 
      || (tokens[0].type == LESSTHANEQUAL) 
      || (tokens[0].type == GREATERTHAN)
      || (tokens[0].type == GREATERTHANEQUAL)
      || (tokens[0].type == EQUAL)
      || (tokens[0].type == NOTEQUAL)) {
      TOKENTYPE operation = tokens[0].type;
      GetNextToken(tokens);
      ParseComparator(tokens, datatypeRHS);

      if( (datatypeLHS != INTEGERTYPE) || (datatypeRHS != INTEGERTYPE)) {
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting integer operands");
      }

      char Tlabel[SOURCELINELENGTH+1],Elabel[SOURCELINELENGTH+1];

      code.EmitFormattedLine("","CMPI");
      sprintf(Tlabel,"T%04d",code.LabelSuffix());
      sprintf(Elabel,"E%04d",code.LabelSuffix());
      switch(operation) {
         case LESSTHAN:
            code.EmitFormattedLine("","JMPL",Tlabel);
            break;
         case LESSTHANEQUAL:
            code.EmitFormattedLine("","JMPLE",Tlabel);
            break;
         case EQUAL:
            code.EmitFormattedLine("","JMPE",Tlabel);
            break;
         case GREATERTHAN:
            code.EmitFormattedLine("","JMPG",Tlabel);
            break;
         case GREATERTHANEQUAL:
            code.EmitFormattedLine("","JMPGE",Tlabel);
            break;
         case NOTEQUAL:
            code.EmitFormattedLine("","JMPNE",Tlabel);
            break;
      }
      datatype = BOOLEANTYPE;
      code.EmitFormattedLine("","PUSH","#0X0000");
      code.EmitFormattedLine("","JMP",Elabel);
      code.EmitFormattedLine(Tlabel,"PUSH","#0XFFFF");
      code.EmitFormattedLine(Elabel,"EQU","*");
   } else {
      datatype = datatypeLHS;
   }
   ExitModule("Comparison");
}

void ParseComparator(TOKEN tokens[], DATATYPE &datatype)
{
   void ParseTerm(TOKEN tokens[], DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   DATATYPE datatypeLHS, datatypeRHS;

   EnterModule("Comparator");

   ParseTerm(tokens,datatypeLHS);

   if( (tokens[0].type == PLUS) || (tokens[0].type == MINUS)) {
      while( (tokens[0].type == PLUS) || (tokens[0].type == MINUS) ) {
         TOKENTYPE operation = tokens[0].type;

         GetNextToken(tokens);
         ParseTerm(tokens, datatypeRHS);

         if( (datatypeLHS != INTEGERTYPE) || (datatypeRHS != INTEGERTYPE)) {
            ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting integer operands");
         }

         switch(operation) {
            case PLUS:
               code.EmitFormattedLine("","ADDI");
               break;
            case MINUS:
               code.EmitFormattedLine("","SUBI");
               break;
         }
         datatype = INTEGERTYPE;
      }
   } else {
      datatype = datatypeLHS;
   }
   ExitModule("Comparator");
}

void ParseTerm(TOKEN tokens[], DATATYPE &datatype)
{
   void ParseFactor(TOKEN tokens[], DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   DATATYPE datatypeLHS, datatypeRHS;

   EnterModule("Term");

   ParseFactor(tokens,datatypeLHS);
   if( (tokens[0].type == MULIPLY) || (tokens[0].type == DIVIDE)) {
      while( (tokens[0].type == MULIPLY) || (tokens[0].type == DIVIDE)) {
         TOKENTYPE operation = tokens[0].type;

         GetNextToken(tokens);
         ParseFactor(tokens, datatypeRHS);

         if( (datatypeLHS != INTEGERTYPE) || (datatypeRHS != INTEGERTYPE)) {
            ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting integer operands");
         }

         switch(operation) {
            case MULIPLY:
               code.EmitFormattedLine("","MULI");
               break;
            case DIVIDE:
               code.EmitFormattedLine("","DIVI");
               break;
         }
         datatype = INTEGERTYPE;
      }
   } else {
      datatype = datatypeLHS;
   }
   ExitModule("Term");
}

void ParseFactor(TOKEN tokens[], DATATYPE &datatype)
{
   void ParseSecondary(TOKEN tokens[], DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   EnterModule("Factor");

   if( (tokens[0].type == PLUS) || (tokens[0].type == MINUS)) {
      DATATYPE datatypeRHS;
      TOKENTYPE operation = tokens[0].type;

      GetNextToken(tokens);
      ParseSecondary(tokens,datatypeRHS);

      if(datatypeRHS != INTEGERTYPE) {
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting integer operand");
      }
      switch(operation) {
         case PLUS:
            //Do nothing
            break;
         case MINUS:
            code.EmitFormattedLine("","NEGI");
            break;
      }
      datatype = INTEGERTYPE;
   } else {
      ParseSecondary(tokens,datatype);
   }

   ExitModule("Factor");
}

void ParseSecondary(TOKEN tokens[], DATATYPE &datatype)
{
   void ParsePrimary(TOKEN tokens[], DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   DATATYPE datatypeLHS, datatypeRHS;

   EnterModule("Secondary");

   ParsePrimary(tokens, datatypeLHS);

   datatype = datatypeLHS;

   ExitModule("Secondary");
}

void ParsePrimary(TOKEN tokens[], DATATYPE &datatype)
{
   void ParseExpression(TOKEN tokens[], DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   EnterModule("Primary");

   switch(tokens[0].type)
   {
      case INTEGER:
         {
            char operand[SOURCELINELENGTH+1];

            sprintf(operand,"#0D%s",tokens[0].lexeme);
            code.EmitFormattedLine("","PUSH",operand);
            datatype = INTEGERTYPE;
            GetNextToken(tokens);
         }
         break;
      case TRUE:
         code.EmitFormattedLine("","PUSH","#0XFFFF");
         datatype = BOOLEANTYPE;
         GetNextToken(tokens);
         break;
      case FALSE:
         code.EmitFormattedLine("","PUSH","#0X0000");
         datatype = BOOLEANTYPE;
         GetNextToken(tokens);
         break;
      case OPARENTHESIS:
         GetNextToken(tokens);
         ParseExpression(tokens,datatype);
         if ( tokens[0].type != CPARENTHESIS )
            ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting )");
         GetNextToken(tokens);
         break;
      default:
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting integer, true, false, or (");
         break;
   }
}
//-----------------------------------------------------------
void Callback1(int sourceLineNumber,const char sourceLine[])
//-----------------------------------------------------------
{
   cout << setw(4) << sourceLineNumber << " ";
}

//-----------------------------------------------------------
void Callback2(int sourceLineNumber,const char sourceLine[])
//-----------------------------------------------------------
{
    //cout << sourceLine << endl;
    char line[SOURCELINELENGTH+1];
    // CODEGENERATION
    sprintf(line,"; %4d %s",sourceLineNumber,sourceLine);
    code.EmitUnformattedLine(line);
   // ENDCODEGENERATION
}

//-----------------------------------------------------------
void GetNextToken(TOKEN tokens[])
//-----------------------------------------------------------
{
   const char *TokenDescription(TOKENTYPE type);

   int i;
   TOKENTYPE type;
   char lexeme[SOURCELINELENGTH+1];
   int sourceLineNumber;
   int sourceLineIndex;
   char information[SOURCELINELENGTH+1];

//============================================================
// Move look-ahead "window" to make room for next token-and-lexeme
//============================================================
   for (int i = 1; i <= LOOKAHEAD; i++)
      tokens[i-1] = tokens[i];

   char nextCharacter = reader.GetLookAheadCharacter(0).character;

//============================================================
// "Eat" white space and comments
//============================================================
   do
   {
//    "Eat" any white-space (blanks and EOLCs and TABCs) 
      while ( (nextCharacter == '/')
           || (nextCharacter == READER<CALLBACKSUSED>::EOLC)
           || (nextCharacter == READER<CALLBACKSUSED>::TABC) )
         nextCharacter = reader.GetNextCharacter().character;

//    "Eat" line comment
      if ( (nextCharacter == '<') && (reader.GetLookAheadCharacter(1).character == '~') )
      {

#ifdef TRACESCANNER
   sprintf(information,"At (%4d:%3d) begin line comment",
      reader.GetLookAheadCharacter(0).sourceLineNumber,
      reader.GetLookAheadCharacter(0).sourceLineIndex);
   lister.ListInformationLine(information);
#endif

         do
            nextCharacter = reader.GetNextCharacter().character;
         while ( nextCharacter != READER<CALLBACKSUSED>::EOLC );
      }

//    "Eat" block comments (nesting allowed)

      if ( (nextCharacter == '<') && (reader.GetLookAheadCharacter(1).character == '<') )
      {
         int depth = 0;

         do
         {
            if ( (nextCharacter == '<') && (reader.GetLookAheadCharacter(1).character == '<') )
            {
               depth++;

#ifdef TRACESCANNER
   sprintf(information,"At (%4d:%3d) begin block comment depth = %d",
      reader.GetLookAheadCharacter(0).sourceLineNumber,
      reader.GetLookAheadCharacter(0).sourceLineIndex,
      depth);
   lister.ListInformationLine(information);
#endif

               nextCharacter = reader.GetNextCharacter().character;
               nextCharacter = reader.GetNextCharacter().character;
            }
            else if ( (nextCharacter == '>') && (reader.GetLookAheadCharacter(1).character == '>') )
            {

#ifdef TRACESCANNER
   sprintf(information,"At (%4d:%3d)   end block comment depth = %d",
      reader.GetLookAheadCharacter(0).sourceLineNumber,
      reader.GetLookAheadCharacter(0).sourceLineIndex,
      depth);
   lister.ListInformationLine(information);
#endif

               depth--;
               nextCharacter = reader.GetNextCharacter().character;
               nextCharacter = reader.GetNextCharacter().character;
            }
            else
               nextCharacter = reader.GetNextCharacter().character;
         }
         while ( (depth != 0) && (nextCharacter != READER<CALLBACKSUSED>::EOPC) );
         if ( depth != 0 ) 
            ProcessCompilerError(reader.GetLookAheadCharacter(0).sourceLineNumber,
                                 reader.GetLookAheadCharacter(0).sourceLineIndex,
                                 "Unexpected end-of-program");
      }
      
   } while ( (nextCharacter == '/')
          || (nextCharacter == READER<CALLBACKSUSED>::EOLC)
          || (nextCharacter == READER<CALLBACKSUSED>::TABC)
          || (nextCharacter == '<')
          || ((nextCharacter == '<') && (reader.GetLookAheadCharacter(1).character == '<')) );
//============================================================
// Scan token
//============================================================
   sourceLineNumber = reader.GetLookAheadCharacter(0).sourceLineNumber;
   sourceLineIndex = reader.GetLookAheadCharacter(0).sourceLineIndex;

// reserved words (and <identifier> ***BUT NOT YET***)
   if ( isalpha(nextCharacter) || nextCharacter == '-' || nextCharacter == '.' )
   {
      char UCLexeme[SOURCELINELENGTH+1];

      i = 0;
      lexeme[i++] = nextCharacter;
      nextCharacter = reader.GetNextCharacter().character;
      while ( isalpha(nextCharacter) || isdigit(nextCharacter) || nextCharacter == '_' || nextCharacter == ' ' || nextCharacter == '-' || nextCharacter == '.')
      {
         lexeme[i++] = nextCharacter;
         nextCharacter = reader.GetNextCharacter().character;
      }
      lexeme[i] = '\0';
      for (i = 0; i <= (int) strlen(lexeme); i++)
         UCLexeme[i] = toupper(lexeme[i]);

      bool isFound = false;

      i = 0;
      while ( !isFound && (i <= (sizeof(TOKENTABLE)/sizeof(TOKENTABLERECORD))-1) )
      {
         if ( TOKENTABLE[i].isReservedWord && (strcmp(UCLexeme,TOKENTABLE[i].description) == 0) )
            isFound = true;
         else
            i++;
      }
      if ( isFound )
         type = TOKENTABLE[i].type;
      else
         type = IDENTIFIER;
   }
   else
   {
      switch ( nextCharacter )
      {
// <string>
         case '*': 
            i = 0;
            nextCharacter = reader.GetNextCharacter().character;
            while ( nextCharacter != '*' )
            {
               if ( nextCharacter == '\\' )
                  nextCharacter = reader.GetNextCharacter().character;
               else if ( nextCharacter == READER<CALLBACKSUSED>::EOLC )
                  ProcessCompilerError(sourceLineNumber,sourceLineIndex,
                                       "Invalid string literal");
               lexeme[i++] = nextCharacter;
               nextCharacter = reader.GetNextCharacter().character;
            }
            reader.GetNextCharacter();
            lexeme[i] = '\0';
            type = STRING;
            break;
         case READER<CALLBACKSUSED>::EOPC: 
            {
               static int count = 0;
   
               if ( ++count > (LOOKAHEAD+1) )
                  ProcessCompilerError(sourceLineNumber,sourceLineIndex,
                                       "Unexpected end-of-program");
               else
               {
                  type = EOPTOKEN;
                  reader.GetNextCharacter();
                  lexeme[0] = '\0';
               }
            }
            break;
         case ',':
            type = COMMA;
            lexeme[0] = nextCharacter; lexeme[1] = '\0';
            reader.GetNextCharacter();
            break;
         case '#': 
            type = PERIOD;
            lexeme[0] = nextCharacter; lexeme[1] = '\0';
            reader.GetNextCharacter();
            break;
         default:  
            type = UNKTOKEN;
            lexeme[0] = nextCharacter; lexeme[1] = '\0';
            reader.GetNextCharacter();
            break;
      }
   }

   tokens[LOOKAHEAD].type = type;
   strcpy(tokens[LOOKAHEAD].lexeme,lexeme);
   tokens[LOOKAHEAD].sourceLineNumber = sourceLineNumber;
   tokens[LOOKAHEAD].sourceLineIndex = sourceLineIndex;

#ifdef TRACESCANNER
   sprintf(information,"At (%4d:%3d) token = %12s lexeme = \\%s\\",
      tokens[LOOKAHEAD].sourceLineNumber,
      tokens[LOOKAHEAD].sourceLineIndex,
// BUGFIX  8-29-2018 (Bug found by Dhvani Patel)
      TokenDescription(type),lexeme);
   lister.ListInformationLine(information);
#endif

}

//-----------------------------------------------------------
const char *TokenDescription(TOKENTYPE type)
//-----------------------------------------------------------
{
   int i;
   bool isFound;
   
   isFound = false;
   i = 0;
   while ( !isFound && (i <= (sizeof(TOKENTABLE)/sizeof(TOKENTABLERECORD))-1) )
   {
      if ( TOKENTABLE[i].type == type )
         isFound = true;
      else
         i++;
   }
   return ( isFound ? TOKENTABLE[i].description : "???????" );
}