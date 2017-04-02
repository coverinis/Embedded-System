@ mycode.s :
@ Test code for STM32 and linking assembly to C

@ This is a comment. Anything after an @ symbol is ignored.
@@ This is also a comment. Some people use double @@ symbols. 


    .code   16              @ This directive selects the instruction set being generated. 
                            @ The value 16 selects Thumb, with the value 32 selecting ARM.

    .text                   @ Tell the assembler that the upcoming section is to be considered
                            @ assembly language instructions - Code section (text -> ROM)

@@ Function Header Block
    .align  2               @ Code alignment - 2^n alignment (n=2)
                            @ This causes the assembler to use 4 byte alignment

    .syntax unified         @ Sets the instruction set to the new unified ARM + THUMB
                            @ instructions. The default is divided (separate instruction sets)

    .global mytest          @ Make the symbol name for the function visible to the linker

    .code   16              @ 16bit THUMB code (BOTH .code and .thumb_func are required)
    .thumb_func             @ Specifies that the following symbol is the name of a THUMB
                            @ encoded function. Necessary for interlinking between ARM and THUMB code.

    .type   mytest, %function   @ Declares that mytest symbol is a function (not strictly required)

@ Function Declaration : int mytest(int x)
@
@ Input: r0 (i.e. r0 holds x)
@ Returns: r0
@ 

@ Here is the actual function
mytest:

    push {r4-r7, lr}                @ Put aside registers we want to restore later

    mov  r0, #1                     @ r0 holds our argument for the LED toggle function
                                    @ So pass it a value

    bl   BSP_LED_Toggle             @ call BSP C function using Branch with Link (bl)
    
    ldr  r1, =myTickCount
    ldr  r0, [r1]

    pop  {r4-r7, lr}                @ Bring all the register values back

    bx lr                           @ Return (Branch eXchange) to the address held in the link register (lr) 

    .size   mytest, .-mytest    @@ - symbol size (not req)

@@ Function Header Block
    .align  2                   @@ 2^n alignment (n=2)
    .syntax unified             @@ Unified Syntax
    .global my_Tick             @@ Expose my_Tick to the linker
    .code   16                  @@ - 16bit THUMB code (BOTH are required!)
    .thumb_func                 @@ /

    .type   my_Tick, %function  @@ - symbol type (not req)

@@ Declaration : void my_Tick( void )
@@ Uses nothing

my_Tick:
    
    push {lr}

    ldr  r1, =myTickCount   @@ Address of myTickCount stored in r1
    ldr  r0, [r1]           @@ Load r0 with the address pointed at by r1 (myTickCount address)
    add  r0, r0, #1         @@ Increment r0
    str  r0, [r1]           @@ Store the current r0 value back to the address pointed at by r1

    pop {lr}

    bx lr                  @@ Return to the address stored in lr

    .size   my_Tick, .-my_Tick    @@ - symbol size (not req)

@@ <function block>
    .align  2               @@ - 2^n alignment (n=2)
    .syntax unified
    .global my_Loop          @@ - Symbol name for function
    .code   16              @@ - 16bit THUMB code (BOTH are required!)
    .thumb_func             @@ /
    .type   my_Loop, %function   @@ - symbol type (not req)
@@ Declaration : void my_Loop( void )
@@ Uses nothing
my_Loop:
    push {lr}
    pop  {pc}
    .size   my_Loop, .-my_Loop    @@ - symbol size (not req)

@@ <function block>
    .align  2               @@ - 2^n alignment (n=2)
    .syntax unified
    .global my_Init          @@ - Symbol name for function
    .code   16              @@ - 16bit THUMB code (BOTH are required!)
    .thumb_func             @@ /
    .type   my_Init, %function   @@ - symbol type (not req)

@@ Declaration : void my_Init( void )
@@ Uses nothing
my_Init:
    bx lr
    @@push {lr}
    @@pop  {pc}
    .size   my_Init, .-my_Init    @@ - symbol size (not req)


@ Program Data section - Items declared here are read/write

    .data

    .global myTickCount     @ Expose the symbol to the linker

@ Actual declaration of the symbol
myTickCount:
    .word  1                @ A 32-bit variable named myTickCount


@ Assembly file ended by single .end directive on its own line
.end

Things past the end directive are not processed, as you can see here.
