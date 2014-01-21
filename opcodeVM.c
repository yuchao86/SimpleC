/**
 *	opcodeVM code
 *
 *	@E-Mail:yuchao86@gmail.com
 *  @author YuChao
 *  @Copyright under GNU General Public License All rights reserved.
 *  @date 2014-01-21
 *  @see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef enum {
	opHALT,				/* Just stop execution, r, s, t all ignored */
	opIN,					/* Read a value into regs[r], s and t ignored */
	opOUT,				/* Wriet regs[r] to the terminal, s and t ignored */
	opADD,				/* regs[r] = regs[s] + regs[t] */
	opSUB,				/* regs[r] = regs[s] - regs[t] */
	opMUL,				/* regs[r] = regs[s] * regs[t] */
	opDIV,				/* regs[r] = regs[s] / regs[t] */
	opRRLIM,			/* meaningless, we use it to distinguish the opcodes' addressing mode */ 

	opLD,					/* Load dmem[regs[s]+t] into regs[r] */
	opST,					/* Store regs[r] at dmem[regs[s]+t] */
	opRMLIM,			/* meaningless as opRRLIM */

	opLDA,				/* load regs[s]+t into regs[r] */
	opLDC,				/* load t into regs[r] */
	opJLT,				/* if regs[r] < 0, regs[PC_REG] = regs[s]+t */
	opJLE,				/* if regs[r] <= 0, regs[PC_REG] = regs[s]+t */
	opJGT,				/* if regs[r] > 0, regs[PC_REG] = regs[s]+t */
	opJGE,				/* if regs[r] >= 0, regs[PC_REG] = regs[s]+t */
	opJEQ,				/* if regs[r] == 0, regs[PC_REG] = regs[s]+t */
	opJNE,				/* if regs[r] != 0, regs[PC_REG] = regs[s]+t */
	opRALIM				/* meaningless as opRRLIM */
}opCode;

typedef struct {
	opCode iop;
	int iarg1;
	int iarg2;
	int iarg3;
}instruc_t;

char *opCodeTab[] = { "HALT","IN","OUT","ADD","SUB","MUL","DIV","???", \
	"LD","ST","???", \
	"LDA","LDC","JLT","JLE","JGT","JGE","JEQ","JNE","???"};

typedef enum {
	sOKAY,						/* the instruction completes execution */
	sIMEM_ERR,				/* access non-exist instruction memory */
	sDMEM_ERR,				/* access non-exist data memory */
	sDIVIDEZERO_ERR,	/* when a operand is divided by zero */
	sNOINS_ERR				/* the instruction itself doesn't exist */
}opState;

char* opStateTab[] = { "Okay","Instruction Memory Access Error", \
	"Data Memory Access Error","Divided By Zero", \
	"No such instruction"};

#define IADDR_SIZE 1024
#define DADDR_SIZE 1024
#define NR_REGS 8
instruc_t imem[IADDR_SIZE];	/* Memory for instructions */
int dmem[DADDR_SIZE];					/* Memory for data */
int regs[NR_REGS];						/* Registers */
#define PC_REG 7							/* I regard the 7th register as Program Counter */

FILE* ifile;

/* Function declarations */
void die(const char* msg);
opCode map_ins(const char *ins);

void vm_setup(void);
void vm_load(void);
void vm_run(void);

opState ins_exec(instruc_t *inp);

opState in_exec(instruc_t *inp);
opState out_exec(instruc_t *inp);
opState add_exec(instruc_t *inp);
opState sub_exec(instruc_t *inp);
opState mul_exec(instruc_t *inp);
opState div_exec(instruc_t *inp);
opState ld_exec(instruc_t *inp);
opState st_exec(instruc_t *inp);
opState lda_exec(instruc_t *inp);
opState ldc_exec(instruc_t *inp);
opState jlt_exec(instruc_t *inp);
opState jle_exec(instruc_t *inp);
opState jgt_exec(instruc_t *inp);
opState jge_exec(instruc_t *inp);
opState jeq_exec(instruc_t *inp);
opState jne_exec(instruc_t *inp);
/**
 * void vm_setup(void);
 * initializ regs, imen, dmem.
 */
void vm_setup(void)
{
	int i;

	/* Set all registers zero */
	for (i=0; i<NR_REGS; i++) regs[i] = 0;

	/* Set all instructions' opcodes opHALT, and arguments zero */
	for (i=0; i<IADDR_SIZE; i++) {
		imem[i].iop = opHALT;
		imem[i].iarg1 = 0;
		imem[i].iarg2 = 0;
		imem[i].iarg3 = 0;
	}

	/* Set all data memory zero */
	for (i=0; i<DADDR_SIZE; i++) dmem[i] = 0;
}

void die(const char* msg)
{
	printf("fatal: %s", msg);
	exit(-1);
}

opCode map_ins(const char *ins)
{
	opCode op = opHALT;

	while (strcmp(ins, opCodeTab[op])!=0 && op<=opRALIM)
		op++;

	if (op>opRALIM) die("non-exist opcode");
	return op;
}

void vm_load(void)
{
	int i = 0;
	int r, s, t;
	char ins[20];

	while (fscanf(ifile,"%s",ins)!=EOF) {
		printf("opcode: %s\n",ins);
		imem[i].iop = map_ins(ins);

		/* Read iarg1. It must be a register number, thus 0<=iarg1<NR_REGS */
		fscanf(ifile,"%d",&r);
		if (r<0 || r >=NR_REGS)
			die("iarg1: non-exist register");
		imem[i].iarg1 = r;

		/* Read iarg2. It must be a register number. */
		fscanf(ifile,"%d",&s);
		if (s<0 || s>=NR_REGS)
			die("iarg2: non-exist register");
		imem[i].iarg2 = s;

		/* Read iarg3. It is a register number when opclRM, 
			 an integer number when other addressing mode */
		fscanf(ifile,"%d",&t);
		if (imem[i].iop<opRRLIM && (t<0 || t>=NR_REGS))
			die("iarg3: non-exist registers");
		imem[i].iarg3 = t;
		
#ifdef DEBUG
		printf("read instruction: %s %d %d %d\n",opCodeTab[imem[i].iop],imem[i].iarg1,imem[i].iarg2,imem[i].iarg3);
#endif

		++i;
	}
	close(ifile);
}

void vm_run(void)
{
	instruc_t *inp = NULL;
	opState st;
	inp = &imem[regs[PC_REG]];	/* fetch instruction */
	while (inp->iop!=opHALT) {
		if ((st=ins_exec(inp))!=sOKAY)	/* instruction executes */
			die(opStateTab[st]);

		/* If inp is not a jamp instruction,
			 VM will execute next one. */
		if (inp->iop<opJLT)	/* modify the PC register */
			++(regs[PC_REG]);
		inp = &imem[regs[PC_REG]];
	}

	return;
}

opState ins_exec(instruc_t* inp)
{
	switch (inp->iop) {
		case opIN:	return in_exec(inp);
		case opOUT:	return out_exec(inp);
		case opADD: return add_exec(inp);
		case opSUB: return sub_exec(inp);
		case opMUL: return mul_exec(inp);
		case opDIV:	return div_exec(inp);

		case opLD:	return ld_exec(inp);
		case opST:	return st_exec(inp);

		case opLDA:	return lda_exec(inp);
		case opLDC:	return ldc_exec(inp);
		case opJLT:	return jlt_exec(inp);
		case opJLE:	return jle_exec(inp);
		case opJGT:	return jgt_exec(inp);
		case opJGE:	return jge_exec(inp);
		case opJEQ:	return jeq_exec(inp);
		case opJNE:	return jne_exec(inp);
		default:	return sNOINS_ERR;
	}
}

opState in_exec(instruc_t *ins)
{
	int r = ins->iarg1;
	scanf("%d", &regs[r]);
	return sOKAY;
}

opState out_exec(instruc_t *ins)
{
	int r = ins->iarg1;
	printf("%d",regs[r]);
	return sOKAY;
}
opState add_exec(instruc_t *ins)
{
	int r = ins->iarg1;
	int s = ins->iarg2;
	int t = ins->iarg3;

	regs[r] = regs[s] + regs[t];

	return sOKAY;
}

opState sub_exec(instruc_t *ins)
{
	int r = ins->iarg1;
	int s = ins->iarg2;
	int t = ins->iarg3;

	regs[r] = regs[s] - regs[t];
	
	return sOKAY;
}

opState mul_exec(instruc_t *ins)
{
	int r = ins->iarg1;
	int s = ins->iarg2;
	int t = ins->iarg3;

	regs[r] = regs[s] * regs[t];
	
	return sOKAY;
}

opState div_exec(instruc_t *ins)
{
	int r = ins->iarg1;
	int s = ins->iarg2;
	int t = ins->iarg3;

	if (regs[t]==0) return sDIVIDEZERO_ERR;
	regs[r] = regs[s] / regs[t];
	
	return sOKAY;
}

opState ld_exec(instruc_t *ins)
{
	int r = ins->iarg1;
	int s = ins->iarg2;
	int t = ins->iarg3;

	if (regs[s]+t < 0 || regs[s]+t >= DADDR_SIZE)
		return sDMEM_ERR;
	regs[r] = dmem[regs[s]+t];
	
	return sOKAY;
}

opState st_exec(instruc_t *ins)
{
	int r = ins->iarg1;
	int s = ins->iarg2;
	int t = ins->iarg3;

	if (regs[s]+t < 0 || regs[s]+t >= DADDR_SIZE)
		return sDMEM_ERR;
	dmem[regs[s]+t] = regs[r];

	return sOKAY;
}

opState lda_exec(instruc_t *ins)
{
	int r = ins->iarg1;
	int s = ins->iarg2;
	int t = ins->iarg3;

	if (regs[s]+t < 0 || regs[s]+t >= DADDR_SIZE)
		return sDMEM_ERR;
	regs[r] = regs[s]+t;
	
	return sOKAY;
}

opState ldc_exec(instruc_t *ins)
{
	int r = ins->iarg1;
	int t = ins->iarg3;

	regs[r] = t;
	
	return sOKAY;
}

opState jlt_exec(instruc_t *ins)
{
	int r = ins->iarg1;
	int s = ins->iarg2;
	int t = ins->iarg3;

	if (regs[s]+t < 0 || regs[s]+t >= IADDR_SIZE)
		return sIMEM_ERR;

	if (regs[r] < 0)
		regs[PC_REG] = regs[s] + t;

	return sOKAY;
}

opState jle_exec(instruc_t *ins)
{
	int r = ins->iarg1;
	int s = ins->iarg2;
	int t = ins->iarg3;

	if (regs[s]+t < 0 || regs[s]+t >= IADDR_SIZE)
		return sIMEM_ERR;

	if (regs[r] <= 0)
		regs[PC_REG] = regs[s] + t;

	return sOKAY;
}

opState jgt_exec(instruc_t *ins)
{
	int r = ins->iarg1;
	int s = ins->iarg2;
	int t = ins->iarg3;

	if (regs[s]+t < 0 || regs[s]+t >= IADDR_SIZE)
		return sIMEM_ERR;

	if (regs[r] > 0)
		regs[PC_REG] = regs[s] + t;

	return sOKAY;
}

opState jge_exec(instruc_t *ins)
{
	int r = ins->iarg1;
	int s = ins->iarg2;
	int t = ins->iarg3;

	if (regs[s]+t < 0 || regs[s]+t >= IADDR_SIZE)
		return sIMEM_ERR;

	if (regs[r] >= 0)
		regs[PC_REG] = regs[s] + t;

	return sOKAY;
}

opState jeq_exec(instruc_t *ins)
{
	int r = ins->iarg1;
	int s = ins->iarg2;
	int t = ins->iarg3;

	if (regs[s]+t < 0 || regs[s]+t >= IADDR_SIZE)
		return sIMEM_ERR;

	if (regs[r] == 0)
		regs[PC_REG] = regs[s] + t;

	return sOKAY;
}

opState jne_exec(instruc_t *ins)
{
	int r = ins->iarg1;
	int s = ins->iarg2;
	int t = ins->iarg3;

	if (regs[s]+t < 0 || regs[s]+t >= IADDR_SIZE)
		return sIMEM_ERR;

	if (regs[r] != 0)
		regs[PC_REG] = regs[s] + t;

	return sOKAY;
}

int main(int argc, char **argv)
{
	char filename[20];

	if (argc!=2) {
		fprintf(stderr,"usage: %s <INPUT>\n", argv[0]);
		exit(-1);
	}

	strcpy(filename, argv[1]);

	/* ifile is a global variable denoting the input file */
	ifile = fopen(filename, "r");
	if (ifile==NULL) {
		fprintf(stderr,"the file %s doesn't exist\n",argv[1]);
		exit(-1);
	}
	
	vm_setup();	/* Initialize the TM  machine */
	vm_load();	/* Load the assembly input file denoted by ifile*/
	vm_run();		/* The machine startups */

    system("PAUSE");
	return 0;
}

