//-----------------------------------------------------------
// Catherine Etter
// Adapted from SPL4Compiler.cpp
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
   MAIN,
   ENDFUNC,
   ENDLINE,
   NEWLINE,
   QUOTE,
   ASSIGNMENT,
   TRUE,
   FALSE,
   INTDATATYPE,
   DOUBLEDATATYPE,
   STRINGDATATYPE,
   BOOLDATATYPE,
   CHARDATATYPE,
   INPUT,
   IF,
   ELSE,
   ELSEIF,
   WHILE,
   FOR,
   DO,
   OR,
   AND,
   NOT,
   ASSERT,
   CALL,
   RETURN,
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
   { MAIN             ,"-- .- .. -."        ,true},
   { PRINT            ,".--. .-. .. -. -"   ,true}, //PRINT
   { INPUT            ,".. -. .--. ..- -"   ,true}, //INPUT
   { IF               ,".. ..-."            ,true}, //IF
   { ELSE             ,". .-.. ... ."       ,true}, //ELSE
   { ELSEIF           ,". .-.. ... . .. ..-.", true}, //ELSEIF
   { WHILE            ,".-- .... .. .-.. ." ,true}, //WHILE
   { DO               ,"-.. ---"            ,true}, //DO
   { FOR              ,"..-. --- .-."       ,true},
   { ENDLINE          ,"-.-"                ,true}, //k
   { NEWLINE          ,"-. .-.."            ,true}, //N L
   { QUOTE            ,".-..-."             ,true}, //Quote marks ""
   { COMMA            ,"--..--"             ,true}, //Comma ,
   { OR               ,"--- .-."            ,true},
   { AND              ,".- -. -.."          ,true},
   { NOT              ,"-. --- -"           ,true},
   { ASSERT           ,".- ... .-. -"       ,true}, //ASRT
   { CALL             ,"-.-. .- .-.. .-.."  ,true},
   { RETURN           ,".-. - .-. -."       ,true},
   { OPARENTHESIS     ,"-.--."              ,true}, // Parenthesis (
   { CPARENTHESIS     ,"-.--.-"             ,true}, // Parenthesis )
   { ASSIGNMENT       ,"-...-"              ,true}, //=
   { TRUE             ,"- .-. ..- ."        ,true}, // TRUE
   { FALSE            ,"..-. .- .-.. ... ." ,true}, //FALSE
   { INTDATATYPE      ,".. -. -"            ,true}, //INT
   { DOUBLEDATATYPE   ,"-.. -... .-.."      ,true}, //DBL
   { STRINGDATATYPE   ,"... - .-."          ,true}, //STRING
   { BOOLDATATYPE     ,"-... --- --- .-.."  ,true}, //BOOL
   { CHARDATATYPE     ,"-.-. .... .-."      ,true}, //CHR
   { INPUT            ,".. -. .--. ..- -"   ,true}, //INPUT
   { LESSTHAN         ,".-.. -"             ,true},
   { LESSTHANEQUAL    ,".-.. - -...-"       ,true},
   { GREATERTHAN      ,"--. -"              ,true},
   { GREATERTHANEQUAL ,"--. - -...-"        ,true},
   { EQUAL            ,"-...- -...-"        ,true},
   { NOTEQUAL         ,"-. -...-"           ,true},
   { PLUS             ,".-.-."              ,true}, // +
   { MINUS            ,"-....-"             ,true}, // -
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
IDENTIFIERTABLE identifierTable(&lister,MAXIMUMIDENTIFIERS);

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
   lister.ListInformationLine("******* SPL2 Compiler ending");
   cout << "SPL2 compiler ending\n";

   system("PAUSE");
   return( 0 );
   
}

//-----------------------------------------------------------
void ParseSPLProgram(TOKEN tokens[])
//-----------------------------------------------------------
{
   void GetNextToken(TOKEN tokens[]);
   void ParseMAINDefinition(TOKEN tokens[]);
   void ParseFUNCTIONDefinition(TOKEN tokens[]);
   void ParseDataDefinitions(TOKEN tokens[],IDENTIFIERSCOPE identifierScope);

   EnterModule("MORSEProgram");

   ParseDataDefinitions(tokens,GLOBALSCOPE);

#ifdef TRACECOMPILER
   identifierTable.DisplayTableContents("Contents of identifier table after compilation of global data definitions");
#endif
   while(tokens[0].type == FUNCTION) {
      ParseFUNCTIONDefinition(tokens);
   }

   if(tokens[0].type == MAIN) {
      ParseMAINDefinition(tokens);
   } 
   else {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex, "Expecting MAIN");
   }

   if ( tokens[0].type != EOPTOKEN ) 
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                           "Expecting end-of-program");

   ExitModule("MORSEProgram");
}

void ParseMAINDefinition(TOKEN tokens[]) {
   void GetNextToken(TOKEN tokens[]);
   void ParseStatement(TOKEN tokens[]);
   void ParseMAINDefinition(TOKEN tokens[]);
   void ParseFUNCTIONDefinition(TOKEN tokens[]);
   void ParseDataDefinitions(TOKEN tokens[],IDENTIFIERSCOPE identifierScope);


   char line[SOURCELINELENGTH+1];
   char label[SOURCELINELENGTH+1];
   char reference[SOURCELINELENGTH+1];

   EnterModule("MAINDefinition");

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

   identifierTable.EnterNestedStaticScope();

   ParseDataDefinitions(tokens,PROGRAMMODULESCOPE);

   while (tokens[0].type != ENDFUNC) {
      ParseStatement(tokens);
   }

   // CODEGENERATION
   code.EmitFormattedLine("","RETURN");
   code.EmitUnformattedLine("; **** =========");
   sprintf(line,"; **** END (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);
   code.EmitUnformattedLine("; **** =========");
   // ENDCODEGENERATION

#ifdef TRACECOMPILER
   identifierTable.DisplayTableContents("Contents of identifier table at end of compilation of PROGRAM module definition");
#endif

   identifierTable.ExitNestedStaticScope();

   GetNextToken(tokens);

   ExitModule("MAINDefinition");
}
void ParseDataDefinitions(TOKEN tokens[], IDENTIFIERSCOPE identifierScope) {
   void GetNextToken(TOKEN tokens[]);

   EnterModule("DataDefinitions");

   while( (tokens[0].type == INTDATATYPE) || (tokens[0].type == DOUBLEDATATYPE) || (tokens[0].type == STRINGDATATYPE) || (tokens[0].type == BOOLDATATYPE) || (tokens[0].type == CHARDATATYPE)) {
      
      char identifier[MAXIMUMLENGTHIDENTIFIER+1];
      char reference[MAXIMUMLENGTHIDENTIFIER+1];
      DATATYPE datatype;
      bool isInTable;
      int index;

      do {
         switch(tokens[0].type) {
            case INTDATATYPE:
               datatype = INTEGERTYPE;
               break;
            case BOOLDATATYPE:
               datatype = BOOLEANTYPE;
               break;
            default:
               ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting INT or BOOL");
         }

         GetNextToken(tokens);

         if(tokens[0].type != IDENTIFIER) {
            ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting identifier");
         }
         strcpy(identifier,tokens[0].lexeme);
         GetNextToken(tokens);

         if(tokens[0].type == ASSIGNMENT) {
            char literal[MAXIMUMLENGTHIDENTIFIER+1];

            GetNextToken(tokens);
            if((datatype == INTEGERTYPE) && (tokens[0].type == INTEGER)) {
               strcpy(literal,"0D");
               strcat(literal,tokens[0].lexeme);
            }
            else if(((datatype == BOOLEANTYPE) && (tokens[0].type == TRUE)) || ((datatype == BOOLEANTYPE) && (tokens[0].type == FALSE))) {
               //strcpy(literal,tokens[0].lexeme);
               tokens[0].type == TRUE ? strcpy(literal,"TRUE") : strcpy(literal,"FALSE");
            }
            else {
               ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Data type mismatch");
            }
            GetNextToken(tokens);
         }
         

         index = identifierTable.GetIndex(identifier,isInTable);
         if ( isInTable && identifierTable.IsInCurrentScope(index) ) {
            ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Multiply-defined identifier");
         }

         switch(identifierScope) {
            case GLOBALSCOPE:
            // CODEGENERATION
               code.AddRWToStaticData(1,identifier,reference);
            // ENDCODEGENERATION
               identifierTable.AddToTable(identifier,GLOBAL_VARIABLE,datatype,reference);
               break;
            case PROGRAMMODULESCOPE:
            // CODEGENERATION
               code.AddRWToStaticData(1,identifier,reference);
            // ENDCODEGENERATION
               identifierTable.AddToTable(identifier,PROGRAMMODULE_VARIABLE,datatype,reference);
               break;
            case SUBPROGRAMMODULESCOPE:
               sprintf(reference,"FB:0D%d",code.GetFBOffset());
               code.IncrementFBOffset(1);
               identifierTable.AddToTable(identifier,SUBPROGRAMMODULE_VARIABLE,datatype,reference);
               break;
         }
      } while (tokens[0].type == COMMA);

      if(tokens[0].type != ENDLINE) {
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '-.-'");
      }
      GetNextToken(tokens);
   }

   ExitModule("DataDefinitions");
}
//TODO: Handle Voids (NOTYPE)
void ParseFUNCTIONDefinition(TOKEN tokens[])
{
   void ParseFormalParameter(TOKEN tokens[], IDENTIFIERTYPE &identifierType, int &n);
   void ParseDataDefinitions(TOKEN tokens[], IDENTIFIERSCOPE identifierScope);
   void ParseStatement(TOKEN tokens[]);
   void GetNextToken(TOKEN tokens[]);

   bool isInTable;
   DATATYPE datatype;
   char identifier[SOURCELINELENGTH+1];
   char line[SOURCELINELENGTH+1];
   int index;
   char reference[SOURCELINELENGTH+1];

// n = # formal parameters, m = # words of "save-register" space and locally-defined variables/constants
   int n,m;
   char label[SOURCELINELENGTH+1],operand[SOURCELINELENGTH+1],comment[SOURCELINELENGTH+1];

   EnterModule("FUNCTIONDefinition");

   GetNextToken(tokens);
 
   switch(tokens[0].type) {
      case INTDATATYPE:
         datatype = INTEGERTYPE;
         break;
      case BOOLDATATYPE:
         datatype = BOOLEANTYPE;
         break;
      default:
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting INT or BOOL");
   }
   GetNextToken(tokens);

   if(tokens[0].type != IDENTIFIER) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting identifier");
   }
   strcpy(identifier,tokens[0].lexeme);
   index = identifierTable.GetIndex(tokens[0].lexeme,isInTable);
   if (isInTable && identifierTable.IsInCurrentScope(index)) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Multiply-defined identifier");
   }

   identifierTable.AddToTable(identifier,FUNCTION_SUBPROGRAMMODULE,datatype,identifier);
   index = identifierTable.GetIndex(identifier,isInTable);

// CODEGENERATION
   code.EnterModuleBody(FUNCTION_SUBPROGRAMMODULE,index);
   code.ResetFrameData();

   // Reserve frame-space for FUNCTION return value
   code.IncrementFBOffset(1);

   code.EmitUnformattedLine("; **** =========");
   sprintf(line,"; **** FUNCTION module (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);
   code.EmitUnformattedLine("; **** =========");
   code.EmitFormattedLine(tokens[0].lexeme,"EQU","*");
   // ENDCODEGENERATION

   identifierTable.EnterNestedStaticScope();

   GetNextToken(tokens);
   n = 0;
   if (tokens[0].type != OPARENTHESIS) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '('");
   }

   if (tokens[1].type != CPARENTHESIS) {
      do {
         IDENTIFIERTYPE parameterIdentifierType;

         GetNextToken(tokens);
         ParseFormalParameter(tokens,parameterIdentifierType,n);
      } while(tokens[0].type == COMMA);
   }
   else {
      GetNextToken(tokens);
   }

   if (tokens[0].type != CPARENTHESIS) {
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ')'");
      }
   GetNextToken(tokens);

   // CODEGENERATION
   code.IncrementFBOffset(2); // makes room in frame for caller's saved FB register and the CALL return address
   // ENDCODEGENERATION

   ParseDataDefinitions(tokens,SUBPROGRAMMODULESCOPE);

   // CODEGENERATION
   m = code.GetFBOffset()-(n+3);
   code.EmitFormattedLine("","PUSHSP","","set FUNCTION module FB = SP-on-entry + 2(n+3)");
   sprintf(operand,"#0D%d",2*(n+3));
   sprintf(comment,"n = %d",n);
   code.EmitFormattedLine("","PUSH",operand,comment);
   code.EmitFormattedLine("","ADDI");
   code.EmitFormattedLine("","POPFB");
   code.EmitFormattedLine("","PUSHSP","","FUNCTION module SP = SP-on-entry + 2m");
   sprintf(operand,"#0D%d",2*m);
   sprintf(comment,"m = %d",m);
   code.EmitFormattedLine("","PUSH",operand,comment);
   code.EmitFormattedLine("","SUBI");
   code.EmitFormattedLine("","POPSP");
   code.EmitUnformattedLine("; statements to initialize frame data (if necessary)");
   code.EmitFrameData();
   sprintf(label,"MODULEBODY%04d",code.LabelSuffix());
   code.EmitFormattedLine("","CALL",label);
   code.EmitFormattedLine("","PUSHFB","","restore caller's SP-on-entry = FB - 2(n+3)");
   sprintf(operand,"#0D%d",2*(n+3));
   code.EmitFormattedLine("","PUSH",operand);
   code.EmitFormattedLine("","SUBI");
   code.EmitFormattedLine("","POPSP");
   code.EmitFormattedLine("","RETURN","","return to caller");
   code.EmitUnformattedLine("");
   code.EmitFormattedLine(label,"EQU","*");
   code.EmitUnformattedLine("; statements in body of FUNCTION module (*MUST* execute RETURN)");
   // ENDCODEGENERATION

   while (tokens[0].type != ENDFUNC) {
      ParseStatement(tokens);
   }

/*   // CODEGENERATION
   code.EmitFormattedLine("","RETURN");
   code.EmitUnformattedLine("");
   code.EmitUnformattedLine("; **** =========");
   sprintf(line,"; **** END (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);
   code.EmitUnformattedLine("; **** =========");
   code.ExitModuleBody();
   // ENDCODEGENERATION*/

   // CODEGENERATION
   sprintf(operand,"#0D%d",tokens[0].sourceLineNumber);
   code.EmitFormattedLine("","PUSH",operand);
   code.EmitFormattedLine("","PUSH","#0D3");
   code.EmitFormattedLine("","JMP","HANDLERUNTIMEERROR");
   code.EmitUnformattedLine("; **** =========");
   sprintf(line,"; **** END (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);
   code.EmitUnformattedLine("; **** =========");
   code.ExitModuleBody();
   // ENDCODEGENERATION

   identifierTable.ExitNestedStaticScope();

#ifdef TRACECOMPILER
   identifierTable.DisplayTableContents("Contents of identifier table at end of compilation of PROCEDURE module definition");
#endif

   GetNextToken(tokens);

   ExitModule("FUNCTIONDefinition");
}

void ParseFormalParameter(TOKEN tokens[], IDENTIFIERTYPE &identifierType, int &n) {
   void GetNextToken(TOKEN tokens[]);

   char identifier[MAXIMUMLENGTHIDENTIFIER+1],reference[MAXIMUMLENGTHIDENTIFIER+1];
   bool isInTable;
   int index;
   DATATYPE datatype;

   EnterModule("FormalParameter");

   identifierType = IN_PARAMETER;
   sprintf(reference,"FB:0D%d",code.GetFBOffset());
   code.IncrementFBOffset(1);
   n += 1;

   switch ( tokens[0].type )
   {
      case INTDATATYPE:
         datatype = INTEGERTYPE;
         break;
      case BOOLDATATYPE:
         datatype = BOOLEANTYPE;
         break;
      default:
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting INT or BOOL");
   }
   GetNextToken(tokens);

   if(tokens[0].type != IDENTIFIER) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting identifier");
   }
   strcpy(identifier,tokens[0].lexeme);
   GetNextToken(tokens);

   index = identifierTable.GetIndex(identifier,isInTable);
   if (isInTable && identifierTable.IsInCurrentScope(index)) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Multiply-defined identifier");
   }

   identifierTable.AddToTable(identifier,identifierType,datatype,reference);

   ExitModule("FormalParameter");
}

void ParseStatement(TOKEN tokens[]) {
   void ParseAssertion(TOKEN tokens[]);
   void GetNextToken(TOKEN tokens[]);
   void ParsePRINTStatement(TOKEN tokens[]);
   void ParseINPUTStatement(TOKEN tokens[]);
   void ParseAssignmentStatement(TOKEN tokens[]);
   void ParseIFStatement(TOKEN tokens[]);
   void ParseDOWHILEStatement(TOKEN tokens[]);
   void ParseFORStatement(TOKEN tokens[]);
   void ParseCALLStatement(TOKEN tokens[]);
   void ParseRETURNStatement(TOKEN tokens[]);

   EnterModule("Statement");

   switch ( tokens[0].type )
   {
      case PRINT:
         ParsePRINTStatement(tokens);
         break;
      case INPUT:
         ParseINPUTStatement(tokens);
         break;
      case IDENTIFIER:
         ParseAssignmentStatement(tokens);
         break;
      case IF:
         ParseIFStatement(tokens);
         break;
      case DO:
         ParseDOWHILEStatement(tokens);
         break;
      case FOR:
         ParseFORStatement(tokens);
         break;
      case ASSERT:
         ParseAssertion(tokens);
         break;
      case CALL:
         ParseCALLStatement(tokens);
         break;
      case RETURN:
         ParseRETURNStatement(tokens);
         break;
      default:
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                              "Expecting beginning-of-statement");
         break;
   }

   ExitModule("Statement");
}

void ParseAssertion(TOKEN tokens[]) {
   void GetNextToken(TOKEN tokens[]);
   void ParseExpression(TOKEN tokens[], DATATYPE &datatype);

   char line[SOURCELINELENGTH+1];
   DATATYPE datatype;

   EnterModule("Assertion");

   sprintf(line,"; **** %4d: { assertion }",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);

   GetNextToken(tokens);

   if(tokens[0].type != OPARENTHESIS) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '-.--.'");
   }
   GetNextToken(tokens);

   ParseExpression(tokens, datatype);

// STATICSEMANTICS
   if ( datatype != BOOLEANTYPE )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting boolean expression");
// ENDSTATICSEMANTICS

   char Elabel[SOURCELINELENGTH+1],operand[SOURCELINELENGTH+1];

   code.EmitFormattedLine("","SETT");
   sprintf(Elabel,"E%04d",code.LabelSuffix());
   code.EmitFormattedLine("","JMPT",Elabel);
   sprintf(operand,"#0D%d",tokens[0].sourceLineNumber);
   code.EmitFormattedLine("","PUSH",operand);
   code.EmitFormattedLine("","PUSH","#0D1");
   code.EmitFormattedLine("","JMP","HANDLERUNTIMEERROR");
   code.EmitFormattedLine(Elabel,"EQU","*");
   code.EmitFormattedLine("","DISCARD","#0D1");
// ENDCODEGENERATION

   if(tokens[0].type != CPARENTHESIS) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '-.--.-'");
   }
   GetNextToken(tokens);

   ExitModule("Assertion");
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
         case NEWLINE:
            // CODEGENERATION
            code.EmitFormattedLine("","SVC","#SVC_WRITE_ENDL");
            // ENDCODEGENERATION

            GetNextToken(tokens);
            break;
         default:
         {
            ParseExpression(tokens,datatype);
            // CODEGENERATION
            switch (datatype)
            {
               case INTEGERTYPE:
                  code.EmitFormattedLine("","SVC","#SVC_WRITE_INTEGER");
                  break;
               case BOOLEANTYPE:
                  code.EmitFormattedLine("","SVC","#SVC_WRITE_BOOLEAN");
                  break;
            }
            // ENDCODEGENERATION
         }
      }
   } while ( tokens[0].type == COMMA );
   if ( tokens[0].type != ENDLINE )
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                           "Expecting '-.-'");

   GetNextToken(tokens);

   ExitModule("PRINTStatement");
}
void ParseINPUTStatement(TOKEN tokens[]) {
   void ParseVariable(TOKEN tokens[], bool asLValue, DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   char reference[SOURCELINELENGTH+1];
   char line[SOURCELINELENGTH+1];
   DATATYPE datatype;

   EnterModule("INPUTStatement");

   sprintf(line,"; **** INPUT statement (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);

   GetNextToken(tokens);

   if(tokens[0].type == STRING) {
      // CODEGENERATION
      code.AddDSToStaticData(tokens[0].lexeme,"",reference);
      code.EmitFormattedLine("","PUSHA",reference);
      code.EmitFormattedLine("","SVC","#SVC_WRITE_STRING");
      // ENDCODEGENERATION

      GetNextToken(tokens);
   }
   ParseVariable(tokens, true, datatype);

   switch (datatype)
   {
      case INTEGERTYPE:
         code.EmitFormattedLine("","SVC","#SVC_READ_INTEGER");
         break;
      case BOOLEANTYPE:
         code.EmitFormattedLine("","SVC","#SVC_READ_BOOLEAN");
         break;
   }
   code.EmitFormattedLine("","POP","@SP:0D1");
   code.EmitFormattedLine("","DISCARD","#0D1");
// ENDCODEGENERATION

   if (tokens[0].type != ENDLINE) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '-.-'");
   }

   GetNextToken(tokens);

   ExitModule("INPUTStatement");
   //TODO:
}

void ParseIFStatement(TOKEN tokens[]) 
{
   void ParseExpression(TOKEN tokens[], DATATYPE &datatype);
   void ParseStatement(TOKEN tokens[]);
   void GetNextToken(TOKEN tokens[]);

   char line[SOURCELINELENGTH+1];
   char Ilabel[SOURCELINELENGTH+1],Elabel[SOURCELINELENGTH+1];
   DATATYPE datatype;

   EnterModule("IFStatement");

   sprintf(line,"; **** IF statement (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);

   GetNextToken(tokens);

   if(tokens[0].type != OPARENTHESIS) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '-.--.'");
   }
   GetNextToken(tokens);
   ParseExpression(tokens, datatype);
   if(tokens[0].type != CPARENTHESIS) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '-.--.-'");
   }
   GetNextToken(tokens);

   if(datatype != BOOLEANTYPE) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting boolean expression");
   }

   // CODEGENERATION
   sprintf(Elabel,"E%04d",code.LabelSuffix());
   code.EmitFormattedLine("","SETT");
   code.EmitFormattedLine("","DISCARD","#0D1");
   sprintf(Ilabel,"I%04d",code.LabelSuffix());
   code.EmitFormattedLine("","JMPNT",Ilabel);
   // ENDCODEGENERATION

   while( (tokens[0].type != ELSE) && (tokens[0].type != ELSEIF) && (tokens[0].type != ENDFUNC) ) {
      ParseStatement(tokens);
   }

   // CODEGENERATION
   code.EmitFormattedLine("","JMP",Elabel);
   code.EmitFormattedLine(Ilabel,"EQU","*");
   // ENDCODEGENERATION

   while(tokens[0].type == ELSEIF) {
      GetNextToken(tokens);
      if(tokens[0].type != OPARENTHESIS) {
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '-.--.'");
      }
      GetNextToken(tokens);
      ParseExpression(tokens, datatype);
      if(tokens[0].type != CPARENTHESIS) {
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '-.--.-'");
      }
      GetNextToken(tokens);
   

      // CODEGENERATION
         code.EmitFormattedLine("","SETT");
         code.EmitFormattedLine("","DISCARD","#0D1");
         sprintf(Ilabel,"I%04d",code.LabelSuffix());
         code.EmitFormattedLine("","JMPNT",Ilabel);
      // ENDCODEGENERATION
      
      while( (tokens[0].type != ELSE) && (tokens[0].type != ELSEIF) && (tokens[0].type != ENDFUNC) ) {
         ParseStatement(tokens);
      }

      // CODEGENERATION
      code.EmitFormattedLine("","JMP",Elabel);
      code.EmitFormattedLine(Ilabel,"EQU","*");
      // ENDCODEGENERATION
   }
   if(tokens[0].type == ELSE) {
      GetNextToken(tokens);
      while(tokens[0].type != ENDFUNC) {
         ParseStatement(tokens);
      }
   }
   GetNextToken(tokens);

   // CODEGENERATION
   code.EmitFormattedLine(Elabel,"EQU","*");
   // ENDCODEGENERATION

   ExitModule("IFStatement");
}

void ParseDOWHILEStatement(TOKEN tokens[]) 
{
   void ParseExpression(TOKEN tokens[], DATATYPE &datatype);
   void ParseStatement(TOKEN tokens[]);
   void GetNextToken(TOKEN tokens[]);

   char line[SOURCELINELENGTH+1];
   char Dlabel[SOURCELINELENGTH+1],Elabel[SOURCELINELENGTH+1];
   DATATYPE datatype;

   EnterModule("DOWHILEStatement");

   sprintf(line,"; **** DO-WHILE statement (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);

   GetNextToken(tokens);

   // CODEGENERATION
   sprintf(Dlabel,"D%04d",code.LabelSuffix());
   sprintf(Elabel,"E%04d",code.LabelSuffix());
   code.EmitFormattedLine(Dlabel,"EQU","*");
   // ENDCODEGENERATION

   while(tokens[0].type != WHILE) {
      ParseStatement(tokens);
   }
   GetNextToken(tokens);
   if(tokens[0].type != OPARENTHESIS) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '-.--.'");
   }
   GetNextToken(tokens);
   ParseExpression(tokens, datatype);
   if(tokens[0].type != CPARENTHESIS) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '-.--.-'");
   }
   GetNextToken(tokens);
   if(datatype != BOOLEANTYPE) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting boolean expression");
   }

   // CODEGENERATION
   code.EmitFormattedLine("","SETT");
   code.EmitFormattedLine("","DISCARD","#0D1");
   code.EmitFormattedLine("","JMPNT",Elabel);
   // ENDCODEGENERATION
   GetNextToken(tokens);

   // CODEGENERATION
   code.EmitFormattedLine("","JMP",Dlabel);
   code.EmitFormattedLine(Elabel,"EQU","*");
   // ENDCODEGENERATION
/*
   while(tokens[0].type != ENDFUNC) {
      ParseStatement(tokens);
   }
   GetNextToken(tokens);

   // CODEGENERATION
   code.EmitFormattedLine("","JMP",Dlabel);
   code.EmitFormattedLine(Elabel,"EQU","*");
   // ENDCODEGENERATION
*/
   ExitModule("DOWHILEStatement");
}

void ParseFORStatement(TOKEN tokens[])
{
   void ParseVariable(TOKEN tokens[], bool asLValue, DATATYPE &datatype);
   void ParseExpression(TOKEN tokens[], DATATYPE &datatype);
   void ParseStatement(TOKEN tokens[]);
   void GetNextToken(TOKEN tokenks[]);

   char line[SOURCELINELENGTH+1];
   char Dlabel[SOURCELINELENGTH+1],Llabel[SOURCELINELENGTH+1],
        Clabel[SOURCELINELENGTH+1],Elabel[SOURCELINELENGTH+1];
   char operand[SOURCELINELENGTH+1];
   DATATYPE datatype;

   EnterModule("FORStatement");

   sprintf(line,"; **** FOR statement (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);

   GetNextToken(tokens);

   if(tokens[0].type != OPARENTHESIS) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '-.--.'");
   }
   GetNextToken(tokens);

   ParseVariable(tokens, true, datatype);

   if(datatype != INTEGERTYPE) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting integer variable");
   }

   if(tokens[0].type != ASSIGNMENT) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '-...-'");
   }
   GetNextToken(tokens);

   ParseExpression(tokens, datatype);
   if(datatype != INTEGERTYPE) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting integer data type");
   }

   // CODEGENERATION
   code.EmitFormattedLine("","POP","@SP:0D1");
   // ENDCODEGENERATION

   if(tokens[0].type != ENDLINE) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting -.-");
   }
   GetNextToken(tokens);

//Mid Expression
   ParseExpression(tokens, datatype);
   if(datatype != INTEGERTYPE) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting boolean data type");
   }

   if(tokens[0].type != ENDLINE) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting -.-");
   }
   GetNextToken(tokens);

   ParseExpression(tokens, datatype);
   if(datatype != INTEGERTYPE) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting integer data type");
   }

   if(tokens[0].type != CPARENTHESIS) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '-.--.-'");
   }
   GetNextToken(tokens);

   //for ( i = 1 . i < 3 . i = i + 1 )

   // CODEGENERATION
   sprintf(Dlabel,"D%04d",code.LabelSuffix());
   sprintf(Llabel,"L%04d",code.LabelSuffix());
   sprintf(Clabel,"C%04d",code.LabelSuffix());
   sprintf(Elabel,"E%04d",code.LabelSuffix());

   code.EmitFormattedLine("","SETNZPI");
   code.EmitFormattedLine("","JMPNZ",Dlabel);
   sprintf(operand,"#0D%d",tokens[0].sourceLineNumber);
   code.EmitFormattedLine("","PUSH",operand);
   code.EmitFormattedLine("","PUSH","#0D2");
   code.EmitFormattedLine("","JMP","HANDLERUNTIMEERROR");

   code.EmitFormattedLine(Dlabel,"SETNZPI");
   code.EmitFormattedLine("","JMPN",Llabel);
   code.EmitFormattedLine("","SWAP");
   code.EmitFormattedLine("","MAKEDUP");
   code.EmitFormattedLine("","PUSH","@SP:0D3");
   code.EmitFormattedLine("","SWAP");
   code.EmitFormattedLine("","CMPI");
   code.EmitFormattedLine("","JMPLE",Clabel);
   code.EmitFormattedLine("","JMP",Elabel);
   code.EmitFormattedLine(Llabel,"SWAP");
   code.EmitFormattedLine("","MAKEDUP");
   code.EmitFormattedLine("","PUSH","@SP:0D3");
   code.EmitFormattedLine("","SWAP");
   code.EmitFormattedLine("","CMPI");
   code.EmitFormattedLine("","JMPGE",Clabel);
   code.EmitFormattedLine("","JMP",Elabel);
   code.EmitFormattedLine(Clabel,"EQU","*");
   // ENDCODEGENERATION

   while(tokens[0].type != ENDFUNC) {
      ParseStatement(tokens);
   }

   GetNextToken(tokens);
   // CODEGENERATION
   code.EmitFormattedLine("","SWAP");
   code.EmitFormattedLine("","MAKEDUP");
   code.EmitFormattedLine("","PUSH","@SP:0D3");
   code.EmitFormattedLine("","ADDI");
   code.EmitFormattedLine("","POP","@SP:0D3");
   code.EmitFormattedLine("","JMP",Dlabel);
   code.EmitFormattedLine(Elabel,"DISCARD","#0D3");
   // ENDCODEGENERATION

   ExitModule("FORStatement");
}

void ParseCALLStatement(TOKEN tokens[])
{
   void GetNextToken(TOKEN tokens[]);
   void ParseVariable(TOKEN tokens[],bool asLValue,DATATYPE &datatype);
   void ParseExpression(TOKEN tokens[],DATATYPE &datatype);

   char line[SOURCELINELENGTH+1];
   bool isInTable;
   int index,parameters;

   EnterModule("CALLStatement");

   sprintf(line,"; **** CALL statement (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);

   GetNextToken(tokens);

   if(tokens[0].type != IDENTIFIER) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting identifier");
   }
   // STATICSEMANTICS
   index = identifierTable.GetIndex(tokens[0].lexeme,isInTable);
   if (!isInTable) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Undefined identifier");
   }
   if (identifierTable.GetType(index) != FUNCTION_SUBPROGRAMMODULE) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting FUNCTION identifier");
   }
   // ENDSTATICSEMANTICS
   GetNextToken(tokens);
   parameters = 0;

   if(tokens[0].type == OPARENTHESIS) {
      DATATYPE expressionDatatype, variableDatatype;
      do {
         GetNextToken(tokens);
         parameters++;

         // CODEGENERATION   
         // STATICSEMANTICS
         switch ( identifierTable.GetType(index+parameters) )
         {
            case IN_PARAMETER:
               ParseExpression(tokens,expressionDatatype);
               if (expressionDatatype != identifierTable.GetDatatype(index+parameters)) {
                  ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                     "Actual parameter data type does not match formal parameter data type");
               }
               break;
         }
         // ENDSTATICSEMANTICS
         // ENDCODEGENERATION
      } while(tokens[0].type == COMMA);

      if(tokens[0].type != CPARENTHESIS) {
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting -.--.-");
      }
      GetNextToken(tokens);
   }

   // STATICSEMANTICS
   if (identifierTable.GetCountOfFormalParameters(index) != parameters) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
         "Number of actual parameters does not match number of formal parameters");
   }
   // ENDSTATICSEMANTICS

   // CODEGENERATION
   code.EmitFormattedLine("","PUSHFB");
   code.EmitFormattedLine("","CALL",identifierTable.GetReference(index));
   code.EmitFormattedLine("","POPFB");
   for (parameters = identifierTable.GetCountOfFormalParameters(index); parameters >= 1; parameters--)
   {
      switch ( identifierTable.GetType(index+parameters) )
      {
         case IN_PARAMETER:
            code.EmitFormattedLine("","DISCARD","#0D1");
            break;
      }
   }
   // ENDCODEGENERATION

   if(tokens[0].type != ENDLINE) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '-.-'");
   }
   GetNextToken(tokens);

   ExitModule("CALLStatement");
}

void ParseRETURNStatement(TOKEN tokens[]) 
{
   void GetNextToken(TOKEN tokens[]);
   void ParseExpression(TOKEN tokens[], DATATYPE &datatype);

   char line[SOURCELINELENGTH+1];

   EnterModule("RETURNStatement");

   sprintf(line,"; **** RETURN statement (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);

   GetNextToken(tokens);

   if(code.IsInModuleBody(FUNCTION_SUBPROGRAMMODULE)) {
      DATATYPE expressionDatatype;

      if ( tokens[0].type != OPARENTHESIS )
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '('");
      GetNextToken(tokens);
   
      ParseExpression(tokens,expressionDatatype);

      if ( expressionDatatype != identifierTable.GetDatatype(code.GetModuleIdentifierIndex()) )
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
            "RETURN expression data type must match FUNCTION data type");
   
      code.EmitFormattedLine("","POP","FB:0D0","pop RETURN expression into function return value");
      code.EmitFormattedLine("","RETURN");

      if ( tokens[0].type != CPARENTHESIS )
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ')'");
      GetNextToken(tokens);
   }
   else {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
         "RETURN only allowed in FUNCTION module body");
   }

   if (tokens[0].type != ENDLINE) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting -.-");
   }

   GetNextToken(tokens);

   ExitModule("RETURNStatement");
}

void ParseAssignmentStatement(TOKEN tokens[]) 
{
   void ParseVariable(TOKEN tokens[], bool asLValue, DATATYPE &datatype);
   void ParseExpression(TOKEN tokens[], DATATYPE &datatype);
   void GetNextToken(TOKEN tokens[]);

   char line[SOURCELINELENGTH+1];
   DATATYPE datatypeLHS, datatypeRHS;
   int n;

   EnterModule("AssignmentStatement");

   sprintf(line,"; **** assignment statement (%4d)",tokens[0].sourceLineNumber);
   code.EmitUnformattedLine(line);

   ParseVariable(tokens, true, datatypeLHS);
   n = 1;
   while(tokens[0].type == COMMA) {
      DATATYPE datatype;

      GetNextToken(tokens);
      ParseVariable(tokens, true, datatype);
      n++;

      if(datatype != datatypeLHS) {
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Mixed-mode variables not allowed");
      }
   }
   if(tokens[0].type != ASSIGNMENT) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '-...-'");
   }
   GetNextToken(tokens);

   ParseExpression(tokens, datatypeRHS);
   if(datatypeLHS != datatypeRHS) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Data type mismatch");
   }

   // CODEGENERATION
   for (int i = 1; i <= n; i++)
   {
      code.EmitFormattedLine("","MAKEDUP");
      code.EmitFormattedLine("","POP","@SP:0D2");
      code.EmitFormattedLine("","SWAP");
      code.EmitFormattedLine("","DISCARD","#0D1");
   }
   code.EmitFormattedLine("","DISCARD","#0D1");
   // ENDCODEGENERATION
   if(tokens[0].type != ENDLINE) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '-.-'");
   }
   GetNextToken(tokens);

   ExitModule("AssignmentStatement");
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
   void ParseVariable(TOKEN tokens[], bool asLValue, DATATYPE &datatype);
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
      case IDENTIFIER:
         {
            bool isInTable;
            int index;

            index = identifierTable.GetIndex(tokens[0].lexeme,isInTable);
            if(!isInTable) {
               ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Undefined identifier");
            }
            //Variable Ref
            if(identifierTable.GetType(index) != FUNCTION_SUBPROGRAMMODULE) {
               ParseVariable(tokens, false, datatype);
            }
            else {
               //Function sub_programmodule
               char operand[MAXIMUMLENGTHIDENTIFIER+1];
               int parameters;

               GetNextToken(tokens);
               if(tokens[0].type != OPARENTHESIS) {
                  ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting '('");
               }
               // CODEGENERATION
               code.EmitFormattedLine("","PUSH","#0X0000","reserve space for function return value");
               // ENDCODEGENERATION

               datatype = identifierTable.GetDatatype(index);
               parameters = 0;
               if (tokens[1].type == CPARENTHESIS) {
                  GetNextToken(tokens);
               }
               else {
                  do {
                     DATATYPE expressionDatatype;

                     GetNextToken(tokens);
                     ParseExpression(tokens,expressionDatatype);
                     parameters++;
                     
// STATICSEMANTICS
                     if(expressionDatatype != identifierTable.GetDatatype(index+parameters)) {
                        ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                           "Actual parameter data type does not match formal parameter data type");
                     }
// ENDSTATICSEMANTICS

                  }while(tokens[0].type == COMMA);
               }
               // STATICSEMANTICS
               if(identifierTable.GetCountOfFormalParameters(index) != parameters) {
                  ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,
                     "Number of actual parameters does not match number of formal parameters");
               }
                     // ENDSTATICSEMANTICS

               if(tokens[0].type != CPARENTHESIS) {
                  ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting ')'");
               }
               GetNextToken(tokens);

               // CODEGENERATION
               code.EmitFormattedLine("","PUSHFB");
               code.EmitFormattedLine("","CALL",identifierTable.GetReference(index));
               code.EmitFormattedLine("","POPFB");
               sprintf(operand,"#0D%d",parameters);
               code.EmitFormattedLine("","DISCARD",operand);
               // ENDCODEGENERATION

            }
         }
         break;
      default:
         ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting integer, true, false, or (");
         break;
   }
   ExitModule("Primary");
}
void ParseVariable(TOKEN tokens[], bool asLValue, DATATYPE &datatype) {
   void GetNextToken(TOKEN tokens[]);

   bool isInTable;
   int index;
   IDENTIFIERTYPE identifierType;

   EnterModule("Variable");

   if(tokens[0].type != IDENTIFIER) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Expecting identifier");
   }
   index = identifierTable.GetIndex(tokens[0].lexeme,isInTable);
   if(!isInTable) {
      ProcessCompilerError(tokens[0].sourceLineNumber,tokens[0].sourceLineIndex,"Undefined identifier");
   }

   identifierType = identifierTable.GetType(index);
   datatype = identifierTable.GetDatatype(index);

   if(asLValue) {
      code.EmitFormattedLine("","PUSHA",identifierTable.GetReference(index));
   }
   else {
      code.EmitFormattedLine("","PUSH",identifierTable.GetReference(index));
   }

   GetNextToken(tokens);
   ExitModule("Variable");
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
   else if(isdigit(nextCharacter)) 
   {
      i = 0;
      lexeme[i++] = nextCharacter;
      nextCharacter = reader.GetNextCharacter().character;
      while ( isdigit(nextCharacter) )
      {
         lexeme[i++] = nextCharacter;
         nextCharacter = reader.GetNextCharacter().character;
      }
      lexeme[i] = '\0';
      type = INTEGER;
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