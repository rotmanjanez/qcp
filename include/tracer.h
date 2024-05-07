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
   int indent = 0;
   bool newLine;

   // Function to print the current indentation and branching
   void printIndent(bool normalLog = true) {
      if (indent > 20) {
         // todo: remove
         throw std::runtime_error("Indentation limit reached");
      }
      if (newLine) {
         for (int i = 0; i < indent; ++i) {
            if (i + 1 == indent) {
               if (normalLog) {
                  *os << "|   ";
               } else {
                  *os << "├── ";
               }
            } else {
               *os << "|   ";
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
      printIndent(false);
      *os << "enter " << static_cast<std::string>(enter) << std::endl;
      newLine = true;
      indent++;
      return *this;
   }

   Tracer& operator<<(const EXIT&) {
      newLine = true;
      indent--;
      printIndent();
      *os << "*" << std::endl;
      newLine = true;
      return *this;
   }
};
// ---------------------------------------------------------------------------
#endif // TRACER_H