//Author: Ethan Shook
//Date: 11-2-2020
//Description: Lexical and syntactical analyzer for a set of grammar rules defined
//             for the ficticious DanC language.

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <map>
#include <sstream>
#include <iterator>
#include <algorithm>

using std::cout; using std::endl; using std::string;

//Declare static variables for program.
static std::map<string, string> lookups;
static std::ifstream content;
static char nextChar;
static string lexeme;
static string previousLexeme;
static string token;
static int charClass;

//Initialize static variables
static string outputSpaces = "        ";
static bool isValidCode = true;
static int line = 1;

//Create enumerated types for charClass
enum charClass{ UNKNOWN = 0, LETTER = 1, DIGIT = 2} ;

//Create possible comparison operators
//static std::vector<string> comparisonOps {"LESSER_OP", "GREATER_OP", "EQUAL_OP", "NEQUAL_OP", "LEQUAL_OP", "GEQUAL_OP"}; //I didn't want to use c++11 anyways
static char* tempArray[] = {"LESSER_OP", "GREATER_OP", "EQUAL_OP", "NEQUAL_OP", "LEQUAL_OP", "GEQUAL_OP"};
static std::vector<string> comparisonOps(tempArray, tempArray+6);

//Store error messages
static std::vector<std::pair<int, string> >  errorMessages;

//Forward declare functions.
void CodeAnalysis();
void CreateLookups();
void GetChar();
void AddChar();
void GetLexeme();
void GetFileContents(char*);

void SyntacticAnalysis();
void Comparison();
void WhileStatement();
void WhileInterior();
void ReadWriteStatement();
void Assignment();
void Expression();
void Term();
void Factor();
void GetNewLine();
void PrintErrors();
void Error(int, string);


int main(int argc, char* argv[])
{
  cout << "DanC Parser :: R11469438" << endl;

  if(argc < 2) //Check for a second argument
  {
    Error(2, "Not enough arguments to parser provided. Please provide the file of execution and the file name to parse.");
    return 2; //Return 2 if no file path was provided
  }
  //Get the file open to an ifstream
  GetFileContents(argv[1]);
  if (!isValidCode) //If an error was thrown, return 3
    return 3;

  CodeAnalysis(); //Do lexical and syntactic analysis

  content.close(); //Close the ifstream

  if(!isValidCode) //If the code isn't valid and hasn't stopped yet, it is a syntactic error, return 1
  {
    PrintErrors();
    return 1;
  }

  return 0; //If the code finished execution sucessfully, and there were no errors, return 0.
}

void CodeAnalysis()
{
  CreateLookups(); //Create the lookup table

  GetChar();
  //Get a lexeme, and run through the first statement
  GetLexeme();

  SyntacticAnalysis();
  while(nextChar != EOF)
  {
    //Once the first statement is finished, all of the rest may now be processed.
    //These two 'if's must be at the top, otherwise the language rule S' -> ;SS' is violated.
    //S statements do not end with a semicolon.
    if (token == "UNKNOWN")
    {
      Error(1, "Errant unknown token: " + lexeme);
      return;
    }
    else if (token != "SEMICOLON") //If we are to have another statement
    {
      line--; //This will already be pointing to the next lexeme typically before it realizes an error has occurred.
      Error(1, "Missing end of line ';' before " + lexeme);
      line++;
      return;
    }
    GetLexeme();

    SyntacticAnalysis();
  }
}

void SyntacticAnalysis()
{
  //lexeme.erase(std::remove(lexeme.begin(), lexeme.end(), '\n'), lexeme.end());
  //From the top, check what lexeme we have, one of the following if/else must return true, or the language is violated.
  if(token == "IDENT") // From top. If V:=E
  {
    GetLexeme();

    if(token == "ASSIGN_OP") //The next lexeme is this
    {
      GetLexeme();
      Expression();
      if (token == "RIGHT_PAREN") //Check a potential error, for specific feedback
      {
        Error(1, "Right parenthesis ')' missing opening '('");
        return;
      }
    }
    else
    {
      Error(1, "Invalid argument '" + lexeme + "' following IDENT"); //IF a statement starts with an IDENT and is not followed by assignment, it is invalid.
      return;
    }
  }
  else if(token == "KEY_READ" || token == "KEY_WRITE")
  {
    GetLexeme();
    ReadWriteStatement();
  }
  else if(token == "KEY_WHILE")
  {
      GetLexeme();
      WhileStatement();
      //cout << lexeme << outputSpaces.substr(0,7-lexeme.length()) << token << endl;
  }
  else
  {
    if(token == "KEY_OD") //Check for a specific error
    {
      line--;
      Error(1, "Errant semicolon preceeding od.");
      line++;
    }
    else //Catch non specific errors.
      Error(1, "Errant unknown token: " + lexeme);
  }
}
//Handles while C do S od
void WhileStatement()
{
  Comparison(); //Check that the comparison is valid

  if (token == "KEY_DO") //If the token following the comparison is do, it is valid.
  { //Handle the interior S and od of the while loop
    WhileInterior();
  }
  else
  {
      Error(1, "Improper heading to while statement, did you mean to write 'do' after '" + lexeme + "'?");
      GetNewLine(); //Handle error checking the interior, ignoring the rest of the 'while' line.
      //Will not work if the while statement is split onto multiple lines. Only a demon would do such a thing, however
      WhileInterior();
  }
  //cout << "here " << line << " "<< lexeme << outputSpaces.substr(0,7-lexeme.length()) << token << endl;
}
//Handles the interior S od of while C do S od
void WhileInterior()
{
  //Get lexeme and process the line
  GetLexeme();  //Just like earlier, an S statement is not followed by a semi colon, which each preceeding statement (if any) is, that is not a loop.
  SyntacticAnalysis();
  while (token != "KEY_OD")
  {
    if (token != "SEMICOLON") //If we are to have another statement
    {
      line--;
      Error(1, "Missing end of line ';' before " + lexeme);
      line++;
      GetLexeme;
      return;
    }
    else if (nextChar == EOF) //Make sure this loop ends eventually.
    {
      Error(1, "No closure to while loop.");
    }
    //Process S
    GetLexeme();
    SyntacticAnalysis();
  } //No ; should preceede KEY_DO
  GetLexeme();
}
//Handles C
void Comparison()
{
  Expression(); //Process one expression
  GetLexeme();
  //Check for a single valid comparison operator
  if (std::find(comparisonOps.begin(), comparisonOps.end(), token) != comparisonOps.end())
  {
    Expression(); //Process the next expression
  }
  GetLexeme();
}

//Handles read(V) and write(V)
void ReadWriteStatement()
{
  //Check read is followed properly by (V), otherwise, there is an error.
  if (token == "LEFT_PAREN")
  {
    GetLexeme();

    if(token == "IDENT")
    {
      GetLexeme();

      if(token == "RIGHT_PAREN")
      {
        GetLexeme();
         //Now return
      }
      else
        Error(1, "Invalid closure to read statement, did you mean to write ')'?");
    }
    else
      Error(1, lexeme + " is an invalid interior of read statement.");
  }
  else
    Error(1, lexeme + " is an invalid character following read statement.");
}
//Handles E
void Expression()
{
  Term();//Check for a leading term

  while (token == "ADD_OP" || token == "SUB_OP") //Check for an operator between terms
  {
    GetLexeme();

    if (token == "RIGHT_PAREN") //Check for errant right parens with no closure.
    {
      Error(1, "Right parenthesis ')' missing opening '('");
      return;
    }
    Term(); //Check for each following term after an operator
  }
}
//Handles T
void Term()
{
  Factor();//Check for a leading factor

  while (token == "MULT_OP" || token == "DIV_OP")//Check for an operator between factors
  {
    GetLexeme();

    if (token == "RIGHT_PAREN") //Check for an errant right parents with no closure
    {
      Error(1, "Right parenthesis ')' missing opening '('");
      return;
    }
    Factor(); //Check for each following term after an operator
  }
}
//Handles F
void Factor()
{
  if (token == "IDENT" || token == "INT_LIT") //Check for a leading integer or IDENT
  {
    GetLexeme();

  }
  else
  {
    if(token == "LEFT_PAREN") //If a parens is seen, an expression follows
    {
      GetLexeme();

      Expression();
      if(token == "RIGHT_PAREN") //When the expression finishes, if the next is not a right paren, an error is thrown
      {
        GetLexeme();

      }
      else
      {
        Error(1, "Left parenthesis '(' missing closure ')'.");
        return;
      }
    }
    else
    {
      Error(1, "Expected expression following operator. Instead saw: " + lexeme); //Must have an expression following a left parenthesis, cannot be null
      return;
    }
  }
}

//Handles errors by code and message to cut down on error message lines, and clean interior of functions
void Error(int errorCode, string errorMsg)
{
  isValidCode = false;
  if(errorCode == 1) //Syntactic error
  {
    //No duplicates, check for errors on the same line, which are typically errant
    for(std::vector<std::pair<int, string> >::iterator index = errorMessages.begin(); index != errorMessages.end(); index++)
    {
      if(index->first == line)
        return;
    }
    //Push a line/msg pair onto the messages vector, to report multiple errors.
    std::pair<int, string> error(line, errorMsg);
    errorMessages.push_back(error);
  }
  else if(errorCode == 2) //No file provided
  {
    cout << "ERROR : " <<  errorMsg << endl << "Exited with code: " << errorCode << endl;
  }
  else if(errorCode == 3) //Invalid file path
  {
    cout << "ERROR : " <<  errorMsg << endl << "Exited with code: " << errorCode << endl;
  }
}
//Prints all errors caught in program to the screen
void PrintErrors()
{
  for(std::vector<std::pair<int, string> >::iterator index = errorMessages.begin(); index != errorMessages.end(); index++)
  {
    cout << "ERROR on line: " << index->first << " | " << index->second << endl;
  }
}

void GetChar()
{
  nextChar = content.get();
    if(isalpha(nextChar))
      charClass = LETTER;
    else if(isdigit(nextChar))
      charClass = DIGIT;
    else
      charClass = UNKNOWN;
}

void AddChar()
{
  if(!isspace(nextChar))
    lexeme += nextChar;
}

void GetNewLine()
{
  while(nextChar != '\n')
  {
    GetChar();
  }
  line++;
}

void GetNonBlank()
{
  while(isspace(nextChar))
  {
    if (nextChar == '\n')
      line++;
    GetChar();
  }
}

////////////// Main Handler for Lexical Analysis //////////////////

//Handle getting a single lexeme and determing it's token value.
void GetLexeme()
{
  lexeme = "";
  token = "";
  //If the next character is a blank, keep going until a nonblank is encountered then evaluate switch
  GetNonBlank();
  //Initialize an iterator to go through the lookups table
  std::map<string, string>::iterator value;
  switch(charClass)
  {
    case LETTER:
    //If new lexeme starts with a letter, continue until a nonchar or nondigit is reached.
      while(charClass == LETTER)
      {
        AddChar();
	      GetChar();
        if(charClass == DIGIT) //Digits are not allowed in IDENTs
        {
          Error(1, "Digits are not allowed in an identifier.");
          GetLexeme();
        }
      }
      value = lookups.find(lexeme);
      if(value != lookups.end())//If the lexeme is in the lookup table after loop finishes, it is not an IDENT.
      {
        token = value->second;
	      break;
      }
      else//Else, if it isn't in the table, it is an IDENT
      {
        token = "IDENT";
        break;
      }

    case DIGIT:
    //Keep adding to the lexeme if the next char is a digit.
      while(charClass == DIGIT)
      {
        AddChar();
	      GetChar();
      }
      //When a non digit is reached, break and token as INT_LIT
      token = "INT_LIT";
      break;

    case UNKNOWN:
    //If the next char isn't any of these. This while will execute a maximum of two times based on valid lexemes
      while(charClass != LETTER && charClass != DIGIT && nextChar != EOF)
      { //Comparison operatos such as 1 < 2 < 3 is not legal in our language
        AddChar();
        //Special case, if the first char is :, check if the next is =. If it is, continue and proceed to the next if check.
        if(lexeme == ":")
	      {
          if(content.peek() == '=')
            GetChar();
            continue;
	      }
  	 value = lookups.find(lexeme);
     //If lexeme is in the lookup table.
  	 if(value != lookups.end())
	   {
       //token is the value of the key-value pair.
	     token = value->second;

  	   if(lexeme  == "<" || lexeme == ">")//Ensure that these are not followed by > or =, which is an entirely different lexeme.
  	     if(content.peek() == '>' || content.peek() == '=')//Char literal to prevent decay to pointer
  	     {
  	       GetChar();
  	       continue;
  	     }

	    GetChar(); //If we break, GetChar still must iterate the pointer to prevent an infinite loop. Set up for next lexeme.
	    break;
  	  }
  	  else
  	  {
  	    token = "UNKNOWN";
  	    GetChar();
  	    break;
  	  }
    }
    break;
  }

  //cout << lexeme << outputSpaces.substr(0,7-lexeme.length()) << token << endl;
}

//////////// Utility //////////////////
//Create a lookup table for most lexeme/token pairs
void CreateLookups()
{
  lookups.insert(std::make_pair(":=", "ASSIGN_OP"));
  lookups.insert(std::make_pair("<", "LESSER_OP"));
  lookups.insert(std::make_pair(">", "GREATER_OP"));
  lookups.insert(std::make_pair("=", "EQUAL_OP"));
  lookups.insert(std::make_pair("<>", "NEQUAL_OP"));
  lookups.insert(std::make_pair("<=", "LEQUAL_OP"));
  lookups.insert(std::make_pair(">=", "GEQUAL_OP"));
  lookups.insert(std::make_pair("+", "ADD_OP"));
  lookups.insert(std::make_pair("-", "SUB_OP"));
  lookups.insert(std::make_pair("*", "MULT_OP"));
  lookups.insert(std::make_pair("/", "DIV_OP"));
  lookups.insert(std::make_pair("read", "KEY_READ"));
  lookups.insert(std::make_pair("write", "KEY_WRITE"));
  lookups.insert(std::make_pair("while", "KEY_WHILE"));
  lookups.insert(std::make_pair("do", "KEY_DO"));
  lookups.insert(std::make_pair("od", "KEY_OD"));
  lookups.insert(std::make_pair("(", "LEFT_PAREN"));
  lookups.insert(std::make_pair(")", "RIGHT_PAREN"));
  lookups.insert(std::make_pair(";", "SEMICOLON"));
}

///////////// Get Data from File and Clean it ////////////////

//Open file and assign it globally
void GetFileContents(char* path)
{
  //std::ifstream file(path);
  content.open(path);

  if(!content)//Make sure file exists. Stop program if path is invalid
  {
    Error(3, "Invalid file path to source code for parsing.");
  }
}
