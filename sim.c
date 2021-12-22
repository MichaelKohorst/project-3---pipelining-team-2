#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define NUMMEMORY 65536 /* maximum number of data words in memory */
#define NUMREGS 8 /* number of machine registers */

#define ADD 0
#define NAND 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5
#define HALT 6
#define NOOP 7

#define NOOPINSTRUCTION 0x1c00000

typedef struct IFIDstruct{
	int instr;
	int pcplus1;
} IFIDType;

typedef struct IDEXstruct{
	int instr;
	int pcplus1;
	int readregA;
	int readregB;
	int offset;
} IDEXType;

typedef struct EXMEMstruct{
	int instr;
	int branchtarget;
	int aluresult;
	int readreg;
} EXMEMType;

typedef struct MEMWBstruct{
	int instr;
	int writedata;
} MEMWBType;

typedef struct WBENDstruct{
	int instr;
	int writedata;
} WBENDType;

typedef struct statestruct{
	int pc;
	int instrmem[NUMMEMORY];
	int datamem[NUMMEMORY];
	int reg[NUMREGS];
	int numMemory;
	IFIDType IFID;
	IDEXType IDEX;
	EXMEMType EXMEM;
	MEMWBType MEMWB;
	WBENDType WBEND;
	int cycles;       /* Number of cycles run so far */
	int fetched;     /* Total number of instructions fetched */
	int retired;      /* Total number of completed instructions */
	int branches;  /* Total number of branches executed */
	int mispreds;  /* Number of branch mispredictions*/
} statetype;

void  memStage(statetype* state, statetype* newstate, int instr, int branchtarget, int aluresult, int readreg);//Intializes all these for later use.
void  wbStage(statetype* state, statetype* newstate, int instr, int writedata);
void stall(statetype* state, statetype* newstate);
void  ifStage(statetype* state, statetype* newstate, int pc);
void  idStage(statetype* state, statetype* newstate, int instr, int pcplus1);
void  exStage(statetype* state, statetype* newstate, int instr, int pcplus1, int readregA, int readregB, int offset);
void printInstruction(int instr);
int checkLWstall(statetype* state, statetype* newstate, int dReg);
int forwardedRegB(statetype* state, statetype* newstate, int regB, int regBValue);
int forwardedRegA(statetype* state, statetype* newstate, int regA, int regAValue);
void beqClear(statetype* state, statetype* newstate);

int field0(int instruction){
	return( (instruction>>19) & 0x7);
}

int field1(int instruction){
	return( (instruction>>16) & 0x7);
}

int field2(int instruction){
	return(instruction & 0xFFFF);
}

int opcode(int instruction){
	return(instruction>>22);
}

int signExtend(int num){
	// convert a 16-bit number into a 32-bit integer
	if (num & (1<<15) ) {
		num -= (1<<16);
	}
	return num;
}


void printInstruction(int instr){
char opcodeString[10];
if (opcode(instr) == ADD) {
strcpy(opcodeString, "add");
} else if (opcode(instr) == NAND) {
strcpy(opcodeString, "nand");
} else if (opcode(instr) == LW) {
strcpy(opcodeString, "lw");
} else if (opcode(instr) == SW) {
strcpy(opcodeString, "sw");
} else if (opcode(instr) == BEQ) {
strcpy(opcodeString, "beq");
} else if (opcode(instr) == JALR) {
strcpy(opcodeString, "jalr");
} else if (opcode(instr) == HALT) {
strcpy(opcodeString, "halt");
} else if (opcode(instr) == NOOP) {
strcpy(opcodeString, "noop");
} else {
strcpy(opcodeString, "data");
}

if(opcode(instr) == ADD || opcode(instr) == NAND){
printf("%s %d %d %d\n", opcodeString, field2(instr), field0(instr), field1(instr));
}
else if(0 == strcmp(opcodeString, "data")){
printf("%s %d\n", opcodeString, signExtend(field2(instr)));
}
else{
printf("%s %d %d %d\n", opcodeString, field0(instr), field1(instr),
signExtend(field2(instr)));
}
}
void printstate(statetype* stateptr){
    int i;
    printf("\n@@@\nstate before cycle %d starts\n", stateptr->cycles);
    printf("\tpc %d\n", stateptr->pc);

    printf("\tdata memory:\n");
	for (i=0; i<stateptr->numMemory; i++) {
	    printf("\t\tdatamem[ %d ] %d\n", i, stateptr->datamem[i]);
	}
    printf("\tregisters:\n");
	for (i=0; i<NUMREGS; i++) {
	    printf("\t\treg[ %d ] %d\n", i, stateptr->reg[i]);
	}
    printf("\tIFID:\n");
	printf("\t\tinstruction ");
	printInstruction(stateptr->IFID.instr);
	printf("\t\tpcplus1 %d\n", stateptr->IFID.pcplus1);
    printf("\tIDEX:\n");
	printf("\t\tinstruction ");
	printInstruction(stateptr->IDEX.instr);
	printf("\t\tpcplus1 %d\n", stateptr->IDEX.pcplus1);
	printf("\t\treadregA %d\n", stateptr->IDEX.readregA);
	printf("\t\treadregB %d\n", stateptr->IDEX.readregB);
	printf("\t\toffset %d\n", stateptr->IDEX.offset);
    printf("\tEXMEM:\n");
	printf("\t\tinstruction ");
	printInstruction(stateptr->EXMEM.instr);
	printf("\t\tbranchtarget %d\n", stateptr->EXMEM.branchtarget);
	printf("\t\taluresult %d\n", stateptr->EXMEM.aluresult);
	printf("\t\treadreg %d\n", stateptr->EXMEM.readreg);
    printf("\tMEMWB:\n");
	printf("\t\tinstruction ");
	printInstruction(stateptr->MEMWB.instr);
	printf("\t\twritedata %d\n", stateptr->MEMWB.writedata);
    printf("\tWBEND:\n");
	printf("\t\tinstruction ");
	printInstruction(stateptr->WBEND.instr);
	printf("\t\twritedata %d\n", stateptr->WBEND.writedata);
}


void print_stats(statetype* state)
{
	printf("total of %d cycles executed\n", state->cycles);
	printf("total of %d instructions fetched\n", state->fetched);
	printf("total of %d instructions retired\n", state->retired);
	printf("total of %d branches executed\n", state->branches);
	printf("total of %d branch mispredictions\n", state->mispreds);
}

void run(statetype* state, statetype* newstate){
	state->fetched = -3;// Fetched and Retired are set to -3 to compensate for the 3 retired noops and 3 fetched after halt.
	state->retired = -3;
	state->branches = 0;
	state->mispreds = 0;
	state->IFID.instr = NOOPINSTRUCTION; //All buffers are set to have noop instructions with zero values filling them.
	state->IDEX.instr = NOOPINSTRUCTION;
	state->EXMEM.instr = NOOPINSTRUCTION; 
	state->MEMWB.instr = NOOPINSTRUCTION;
	state->WBEND.instr = NOOPINSTRUCTION;
	state->IFID.instr = state->IFID.instr;
	state->IFID.pcplus1 = 0;
	state->IDEX.pcplus1 = 0;
	state->IDEX.readregA = 0;
	state->IDEX.readregB = 0;
	state->IDEX.offset = 0;
	state->EXMEM.branchtarget = 0;
	state->EXMEM.aluresult = 0;
	state->EXMEM.readreg = 0;
	state->MEMWB.writedata = 0;
	state->WBEND.writedata = 0;
	// Primary loop
	while(1){
		printstate(state);
		/* check for halt */
		if(HALT == opcode(state->MEMWB.instr)) {
			printf("machine halted\n");
			print_stats(state);
			break;
		}
		*newstate = *state;
		newstate->retired = state->retired + 1;//incrementing both as they will finish one.
		newstate->fetched = state->fetched + 1;
		newstate->cycles++;
		/*------------------ IF stage ----------------- */
		ifStage(state, newstate, state->pc);//Each buffer will send each respective data to the next stage. 
		/*------------------ ID stage ----------------- */
		 idStage(state, newstate, state->IFID.instr,state->IFID.pcplus1);
		/*------------------ EX stage ----------------- *///forward here by check the previous add loads and nands
		exStage(state, newstate, state->IDEX.instr,state->IDEX.pcplus1,state->IDEX.readregA,state->IDEX.readregB,state->IDEX.offset);
		/*------------------ MEM stage ----------------- */
		memStage(state, newstate,state->EXMEM.instr, state->EXMEM.branchtarget, state->EXMEM.aluresult, state->EXMEM.readreg);
		/*------------------ WB stage ----------------- */
		wbStage(state, newstate, state->MEMWB.instr, state->MEMWB.writedata);
		*state = *newstate; 	/* this is the last statement before the end of the loop.  
					It marks the end of the cycle and updates the current
					state with the values calculated in this cycle
					â€“ AKA â€œClock Tickâ€. */
	}
	
}

void  ifStage(statetype* state, statetype* newstate, int pc)
{
	newstate->IFID.instr = state->instrmem[pc];//fetch the next instrution.
	newstate->IFID.pcplus1 = pc + 1;//Fetch pc + 1.
	newstate->pc = pc + 1;//Increment pc to pc plus one.
}//idStage

void  idStage(statetype* state, statetype* newstate, int instr, int pcplus1)
{
	int op = opcode(instr);
	newstate->IDEX.instr = instr;//pass through both instruction and pc + 1
	newstate->IDEX.pcplus1 = pcplus1;	
	newstate->IDEX.readregA = state->reg[field0(instr)];//regA contents
	newstate->IDEX.readregB = state->reg[field1(instr)];//regB contents
	newstate->IDEX.offset = signExtend(field2(instr));//offset portion
}//idStage

void  exStage(statetype* state, statetype* newstate, int instr, int pcplus1, int readregA, int readregB, int offset)
{
	int op = opcode(instr);//retrieve opcode for this instruction.
	int forwardedRegAValue = readregA;//Value will either be the forwarded value if needed or the normal value 
	int forwardedRegBValue = readregB;//Value will either be the forwarded value if needed or the normal value
	newstate->EXMEM.instr = instr;
	newstate->EXMEM.branchtarget = pcplus1 + offset;//pass through both instruction and pc + 1
	if(op == ADD){//add
		newstate->EXMEM.readreg = offset;//need offset because this the register to change
		forwardedRegAValue = forwardedRegA(state,newstate, field0(instr), readregA);//updates regA value if needed
		forwardedRegBValue = forwardedRegB(state,newstate, field1(instr), readregB);//updates regB value if needed	
		newstate->EXMEM.aluresult = forwardedRegAValue + forwardedRegBValue;//aluresult is the add result
	}//add

	else if(op == NAND)
	{//nand
		newstate->EXMEM.readreg = offset;//offset is the register we will change
		forwardedRegAValue = forwardedRegA(state,newstate, field0(instr), readregA);//updates regA value if needed
		forwardedRegBValue = forwardedRegB(state,newstate, field1(instr), readregB);//updates regB value if needed	
		newstate->EXMEM.aluresult = ~(forwardedRegAValue & forwardedRegBValue);//This is the result of the nand of the two registers.
	}//nand
		
	else if(op == LW){//lw
		int LWcheck = 0;
		newstate->EXMEM.readreg = field0(instr);//regA will be stored with something so it is sent through
		forwardedRegBValue = forwardedRegB(state,newstate, field1(instr), readregB);//Only regB for LW has the potential to need to have a forwarded value.		
		//-----------------------------------------------------------------------------------------------------------------
		LWcheck = checkLWstall(state,  newstate, field0(instr));//checks the destination reg of previous LW//This method looks at up coming instruction and determines if it needs a stall.
		if(LWcheck == 1)
		{
			stall(state, newstate);//Method to stall the code if needed.
		}
		newstate->EXMEM.aluresult = offset + forwardedRegBValue; //alu result is the spot from mem we will retrieve, base plus offset
	}//lw	

	else if(op == SW){//sw
		forwardedRegAValue = forwardedRegA(state,newstate, field0(instr), readregA);//updates regA value if needed
		forwardedRegBValue = forwardedRegB(state,newstate, field1(instr), readregB);//updates regB value if needed
		newstate->EXMEM.readreg = forwardedRegAValue ;//the contents of regA will be store in memory, this value will either be changed to the correct value or it will remain the same
		newstate->EXMEM.aluresult = offset + forwardedRegBValue;//alu result is the location of memory change, base plus offset, regB will either get a forwarded value or it will remain itself
	}//sw

	else if(op == BEQ){//beq
		forwardedRegAValue = forwardedRegA(state,newstate, field0(instr), readregA);//updates regA value if needed
		forwardedRegBValue = forwardedRegB(state,newstate, field1(instr), readregB);//updates regB value if needed		
		newstate->EXMEM.aluresult = forwardedRegBValue-forwardedRegAValue;//checks equality.
	}//BEQ

	else{//halt and noop
		newstate->EXMEM.readreg = readregA;//does not matter
		newstate->EXMEM.aluresult = readregA + readregB;//does not matter
	}//normal

}//exStage

void  memStage(statetype* state, statetype* newstate, int instr, int branchtarget, int aluresult, int readreg)
{
	newstate->MEMWB.instr = instr;//Sends through the instruction
	int op = opcode(instr);//Grabs the opcode of the current instruction
	if(op == BEQ){//beq finishes right here
		newstate->branches = newstate->branches + 1;//Increase number of branches
		if(aluresult == 0){//If equal register A and B then branch to new location
			beqClear(state, newstate);//clear previous because they were an incorrect prediction.
			newstate->MEMWB.writedata = readreg;
			newstate->pc = branchtarget;}//alu = 0
		else{
			newstate->MEMWB.writedata = readreg;}//alu != 0
	}//opcode BEQ
	else if(op == SW){//SW finishs right here
		newstate->datamem[aluresult] = readreg;//changes the datamemory of the base plus offset to the value in registerA.
		newstate->MEMWB.writedata = readreg;}
	else if(op == LW)//lw sends through the data memory number
	{
		newstate->MEMWB.writedata = newstate->datamem[aluresult];
	}
	else{//add, nanduse aluresult for the write back
		newstate->MEMWB.writedata = aluresult;}
}//memStage

void  wbStage(statetype* state, statetype* newstate, int instr, int writedata)
{
	newstate->WBEND.instr = instr;//Sends through the instruction
	int op = opcode(instr);//Grabs the opcode of the current instruction
	if(op == ADD || op == NAND)//If add or nand then grab the dest reg. 
	{
		int destReg = field2(instr);
		newstate->reg[destReg] = writedata;//Change register to the updated value.
		newstate->WBEND.writedata = writedata;
	}
	else if(op == LW)//lw case uses regA and base plus offset which is writedata
	{
		newstate->reg[field0(instr)] = writedata;// If load word then change the register to the correct value.
		newstate->WBEND.writedata = writedata;
	}
	else{
		newstate->WBEND.writedata = writedata;}
}//wbStage

void beqClear(statetype* state, statetype* newstate)
{
	newstate->IFID.instr = NOOPINSTRUCTION;//Set the three previous instructions to noops because it was an incorrect guess. 
	newstate->IFID.pcplus1 = 0;
	newstate->IDEX.instr =  NOOPINSTRUCTION;
	newstate->IDEX.pcplus1 = 0;
	newstate->IDEX.readregA = 0;//get regA contents
	newstate->IDEX.readregB = 0;//get regB contents
	newstate->IDEX.offset = 0;
 	newstate->EXMEM.instr = NOOPINSTRUCTION;
	newstate->EXMEM.branchtarget = 0;
	newstate->EXMEM.aluresult=0; 
	newstate->EXMEM.readreg = 0;
	newstate->mispreds = state->mispreds + 1;// This means we had a miss prediction and thereby need to increase the number wrongly predicted. 
	newstate->fetched = state->fetched - 2;
	
	
}

void stall(statetype* state, statetype* newstate)//A lw stall is need so this is called.
{
	newstate->pc = state->pc;//do not increase the pc because it will be sent through in the next call.
	newstate->IFID.instr = state->IFID.instr;//Reset the buffers of all before the execute stage so that they will be stalled.
	newstate->IFID.pcplus1 = state->IFID.pcplus1;//set to old value
	newstate->IDEX.instr =  NOOPINSTRUCTION;//the noop stall
	newstate->IDEX.pcplus1 = state->IDEX.pcplus1;//set to old value
	newstate->IDEX.readregA = 0;//get regA contents
	newstate->IDEX.readregB = 0;//get regB contents
	newstate->IDEX.offset = 0;//set to zero.
	newstate->fetched = state->fetched;//Do not increase the value fetched because it will be fetched again. 
	newstate->retired = state->retired;

}

int forwardedRegA(statetype* state, statetype* newstate, int regA, int regAValue)
{
	int op1 = opcode(state->EXMEM.instr);//opcode of previous instruction
	int op2 = opcode(state->MEMWB.instr);//opcode of instruction 2 back
	int op3 = opcode(state->WBEND.instr);//opcode of instruction 3 back
	int updated = 0;
	int result = regAValue;//this value will either be a forwarded value or its default value.
	if(op1 == ADD || op1 == NAND)// if add or nand check field 2 for similarity of regs
	{//forward
		if(regA == state->EXMEM.readreg)//if same then we must update value to be up to date
		{
			updated = 1;//the soon back we find it the more up to date we will be so there is no reason to look back. 
			result = state->EXMEM.aluresult;//new updated value.		
		}
	}//add or nand for ADD first level

	if((op2 == ADD || op2 == NAND) && updated == 0)////next instruction back we look at field 2 for add and nand
	{//forward                      
		if(regA == field2(state->MEMWB.instr))
		{
			updated = 1;
			result = state->MEMWB.writedata;		
		}
	}//add or nand for ADD second level



	if(op2 == LW && updated == 0)
	{//forward                                                
		if(regA == field0(state->MEMWB.instr))////next instruction back we look at field 0 for LW
		{
			updated = 1;
			result = state->MEMWB.writedata;		
		}
	}//lw for ADD second level



	if((op3 == ADD || op3 == NAND) && updated == 0)////next instruction backwe look at field 3 for add and nand
	{//forward                      
		if(regA == field2(state->WBEND.instr))
		{
			printf("regAValue before level 3 ADD and NAND %d \n", regAValue);
			result = state->WBEND.writedata;		
		}
	}//add or nand for ADD third level

	if(op3 == LW && updated == 0) //next instruction back we look at field 0 for LW
	{//forward  
		printf("regAValue before level 3LW %d \n", regAValue);                                              
		if(regA == field0(state->WBEND.instr))
		{
			printf("regAValue before level 3LW %d \n", regAValue);
			result = state->WBEND.writedata;		
		}
	}//lw for ADD third level
	return result;
}//forwardedRegA

int forwardedRegB(statetype* state, statetype* newstate, int regB, int regBValue)// This function does the same thing as forwardedRegB but it looks at field 1 to compare to past instructions destination registers.
{	
	int op1 = opcode(state->EXMEM.instr);
	int op2 = opcode(state->MEMWB.instr);
	int op3 = opcode(state->WBEND.instr);
	int updated = 0;
	int result = regBValue;
	if(op1  == ADD || op1  == NAND){//forward
			if(regB == state->EXMEM.readreg)
			{
				updated = 1;
				result = state->EXMEM.aluresult;		
			}
					}//add or nand for ADD first level

		if((op2 == ADD || op2 == NAND) && updated == 0){//forward                      
			if(regB  == field2(state->MEMWB.instr))
			{
				updated = 1;
				result = state->MEMWB.writedata;		
			}
		}//add or nand for ADD second level



		if(op2 == LW && updated == 0){//forward                                                
			if(regB  == field0(state->MEMWB.instr))
			{
				updated = 1;
				result = state->MEMWB.writedata;		
			}
		}//lw for ADD second level



		if((op3 == ADD || op3 == NAND) && updated == 0){//forward                      
			if(regB  == field2(state->WBEND.instr))
			{
				result = state->WBEND.writedata;		
			}
		}//add or nand for ADD third level

		if(op3 == LW && updated == 0){//forward                                                
			if(regB  == field0(state->WBEND.instr))
			{
				result = state->WBEND.writedata;		
			}
		}//lw for ADD third level
	return result;
}//forwardedRegB


int checkLWstall(statetype* state, statetype* newstate, int dReg)// This checks to see if a load word stall is need. 
{
	int result = 0;
	int opIFID = opcode(state->IFID.instr);//grab the opcode of the next instruction
	if(opIFID == LW)//If the next instruction is a lw then we check the field 2 or register B to see if it is the destination reg for the current instruction
	{
		if(dReg == field1(state->IFID.instr))
		{
			result = 1;//One means we a stall is needed
		}
	}
	else if(opIFID == NAND || opIFID == ADD || opIFID == SW || opIFID == BEQ)//IF the next instruction is ADD, SW, NAND, BEQ then we check RegA and RegB aka field zero and field 1.
	{
		if(dReg == field0(state->IFID.instr))// regA is the same is our LW's destination reg so we have to stall
		{
			result = 1;
		}
		else if(dReg == field1(state->IFID.instr))// regB is the same is our LW's destination reg so we have to stall
		{
			result = 1;
		}
	}
	return result;
}


int main(int argc, char** argv){

	/** Get command line arguments **/
	char* fname;

	opterr = 0;

	int cin = 0;

	while((cin = getopt(argc, argv, "i:")) != -1){
		switch(cin)
		{
			case 'i':
				fname=(char*)malloc(strlen(optarg));
				fname[0] = '\0';

				strncpy(fname, optarg, strlen(optarg)+1);
				break;
			case '?':
				if(optopt == 'i'){
					printf("Option -%c requires an argument.\n", optopt);
				}
				else if(isprint(optopt)){
					printf("Unknown option `-%c'.\n", optopt);
				}
				else{
					printf("Unknown option character `\\x%x'.\n", optopt);
					return 1;
				}
				break;
			default:
				abort();
		}
	}

	FILE *fp = fopen(fname, "r");
	if (fp == NULL) {
		printf("Cannot open file '%s' : %s\n", fname, strerror(errno));
		return -1;
	}

	/* count the number of lines by counting newline characters */
	int line_count = 0;
	int c;
	while (EOF != (c=getc(fp))) {
		if ( c == '\n' ){
			line_count++;
		}
	}
	// reset fp to the beginning of the file
	rewind(fp);

	statetype* state = (statetype*)malloc(sizeof(statetype));
	statetype* newstate = (statetype*)malloc(sizeof(statetype));	

	state->pc = 0;
	
	memset(state->instrmem, 0, NUMMEMORY*sizeof(int));
	memset(state->datamem, 0, NUMMEMORY*sizeof(int));
	memset(state->reg, 0, NUMREGS*sizeof(int));

	state->numMemory = line_count;

	char line[256];

	int i = 0;
	while (fgets(line, sizeof(line), fp)) {
		/* note that fgets doesn't strip the terminating \n, checking its
		   presence would allow to handle lines longer that sizeof(line) */
		state->instrmem[i] = atoi(line);
		state->datamem[i] = atoi(line);
		i++;
	}
	fclose(fp);

	/** Run the simulation **/
	run(state,newstate);

	free(state);
	free(fname);

}
