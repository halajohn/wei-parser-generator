# Feature

The "Wei Parser Generator" (wpg) is an LL(k) parser generator, it has the following features:

* Support user-defined lookahead depth
* Support pure BNF syntax and Extended BNF (a.k.a EBNF) syntax.
* If a grammar only contains pure BNF syntax, wpg can automatically perform:
    * left recursion removal. (Using Paull's algorithm)
    * left factoring.
* wpg can convert a grammar to a graph using the "graphviz" package.

Graphviz: [http://www.graphviz.org/](http://www.graphviz.org/)

For example, wpg will generate the following graph for the following grammar after left recursion removal.

![](http://lh6.google.com/wei.hu.tw/RzsrrG461PI/AAAAAAAAAEw/pqmBrDrTy7c/grammar_tree_grammar.jpg)

![](http://lh6.google.com/wei.hu.tw/RzsrrG461OI/AAAAAAAAAEo/xXjdAzgyJsw/grammar_tree.jpg)

# Build

Wei Parser Generator supports Microsoft Visual C++ 2005 (express) compiler now. However, theoretically, any C++ standard compliant compiler can compile wpg without problems.

Wei Parser Generator (wpg) uses C++ (STL) and the [boost](http://www.boost.org/) library and [Wei Common Library](http://code.google.com/p/wei-common-library/), so you need to install a C++ standard compliant compiler and the boost library and Wei Common Library.

boost: [http://www.boost.org/](http://www.boost.org/)  
Wei Common Library: [http://code.google.com/p/wei-common-library/](http://code.google.com/p/wei-common-library/)

# Grammar File Syntax Specification

The content of a grammar file can be devided into 3 parts:

* Attribute block: Pass parameters to wpg.
* Terminal name block: Specify all the terminal names this grammar will use.
* Rule block: Specify the grammar rules.

Here is an example grammar for wpg:

![](http://lh6.google.com/wei.hu.tw/RzsrrG461NI/AAAAAAAAAEg/trO6w-yKRKY/grammar_file.jpg)

---
Note: The line number at the left side is for convenience, and it doesn't belong to the grammar.
---

Explain:

At the start of a grammar file, you have to use a left brace and a right brace to create the attribute block. You can pass parameters to wpg through this block. Each parameter occupies one line, and should be ended with a semicolon.

* The "k = 2;" statement at the line 1 is used for specifying the lookahead depth. That is to say, wpg with "k = 2;" will generate an LL(2) parser.
* The "using_pure_BNF = yes;" statement at the line 2 is used for specifying this grammar only contains BNF syntax. (i.e. contains no EBNF syntax)
  Note: EBNF syntax contains the following 4 forms: * (a)* appear zero or more times * (a)+ appear one or more times * (a)? appear zero or one time * (a|b) appear 'a' or 'b'
* The "use_paull_algo = yes;" statement at the line 3 is used for specifying whether wpg performs the left recursion removal. Note that this option is only useful if you specify "using_pure_BNF = yes;".
* The "enable_left_factor = yes;" statement at the line 4 is used for specifying whether wpg performs the left factoring in the code generation stage to avoid the ambiguities. Note that this option is only useful if you specify "using_pure_BNF = yes;".

After the attribute block, you need to specify all terminal names this grammar may use. Note that there should be at least one empty line (at the line 6 of this example) between the left brace (which ends the attribute block) and the first terminal name. The terminal name block should be ended with a semicolon (at the line 13 of this example). The region between the line 7 and the line 13 of this example is its terminal name block.

After the terminal name block, you can specify each rule of this grammar. Note that there should be at least one empty line (at the line 14 of this example) between the semicolon (which ends the terminal name block) and the first rule, and each rule should be ended with a semicolon. If there are more than one alternatives in one rule, you should use a vertical bar (|) to distinguish them. You can use EBNF syntax in one rule, too.

The following are more examples for writing a rule:

"A" : "a" ("B" "B" ("d" "e")* | "c")* "a" ;

"B" : "e" | "f" ;

# Execute

The executable file of "Wei Parser Generator" is "wpg.exe". You can use the following command to parse a grammar file.

wpg.exe grammar_file

# Unit Test

I write some unit testing files for wpg, and put them all into the "unit_test_grammar_analyser" directory. There are 2 shell scripts to automatically do the test:

* regex_test.sh Test EBNF grammar
* test.sh Test BNF grammar

The execution result:

![](http://lh6.google.com/wei.hu.tw/RzsrrG461RI/AAAAAAAAAFA/YyHKzIipxb4/unit_testing.jpg)

# How to build the generated files

After parsing, Wei Parser Generator will generate some source (.cpp) and header (.hpp) files. wpg will only use pure C++ (include STL) in these generated files without any additional libraries, so you only need to use a C++ standard compliant compiler to compile them.

# Installation Wizard

Using [NSIS](http://nsis.sourceforge.net/) with the "create.nsi" file in the "installwizard" directory, you can make an install wizard program conveniently.

![](http://lh6.google.com/wei.hu.tw/RzsrrG461QI/AAAAAAAAAE4/bYSnOkBDdlQ/installwizard.jpg)
