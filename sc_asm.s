@ Test code for my own new function called from C

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

    .global sc_led_demo       @ Make the symbol name for the function visible to the linker

    .code   16              @ 16bit THUMB code (BOTH .code and .thumb_func are required)
    .thumb_func             @ Specifies that the following symbol is the name of a THUMB
                            @ encoded function. Necessary for interlinking between ARM and THUMB code.

    .type   sc_led_demo, %function   @ Declares that the symbol is a function (not strictly required)

@ Function Declaration : int busy_delay(int cycles)
@
@ Input: r0 (i.e. r0 holds number of cycles to delay)
@ Returns: r0
@

@ Here is the actual function
busy_delay:
    push {r4}
    mov r4, r0

delay_loop:
    subs r4, r4, #1
    bgt delay_loop
    mov r0, #0                      @ Return zero (always successful)
    pop {r4}
    bx lr                           @ Return (Branch eXchange) to the address held in the link register (lr)

@ Function Declaration : int sc_led_demo(int count, int delay)
@
@ Input: r0, r1 (i.e. r0 holds count for demo loop, r1 holds delay of LED)
@ Returns: void
@

@ Here is the actual function
sc_led_demo:
    push {r4-r7,lr}
    mov  r4, r0             @Count value
    mov  r5, r1             @Delay value

loop_demo:
    cbz  r4, loop_demo_exit @IF counter is zero exit loop_demo
    mov  r6, #0             @LED value
    mov  r7, #8

loop_led:
    cbz  r7, loop_led_exit  @IF counter is zero exit loop_led

    mov  r0, r6             @Input LED value
    bl   BSP_LED_Toggle     @call LED toggle

    mov  r0, r5             @Input delay amount
    bl   busy_delay         @call the delay function

    subs r7, #1             @decrement loop counter
    add  r6, #1             @increment light
    b    loop_led           @branch to begining of loop

loop_led_exit:
    subs r4, #1             @decrement the demo loop counter
    b loop_demo             @branch back to begining of loop
loop_demo_exit:

    pop {r4-r7,lr}          @restore Link Register
    bx lr                   @ Return (Branch eXchange) to the address in the link register (lr)

    .size   sc_led_demo, .-sc_led_demo    @@ - symbol size (not req)

@ Assembly file ended by single .end directive on its own line
.end
