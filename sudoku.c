// TODO: Allow for dynamically sized sudoku grids of n^2
// TODO: Allow command arguments to increase usability

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAD_SIZE 15
#define VALUES 9
#define SUB_SIZE 3

char grid[VALUES][VALUES]; // Don't need it to be int as no processing done
int possible[VALUES][VALUES][VALUES]; // Stores all possibilities (Space n^3)!
int possibleCount[VALUES][VALUES]; // Trades off space in favour of time
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

// Stores all columns as a bitmap of numbers
void get_columns_bitmap(int array[VALUES][VALUES]) {
  for (int col = 0; col < VALUES; col++) {
    for (int row = 0; row < VALUES; row++) {
      if (grid[row][col] == '-') {
        continue;
      }
      for (int val = 0; val < VALUES; val++) {
        if (grid[row][col] == valid[val+1]) {
          array[col][val] = 1;
        }
      }
    }
  }
}

// Stores all subGrids as bitmap before main loop
void get_subgrids_bitmap(int array[SUB_SIZE][SUB_SIZE][VALUES]) {
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
              array[row][col][val] = 1;
            }
          }
        }
      }
    }
  }
}

// Stores all the current row as a bitmap of numbers
void get_row_bitmap(int row, int array[VALUES]) {
  for (int col = 0; col < VALUES; col++) {
    if (grid[row][col] == '-') {
      continue;
    }
    for (int val = 0; val < VALUES; val++) {
      if (grid[row][col] == valid[val+1]) {
        array[val] = 1;
      }
    }
  }
}

// Initializes a bitmap to hold possible values
void pre_process(int* cellsLeft) {
  int allCols[VALUES][VALUES]; // Stores all columns as bitmap before main loop
  memset(allCols, 0, sizeof(allCols)); // Initialize before use
  get_columns_bitmap(allCols);

  int subGrids[SUB_SIZE][SUB_SIZE][VALUES];
  memset(subGrids, 0, sizeof(subGrids)); // Initialize before use
  get_subgrids_bitmap(subGrids);

  int curRow[VALUES]; // Stores row values as bitmap in the currentRow
  for (int row = 0; row < VALUES; row++) {
    memset(curRow, 0, sizeof(curRow)); // Initialize before (re)use
    get_row_bitmap(row, curRow);

    // Update the main bitmap to store possibilities per cell
    for (int col = 0; col < VALUES; col++) {
      for (int val = 0; val < VALUES; val++) {
        // Check if already full, is in the row, column or subGrid
        if (grid[row][col] == '-' && curRow[val] == 0 && allCols[col][val] == 0
        && subGrids[row / SUB_SIZE][col / SUB_SIZE][val] == 0) {
          if (possibleCount[row][col] == 0) {
            *cellsLeft += 1;
          }
          possible[row][col][val] = 1; // Mark as possible if none match
          possibleCount[row][col] += 1;
        }
      }
    }
  }
}

// Gets the next pair of row and column where there is only one possibility
int* find_next() {
  for (int row = 0; row < VALUES; row++) {
    for (int col = 0; col < VALUES; col++) {
      if (possibleCount[row][col] == 1) {
        int* pair = calloc(2, sizeof(int));
        pair[0] = row;
        pair[1] = col;
        return pair;
      }
    }
  }
  return NULL;
}

// Assumes there is only one value in the bitmap and retrieves it
int get_possible_value(int* pair) {
  for (int i = 0; i < VALUES; i++) {
    if (possible[pair[0]][pair[1]][i] == 1) {
      return i+1;
    }
  }
  return -1;
}

// Removes a value from the possibilities of all others in the same row
void remove_value_from_row(int* pair, int value) {
  for (int col = 0; col < VALUES; col++) {
    if (pair[1] == col) {
      continue;
    }
    // Only if the value is in its possibilities
    if (possible[pair[0]][col][value - 1] == 1) {
      possible[pair[0]][col][value - 1] = 0;
      possibleCount[pair[0]][col] -= 1;
    }
  }
}

// Removes a value from the possibilities of all others in the same column
void remove_value_from_column(int* pair, int value) {
  for (int row = 0; row < VALUES; row++) {
    if (pair[0] == row) {
      continue;
    }
    // Only if the value is in its possibilities
    if (possible[row][pair[1]][value - 1] == 1) {
      possible[row][pair[1]][value - 1] = 0;
      possibleCount[row][pair[1]] -= 1;
    }
  }
}

// Removes a value from the possibilities of all others in the same subGrid
void remove_value_from_subgrid(int* pair, int value) {
  int subRow = (pair[0] / SUB_SIZE) * SUB_SIZE;
  int subCol = (pair[1] / SUB_SIZE) * SUB_SIZE;
  for (int i = 0; i < VALUES; i++) {
    int inRow = i / SUB_SIZE;
    int inCol = i % SUB_SIZE;
    if (inCol + subCol == pair[0] && inRow + subRow == pair[1]) {
      continue;
    }
    if (possible[subRow + inRow][subCol + inCol][value - 1] == 1) {
      possible[subRow + inRow][subCol + inCol][value - 1] = 0;
      possibleCount[subRow + inRow][subCol + inCol] -= 1;
    }
  }
}

// Process the next iteration of the loop
int process_next(int* cellsLeft) {
  if (*cellsLeft == 0) {
    return 1;
  }
  int* pair = find_next(); // Find cell with only one left
  if (pair == NULL) { // If not found, report
    fprintf(stderr, "Didn't find next pair. Cells left: %i", *cellsLeft);
    return 1;
  }
  int value = get_possible_value(pair);
  if (value == -1) {
    fprintf(stderr, "Empty cell processed");
    return 1;
  }

  grid[pair[0]][pair[1]] = valid[value];
  possible[pair[0]][pair[1]][value - 1] = 0;
  possibleCount[pair[0]][pair[1]] -= 1;
  *cellsLeft -= 1;

  remove_value_from_row(pair, value);
  remove_value_from_column(pair, value);
  remove_value_from_subgrid(pair, value);

  // Show affected
  //printf("Added %c at (%i, %i)\n", valid[value], pair[0], pair[1]);
  free(pair);
  return 0;
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

// Prints a grid based output which varies depending on the mode passed to it
// {0 -> Char, 1 -> Array, 2 -> Int}
void print_grid(int mode) {
  printf("_________________________\n"); // Top
  for (int row = 0; row < VALUES; row++) {
    if (row % 3 == 0 && row != 0) {
      printf("-------------------------\n"); // Row sectioner
    }
    printf("| "); // Left
    for (int col = 0; col < VALUES; col++) {
      switch (mode) {
        case 0:
          printf("%c ", grid[row][col]);
          break;
        case 1:
          print_array(possible[row][col], VALUES, 1);
          break;
        case 2:
          printf("%i ", possibleCount[row][col]);
          break;
      }
      if ((col+1) % 3 == 0) {
        printf("| "); // Col sectioner and Right
      }
    }
    printf("\n");
  }
  printf("_________________________\n"); // Bottom
}

// Prints the grid's possible contents
void print_possible() {
  print_grid(1); // Print all possibilities in each cell
  print_grid(2); // Print the number of possibilities in each cell
}

// Processes the command line arguments provided
int process_arguments(int argc, char* argv[], int* flags,
                      char** in_csv, char** out_csv) {
  // Check if enough arguments provided
  if (argc < 2) {
    fprintf(stderr, "Not enough arguments\n");
    return 1;
  }
  // Go through all arguments (not first since that's the command)
  int inFound = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp("-l", argv[i]) == 0) { // List flag
      if (*flags % 2 == 0) { // Check if already specified
        *flags += 1;
      } else {
        fprintf(stderr, "Duplicate -l flag found\n");
        return 1;
      }
    } else if (strcmp("-o", argv[i]) == 0) { // Out flag + path
      if ((*flags / 2) % 2 == 0) { // Check if already specified
        *flags += 2;
        if (++i != argc && argv[i][0] != '-') { // Check next argument
          *out_csv = argv[i];
        } else {
          fprintf(stderr, "Output file path for -o not specified\n");
          return 1;
        }
      } else {
        fprintf(stderr, "Duplicate -o flag found\n");
        return 1;
      }
    } else { // Input path
      if (inFound == 0) {  // Check if already specified
        *in_csv = argv[i];
        inFound++;
      } else {
        fprintf(stderr, "Unexpected argument found\n");
        return 1;
      }
    }
  }
  return 0;
}

int main(int argc, char* argv[]) {
  // Read and store arguments in easy to use manner
  char* in_csv = NULL;
  char* out_csv = NULL;
  int flags = 0; // flag structure -> +1 for -l, +2 for -o
  if (process_arguments(argc, argv, &flags, &in_csv, &out_csv) != 0) {
    fprintf(stderr, "Usage: ./sudoku.out [-l] [-o <new_csv_path>] <csv_path>\n"
                    "Options:\n"
                    " -l lists steps in solving\n"
                    " -o exports solved grid to a new csv");
    return 1;
  }

  // Load up contents of the file into grid
  if (read_csv(in_csv) != 0) {
    fprintf(stderr, "File could not be read");
    return 1;
  }

  // Show the contents of the file
  printf("Input:\n");
  print_grid(0);

  // Calculate the result
  int cellsLeft = 0;
  pre_process(&cellsLeft);
  while (process_next(&cellsLeft) != 1) {}

  // Display solution
  printf("Output:\n");
  print_grid(0);
}
