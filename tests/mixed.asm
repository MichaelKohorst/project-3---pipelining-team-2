lw 1 0 one      # load reg 1 with 1 
    lw 3 0 alot    # load reg 3 with 50
  add 2 1 2         # begin incrementing reg 2
  add  7 7 7        # add register 7 with register 7
  nand 7 5 7        # nand register reg 5 and reg 7 into reg 7
  nand 7 5 7
  beq 0 2 1 		#branch that will always take branch not taken
  sw 7 0 one          # store one into reg 7 with base of 0
  sw 7 0 alot
  lw 7 0 one           # load reg 7 with 1 
  lw 7 0 alot         # load reg 7 with 50
  noop
done halt           # end of program
one .fill 1
alot .fill 50