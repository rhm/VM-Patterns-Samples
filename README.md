# VM Patterns Samples

This repository contains the code that I wrote to support a talk I gave a private coders conference titled "Virtual Machines as a pattern for implementing designable gameplay systems". Yes, that is a long title for a talk. 

Anyway, as promised at the talk, this repository contains a Visual Studio 2013 solution with three projects:

* Formulas contains the expression parser and the virtual machine implementation for evaluating the expressions as well as supporting classes like the VariableLayout and VariablePack classes. It also has a decent number of unit tests.
* BehaviourTree contains the Object-oriented and virtual machine implementations of a simple behaviour tree system. It should be obious but the Object-orented implementation is in the files ending OO and the virtual machine implementation in files ending VM. There are also some tests in the files ending OOTests and VMTests.
* Common contains the minimalist unit test framework I wrote as well as the Name class

Note that the expression parser was built using flex/bison. The generated files are included in the reprository, but if you change any of them you will need flex and bison in your system PATH in order to rebuild. The easiest place to get these from is to install the Windows port of git, which includes a lot of popular Unix commands. You could also use cygwin.


## Slides

PDF (10MB)
https://drive.google.com/file/d/0BzGtHnqat7PFcnkxOFNjNXdVNEU/view?usp=sharing

PPT (2MB)
https://drive.google.com/file/d/0BzGtHnqat7PFRVh6Rm4zRmlqNEU/view?usp=sharing

