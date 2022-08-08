// This file is part of www.nand2tetris.org
// and the book "The Elements of Computing Systems"
// by Nisan and Schocken, MIT Press.
// File name: projects/04/Mult.asm

// Multiplies R0 and R1 and stores the result in R2.
// (R0, R1, R2 refer to RAM[0], RAM[1], and RAM[2], respectively.)
//
// This program only needs to handle arguments that satisfy
// R0 >= 0, R1 >= 0, and R0*R1 < 32768.

// Put your code here.
//Hack programmer is aware of two 16-bit registers D,A 
//Can be manipulated by arithmetic/logical instructions
    //A=D-1
    //D=!A 
//D used solely to store data values
//A doubles as data and address register
//Hack requires that memory access instrs operate on an implicit
//memory location always labled M 
    //M always refers to the memory word whose address is the current
    //value of the A register 
    //M = &A 
    //For example, if we want to do 
    //  D = RAM[516] - 1
    //we need an instruction to set A to 516, then 
    //  D = M - 1
// A is read by jump instructions, and the jump instructions don't 
// specify an address themselves
// @value stores the specified value in A register 

@i //declare variable i, A=&i
M = 0 //*A = 0
@R2 //A=&R2
M = 0 //*A = 0

(LOOP_PREDICATE)
    @i          //A = &i;
    D=M         //D = *A, which is i 
    @R0         //A = &R0 
    D=D-M       //D = D - *A 
    @LOOP_END   //A = (LOOP_END) 
    D;JGT       //if(i-R0 > 0) goto LOOP_END 
(LOOP_BODY)
    @R2         //A = &R2
    D=M         //D = *A 
    @R1         //A = &R1
    M=D+M       //*A = D + *A
    @i          //A = &i
    M=M+1       //*A = *A + 1
    @LOOP_PREDICATE //A = (LOOP_PREDICATE)
    0;JMP
(LOOP_END)
    @END
    0;JMP //infinite loop 