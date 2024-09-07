// R2 = R0 * R1
// assume R0 >= 0, R1 >= 0, R0 * R1 < 32768 (2^15)

@R2
M=0 // R2 = 0
@R0
D=M
@idx
M=D // idx = R0
D=M

(LOOP)
  @END
  D;JLE
  @R1
  D=M
  @R2
  M=D+M
  @idx
  M=M-1
  D=M
  @LOOP
  0;JMP

(END)
  @END
  0;JMP