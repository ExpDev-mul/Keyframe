/*

  Keyframe Language Interpreter

  Code Sample:

  --------
  dec text = "Hello world!"

  function test(){
    for i (1, 4){
      if (true){
        print(text)
        if (false){
          print("this wont print")
        }
      }
    }
  }

  test()
  --------


  All rights reserved to Or Pinto ©

*/

// Library inclusions
#include <iostream>
#include <string>
#include <vector>
#include <tuple>


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

std::string removeFirstAndLast(std::string str){
  // Removes the first and last characters of a string, mainly useful for lexical analysis of string experssions.
  return str.substr(1, str.length() - 2);
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
      char symbols[] = {':', ',', '(', ')', '{', '}', '[', ']', '=', '+', '!'};
      std::string keywords[] = {"dec", "print", "if", "for", "function", "return", "and", "or"};
      
      std::string capture = ""; // A substring of each token capture at a given pos
      size_t pos = 0; // A sliding pointer along the string to capture individual chars
      size_t quotes = 0; // 0 quotes means we are not capturing a string, 1 means we are capturing a string
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
        
        if ((std::isspace(curr) && quotes == 0) || isSymbol){
          // When spaces (not within strings) or any symbols are captured we have to push back the current token
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
              if (capture == "true" || capture == "false"){
                tokens.push_back(Token("boolean", capture));
              } else if (capture[0] == '"' && capture[capture.size() - 1] == '"'){
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

        if (curr == '"'){
          quotes = quotes == 0 ? 1 : 0;
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

std::vector<std::string> constructMemoryVariable(std::string& type, 
                                                 std::string& name, 
                                                 std::string value){
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
    std::vector<std::tuple<std::string, std::vector<Token>>> memory_functions; // Unique memory for function tokens

    /*

      The memory layout is as follows:

      [ [ variable_type, variable_name, variable_value ], ... ]

      The memory functions layout is as follows:

      [ [ (function_name  , function_tokens) ], ... ]

    */

    Interpreter(std::vector<Token> tokens) : tokens(tokens) {}

    std::vector<std::string> find_variable(std::string name){
      for (unsigned int i = 0; i < memory.size(); i++){
        if (memory[i][1] == name){
          return memory[i];
        }
      }

      return std::vector<std::string>();
    }

    Token run_function(std::string name){
      for (unsigned int i = 0; i < memory_functions.size(); i++){
        std::tuple<std::string, std::vector<Token>> function_metadata = memory_functions[i];
        std::string local_name = std::get<0>(function_metadata);
        std::vector<Token> local_tokens = std::get<1>(function_metadata);
        if (local_name == name){
          Token ret = execute(local_tokens); // Store what the function call has returned
          return ret;
        }
      }
      
      return Token("search", "LOST"); // LOST indicates that the search did not find anything
    }

    Token evaluate_experssion(std::vector<Token> local_tokens){
      /* 
        Nested tokens within some experession will be evaluated using this method 
        The indexing checks are done to prevent Segmentation Faults
      */
            
      // The expression is potentially a boolean expression
      if (local_tokens.size() > 3 && local_tokens[1].type == "symbol" && local_tokens[1].value == "=" && local_tokens[2].type == "symbol" && local_tokens[2].value == "="){
        Token left = local_tokens[0];
        Token right = local_tokens[3];
        if (left.type == right.type){
          if (left.value == right.value){
            return Token("boolean", "true");
          } else {
            return Token("boolean", "false");
          }
        } else {
          // Comparison between different types.
          return Token("run_error", "Attempt to compare different types");
        }
      }

      

      Token first_token = local_tokens[0];
      if (first_token.type == "string" || first_token.type == "number" || first_token.type == "boolean"){
        const Token after_token = local_tokens[1];
        
        if (after_token.type == "symbol" && after_token.value == "+"){
          // We are evaluating an expression with operators
          if (first_token.type == "string"){
            // String concatenation
            std::string comp_value = removeFirstAndLast(first_token.value);

            size_t i = 2; // Iterate through concatenation orders (ex. "a" + "b" + "c" + ...)
            while (i < local_tokens.size()){
              const Token prev_token = local_tokens[i - 1];
              if (prev_token.type == "symbol" && prev_token.value == "+"){
                const std::string concat_string = removeFirstAndLast(local_tokens[i].value);
                comp_value += (concat_string);
                i += 2;
              }
            }
            
            return Token("string", "\"" + comp_value + "\""); // Artifically sorround with quotation marks
          } else {
            return Token("run_error", "Attempt to concatenate string with differing type.");
          }
        }

        if (after_token.type == "keyword" && (after_token.value == "and" || after_token.value == "or")){
          // Boolean logical operators
          bool evaluated_value = first_token.value == "false" ? false : true;

          size_t i = 2; // Iterate through concatenation orders (ex. "a" + "b" + "c" + ...)
          while (i < local_tokens.size()){
            const Token prev_token = local_tokens[i - 1];
            if (prev_token.type == "keyword" && (prev_token.value == "and" || prev_token.value == "or")){
              const std::string other_value = local_tokens[i].value;
              const bool converted = other_value == "true" ? true : false;
                evaluated_value = prev_token.value == "and" ? converted && evaluated_value : converted || evaluated_value;
              i += 2;
            }
          }

          return Token("boolean", evaluated_value ? "true" : "false");
        }
        
        if (first_token.type == "string"){
          // With strings uniquely we neglect the quotation marks and subtract them off the original string
          return Token("string", removeFirstAndLast(first_token.value));
        }
        
        return local_tokens[0]; // If the expression is already of a known datatype we can instantly return it
      }

      if (local_tokens[0].type == "unknown"){
        if (local_tokens.size() > 1 &&  local_tokens[1].type == "symbol" && local_tokens[1].value == "("){
          // Potential function
          if (local_tokens.size() > 2 && local_tokens[2].type == "symbol" && local_tokens[2].value == ")"){
            // A function call was made, we will return what the function call returns
            return run_function(local_tokens[0].value); // Run the function with the function's name
          }
        }

        // If this is not a function then it may be a variable
        std::vector<std::string> variable = find_variable(local_tokens[0].value);
        if (variable.size() > 0){
          return Token(variable[0], variable[2]);
        }
      }

      return Token("", "");
    }

    void output_log(std::string message, size_t line){
      std::cout << message << " (line " << line << ")" << std::endl; 
    }

    std::tuple<std::vector<Token>, int> get_nested_tokens(size_t start, std::string open, std::string close){
      /*

        Returns all localized nested tokens between characters (start, close), and an index-pointer to the end of the nested tokens.

      */
      
      std::vector<Token> nested_tokens;
      size_t j = start;
      size_t brackets = 1; // Counter for how many brackets are left within the scope of this for loop. Once this reaches zero we have gone off the for loop.
      while (j < tokens.size() && brackets > 0){
        Token j_curr = tokens[j];
        if (j_curr.type == "symbol" && j_curr.value == open){
          brackets++;
        } else if (j_curr.type == "symbol" && j_curr.value == close) {
          brackets--;
        }

        if (brackets == 0){
          break;
        }

        nested_tokens.push_back(tokens[j]);
        j++;
      }

      return std::make_tuple(nested_tokens, j);
    }

    Token execute(){
      // Iterate through the tokens
      size_t l = 1; // Current line index
      size_t i = 0; // Current token index
      while (i < tokens.size()){
        // First run through, evaluating any boolean expressions or mathematical expressions
        Token curr = tokens[i];

        if (curr.type == "ignore"){ // Skip scan on ignored tokens
          continue;
        }
        
        if (curr.type == "newline"){
          l++;
          continue;
        }

        /*
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
        */

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

                
                Token value = tokens[i + 3];
                if (value.type == "string" || value.type == "number" || value.type == "boolean"){
                  // We want to remove the quotation marks from strings
                  std::vector<std::string> memoryVariable = constructMemoryVariable(
                    value.type,
                    name.value, 
                    value.type == "string" ? value.value.substr(1, value.value.length() - 2) : value.value
                  );
                  
                  memory.push_back(memoryVariable);
                  i = i + 3;
                }

                // It may be containing an expression within, such as var x = (...), therefore we should check for brackets.
                if (value.type == "symbol" && value.value == "("){
                  std::vector<Token> arguments_tokens;
                  int j;
                  std::tie(arguments_tokens, j) = get_nested_tokens(i + 4, "(", ")");

                  i = j;
                  Token brackets_value = evaluate_experssion(arguments_tokens);
                  std::vector<std::string> memoryVariable = constructMemoryVariable(
                    brackets_value.type,
                    name.value, 
                    brackets_value.type == "string" ? brackets_value.value.substr(1, brackets_value.value.length() - 2) : brackets_value.value
                  );

                  memory.push_back(memoryVariable);
                }
              }
            }
          }
          
          if (curr.value == "print"){
            // Printing
            const Token left_bracket = tokens[i + 1];
            if (left_bracket.type == "symbol" && left_bracket.value == "("){
              // We have captured all the code within the for print's boundaries
              std::vector<Token> arguments_tokens;
              int j;
              std::tie(arguments_tokens, j) = get_nested_tokens(i + 2, "(", ")");
              
              i = j;
              Token value = evaluate_experssion(arguments_tokens);
              if (value.type == "string" || value.type == "number" || value.type == "boolean"){
                if (value.type == "string"){
                  output_log(value.value, l);
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
                          std::vector<Token> exceution_tokens;
                          
                          int j;
                          std::tie(exceution_tokens, j) = get_nested_tokens(i + 8, "{", "}");

                          // We have captured all the code within the for loop's boundaries
                          for (int k = std::stof(start.value); k <= std::stof(end.value); k++){
                            execute(exceution_tokens);
                          }
                          
                          i = j;
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
                    int j;
                    std::tie(nested_tokens, j) = get_nested_tokens(i + 5, "{", "}");

                    // Insert the function into the functions memory
                    memory_functions.push_back(std::make_tuple(name.value, nested_tokens));

                    i = j;
                  }
                }
              }
            }
          }

          if (curr.value == "if"){
            // If statement
            const Token symbol_one = tokens[i + 1];
            if (symbol_one.type == "symbol" && symbol_one.value == "("){
              std::vector<Token> arguments_tokens;
              int j;
              std::tie(arguments_tokens, j) = get_nested_tokens(i + 2, "(", ")");

              i = j;
              Token value = evaluate_experssion(arguments_tokens);
              if ((value.type == "boolean") && (value.value == "true" || value.value == "false")){
                const Token symbol_two = tokens[i];
                if (symbol_two.type == "symbol" && symbol_two.value == ")"){
                  const Token symbol_three = tokens[i + 1];
                  if (symbol_three.type == "symbol" && symbol_three.value == "{"){
                    // We need to capture all the code within the condition's boundaries
                    std::vector<Token> exceution_tokens;
                    int j;
                    std::tie(exceution_tokens, j) = get_nested_tokens(i + 2, "{", "}");

                    // We have captured all the code within the for loop's boundaries
                    i = j;
                    
                    if (value.value == "true"){
                      execute(exceution_tokens);
                    }
                  }
                }
              }
            }
          }

          if (curr.value == "return"){
            // Return statement
            const Token left_bracket = tokens[i + 1];
            if (left_bracket.type == "symbol" && left_bracket.value == "("){
              std::vector<Token> arguments_tokens;
              int j;
              std::tie(arguments_tokens, j) = get_nested_tokens(i + 2, "(", ")");

              i = j;
              Token value = evaluate_experssion(arguments_tokens);
              return value;
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
              run_function(name.value);

              i = i + 2;
            }
          }
        }
        
        i++;
      }

      return Token("run_type", "SUCCESS"); // SUCCESS indicates succesfull execution
    }

    Token execute(std::vector<Token> local_tokens){
      std::vector<Token> old_tokens = tokens;
      tokens = local_tokens;
      Token ret = execute();
      tokens = old_tokens;
      return ret;
    }

    void printMemory(){
      std::cout << std::endl << std::endl;
      std::cout << "Full Memory Log: " << std::endl;
      for (unsigned int i = 0; i < memory.size(); i++){
        std::cout << "[" << memory[i][0] << ", " << memory[i][1] << " = " << memory[i][2] << "]" << std::endl;
      }

      for (unsigned int i = 0; i < memory_functions.size(); i++){
        std::cout << "[" << std::get<0>(memory_functions[i]) << "]" << std::endl;
      }
    }
};

int main() {
  std::cout << std::endl << std::endl;
  std::cout << "Program Execution Output:" << std::endl;

  std::string test = "dec a = (true or false)";
  Lexer lexer = Lexer(test);
  std::vector<Token> tokens = lexer.tokenize();
  
  for (unsigned int i = 0; i < tokens.size(); i++){
    tokens[i].print();
  }
  
  Interpreter intr = Interpreter(tokens);
  intr.execute();
  intr.printMemory();
}
