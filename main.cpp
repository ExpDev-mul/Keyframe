/*

  Keyframe Language Interpreter


  All rights reserved to Or Pinto ©

*/

// Library inclusions
#include <iostream>
#include <string>
#include <vector>


// Helpers
int stringIsNumber(std::string str){
  /*

    0 - Not a number
    1 - An integer
    2 - A decimal
    
  */
  
  if (str.empty()) return 0;

  unsigned short points = 0;
  for (unsigned int i = 0; i < str.length(); i++){
    if (str[i] == '.'){
      points++;
      if (points > 1){
        return 0;
      }

      continue;
    }
    
    if (!isdigit(str[i])){
      // If it is not a valid digit or a dot, then this not not be a string.
      return 0;
    }
  }

  return (points == 0 ? 1 : 2);
}

class Token
{
  /*

    A token class providing flexible construction of input tokens.
    Token(std::string type, std::string value)

  */

  public:
    std::string type;
    std::string value;
    Token(std::string t, std::string v) : type(t), value(v) {}
    void print() const
    {
      // Print out the token metadata    
      std::cout << "Token(" << type << ", " << value << ")" << std::endl;
    }
};



class Lexer{
  public:
    // Public variables
    std::string& input;

    // Constructor
    Lexer(std::string& input) : input(input) {}

    std::vector<Token> tokenize() const{
      std::vector<Token> tokens; // A vector consisting of all the tokens
      
      // Recognized symbols
      char symbols[] = {':', ',', '(', ')', '{', '}', '[', ']', '='};
      std::string keywords[] = {"dec", "print", "if", "true", "false"};
      
      std::string capture = ""; // A substring of each token capture at a given pos
      size_t pos = 0; // A sliding pointer along the string to capture individual chars
      while (pos < input.size()){ // Iterate through the whole string
        char curr = input[pos]; // Store the current char

        if (curr == '\n'){
          // Breaking into a new line
          continue;
        }


        bool isSymbol = false;
        for (unsigned int i = 0; i < sizeof(symbols); i++){
          if (curr == symbols[i]){
            // Found symbol
            isSymbol = true;
            break;
          }
        }
        
        if (std::isspace(curr) || isSymbol){
          // When spaces or any symbols are captured we have to push back the current token
          if (capture.size() > 0){ // There is a token to capture
            bool recognized = false;
            for (unsigned int i = 0; i < sizeof(keywords); i++){
              if (capture == keywords[i]){
                // This is a recognized keyword
                tokens.push_back(Token("keyword", capture));
                recognized = true;
                break;
              }
            }

            if (!recognized){
              // Undetected symbol
              if (capture[0] == '"' && capture[capture.size() - 1] == '"'){
                tokens.push_back(Token("string", capture));
              } else if (stringIsNumber(capture)) {
                tokens.push_back(Token("number", capture));
              } else {
                tokens.push_back(Token("unknown", capture));
              }
            }
            
            capture = "";

            if (isSymbol){
              std::string symbol = std::string(1, curr);
              tokens.push_back(Token("symbol", symbol));
            }
          }
        } else {
          capture += curr; // Concat current char to capture string
        }
        
        pos++; // Advance the sliding pointer
      }

      if (capture.size() > 0){ // There is a token to capture
        if (capture == "dec"){
          // Variable declaration
          tokens.push_back(Token("keyword", "declare"));
        } else {
          // Undetected symbol
          if (capture[0] == '"' && capture[capture.size() - 1] == '"'){
            tokens.push_back(Token("string", capture));
          } else if (stringIsNumber(capture)) {
            tokens.push_back(Token("number", capture));
          } else {
            tokens.push_back(Token("unknown", capture));
          }
        }
      }

      return tokens;
    }
};

std::vector<std::string> constructMemoryVariable( std::string& type, 
                                                  std::string& name, 
                                                  std::string& value){
  std::vector<std::string> variable;
  variable.push_back(type);
  variable.push_back(name);
  variable.push_back(value);
  return variable;
}

class Interpreter{
  /* 
    The interpreter receives tokens and executes the code. 
  */

  public:
    std::vector<Token> tokens;
    std::vector<std::vector<std::string>> memory;
    /*

      The memory layout is as follows:

      [ [ variable_type, variable_name, variable_value ], ... ]

    */

    Interpreter(std::vector<Token> tokens) : tokens(tokens) {}

    std::vector<std::string> findVariable(std::string name){
      for (unsigned int i = 0; i < memory.size(); i++){
        if (memory[i][1] == name){
          return memory[i];
        }
      }

      return std::vector<std::string>();
    }

    void execute(){      
      // Iterate through the tokens

      std::cout << std::endl << std::endl;
      std::cout << "Program Execution Output:" << std::endl;
      
      size_t i = 0;
      while (i < tokens.size()){
        // First run through, evaluating any boolean expressions or mathematical expressions
        Token curr = tokens[i];
        if (curr.type == "symbol" && curr.value == "="){
          Token left = tokens[i - 1];
          Token right = tokens[i + 1];
          if (left.type == right.type){
            tokens.erase(tokens.begin() + i - 1);
            tokens.erase(tokens.begin() + i - 1);
            tokens[i - 1].type = "boolean";
            if (left.value == right.value){
              tokens[i - 1].value = "true";
            } else {
              tokens[i - 1].value = "false";
            }
          } else {
            // Comparison between different types.
            std::cout << "Attempt to compare diffferent types" << std::endl;
          }
        }

        i++;
      }

      for (unsigned int i = 0; i < tokens.size(); i++){
        tokens[i].print();
      }
      
      i = 0;
      while (i < tokens.size()){
        Token curr = tokens[i];
        if (curr.type == "keyword"){
          if (curr.value == "dec"){
            // Variable declaration
            Token name = tokens[i + 1];
            if (name.type == "unknown"){
              Token symbol = tokens[i + 2];
              if (symbol.type == "symbol" &&  symbol.value == ":"){
                Token value = tokens[i + 3];;
                if (value.type == "string"){
                  std::vector<std::string> memoryVariable = constructMemoryVariable(value.type, name.value, value.value);
                  memory.push_back(memoryVariable);
                }
              }
            }
          }

          if (curr.value == "print"){
            // Printing
            const Token leftBracket = tokens[i + 1];
            if (leftBracket.type == "symbol" && leftBracket.value == "("){
              const Token value = tokens[i + 2];
              if (value.type == "string" || value.type == "number" || value.type == "boolean"){
                const Token rightBracket = tokens[i + 3];
                if (rightBracket.type == "symbol" && rightBracket.value == ")"){
                  std::cout << value.value << std::endl;
                }
              }

              if (value.type == "unknown"){
                // Attempt to print out a variable
                std::vector<std::string> variable = findVariable(value.value);
                if (variable.size() == 0){
                  std::cout << "Variable " << value.value << " not found.";
                } else {
                  std::cout << variable[2] << std::endl;
                }
              }
            }
          }
        }

        i++;
      }

      printMemory();
    }

    void printMemory(){
      std::cout << std::endl << std::endl;
      std::cout << "Full Memory Log: " << std::endl;
      for (unsigned int i = 0; i < memory.size(); i++){
        std::cout << "[" << memory[i][0] << ", " << memory[i][1] << " = " << memory[i][2] << "]" << std::endl;
      }
    }
};

int main() {
  std::string test = "dec hello: 6=6";
  Lexer lexer = Lexer(test);
  std::vector<Token> tokens = lexer.tokenize();

  for (unsigned int i = 0; i < tokens.size(); i++){
    tokens[i].print();
  }

  Interpreter intr = Interpreter(tokens);
  intr.execute();
}
