#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <algorithm>
#include <map>
#include <cctype> // For std::isalpha and std::isspace

using namespace std;

//vector<string> inputVarOrder;
map<string, int> inputVar;
map<string, int> internalVar;
map<string, int> vars;
vector<string> expressions;

//trims spaces out to avoid bugs with checking in different maps;
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    size_t last = str.find_last_not_of(" \t\n\r");
    if (first == string::npos || last == string::npos)
        return "";
    return str.substr(first, (last - first + 1));
}

//parses the graph file by finding "input_var" for the first time and then adding the variable to the map "inputVar"
//parses the graph file by finding "internal_var" for the first time and then adding the variable to the map "internalVar"
//parses the graph file by finding "->" and parsing string to make expression and then adding the expression to "expressions" vector
void parseGraphFile(const string& filename) {
    ifstream file(filename);
    string line;

    if (!file.is_open()) {
        cerr << "Failed to open file: " << filename << endl;
        return;
    }

    while (getline(file, line)) {
        // Find and remove the semicolon from the current line, if present
        size_t semicolonPos = line.find(';');
        if (semicolonPos != string::npos) {
            line.erase(semicolonPos, 1);
        }

        // Check if the line contains the input_var declaration
        if (line.find("input_var") != string::npos) {
            // Extract the variables list, removing the 'input_var ' prefix
            string variableList = line.substr(line.find(' ') + 1);
            stringstream ss(variableList);
            string variable;

            // Split variables separated by commas and add them to the map
            while (getline(ss, variable, ',')) {
                string trimmedVar = trim(variable);
                inputVar[trimmedVar] = 0;
                vars[trimmedVar] = 0;
                //inputVarOrder.push_back(trimmedVar);// Initialize the variable in the map with a value of 0
            }
        }
        if(line.find("internal_var") != string::npos) {
          // Extract the variables list, removing the 'input_var ' prefix
          string variableList = line.substr(line.find(' ') + 1);
          stringstream ss(variableList);
          string variable;

          // Split variables separated by commas and add them to the map
          while (getline(ss, variable, ',')) {
                string trimmedVar = trim(variable);
                internalVar[trimmedVar] = 0; // Initialize the variable in the map with a value of 0
                vars[trimmedVar] = 0;
          }
        }
      if(line.find("->") != string::npos) {
          // Split the line into left and right parts based on '->'
          size_t arrowPos = line.find("->");
          string leftPart = line.substr(0, arrowPos);
          string rightPart = line.substr(arrowPos + 2);

          // Trim spaces from the right and left part
        leftPart.erase(remove_if(leftPart.begin(), leftPart.end(), ::isspace), leftPart.end());
        rightPart.erase(remove_if(rightPart.begin(), rightPart.end(), ::isspace), rightPart.end());
        char operation = 0; // Use as needed, for cases like '- b -> p0'
        if (!leftPart.empty() && (leftPart[0] == '+' || leftPart[0] == '-' || leftPart[0] == '*' || leftPart[0] == '/')) {
          operation = leftPart[0];
          leftPart = leftPart.substr(1); // Remove operation character
        }

        if(operation) {
          expressions.push_back(rightPart + " = " + rightPart  + " " + operation + " " + leftPart);
        }
        else {
          expressions.push_back(rightPart + " = " + leftPart);
        }
      }
    }

    file.close();
}

void parseInputFile(const string &filename) {
    ifstream file(filename);
    string line;

    if (!file.is_open()) {
        cerr << "Failed to open file: " << filename << endl;
        return;
    }

    while (getline(file, line)) {
        stringstream ss(line);
        string substr;
        auto i = inputVar.begin();// Index to track the current variable name in inputVarOrder
        while (getline(ss, substr, ',')) {
          i->second = stoi(substr);
          vars[i->first] = stoi(substr);
          i++; // Move to the next variable name for the next value
        }
    }

    file.close();
}

void evaluateExpression(const string& expr, map<string, int>& vars) {
    istringstream iss(expr);
    string var, equals, lhs, op, rhs;
    int result;

    iss >> var >> equals; // Parse the variable to assign to and the equals sign

    if (iss >> lhs) { // Attempt to read the next part as lhs
        if (iss >> op >> rhs) { // If there's an operator, then it's an operation assignment
            // Ensure lhs and rhs are valid variables
            if (vars.find(lhs) == vars.end() || vars.find(rhs) == vars.end()) {
                cerr << "Error: One of the variables in the expression '" << expr << "' is not defined." << endl;
                return;
            }

            int lhsValue = vars[lhs];
            int rhsValue = vars[rhs];

            // Perform the operation
            if (op == "+") {
                result = lhsValue + rhsValue;
                
            } else if (op == "-") {
                result = lhsValue - rhsValue;
            } else if (op == "*") {
                result = lhsValue * rhsValue;
            } else if (op == "/") {
                if (rhsValue == 0) {
                    cerr << "Error: Division by zero in the expression '" << expr << "'." << endl;
                    return;
                }
                result = lhsValue / rhsValue;
            } else {
                cerr << "Error: Unsupported operation '" << op << "' in the expression '" << expr << "'." << endl;
                return;
            }
        } else { // If there's no operator, it's a direct assignment
            iss.clear();
            iss.str(lhs); // Use lhs as it was the next token read, assuming it's the variable name for direct assignment
            if (vars.find(lhs) == vars.end()) {
                cerr << "Error: The variable '" << lhs << "' is not defined." << endl;
                return;
            }
            result = vars[lhs];
          
        }
      
        // Assign the result to the var
        vars[var] = result;
    }
}

int main() {

  string graphFile = "s2.txt";
  parseGraphFile(graphFile);

  string inputFile = "input2.txt";
  parseInputFile(inputFile);
  
  int pipe_fd[2]; // Pipe file descriptors: 0 for read, 1 for write
  pid_t pid;

  string exprString;
  for (const auto& expr : expressions) {
      exprString += expr + "\n";
  }

  if (pipe(pipe_fd) == -1) {
      cerr << "Pipe failed" << endl;
      return 1;
  }

  pid = fork();
  if (pid < 0) {
      cerr << "Fork failed" << endl;
      return 1;
  }

  if (pid > 0) { // Parent process
      close(pipe_fd[0]); // Close unused read end
      ssize_t bytes_written = write(pipe_fd[1], exprString.c_str(), exprString.length() + 1);
      if (bytes_written == -1) {
          cerr << "Write to pipe failed" << endl;
          return 1;
      }
      close(pipe_fd[1]); // Close write end
      wait(nullptr); // Wait for the child process to finish
  } else { // Child process
      close(pipe_fd[1]); // Close unused write end
      char readBuffer[1024];
      ssize_t bytes_read = read(pipe_fd[0], readBuffer, sizeof(readBuffer)); // Read serialized expressions
      if (bytes_read == -1) {
          cerr << "Read from pipe failed" << endl;
          return 1;
      }
      readBuffer[bytes_read] = '\0'; // Null-terminate the string
      close(pipe_fd[0]); // Close read end

      string inputString(readBuffer);
      istringstream input(inputString);
      string line;

      while (getline(input, line)) {
          evaluateExpression(line, vars);
      }

      for (const auto& var : vars) {
          if(internalVar.find(var.first) != internalVar.end()) {
            cout << var.first << " = " << var.second << endl;
          }
      }
  }

  return 0;
}
