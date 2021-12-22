lw 1 0 one      # load reg 1 with 1 
lw 3 0 alot	# load reg 3 with 4
start add 2 1 2         # begin incrementing reg 2
  add  1 1 7		# add register 1 with register 1 into 7
  nand 7 1 3		#nand register reg 1 and reg 3 into reg 7
  nand 7 1 7
  sw 7 0 one      	#store one into reg 7 with base of 0
  sw 7 0 alot		#store alot into reg 7 with base of 0
  lw 6 0 one       	# load reg 6 with one 
  beq 6 6 1
  noop
  noop
  beq 2 3 done     	# goto end of program when reg2==4
  beq 0 0 start 	# go back to the beginning of the loop
done halt       	# end of program
one .fill 1
alot .fill 4
stAdd .fill start # will contain the address of start (2)