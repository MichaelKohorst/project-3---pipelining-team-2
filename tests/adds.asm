 lw 1 0 one       # load reg 1 with 1 
  lw 2 0 two       # load reg 2 with 2 
  lw 3 0 three       # load reg 3 with 3
  add 1 1 1        # begin incrementing reg 2
  add 2 1 2 
  add 3 1 3 
  halt           # end of program
one   .fill 1
two   .fill 2
three .fill 3