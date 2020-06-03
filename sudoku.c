#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define ROW_BUF_SIZE 500
#define PAD_SIZE 15

char*** grid; // Don't need it to be int as no processing done
int*** possible; // Stores all possibilities (Space n^3)!
int** possibleCount; // Trades off space in favour of time
char** valid; // Stores the valid symbols in the sudoku grid
int VALUES, SUB_SIZE;


/* --- INITIALIZERS --- */

// Processes the command line arguments provided
int process_arguments(int argc, char* argv[],
                      int* verbosity, int* bulkFlag, int* timeFlag,
                      int* forceFlag, int* debugFlag,
                      char** inCsv, char** outCsv, char** missing) {
  // Check if enough arguments provided
  if (argc < 2) {
    fprintf(stderr, "Not enough arguments\n");
    return 1;
  }
  // Go through all arguments (not first since that's the command)
  int inFound = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp("-v", argv[i]) == 0) { // Verbose flag
      if (*verbosity < 1) { // Check if already specified
        *verbosity = 1;
      } else if (*verbosity == 1) {
        fprintf(stderr, "Duplicate -v flag found\n");
        return 1;
      }
    } else if (strcmp("-vv", argv[i]) == 0) { // Extra Verbose flag
      if (*verbosity < 2) { // Check if already specified (overwrites -v)
        *verbosity = 2;
      } else {
        fprintf(stderr, "Duplicate -vv flag found\n");
        return 1;
      }
    } else if (strcmp("-t", argv[i]) == 0) { // Time flag
      if (*timeFlag == 0) { // Check if already specified
        *timeFlag = 1;
      } else {
        fprintf(stderr, "Duplicate -t flag found\n");
        return 1;
      }
    } else if (strcmp("-b", argv[i]) == 0) { // Bulk flag
      if (*bulkFlag == 0) { // Check if already specified
        *bulkFlag = 1;
      } else {
        fprintf(stderr, "Duplicate -b flag found\n");
        return 1;
      }
    } else if (strcmp("-d", argv[i]) == 0) { // Debug flag
      if (*debugFlag == 0) { // Check if already specified
        *debugFlag = 1;
      } else {
        fprintf(stderr, "Duplicate -d flag found\n");
        return 1;
      }
    } else if (strcmp("-o", argv[i]) == 0) { // Out flag + path
      if (*outCsv == NULL) { // Check if already specified
        if (++i != argc && argv[i][0] != '-') { // Check next argument
          *outCsv = argv[i];
        } else {
          fprintf(stderr, "Output file path for -o not specified\n");
          return 1;
        }
      } else {
        fprintf(stderr, "Duplicate -o flag found\n");
        return 1;
      }
    } else if (strcmp("-m", argv[i]) == 0) { // Mising flag + arg
      if (*missing == NULL) { // Check if already specified
        if (++i != argc && argv[i][0] != '-') { // Check next argument
          *missing = argv[i];
        } else {
          fprintf(stderr, "Missing argument for -m not specified\n");
          return 1;
        }
      } else {
        fprintf(stderr, "Duplicate -m flag found\n");
        return 1;
      }
    } else if (strcmp("-f", argv[i]) == 0) { // Force flag [exclusive]
      if (*forceFlag == 0) { // Check if already specified
        *forceFlag = 1;
      } else {
        if (*forceFlag == 1) {
          fprintf(stderr, "Duplicate -f flag found\n");
        } else {
          fprintf(stderr, "Conflicting -f flag found\n");
        }
        return 1;
      }
    } else if (strcmp("-nf", argv[i]) == 0) { // Force flag [disable]
      if (*forceFlag == 0) { // Check if already specified
        *forceFlag = -1;
      } else {
        if (*forceFlag == -1) {
          fprintf(stderr, "Duplicate -nf flag found\n");
        } else {
          fprintf(stderr, "Conflicting -nf flag found\n");
        }
        return 1;
      }
    } else { // Input path
      if (inFound == 0) {  // Check if already specified
        *inCsv = argv[i];
        inFound++;
      } else {
        fprintf(stderr, "Unexpected argument found\n");
        return 1;
      }
    }
  }
  return 0;
}

// Allocates and initializes the global variables
int initialize_globals(char buffer[ROW_BUF_SIZE], int isBulk) {
  // Gets number of values
  if (!isBulk) {
    int values = 1;
    for (int i = 0; buffer[i]; i++) {
      values += (buffer[i] == ',');
    }
    VALUES = values;
    float root = sqrt((float) VALUES);
    if (floor(root) != root) { // If not a square number
      fprintf(stderr, "File doesn't have square number of values\n");
      return 1;
    }
    SUB_SIZE = (int) root;
  }

  // Calloc all dependant on values here
  grid = calloc(VALUES, sizeof(char*));
  for (int i = 0; i < VALUES; i++) {
    grid[i] = calloc(VALUES, sizeof(char*));
  }

  possible = calloc(VALUES, sizeof(int*));
  for (int i = 0; i < VALUES; i++) {
    possible[i] = calloc(VALUES, sizeof(int*));
    for (int j = 0; j < VALUES; j++) {
      possible[i][j] = calloc(VALUES, sizeof(int));
    }
  }

  possibleCount = calloc(VALUES, sizeof(int*));
  for (int i = 0; i < VALUES; i++) {
    possibleCount[i] = calloc(VALUES, sizeof(int));
  }

  valid = calloc(VALUES + 1, sizeof(char*));
  valid[0] = strdup("-");
  return 0;
}

// Frees the dynamically allocated global variables
void free_globals() {
  // No need to free cells, since they reference those in valid
  for (int i = 0; i < VALUES; i++) {
    free(grid[i]); // Free row
  }
  free(grid); // Free all

  for (int i = 0; i < VALUES; i++) {
    for (int j = 0; j < VALUES; j++) {
      free(possible[i][j]); // Free array
    }
    free(possible[i]); // Free row
  }
  free(possible); // Free all

  for (int i = 0; i < VALUES; i++) {
    free(possibleCount[i]); // Free row
  }
  free(possibleCount); // Free all

  for (int i = 0; i <= VALUES; i++) {
    free(valid[i]); // Free values
  }
  free(valid);
}

// Add the token to the valid array if not full
int addToValid(int* tokenCount, char* token) {
  int new = 1;
  for (int i = 0; i < *tokenCount; i++) {
    if (strcmp(valid[i], token) == 0) {
      new = 0;
    }
  }
  if (new == 1) {
    if (*tokenCount >= VALUES + 1) {
      fprintf(stderr, "Too many different valid tokens in grid\n");
      return 1;
    }
    valid[*tokenCount] = strdup(token);
    (*tokenCount)++;
  }
  return 0;
}

// Sanitize, and set the grid to reference a valid token
int process_token(int* tokenCount, char* token, int row, int col) {
  if (strchr(token, '\n') != NULL) { // Sanitize token
    printf("\\n converted at (%i, %i)\n", row, col);
    *strchr(token, '\n') = '\0';
  }
  if (strchr(token, '\r') != NULL) {
    *strchr(token, '\r') = '\0';
  }

  if (addToValid(tokenCount, token) != 0) { // If not in valid, add to it
    return 1;
  };

  for (int i = 0; i < *tokenCount; i++) { // Set reference to valid string
    if (strcmp(valid[i], token) == 0) {
      grid[row][col] = valid[i];
    }
  }
  return 0;
}

// Reads from a csv file and loads into the grid
int read_csv(const char* fileName, char* missing) {
  FILE* input = fopen(fileName, "r");
  if (input == NULL) {
    fprintf(stderr, "CSV file could not be opened for reading\n");
    return -1;
  }
  char buffer[ROW_BUF_SIZE];
  int row = 0;

  if (initialize_globals(fgets(buffer, ROW_BUF_SIZE, input), 0) != 0) {
    return -1;
  }
  int tokenCount = 1;

  do { // Read through file
    buffer[strcspn(buffer, "\n")] = 0;
    char* token = strtok(buffer, ","); // Split the line with ',' as delimiter
    int col = 0;
    while (token != NULL && col < VALUES) { // Store all values into the grid
      if (process_token(&tokenCount, token, row, col) != 0) {
        fclose(input);
        return 1;
      }
      token = strtok(NULL, ",");
      col++;
    }
    if (col < VALUES) { // If fewer values than expected detected
      fprintf(stderr,"Too few columns in row %i: Expected %i, not %i\n",
              row + 1, VALUES, col);
      fclose(input);
      return 1;
    }
    row++;
    memset(buffer, 0, sizeof(buffer));
  } while (fgets(buffer, ROW_BUF_SIZE, input) && row < VALUES);
  fclose(input);
  if (row < VALUES) { // If fewer rows than expected detected
    fprintf(stderr, "Too few rows in grid: Expected %i, not %i\n", VALUES, row);
    return 1;
  }

  if (tokenCount == VALUES) { // If missing one value
    if (missing != NULL) {
      valid[VALUES] = strdup(missing);
    } else {
      fprintf(stderr, "Missing one values so X substituted\n");
      valid[VALUES] = strdup("X");
    }
  } else if (tokenCount < VALUES) { // If not enough values
    fprintf(stderr, "Not enough values provided, expected: %i\n", VALUES);
    return 1;
  }
  return 0;
}


/* --- PRE PROCESS POSSIBILITIES --- */

// Stores all columns as a bitmap of numbers
void get_columns_bitmap(int array[VALUES][VALUES]) {
  for (int col = 0; col < VALUES; col++) {
    for (int row = 0; row < VALUES; row++) {
      if (strcmp(grid[row][col], "-") == 0) {
        continue;
      }
      for (int val = 0; val < VALUES; val++) {
        if (strcmp(grid[row][col], valid[val + 1]) == 0) {
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
          int curRow = row * SUB_SIZE + subRow;
          int curCol = col * SUB_SIZE + subCol;
          if (strcmp(grid[curRow][curCol], "-") == 0) {
            continue;
          } // If not empty, find and mark value
          for (int val = 0; val < VALUES; val++) {
            if (strcmp(grid[curRow][curCol], valid[val + 1]) == 0) {
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
    if (strcmp(grid[row][col], "-") == 0) {
      continue;
    }
    for (int val = 0; val < VALUES; val++) {
      if (strcmp(grid[row][col], valid[val + 1]) == 0) {
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
        if (strcmp(grid[row][col], "-") == 0
        && curRow[val] == 0 && allCols[col][val] == 0
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


/* --- VALUE OBTAINERS --- */

// Dynamically allocated and initializes a pair of values
int* make_pair(int row, int col) {
  int* pair = calloc(2, sizeof(int));
  pair[0] = row;
  pair[1] = col;
  return pair;
}

// Gets the next pair of row and column where there is only one possibility
int* find_single() {
  for (int row = 0; row < VALUES; row++) {
    for (int col = 0; col < VALUES; col++) {
      if (possibleCount[row][col] == 1) {
        return make_pair(row, col);
      }
    }
  }
  return NULL;
}

// Assumes there is only one value in the bitmap and retrieves it
int get_possible_value(int* pair) {
  for (int i = 0; i < VALUES; i++) {
    if (possible[pair[0]][pair[1]][i] == 1) {
      return i + 1;
    }
  }
  return -1;
}


/* --- UNIQUE FINDERS --- */

// Gets pair of row and col that is only place for a possible value in a row
// (968 found in 1 million)
int* unique_in_row(int* value) {
  int checkArray[VALUES][2]; // checkArray[X][0] = frequency, [1] = last found
  for (int row = 0; row < VALUES; row++) { // Check in each row
    memset(checkArray, 0, sizeof(checkArray)); // Reset array
    for (int col = 0; col < VALUES; col++) { // Update value in checkArray
      // Go through possibilities in cell and add to checkArray
      for (int val = 0; val < VALUES; val++) {
        if (possible[row][col][val] == 1) {
          checkArray[val][0] += 1; // Increase the frequency of the value
          checkArray[val][1] = col; // Set last found to this column
        }
      }
    }
    // Go through all values and see if frequency is 1, if so, output it
    for (int val = 0; val < VALUES; val++) {
      if (checkArray[val][0] == 1) {
        *value = val + 1;
        return make_pair(row, checkArray[val][1]);
      }
    }
  }
  return NULL;
}

// Gets pair of row and col that is only place for a possible value in a col
// (2 found in 1 million)
int* unique_in_col(int* value) {
  int checkArray[VALUES][2]; // checkArray[X][0] = frequency, [1] = last found
  for (int col = 0; col < VALUES; col++) { // Check in each subGrid
    memset(checkArray, 0, sizeof(checkArray)); // Reset array
    for (int row = 0; row < VALUES; row++) { // Go through cells in subGrid
      // Go through possibilities in cell and add to checkArray
      for (int val = 0; val < VALUES; val++) {
        if (possible[row][col][val] == 1) {
          checkArray[val][0] += 1; // Increase the frequency of the value
          checkArray[val][1] = row; // Set last found to this row
        }
      }
    }
    // Go through all values and see if frequency is 1, if so, output it
    for (int val = 0; val < VALUES; val++) {
      if (checkArray[val][0] == 1) {
        *value = val + 1;
        return make_pair(checkArray[val][1], col);
      }
    }
  }
  return NULL;
}

// Gets pair of row and col that is only place for a possible value in a subGrid
// Not sure if ever needed so couldn't be tested (0 found in 1 million)
int* unique_in_subgrid(int* value) {
  int checkArray[VALUES][2]; // checkArray[X][0] = frequency, [1] = last found
  for (int col = 0; col < SUB_SIZE; col++) { // Of main grid
    for (int row = 0; row < SUB_SIZE; row++) {
      memset(checkArray, 0, sizeof(checkArray)); // Reset array
      for (int subCol = 0; subCol < SUB_SIZE; subCol++) { // Of subGrid
        for (int subRow = 0; subRow < SUB_SIZE; subRow++) {
          int curRow = row * SUB_SIZE + subRow;
          int curCol = col * SUB_SIZE + subCol;
          // Go through possibilities in cell and add to checkArray
          for (int val = 0; val < VALUES; val++) {
            if (possible[curRow][curCol][val] == 1) {
              checkArray[val][0] += 1; // Increase the frequency of the val
              int curIndex = curCol * SUB_SIZE + curRow;
              checkArray[val][1] = curIndex; // Set last found to curIndex
            }
          }
        }
      }
      // Go through all values and see if frequency is 1, if so, output it
      for (int val = 0; val < VALUES; val++) {
        if (checkArray[val][0] == 1) {
          *value = val + 1;
          int pairRow = (row * SUB_SIZE) + (checkArray[val][1] / SUB_SIZE);
          int pairCol = (col * SUB_SIZE) + (checkArray[val][1] % SUB_SIZE);
          return make_pair(pairRow, pairCol);
        }
      }
    }
  }
  return NULL;
}

// Gets a pair of row and col which is the only possible location for a
//  certain value in a region using other three "unique_in" functions
int* unique_in_range(int mode, int* value) {
  int* output = NULL;
  if ((output = unique_in_row(value)) != NULL) {
    return output;
  }
  if ((output = unique_in_col(value)) != NULL) {
    return output;
  }
  if ((output = unique_in_subgrid(value)) != NULL) {
    return output;
  }
  return NULL;
}


/* --- REMOVERS --- */

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


/* --- CHECKERS [BRUTE FORCE] --- */

// Checks if the value is valid (no duplicates) in the given row
int valid_in_row(int value, int row) {
  for (int col = 0; col < VALUES; col++) {
    if (grid[row][col] == valid[value]) {
      return 1;
    }
  }
  return 0;
}

// Checks if the value is valid (no duplicates) in the given column
int valid_in_col(int value, int col) {
  for (int row = 0; row < VALUES; row++) {
    if (grid[row][col] == valid[value]) {
      return 1;
    }
  }
  return 0;
}

// Checks if the value is valid (no duplicates) in the given subgrid
int valid_in_subgrid(int value, int row, int col) {
  int subRow = SUB_SIZE * (row / SUB_SIZE);
  int subCol = SUB_SIZE * (col / SUB_SIZE);
  for (int pos = 0; pos < VALUES; pos++) {
    if (grid[subRow + (pos / SUB_SIZE)][subCol + (pos % SUB_SIZE)]
        == valid[value]) {
      return 1;
    }
  }
  return 0;
}

// Attempts to solve using a brute force mechanism
int brute_force_rec(int cur_row, int cur_col, int verbosity) {
  if (verbosity == 2) {
    printf("Brute Force(%i, %i)\n", cur_row, cur_col);
  }
  int col_set = 0;
  for (int row = cur_row; row < VALUES; row++) {
    for (int col = 0; col < VALUES; col++) {
      if (col_set == 0) {
        col = cur_col;
        col_set = 1;
      }
      if (grid[row][col] == valid[0]) {
        int value = 0;
        while (1) {
          value += 1;
          // Check if value is valid
          if (value > VALUES) {
            // Reset current value to '-' if all values failed (rollback)
            grid[row][col] = valid[0];
            return 1;
          }
          // If invalid for that position, then try with the next one
          if (valid_in_row(value, row) != 0 || valid_in_col(value, col) != 0 ||
              valid_in_subgrid(value, row, col) != 0) {
            continue;
          }
          if (verbosity == 2) {
            printf("Forced: %i, %i, %i\n", row, col, value);
          }
          grid[row][col] = valid[value];
          // Return if successful till the end, otherwise keep looping
          if (brute_force_rec(row + (col + 1) / VALUES,
                              (col + 1) % VALUES, verbosity) == 0) {
            return 0;
          }
        }
      }
      else if (verbosity == 2) {
        printf("Skipped: %i, %i\n", row, col);
      }
    }
  }
  return 0;
}

// Wrapper function to handle result of recursive brute force appropriately
void brute_force(int verbosity) {
  if (verbosity > 0) {
    fprintf(stderr, "Attempting brute force...\n");
  }
  if (brute_force_rec(0, 0, verbosity) == 1) {
    printf("Brute Force failed.\n");
  }
}

// Prints a grid cell's possible contents
void print_array(int* array, int len, int isPadded) {
  printf("{");
  int count = 0;
  for (int i = 0; i < len; i++) {
    if (array[i] == 1) {
      if (count == 0) {
        printf("%s", valid[i + 1]);
        count += 1;
      } else {
        printf(" %s", valid[i + 1]);
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

// Prints all possible values for each cell in the grid (FOR TESTING)
void print_possible() {
  printf("Possible:\n_________________________\n"); // Top
  for (int row = 0; row < VALUES; row++) {
    if (row % SUB_SIZE == 0 && row != 0) {
      printf("-------------------------\n"); // Row sectioner
    }
    printf("| "); // Left
    for (int col = 0; col < VALUES; col++) {
      print_array(possible[row][col], VALUES, 1);
      if ((col + 1) % SUB_SIZE == 0) {
        printf("| "); // Col sectioner and Right
      }
    }
    printf("\n");
  }
  printf("_________________________\n\n"); // Bottom
}


/* --- REDUNDANCY REMOVERS --- */

int focus_region(int region_bm[SUB_SIZE]) {
  int focus = -1;
  for (int i = 0; i < SUB_SIZE; i++) {
    if (region_bm[i] == 1) {
      if (focus != -1) { // Re-assignment means no focus
        return -1;
      }
      focus = i;
    }
  }
  return focus;
}

// Find value in only overlapping region and remove from rest of other region
void ommision() {
  printf("Using ommision...\n");
  // print_possible();
  int region_bm[SUB_SIZE];
  // ROW
  for (int row = 0; row < VALUES; row++) {
    printf("Row %i\n", row);
    for (int value = 0; value < VALUES; value++) {
      memset(region_bm, 0, sizeof(region_bm));
      for (int col = 0; col < VALUES; col++) {
        if (possible[row][col][value] == 1) {
          region_bm[col / SUB_SIZE] = 1;
        }
      }
      int focus = focus_region(region_bm); // Subgrid for the row
      if (focus != -1) { // Subgrid Ommision by Row
        int subRow = (row / SUB_SIZE) * SUB_SIZE;
        int subCol = focus * SUB_SIZE;
        for (int pos = 0; pos < VALUES; pos++) {
          // Not the same row but in the subGrid
          if (subRow + pos / 3 != row &&
              possible[subRow + pos / 3][subCol + pos % 3][value] == 1) {
            possible[subRow + pos / 3][subCol + pos % 3][value] = 0;
            possibleCount[subRow + pos / 3][subCol + pos % 3] -= 1;
            printf("Using ommision (row -> subgrid), with value %s, focus = %i: ", valid[value + 1], focus);
            printf("Eliminated: %s at (%i, %i)\n", valid[value + 1], subRow + pos / 3, subCol + pos % 3);
          }
        }
      }
    }
  }
  // print_possible();
  // COLUMN
  for (int col = 0; col < VALUES; col++) {
    printf("Column %i\n", col);
    for (int value = 0; value < VALUES; value++) {
      memset(region_bm, 0, sizeof(region_bm));
      for (int row = 0; row < VALUES; row++) {
        if (possible[row][col][value] == 1) {
          region_bm[row / SUB_SIZE] = 1;
        }
      }
      int focus = focus_region(region_bm); // Subgrid for the row
      if (focus != -1) { // Subgrid Ommision by Row
        int subRow = focus * SUB_SIZE;
        int subCol = (col / SUB_SIZE) * SUB_SIZE;
        for (int pos = 0; pos < VALUES; pos++) {
          // Not the same col but in the subGrid
          if (subCol + pos % 3 != col &&
              possible[subRow + pos / 3][subCol + pos % 3][value] == 1) {
            possible[subRow + pos / 3][subCol + pos % 3][value] = 0;
            possibleCount[subRow + pos / 3][subCol + pos % 3] -= 1;
            printf("Using ommision (col -> subgrid), with value %s, focus = %i: ", valid[value + 1], focus);
            printf("Eliminated: %s at (%i, %i)\n", valid[value + 1], subRow + pos / 3, subCol + pos % 3);
          }
        }
      }
    }
  }
  // print_possible();
  // SUBGRID
  int row_bm[SUB_SIZE];
  int col_bm[SUB_SIZE];
  for (int subRow = 0; subRow < SUB_SIZE; subRow++) {
    for (int subCol = 0; subCol < SUB_SIZE; subCol++) {
      printf("Subgrid (%i, %i)\n", subRow, subCol);
      for (int value = 1; value <= VALUES; value++) {
        memset(row_bm, 0, sizeof(row_bm));
        memset(col_bm, 0, sizeof(col_bm));
        for (int pos = 0; pos < VALUES; pos++) {
          if (possible[subRow * SUB_SIZE + (pos / SUB_SIZE)][subCol * SUB_SIZE + (pos % SUB_SIZE)][value]
              == 1) {
            row_bm[pos / SUB_SIZE] = 1;
            col_bm[pos % SUB_SIZE] = 1;
          }
        }
        int focus = focus_region(row_bm);
        if (focus != -1) { // Row Ommision by Subgrid
          for (int i = 0; i < VALUES; i++) {
            // Not the same subGrid but on the row
            if (i / 3 != subCol &&
                possible[subRow * SUB_SIZE + focus][i][value] == 1) {
              possible[subRow * SUB_SIZE + focus][i][value] = 0;
              possibleCount[subRow * SUB_SIZE + focus][i] -= 1;
              printf("Using ommision (subgrid -> row), with value %s, row = %i: ", valid[value + 1], focus);
              printf("Eliminated: %s at (%i, %i)\n", valid[value + 1], subRow * SUB_SIZE + focus, i);
            }
          }
        }
        focus = focus_region(col_bm);
        if (focus != -1) { // Col Ommision by Subgrid
          for (int i = 0; i < VALUES; i++) {
            // Not the same subGrid but on the col
            if (i / 3 != subRow &&
                possible[i][subCol * SUB_SIZE + focus][value] == 1) {
              possible[i][subCol * SUB_SIZE + focus][value] = 0;
              possibleCount[i][subCol * SUB_SIZE + focus] -= 1;
              printf("Using ommision (subgrid -> col), with value %s, col = %i: ", valid[value + 1], focus);
              printf("Eliminated: %s at (%i, %i)\n", valid[value + 1], i, subCol * SUB_SIZE + focus);
            }
          }
        }
      }
    }
  }
}

void naked_pairs() {
  // Loop through all regions and find 2 cells with only identical pair, remove from all other regions involved
}

void hidden_pairs() {
  // Loop through all regions and find 2 cells with pair that only appear there, remove other values from those cells
}

void eliminate_redundant() {
  printf("Removing redundant values...\n");
  ommision();
  // naked_pairs();
  // hidden_pairs();
}


/* --- PROCESS LOOP --- */

// Process the next iteration of the loop
int process_next(int* cellsLeft, int* stripped, int verbosity, int forceFlag) {
  if (*cellsLeft == 0) {
    return 1;
  }
  int value = -1;
  int* pair = find_single(); // Find cell with only one left
  if (pair == NULL) { // If not found, try other methods
    for (int mode = 0; mode < 3; mode++) { // Loop through row, col, subGrid
      if ((pair = unique_in_range(mode, &value)) != NULL) {
        break; // If found, stop checking
      }
    }
    if (pair == NULL) { // If there still is not pair found then try brute force
      if (*stripped == 0) {
        // If still no pair then remove redundant values, mark this, try again
        eliminate_redundant();
        *stripped = 1;
        return 0;
      } else {
        if (verbosity > 0 && forceFlag == -1) {
          fprintf(stderr, "Didn't find next pair. Cells left: %i\n", *cellsLeft);
        }
        if (forceFlag != -1) { // If brute force not disabled, use it to finish
          brute_force(verbosity);
        }
        return 1;
      }
    }
  }
  if (value == -1) { // Only for single, since unique in range sets appropriate value
    value = get_possible_value(pair);
  }
  if (value == -1) {
    fprintf(stderr, "Empty cell processed\n");
    return 1;
  }

  *stripped = 0;
  grid[pair[0]][pair[1]] = valid[value];
  for (int val = 0; val < VALUES; val++) {
    possible[pair[0]][pair[1]][val] = 0;
  }
  possibleCount[pair[0]][pair[1]] = 0;
  *cellsLeft -= 1;

  remove_value_from_row(pair, value);
  remove_value_from_column(pair, value);
  remove_value_from_subgrid(pair, value);

  // Show affected
  if (verbosity > 0) {
    printf("Added %s at (%i, %i)\n", valid[value], pair[0], pair[1]);
  }
  free(pair);
  return 0;
}


/* --- PRINTERS --- */

// PRINT_POSSIBLE

// Prints a line of given length with a given character
void print_line(char type, int length) {
  for (int i = 0; i < length; i++) {
    printf("%c", type);
  }
  printf("\n");
}

// Prints the current state of the grid in a formatted output
void print_grid() {
  const int LINE_LENGTH = 2 * (VALUES + SUB_SIZE) + 1;
  printf("\n"); // Top
  print_line('_', LINE_LENGTH);
  for (int row = 0; row < VALUES; row++) {
    if (row % SUB_SIZE == 0 && row != 0) {
      print_line('-', LINE_LENGTH); // Row sectioner
    }
    printf("| "); // Left
    for (int col = 0; col < VALUES; col++) {
      printf("%s ", grid[row][col]);
      if ((col + 1) % SUB_SIZE == 0) {
        printf("| "); // Col sectioner and Right
      }
    }
    printf("\n");
  }
  print_line('_', LINE_LENGTH);
  printf("\n"); // Bottom
}


/* --- EXPORTERS --- */

// Writes the contents of the grid to a file in csv format
void write_to_file(FILE* output) {
  char buffer[ROW_BUF_SIZE];
  for (int row = 0; row < VALUES; row++) {
    int bufPos = 0;
    memset(buffer, '\0', sizeof(buffer));
    for (int col = 0; col < VALUES; col++) {
      int len = strlen(grid[row][col]);
      strncpy(&buffer[bufPos], grid[row][col], len);
      bufPos += len;
      buffer[bufPos] = ',';
      bufPos += 1;
    }
    buffer[bufPos - 1] = '\0';
    fprintf(output, "%s\n", buffer);
  }
}

// Exports the solved grid as a csv file
int export_grid(char* outCsv) {
  // Append .csv file extension to the fileName provided if not already present
  char* outCsvFull = calloc(strlen(outCsv) + 4, sizeof(char));
  strcpy(outCsvFull, outCsv);
  if (strstr(outCsvFull, ".csv") !=
      &outCsvFull[strlen(outCsvFull) - 4]) {
      strcat(outCsvFull, ".csv");
  }

  // Create and open csv file to export to
  FILE* output = fopen(outCsvFull, "w+");
  free(outCsvFull);
  if (output == NULL) {
    fprintf(stderr, "Output file could not be created\n");
    return 1;
  }
  write_to_file(output);
  fclose(output);
  return 0;
}


// Reads, solves and checks all sudokus in a bulk sudoku file with the initial
// and solution grid stored as VALUES^2 character strings (so it is dependant on
// single character value tokens)
int read_bulk(const char* fileName, int forceFlag) {
  FILE* input = fopen(fileName, "r");
  if (input == NULL) {
    fprintf(stderr, "Bulk CSV file could not be opened for reading\n");
    return -1;
  }
  int solvedCount = 0;
  char buffer[ROW_BUF_SIZE];
  char* current = buffer;
  fgets(buffer, ROW_BUF_SIZE, input); // Skip first line
  while (fgets(buffer, ROW_BUF_SIZE, input)) {
    current = buffer;
    // Use buffer upto comma char to parse in question
    int len = strcspn(current, ",");
    // Check if len corresponds to square grid
    VALUES = (int) sqrt(len);
    float root = sqrt((float) VALUES);
    if (floor(root) != root) { // If not a square number
      fprintf(stderr, "File doesn't have square number of values\n");
      fclose(input);
      return 1;
    }
    SUB_SIZE = (int) root;
    if (initialize_globals(NULL, 1) != 0) { // Don't need to provide buffer
      return -1;
    }

    // Parse into grid (0 equivalent to '-')
    int tokenCount = 1;
    for (int curChar = 0; curChar < len; curChar++) { // All char in question
      // Get character, make it a token and process it
      char inChar = current[curChar];
      if (inChar == '0') { // Replace 0 with '-'
        inChar = '-';
      }
      char token[] = {inChar, '\0'}; // Make char into temp string

      if (process_token(&tokenCount, token,
          curChar / VALUES, curChar % VALUES) != 0) {
        fclose(input);
        return 1;
      }
    }

    if (tokenCount == VALUES) { // If missing one value
      valid[VALUES] = strdup("X");
    } else if (tokenCount < VALUES) { // If not enough values
      fprintf(stderr, "Not enough values provided, expected: %i\n", VALUES);
      return 1;
    }

    // Calculate the result
    if (forceFlag == 1) {
      brute_force(0);
    } else {
      int cellsLeft = 0;
      int stripped = 0;
      pre_process(&cellsLeft);
      while (process_next(&cellsLeft, &stripped, 0, forceFlag) != 1) {}
    }

    // Set current to be first char of solution
    current = &buffer[len + 1];
    int diff = strcspn(current, ",") - len; // counts potential \n
    if (diff < 0) { // If solution smaller
      fprintf(stderr, "Solution smaller than the question\n");
      fclose(input);
      return 1;
    }

    // Check if solution matches
    int matches = 1;
    char missing = 'X';
    for (int curChar = 0; curChar < len - diff; curChar++) { // Solution's chars
      char inChar = current[curChar];
      if (inChar == '0') { // Replace 0 with '-'
        inChar = '-';
      }

      // Check this token against one in grid (only need to compare first char)
      if (inChar != grid[curChar / VALUES][curChar % VALUES][0]) {
        if (grid[curChar / VALUES][curChar % VALUES][0] == 'X') {
          missing = inChar;
        } else {
          matches = 0; // Set to not matching
          break;
        }
      }
    }

    if (!matches) {
      fprintf(stderr, "Sudoku solution does not match expected answer\n");
      fclose(input);
      return 1;
    }

    free_globals(); // Ready for next sudoku

    solvedCount += 1;
    printf("Solved %i\n", solvedCount);
  }
  fclose(input);

  return 0;
}

// Adapter function to handle the reading of a file appropriate to its structure
int read_file(char* inCsv, char* missing, int isBulk, int forceFlag) {
  if (isBulk) {
    return read_bulk(inCsv, forceFlag);
  } else {
    return read_csv(inCsv, missing);
  }
}

int main(int argc, char* argv[]) {
  // Read and store arguments in easy to use manner
  clock_t time;
  char* inCsv = NULL;
  char* outCsv = NULL;
  char* missing = NULL;
  int verbosity = 0; // [0, 1, 2]
  int bulkFlag = 0; // [0, 1]
  int timeFlag = 0; // [0, 1]
  int forceFlag = 0; // [-1, 0, 1]
  int debugFlag = 0; // [0, 1]
  if (process_arguments(argc, argv, &verbosity, &bulkFlag, &timeFlag,
      &forceFlag, &debugFlag, &inCsv, &outCsv, &missing) != 0) {
    fprintf(stderr, "\nUsage: ./sudoku.out [[-v]|[-vv]] [-b] [-t] [[-f]|[-nf]]"
                    " [-d] [-o <new_csv_name>] [-m <missing_arg>] <csv_path>\n"
                    "\nOptions:\n"
                    " -v verbose output, lists steps in solving\n"
                    " -vv extra verbose output, lists brute force steps\n"
                    " -b solves a batch execution of sudokus (one per line)\n"
                    " -t shows the benchmarked time for reading and execution\n"
                    " -f uses brute force for solving immediately\n"
                    " -nf disables use of brute force\n"
                    " -d debug mode, shows all possibilities for each cell\n"
                    " -o exports solved grid to a new csv\n"
                    " -m provide missing argument if not in the grid\n");
    return 1;
  }

  // Start benchmarking
  if (timeFlag == 1) {
    time = clock();
  }

  // Load up contents of the file into grid
  switch (read_file(inCsv, missing, bulkFlag, forceFlag)) {
    case 1: // After Initialized globals
      free_globals();
    case -1:
      fprintf(stderr, "Error reading file\n");
      return 1;
  }

  // Bulk processing end
  if (bulkFlag == 1) {
    printf("Bulk file processed\n");
    if (timeFlag == 1) {
      time = clock() - time; // For benchmarking
      printf("Time elasped: %f\n", ((double) time) / CLOCKS_PER_SEC);
    }
    return 0;
  }

  // Show the contents of the file
  printf("Input:");
  print_grid();

  // Calculate the result
  if (forceFlag == 1) {
    brute_force(verbosity);
  } else {
    int cellsLeft = 0;
    int stripped = 0;
    pre_process(&cellsLeft);
    while (process_next(&cellsLeft, &stripped, verbosity, forceFlag) != 1) {}
  }

  // Display solution
  printf("\nOutput:");
  print_grid();
  if (debugFlag == 1) {
    print_possible();
  }
  if (outCsv != NULL) { // If output set, export
    if (export_grid(outCsv) != 0) {
      free_globals();
      return 1;
    }
  }
  free_globals();

  // End benchmarking
  if (timeFlag == 1) {
    time = clock() - time; // For benchmarking
    printf("Time elasped: %f\n", ((double) time) / CLOCKS_PER_SEC);
  }
}
