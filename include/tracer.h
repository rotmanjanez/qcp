#ifndef TRACER_H
#define TRACER_H
// ---------------------------------------------------------------------------
#include <iostream>
#include <sstream>
#include <string>

class ENTER : public std::string {
   public:
   ENTER(const std::string& s = "") : std::string(s) {}
};

class EXIT : public std::string {
   public:
   EXIT(const std::string& s = "") : std::string(s) {}
};

class Tracer {
   private:
   std::ostream* os;
   std::vector<int> branches; // Keeps track of branching at each level
   bool newLine;

   // Function to print the current indentation and branching
   void printIndent() {
      if (newLine) {
         for (size_t i = 0; i < branches.size(); ++i) {
            if (i + 1 == branches.size()) {
               *os << (branches[i] ? "└── " : "├── ");
            } else {
               *os << (branches[i] ? "    " : "|   ");
            }
         }
         newLine = false;
      }
   }

   public:
   Tracer(std::ostream& out) : os(&out), newLine(true) {}

   template <typename T>
   Tracer& operator<<(const T& value) {
      printIndent();
      *os << value;
      return *this;
   }

   Tracer& operator<<(std::ostream& (*pf)(std::ostream&) ) {
      pf(*os);
      newLine = true;
      return *this;
   }

   Tracer& operator<<(const ENTER& enter) {
      newLine = true;
      printIndent();
      *os << "enter " << static_cast<std::string>(enter) << std::endl;
      newLine = true;
      branches.push_back(0); // Add a new level with a branching indicator
      return *this;
   }

   Tracer& operator<<(const EXIT&) {
      newLine = true;
      printIndent();
      *os << "exit" << std::endl;
      if (!branches.empty()) {
         branches.pop_back(); // Remove the last level
         if (!branches.empty()) {
            branches.back() = 1; // Mark the last level as ended
         }
      }
      newLine = true;
      return *this;
   }
};
// ---------------------------------------------------------------------------
#endif // TRACER_H