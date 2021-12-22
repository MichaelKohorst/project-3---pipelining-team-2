# cisc340-project3
Instructions: To  run our program, make sure you are in the project directory. 
Type make clean, enter, type make, enter. This will set up the simulator file to be run. 
You can then enter for example ./sim -i tests/class.mc That will run the program will the supplied instructions.
You can interchange class.mc with any other test we have provided inside the test folder.

sim.c: This is the c code we have written to run machine code and print the output to the command line.
It will show the state of each register and memory location after operations are ran from the machine code using 
input from our tests. But it runs the code pipelines so that multiple instruction will be worked on at once.

Makefile: Allow us to run the sim.c as it runs given commands of the file.

tests/ class.asm: This is the supplied assembly language code that can be built into runnable machine code.

class.mc: This is the supplied machine code file that is used to test correctness our simulator output.

adds.asm: We created this assembley code as means to test adds and lw words that require forwarding from 3 
instructions before the current, from 2 instructions before the current and 1 instruction before the currrent 
instruction.

adds.mc: This is the machine code for our adds test.

mixed.asm:This test all of the instructions but have no hazards involved

mixedmc: This is the machine code for our mixed test.

lwStall.asm: This test requires both forwarding from a add to nand, lw to add and lw to nand. Also it requires
Load word stalls between lw and add.

lwStall.mc: This is the machine code for our lwStall test.

beqHazard.asm: This test branches that fail to predict the right outcome. This also test a lw stall right before a 
a branch. This includes fowarding between adds, lw forwards to beq and nands to adds. It tests all operations have to
recieve a foward. It has a branch that will fetch halt but the halt will be erased. 

beqHazard.mc: This is the machine code for our lwStall test.
