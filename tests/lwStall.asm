lw 1 0 one       # load reg 1 with 1 
  lw 2 0 two       # load reg 2 with 2 
  lw 3 0 three       # load reg 3 with 3
  add 1 2 3        # add r2 and r3 into r1
  nand 5 1 2        # nand r1 and r2 into 5
  halt              # end of program
one .fill 1
two .fill 2
three .fill 3