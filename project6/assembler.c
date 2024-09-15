/*
Assembler for NAND2Tetris Project 6 written in C
- converts Hack Assembly code in "assembly.txt" 
  to Hack Machine code at "machine.txt"
*/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *substring(const char *org, const int32_t from, const int32_t to) {
  if (from > to || from < 0 || to >= strlen(org)) {
    return NULL; // Invalid range or out of bounds.
  }

  int32_t len = to - from + 1;

  char *newstr = (char *)malloc(len + 1);
  if (!newstr)
    return NULL;

  // Copy the substring
  for (int32_t i = 0; i < len; i++)
    newstr[i] = org[from + i];

  // Null-terminate the new string
  newstr[len] = '\0';

  return newstr;
}

char *preprocess(char *line, int32_t linesize) {
  // erase white space
  // erase full line comment
  // erase inline comment
  char *processed = (char *)malloc(sizeof(char) * 256);
  int32_t processed_i = 0;
  for (int32_t i = 0; i < linesize; i++) {
    if (line[i] == ' ') {
      continue;
    } else if (line[i] == '\0') {
      processed[processed_i] = '\0';
      break;
    } else if (line[i] == '/') {
      if (i + 1 < linesize) {
        if (line[i + 1] == '/') {
          processed[processed_i] = '\0';
          break;
        } else {
          processed[processed_i++] = line[i];
        }
      } else {
        processed[processed_i++] = line[i];
      }
    } else if (line[i] == '\n') {
      processed[processed_i] = '\0';
      break;
    } else {
      processed[processed_i++] = line[i];
    }
  }
  return processed;
}

typedef struct _source_code {
  int32_t max_line;
  int32_t curr_line;
  char **memory;
} SourceCode;

SourceCode *initSourceCode(void) {
  SourceCode *code = (SourceCode *)malloc(sizeof(SourceCode));
  code->max_line = 1024;
  code->curr_line = 0;
  code->memory = (char **)malloc(sizeof(char *) * code->max_line);
  for (int32_t i = 0; i < code->max_line; i++) {
    code->memory[i] = (char *)malloc(sizeof(char) * 256);
  }
  return code;
}

void freeSouceCode(SourceCode *code) {
  if (code != NULL) {
    if (code->memory != NULL) {
      for (int32_t i = 0; i < code->max_line; i++)
        if (code->memory[i] != NULL)
          free(code->memory[i]);
      free(code->memory);
    }
    free(code);
  }
}

void _double_source_code_memory(SourceCode *code) {
  char **newmem = (char **)malloc(sizeof(char *) * code->max_line * 2);
  for (int32_t i = 0; i < code->max_line; i++)
    newmem[i] = code->memory[i];
  free(code->memory);
  code->memory = newmem;
  code->max_line *= 2;
}

void inputSourceCode(SourceCode *code, FILE *fin) {
  char line[256];
  char *temp;

  while (fgets(line, sizeof(line), fin)) {
    temp = preprocess(line, 256);
    if (temp[0] == '\0') {
      free(temp);
      continue;
    }
    if (code->curr_line == code->max_line)
      _double_source_code_memory(code);
    strcpy(code->memory[code->curr_line++], temp);
    free(temp);
  }
}

void outputSourceCode(SourceCode *code, FILE *fout) {
  for (int32_t i = 0; i < code->curr_line; i++) {
    fprintf(fout, "[%" PRId32 "] %s\n", i, code->memory[i]);
  }
}

typedef struct _node {
  char *symbol_str;
  int32_t memory_location;
  struct _node *next;
} Node;

Node *initNode(const char *symbol, const int32_t memory_location) {
  Node *node = (Node *)malloc(sizeof(Node));
  node->symbol_str = (char *)malloc(sizeof(char) * 256);
  strcpy(node->symbol_str, symbol);
  node->memory_location = memory_location;
  node->next = NULL;
  return node;
}

void freeNode(Node *node) {
  if (node != NULL) {
    if (node->symbol_str != NULL)
      free(node->symbol_str);
    free(node);
  }
}

void freeNodeChain(Node *node) {
  Node *itr1 = node;
  Node *itr2;
  while (itr1 != NULL) {
    itr2 = itr1->next;
    freeNode(itr1);
    itr1 = itr2;
  }
}

void setNextNode(Node *currnode, Node *nextnode) {
  if (currnode != NULL)
    currnode->next = nextnode;
}

Node *getNextNode(const Node *node) {
  if (node == NULL)
    return NULL;
  return node->next;
}

typedef struct _symbol_table {
  Node **memory;
  int32_t size;
} SymbolTable;

int32_t hash_function(char *symbol) {
  // assuming that [symbol] is a string consisted of 1~256 charaters,
  // the returned value will be in the range of [1, 66560]
  int32_t strsize = strlen(symbol);
  int32_t hashval = strsize;
  for (int32_t i = 0; i < strsize; i++) {
    if (symbol[i] >= '0' && symbol[i] <= '9')
      hashval += (i + 1) * (symbol[i] - '0' + 1);
    else if (symbol[i] >= 'a' && symbol[i] <= 'z')
      hashval += (i + 1) * (symbol[i] - 'a' + 1);
    else if (symbol[i] >= 'A' && symbol[i] <= 'Z')
      hashval += (i + 1) * (symbol[i] - 'A' + 1);
    else if (symbol[i] == '_')
      hashval += (i + 1);
    else {
      perror("symbols must be consisted of alphabets and numbers only");
      exit(EXIT_FAILURE);
    }
  }
  return hashval;
}

SymbolTable *initSymbolTable(void) {
  // initialize
  SymbolTable *table = (SymbolTable *)malloc(sizeof(SymbolTable));

  table->size = 70000; // precisely 66561 but just to be safe
  table->memory = (Node **)malloc(sizeof(Node *) * table->size);
  for (int32_t i = 0; i < table->size; i++)
    table->memory[i] = NULL;

  return table;
}

void freeSymbolTable(SymbolTable *table) {
  if (table != NULL) {
    if (table->memory != NULL) {
      for (int32_t i = 0; i < table->size; i++) {
        if (table->memory[i] != NULL)
          freeNodeChain(table->memory[i]);
      }
      free(table->memory);
    }
    free(table);
  }
}

int32_t getSymbolTable(SymbolTable *table, char *symbol) {
  // if symbol is in table, return the memory address
  // else, return -1
  int32_t hashval = hash_function(symbol);
  if (hashval >= table->size) {
    perror("error in hash function : overflow");
    exit(EXIT_FAILURE);
  }
  Node *itr = table->memory[hashval];
  if (itr == NULL) {
    printf("symbol %s not found\n", symbol);
    return -1;
  }

  while (itr != NULL) {
    if (strcmp(itr->symbol_str, symbol) == 0)
      return itr->memory_location;

    itr = itr->next;
  }
  return -1;
}

void addHashTable(SymbolTable *table, char *symbol, int32_t memory_address) {
  Node *node = initNode(symbol, memory_address);
  int32_t hashval = hash_function(symbol);
  if (hashval >= table->size) {
    perror("error in hash function : overflow");
    exit(EXIT_FAILURE);
  }
  Node *itr = table->memory[hashval];
  if (itr == NULL) {
    table->memory[hashval] = node;
    return;
  }
  while (itr->next != NULL) {
    itr = getNextNode(itr);
  }
  setNextNode(itr, node);
}

typedef struct _memory_allocation_status {
  bool *memory;
  int32_t max_size;
  int32_t next_not_used;
} MemoryAllocationStatus;

MemoryAllocationStatus *initMemoryAllocationStatus(void) {
  MemoryAllocationStatus *memall =
      (MemoryAllocationStatus *)malloc(sizeof(MemoryAllocationStatus));
  memall->max_size = 24577;
  memall->memory = (bool *)malloc(sizeof(bool) * memall->max_size);
  for (int32_t i = 0; i < memall->max_size; i++)
    memall->memory[i] = false;
  memall->next_not_used = 16;
  return memall;
}

void freeMemoryAllocationStatus(MemoryAllocationStatus *memall) {
  if (memall != NULL) {
    if (memall->memory != NULL)
      free(memall->memory);
    free(memall);
  }
}

void manuallyAllocate(MemoryAllocationStatus *memall, int32_t memory_address) {
  memall->memory[memory_address] = true;
}

int32_t autoAllocate(MemoryAllocationStatus *memall) {
  int32_t i = memall->next_not_used;
  while (memall->memory[i]) {
    i++;
    if (i >= memall->max_size) {
      perror("memory full : 24576 registers are all used");
      exit(EXIT_FAILURE);
    }
  }
  memall->next_not_used = i + 1;
  memall->memory[i] = true;
  return i;
}

void allocatePredefinedSymbol(SymbolTable *table,
                              MemoryAllocationStatus *memall) {
  char sym[256];
  for (int32_t i = 0; i < 10; i++) {
    manuallyAllocate(memall, i);
    sym[0] = 'R';
    sym[1] = '0' + i;
    sym[2] = '\0';
    addHashTable(table, sym, i);
  }
  for (int32_t i = 0; i < 6; i++) {
    manuallyAllocate(memall, i + 10);
    sym[0] = 'R';
    sym[1] = '1';
    sym[2] = '0' + i;
    sym[3] = '\0';
    addHashTable(table, sym, i + 10);
  }

  manuallyAllocate(memall, 16384);
  addHashTable(table, "SCREEN", 16384);

  manuallyAllocate(memall, 24576);
  addHashTable(table, "KBD", 24576);

  manuallyAllocate(memall, 0);
  addHashTable(table, "SP", 0);

  manuallyAllocate(memall, 1);
  addHashTable(table, "LCL", 1);

  manuallyAllocate(memall, 2);
  addHashTable(table, "ARG", 2);

  manuallyAllocate(memall, 3);
  addHashTable(table, "THIS", 3);

  manuallyAllocate(memall, 4);
  addHashTable(table, "THAT", 4);
}

void processLabel(SourceCode *code, SymbolTable *table,
                  MemoryAllocationStatus *memall) {
  int32_t line_number = 0;
  for (int32_t i = 0; i < code->curr_line; i++) {
    if (code->memory[i][0] == '(') {
      int32_t len = strlen(code->memory[i]);
      if (code->memory[i][len - 1] == ')') {
        manuallyAllocate(memall, line_number);
        char *symbol = substring(code->memory[i], 1, len - 2);
        addHashTable(table, symbol, line_number);
        free(symbol);
      }
    } else
      line_number++;
  }
}

int32_t pow2(int32_t exponent) {
  // use bit-shifting
  if (exponent < 0)
    return -1;
  return 1 << exponent;
}

char *decimal_to_binary15(int32_t decimal) {
  char *binary15 = (char *)malloc(sizeof(char) * 16); // 15 + 1(null)
  binary15[15] = '\0';
  int32_t j;
  for (int32_t i = 0; i < 15; i++) {
    j = pow2(14 - i);
    if (decimal < j) {
      binary15[i] = '0';
    } else {
      decimal -= j;
      binary15[i] = '1';
    }
  }
  return binary15;
}

void traslate(char *line, SymbolTable *table, MemoryAllocationStatus *memall,
              FILE *fout) {
  if (line[0] == '@') { // A-instruction
    int32_t len = strlen(line);
    char *symbol = substring(line, 1, len - 1);
    int32_t memory_address;
    if (symbol[0] >= '0' && symbol[0] <= '9') // number
      memory_address = atoi(symbol);
    else // symbol in string
      memory_address = getSymbolTable(table, symbol);
    if (memory_address == -1) {
      memory_address = autoAllocate(memall);
      addHashTable(table, symbol, memory_address);
    }
    char *binary15 = decimal_to_binary15(memory_address);
    fprintf(fout, "0");
    fprintf(fout, "%s", binary15);
    fprintf(fout, "\n");
    free(symbol);
    free(binary15);
  } else { // C-instruction
    char *comp, *dest, *jump;
    char *ac, *d, *j;
    ac = (char *)malloc(sizeof(char) * 256);
    d = (char *)malloc(sizeof(char) * 256);
    j = (char *)malloc(sizeof(char) * 256);

    int32_t len = strlen(line);
    int32_t equal_pos = -1;
    int32_t semicolon_pos = -1;
    for (int32_t i = 0; i < len; i++) {
      if (line[i] == '=')
        equal_pos = i;
      else if (line[i] == ';')
        semicolon_pos = i;
    }

    // dest
    if (equal_pos == -1) { // no dest
      strcpy(d, "000");
    } else { // yes dest
      dest = substring(line, 0, equal_pos - 1);
      if (strcmp(dest, "M") == 0)
        strcpy(d, "001");
      else if (strcmp(dest, "D") == 0)
        strcpy(d, "010");
      else if (strcmp(dest, "DM") == 0)
        strcpy(d, "011");
      else if (strcmp(dest, "A") == 0)
        strcpy(d, "100");
      else if (strcmp(dest, "AM") == 0)
        strcpy(d, "101");
      else if (strcmp(dest, "AD") == 0)
        strcpy(d, "110");
      else if (strcmp(dest, "ADM") == 0)
        strcpy(d, "111");
      else {
        printf("error = %i %i\n", dest[0], dest[1]);
        perror("error in parsing dest");
        exit(EXIT_FAILURE);
      }
      free(dest);
    }

    // jump
    if (semicolon_pos == -1) { // no jump
      strcpy(j, "000");
    } else { // yes jump
      jump = substring(line, semicolon_pos + 1, len - 1);
      if (strcmp(jump, "JGT") == 0)
        strcpy(j, "001");
      else if (strcmp(jump, "JEQ") == 0)
        strcpy(j, "010");
      else if (strcmp(jump, "JGE") == 0)
        strcpy(j, "011");
      else if (strcmp(jump, "JLT") == 0)
        strcpy(j, "100");
      else if (strcmp(jump, "JNE") == 0)
        strcpy(j, "101");
      else if (strcmp(jump, "JLE") == 0)
        strcpy(j, "110");
      else if (strcmp(jump, "JMP") == 0)
        strcpy(j, "111");
      else {
        perror("error in parsing jump");
        exit(EXIT_FAILURE);
      }
      free(jump);
    }

    // comp
    if (equal_pos == -1 && semicolon_pos == -1)
      comp = substring(line, 0, len - 1);
    else if (equal_pos == -1 && semicolon_pos != -1)
      comp = substring(line, 0, semicolon_pos - 1);
    else if (equal_pos != -1 && semicolon_pos == 0 - 1)
      comp = substring(line, equal_pos + 1, len - 1);
    else
      comp = substring(line, equal_pos + 1, semicolon_pos - 1);

    if (strcmp(comp, "0") == 0)
      strcpy(ac, "0101010");
    else if (strcmp(comp, "1") == 0)
      strcpy(ac, "0111111");
    else if (strcmp(comp, "-1") == 0)
      strcpy(ac, "0111010");
    else if (strcmp(comp, "D") == 0)
      strcpy(ac, "0001100");
    else if (strcmp(comp, "A") == 0)
      strcpy(ac, "0110000");
    else if (strcmp(comp, "M") == 0)
      strcpy(ac, "1110000");
    else if (strcmp(comp, "!D") == 0)
      strcpy(ac, "0001101");
    else if (strcmp(comp, "!A") == 0)
      strcpy(ac, "0110001");
    else if (strcmp(comp, "!M") == 0)
      strcpy(ac, "1110001");
    else if (strcmp(comp, "-D") == 0)
      strcpy(ac, "0001111");
    else if (strcmp(comp, "-A") == 0)
      strcpy(ac, "0110011");
    else if (strcmp(comp, "-M") == 0)
      strcpy(ac, "1110011");
    else if (strcmp(comp, "D+1") == 0)
      strcpy(ac, "0011111");
    else if (strcmp(comp, "A+1") == 0)
      strcpy(ac, "0110111");
    else if (strcmp(comp, "M+1") == 0)
      strcpy(ac, "1110111");
    else if (strcmp(comp, "D-1") == 0)
      strcpy(ac, "0001110");
    else if (strcmp(comp, "A-1") == 0)
      strcpy(ac, "0110010");
    else if (strcmp(comp, "M-1") == 0)
      strcpy(ac, "1110010");
    else if (strcmp(comp, "D+A") == 0)
      strcpy(ac, "0000010");
    else if (strcmp(comp, "D+M") == 0)
      strcpy(ac, "1000010");
    else if (strcmp(comp, "D-A") == 0)
      strcpy(ac, "0010011");
    else if (strcmp(comp, "D-M") == 0)
      strcpy(ac, "1010011");
    else if (strcmp(comp, "A-D") == 0)
      strcpy(ac, "0000111");
    else if (strcmp(comp, "A-M") == 0)
      strcpy(ac, "1000111");
    else if (strcmp(comp, "D&A") == 0)
      strcpy(ac, "0000000");
    else if (strcmp(comp, "D&M") == 0)
      strcpy(ac, "1000000");
    else if (strcmp(comp, "D|A") == 0)
      strcpy(ac, "0010101");
    else if (strcmp(comp, "D|M") == 0)
      strcpy(ac, "1010101");
    else {
      perror("error in parsing comp");
      exit(EXIT_FAILURE);
    }
    free(comp);

    fprintf(fout, "111");
    fprintf(fout, "%s", ac);
    fprintf(fout, "%s", d);
    fprintf(fout, "%s", j);
    fprintf(fout, "\n");
  }
}

void assembler(SourceCode *code, SymbolTable *table,
               MemoryAllocationStatus *memall, FILE *fout) {
  for (int32_t i = 0; i < code->curr_line; i++) {
    if (code->memory[i][0] == '(')
      continue;
    traslate(code->memory[i], table, memall, fout);
  }
}

int main(void) {
  // opening input & output file
  FILE *fin = fopen("assembly.txt", "r");
  FILE *fout = fopen("machine.txt", "w");
  if (fin == NULL || fout == NULL) {
    perror("error loading file for read/write");
    exit(EXIT_FAILURE);
  }

  // Initialization
  SourceCode *code = initSourceCode();
  SymbolTable *table = initSymbolTable();
  MemoryAllocationStatus *memall = initMemoryAllocationStatus();

  // add predefined symbols
  allocatePredefinedSymbol(table, memall);

  // input source code
  inputSourceCode(code, fin);

  // process (LABEL)
  processLabel(code, table, memall);

  // process every thing - assemble
  assembler(code, table, memall, fout);

  // free
  freeSouceCode(code);
  freeSymbolTable(table);
  freeMemoryAllocationStatus(memall);

  // closing input & output file
  fclose(fin);
  fclose(fout);
}