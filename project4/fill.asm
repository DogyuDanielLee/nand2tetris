// if key pressed
//   if curr screen is black
//     good don't do anything
//   else (curr screen is white)
//     blacken the screen
// else (key not pressed)
//   if curr screen is black
//     whiten the screen
//   else (curr screen is white)
//     good don't do anything

(LOOP)
  @idx
  M=0
  @KBD
  D=M
  @CHECKNOTPRESSED
  D;JEQ // goto CHECK NOT PRESSED if RAM[KBD] == 0
  @CHECKPRESSED
  0;JMP // goto CHECK NOT PRESSED if RAM[KBD] != 0
  
(CHECKPRESSED)
  @SCREEN
  D=M
  @BLACKEN
  D;JEQ // goto BLACKEN if RAM[SCREEN] == 0 (white)
  @LOOP
  0;JMP // goto LOOP if RAM[SCREEN] != 0 (already black)


(CHECKNOTPRESSED)
  @SCREEN
  D=M
  @LOOP
  D;JEQ // goto LOOP if RAM[SCREEN] == 0 (alreadwhite)
  @WHITEN
  0;JMP // goto WHITEN if RAM[SCREEN] != 0 (black)

(BLACKEN)
  @idx
  M=0
  (LOOPB)
    @8192
    D=A
    @idx
    D=M-D
    @LOOP
    D;JGE // end blacken

    @SCREEN
    D=A
    @idx
    D=D+M
    A=D
    M=-1 // blacken this cell
    @idx
    M=M+1
    @LOOPB
    0;JMP


(WHITEN)
  @idx
  M=0
  (LOOPW)
    @8192
    D=A
    @idx
    D=M-D
    @LOOP
    D;JGE // end whiten

    @SCREEN
    D=A
    @idx
    D=D+M
    A=D
    M=0 // whiten this cell
    @idx
    M=M+1
    @LOOPW
    0;JMP