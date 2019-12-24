#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAD_SIZE 15
// TODO: Update later to be dynamic
#define VALUES 9
#define SUB_SIZE 3

char grid[VALUES][VALUES]; // Don't need it to be int as no processing done
int possible[VALUES][VALUES][VALUES]; // Stores all possibilities (Space n^3)!
char valid[] = {'-', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

// Gets the value from a token
char get_token_value(char* token) {
  for (int cur = 0; token[cur] != '\0'; cur++) {
    if (token[cur] == ' ' || token[cur] == '\n') { // Ignore whitespace
      continue;
    }
    for (int i = 0; i < 11; i++) {
      if (token[cur] == valid[i]) { // If the token is valid, return it
        return token[cur];
      }
    }
    return '-'; // If the token is not vali, return '-'
  }
  return '-'; // If the token is just whitespace, return '-'
}

// Reads from a csv file and loads into the grid
int read_csv(const char* fileName) {
  FILE* input = fopen(fileName, "r");
  char buffer[100];
  int row = 0;
  while (fgets(buffer, 100, input) && row < VALUES) { // Read through file
    buffer[strcspn(buffer, "\n")] = 0;
    char* token = strtok(buffer, ","); // Split the line with ',' as delimiter
    int col = 0;
    while (token != NULL && col < VALUES) { // Store all values into the grid
      grid[row][col] = get_token_value(token);
      token = strtok(NULL, ",");
      col++;
    }
    if (col < VALUES) { // If fewer values than expected detected
      fprintf(stderr,
        "Too few columns in row %i: Expected %i, not %i", row + 1, VALUES, col);
      fclose(input);
      return 1;
    }
    row++;
  }
  fclose(input);
  if (row < VALUES) { // If fewer rows than expected detected
    fprintf(stderr, "Too few rows in grid: Expected %i, not %i.", VALUES, row);
    return 1;
  }
  return 0;
}

// Initializes a bitmap to hold possible values
void pre_process() {
  int allCols[VALUES][VALUES]; // Stores all columns as bitmap before main loop
  memset(allCols, 0, sizeof(allCols)); // Initialize before use

  // Update the bitmap of numbers in all columns, one at a time
  for (int col = 0; col < VALUES; col++) {
    for (int row = 0; row < VALUES; row++) {
      if (grid[row][col] == '-') {
        continue;
      }
      for (int val = 0; val < VALUES; val++) {
        if (grid[row][col] == valid[val+1]) {
          allCols[col][val] = 1;
        }
      }
    }
  }

  // Stores all subGrids as bitmap before main loop
  int subGrids[SUB_SIZE][SUB_SIZE][VALUES];
  memset(subGrids, 0, sizeof(subGrids)); // Initialize before use

  // Update the bitmap of numbers in all subGrids, one at a time
  for (int col = 0; col < SUB_SIZE; col++) { // Of main grid
    for (int row = 0; row < SUB_SIZE; row++) {
      for (int subCol = 0; subCol < SUB_SIZE; subCol++) { // Of subGrid
        for (int subRow = 0; subRow < SUB_SIZE; subRow++) {
          if (grid[row*SUB_SIZE + subRow][col*SUB_SIZE + subCol] == '-') {
            continue;
          } // If not empty, find and mark value
          for (int val = 0; val < VALUES; val++) {
            if (grid[row*SUB_SIZE + subRow][col*SUB_SIZE + subCol]
              == valid[val+1]) {
              subGrids[row][col][val] = 1;
            }
          }
        }
      }
    }
  }

  int curRow[VALUES]; // Stores row values as bitmap in the currentRow
  for (int row = 0; row < VALUES; row++) {
    memset(curRow, 0, sizeof(curRow)); // Initialize before (re)use

    // Update the bitmap of numbers in the current row
    for (int col = 0; col < VALUES; col++) {
      if (grid[row][col] == '-') {
        continue;
      }
      for (int val = 0; val < VALUES; val++) {
        if (grid[row][col] == valid[val+1]) {
          curRow[val] = 1;
        }
      }
    }

    // Update the main bitmap to store possibilities per cell
    for (int col = 0; col < VALUES; col++) {
      for (int val = 0; val < VALUES; val++) {
        // Check if already full, is in the row, column or subGrid
        if (grid[row][col] == '-' && curRow[val] == 0 && allCols[col][val] == 0
        && subGrids[row/SUB_SIZE][col/SUB_SIZE][val] == 0) {
          possible[row][col][val] = 1; // Mark as possible if none match
        }
      }
    }
  }
}

// Prints the grid's current contents
void print_grid() {
  printf("_________________________\n"); // Top
  for (int row = 0; row < VALUES; row++) {
    if (row % 3 == 0 && row != 0) {
      printf("-------------------------\n"); // Row sectioner
    }
    printf("| "); // Left
    for (int col = 0; col < VALUES; col++) {
      printf("%c ", grid[row][col]);
      if ((col+1) % 3 == 0) {
        printf("| "); // Col sectioner and Right
      }
    }
    printf("\n");
  }
  printf("_________________________\n"); // Bottom
}

// Prints a grid cell's possible contents
void print_array(int* array, int len, int isPadded) {
  printf("{");
  int count = 0;
  for (int i = 0; i < len; i++) {
    if (array[i] == 1) {
      if (count == 0) {
        printf("%i", i+1);
        count += 1;
      } else {
        printf(" %i", i+1);
        count += 2;
      }
    }
  }
  printf("}");
  // Prints appropriate padding for formatted output
  if (isPadded) {
    count += 2;
    for (; count < PAD_SIZE; count++) {
      printf(" ");
    }
  }
}

// Prints the grid's possible contents
void print_possible() {
  printf("\n+_______________________+\n"); // Top
  for (int row = 0; row < VALUES; row++) {
    if (row % 3 == 0 && row != 0) {
      printf("-------------------------\n"); // Row sectioner
    }
    printf("| "); // Left
    for (int col = 0; col < VALUES; col++) {
      print_array(possible[row][col], VALUES, 1);
      if ((col+1) % 3 == 0) {
        printf("| "); // Col sectioner and Right
      }
    }
    printf("\n");
  }
  printf("+_______________________+\n"); // Bottom
}

int main() {
  // Load up contents of the file into grid
  if (read_csv("csv/easy.csv") != 0) {
    return 1;
  }
  // Show the contents of the file
  printf("Input:\n");
  print_grid();
  // Calculate solution
  pre_process();
  print_possible();
  // Display solution
  // printf("Output:\n");
  // print_grid();
}
