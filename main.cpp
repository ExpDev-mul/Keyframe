/*

  Keyframe Language Interpreter

  Code Sample:

  --------
  dec a = 5 == 5
  print(a)
  dec t = "hello world"
  for i (1,5){
    print(t)
  }

  function test(){
    print("try to see if it works")
  }

  test()
  --------


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
      std::string keywords[] = {"dec", "print", "if", "true", "false", "for", "function"};
      
      std::string capture = ""; // A substring of each token capture at a given pos
      size_t pos = 0; // A sliding pointer along the string to capture individual chars
      while (pos < input.size()){ // Iterate through the whole string
        char curr = input[pos]; // Store the current char

        if (curr == '\n'){
          // Breaking into a new line
          tokens.push_back(Token("newline", ""));
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
          }

          if (isSymbol){
            std::string symbol = std::string(1, curr);
            tokens.push_back(Token("symbol", symbol));
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
    std::vector<std::vector<std::string>> memory; // Variables memory
    std::vector<std::vector<Token>> memory_functions; // Unique memory for function tokens

    /*

      The memory layout is as follows:

      [ [ variable_type, variable_name, variable_value ], ... ]

      The memory functions layout is as follows:

      [ [ function_name > Token("", function_name) , function_tokens ], ... ]

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

    void output_log(std::string message, size_t line){
      std::cout << message << " (line " << line << ")" << std::endl; 
    }

    void execute(){      
      // Iterate through the tokens
      size_t l = 1; // Current line index
      size_t i = 0; // Current token index
      while (i < tokens.size()){
        // First run through, evaluating any boolean expressions or mathematical expressions
        Token curr = tokens[i];
        if (curr.type == "newline"){
          l++;
          continue;
        }
        
        if (curr.type == "symbol" && curr.value == "=" && tokens[i + 1].type == "symbol" && tokens[i + 1].value == "="){
          Token left = tokens[i - 1];
          Token right = tokens[i + 2];
          if (left.type == right.type){
            tokens.erase(tokens.begin() + i - 1);
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
            output_log("Attempt to compare different types", l);
          }
        }

        i++;
      }

      l = 1; // Current line index
      i = 0;
      while (i < tokens.size()){
        Token curr = tokens[i];
        if (curr.type == "newline"){
          l++;
          continue;
        }

        if (curr.type == "keyword"){
          if (curr.value == "dec"){
            // Variable declaration
            Token name = tokens[i + 1];
            if (name.type == "unknown"){
              Token symbol = tokens[i + 2];
              if (symbol.type == "symbol" &&  symbol.value == "="){
                Token value = tokens[i + 3];;
                if (value.type == "string" || value.type == "number" || value.type == "boolean"){
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
                  output_log(value.value, l);
                }
              }

              if (value.type == "unknown"){
                // Attempt to print out a variable
                std::vector<std::string> variable = findVariable(value.value);
                if (variable.size() == 0){
                  output_log("Variable not found", l);
                } else {
                  output_log(variable[2], l);
                }
              }
            }
          }

          if (curr.value == "for"){
            // For loop declaration
            const Token loop_variable = tokens[i + 1];
            if (loop_variable.type == "unknown"){
              const Token symbol_one = tokens[i + 2];
              if (symbol_one.type == "symbol" && symbol_one.value == "("){
                const Token start = tokens[i + 3];
                if (start.type == "number"){
                  const Token symbol_two = tokens[i + 4];
                  if (symbol_two.type == "symbol" && symbol_two.value == ","){
                    const Token end = tokens[i + 5];
                    if (end.type == "number"){
                      // We thus far have a for loop with: for loop_variable (start,end)
                      const Token symbol_three = tokens[i + 6];
                      if (symbol_three.type == "symbol" && symbol_three.value == ")")
                      {
                        const Token symbol_four = tokens[i + 7];
                        if (symbol_four.type == "symbol" && symbol_four.value == "{"){
                          // We need to capture all the code within the for loop's boundaries
                          std::vector<Token> nested_tokens;
                          size_t j = i + 8;
                          size_t brackets = 1; // Counter for how many brackets are left within the scope of this for loop. Once this reaches zero we have gone off the for loop.
                          while (j < tokens.size() && brackets > 0){
                            Token j_curr = tokens[j];
                            if (j_curr.type == "symbol" && j_curr.value == "{"){
                              brackets++;
                            } else if (j_curr.type == "symbol" && j_curr.value == "}") {
                              brackets--;
                            }

                            if (brackets == 0){
                              break;
                            }

                            nested_tokens.push_back(tokens[j]);
                            j++;
                          }

                          // We have captured all the code within the for loop's boundaries
                          for (int i = std::stof(start.value); i <= std::stof(end.value); i++){
                            execute(nested_tokens);
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }

          if (curr.value == "function"){
            // Function declaration
            const Token name = tokens[i + 1];
            if (name.type == "unknown"){

              const Token symbol_one = tokens[i + 2];
              if (symbol_one.type == "symbol" && symbol_one.value == "("){

                // Without any passed in arguments
                const Token symbol_two = tokens[i + 3];
                if (symbol_two.type == "symbol" && symbol_two.value == ")"){
                  // We thus far have a function with a syntax: function name()
                  const Token symbol_three = tokens[i + 4];
                  if (symbol_three.type == "symbol" && symbol_three.value == "{"){

                    std::vector<Token> nested_tokens;

                    nested_tokens.push_back(Token("", name.value)); // Artificially push in the function name for the first token
                    
                    size_t j = i + 5;
                    size_t brackets = 1; // Counter for how many brackets are left within the scope of this for loop. Once this reaches zero we have gone off.
                    while (j < tokens.size() && brackets > 0){
                      Token j_curr = tokens[j];
                      if (j_curr.type == "symbol" && j_curr.value == "{"){
                        brackets++;
                      } else if (j_curr.type == "symbol" && j_curr.value == "}") {
                        brackets--;
                      }

                      if (brackets == 0){
                        break;
                      }

                      nested_tokens.push_back(tokens[j]);
                      j++;
                    }

                    // Insert the function into the functions memory
                    memory_functions.push_back(nested_tokens);
                  }
                }
              }
            }
          }
        }

        if (curr.type == "unknown"){
          const Token name = curr;
          const Token symbol_one = tokens[i + 1];
          if (symbol_one.type == "symbol" && symbol_one.value == "("){
            const Token symbol_two = tokens[i + 2];
            if (symbol_two.type == "symbol" && symbol_two.value == ")"){
              // We now will execute that function
              for (unsigned int i = 0; i < memory_functions.size(); i++){
                if (memory_functions[i][0].value == name.value){
                  std::vector<Token> actual_tokens(memory_functions[i].begin() + 1, memory_functions[i].end());
                  execute(actual_tokens);
                  break;
                }
              }
            }
          }
        }
        
        i++;
      }
    }

    void execute(std::vector<Token> localTokens){
      std::vector<Token> oldTokens = tokens;
      tokens = localTokens;
      execute();
      tokens = oldTokens;
    }

    void printMemory(){
      std::cout << std::endl << std::endl;
      std::cout << "Full Memory Log: " << std::endl;
      for (unsigned int i = 0; i < memory.size(); i++){
        std::cout << "[" << memory[i][0] << ", " << memory[i][1] << " = " << memory[i][2] << "]" << std::endl;
      }

      for (unsigned int i = 0; i < memory_functions.size(); i++){
        std::cout << "[" << memory_functions[i][0].value << "]" << std::endl;
      }
    }
};

int main() {
  std::cout << std::endl << std::endl;
  std::cout << "Program Execution Output:" << std::endl;
  
  std::string test = "function name(){ print(\"yes\") } print(\"after\") name()";
  Lexer lexer = Lexer(test);
  std::vector<Token> tokens = lexer.tokenize();

  for (unsigned int i = 0; i < tokens.size(); i++){
    tokens[i].print();
  }

  Interpreter intr = Interpreter(tokens);
  intr.execute();
  intr.printMemory();
}
