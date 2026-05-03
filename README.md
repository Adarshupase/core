# CORE (Semantic Editor) 
- currently the name is core will come up with a better name to avoid conflict with core files

The idea behind core is to refactor your code and keep track of your refactors and roll_back if anything goes wrong.
Sort of like git.

Each change you make modify your syntax tree so you don't have to worry about all variables in your program changing when you
make a change to local variable only the variables in that scope . (I know we already have clangd) but this is not a text editor nor a language server this is simply a program that does what it tells it does.

currently only being made for c. but will be possible to extend to other languages given tree-sitter parser has been available for majority of languages.

# Working.
You issue a command that changes the source code / syntax tree according to the command. No regex magic. Simply editing the syntax tree so the program is consistent for example if you want to change field in c struct and you want this to change all the references, all the usage of that field in the program and don't accidentally change a local variable, function argument, or another struct's field with the same name core issues a command that simply does that without you having to manualy do it for every use. You can rollback to the until first refactor you made without much overhead because every edit is a reversible function. You can also redo the edit.


