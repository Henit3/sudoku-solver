#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define ROW_BUF_SIZE 500
#define PAD_SIZE 15

#define PROCESS "\x1B[34m> \x1B[0m"
#define FOUND   "\x1B[33m? \x1B[0m"
#define REMOVE  "\x1B[31m- \x1B[0m"
#define ADD     "\x1B[32m+ \x1B[0m"

char*** grid; // Don't need it to be int as no processing done
int*** possible; // Stores all possibilities (Space n^3)!
int** possibleCount; // Trades off space in favour of time
char** valid; // Stores the valid symbols in the sudoku grid
int VALUES, SUB_SIZE;
char* MODE_LABELS[] = {"rows", "cols", "boxes"};


/* --- INITIALIZERS --- */

// Processes the command line arguments provided
int process_arguments(int argc, char* argv[], int* verbosity, int* bulkFlag,
    int* timeFlag, int* forceFlag, int* debugFlag,
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

// Stores all boxes as bitmap before main loop
void get_boxes_bitmap(int array[SUB_SIZE][SUB_SIZE][VALUES]) {
  for (int col = 0; col < SUB_SIZE; col++) { // Of main grid
    for (int row = 0; row < SUB_SIZE; row++) {
      for (int subCol = 0; subCol < SUB_SIZE; subCol++) { // Of box
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

  int boxes[SUB_SIZE][SUB_SIZE][VALUES];
  memset(boxes, 0, sizeof(boxes)); // Initialize before use
  get_boxes_bitmap(boxes);

  int curRow[VALUES]; // Stores row values as bitmap in the currentRow
  for (int row = 0; row < VALUES; row++) {
    memset(curRow, 0, sizeof(curRow)); // Initialize before (re)use
    get_row_bitmap(row, curRow);

    // Update the main bitmap to store possibilities per cell
    for (int col = 0; col < VALUES; col++) {
      for (int val = 0; val < VALUES; val++) {
        // Check if already full, is in the row, column or box
        if (strcmp(grid[row][col], "-") == 0
        && curRow[val] == 0 && allCols[col][val] == 0
        && boxes[row / SUB_SIZE][col / SUB_SIZE][val] == 0) {
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
// Returns value compatible with possible[] (0-indexed)
int get_possible_value(int* pair) {
  for (int i = 0; i < VALUES; i++) {
    if (possible[pair[0]][pair[1]][i] == 1) {
      return i;
    }
  }
  return -1;
}


/* --- UNIQUE FINDERS --- */
// Return value compatible with possible[] (0-indexed)

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
        *value = val;
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
  for (int col = 0; col < VALUES; col++) { // Check in each box
    memset(checkArray, 0, sizeof(checkArray)); // Reset array
    for (int row = 0; row < VALUES; row++) { // Go through cells in box
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
        *value = val;
        return make_pair(checkArray[val][1], col);
      }
    }
  }
  return NULL;
}

// Gets pair of row and col that is only place for a possible value in a box
// (Less than 1 found in 1 million)
int* unique_in_box(int* value) {
  int checkArray[VALUES][2]; // checkArray[X][0] = frequency, [1] = last found
  for (int col = 0; col < SUB_SIZE; col++) { // Of main grid
    for (int row = 0; row < SUB_SIZE; row++) {
      memset(checkArray, 0, sizeof(checkArray)); // Reset array
      for (int subCol = 0; subCol < SUB_SIZE; subCol++) { // Of box
        for (int subRow = 0; subRow < SUB_SIZE; subRow++) {
          int curRow = row * SUB_SIZE + subRow;
          int curCol = col * SUB_SIZE + subCol;
          // Go through possibilities in cell and add to checkArray
          for (int val = 0; val < VALUES; val++) {
            if (possible[curRow][curCol][val] == 1) {
              checkArray[val][0] += 1; // Increase the frequency of the val
              int curIndex = curRow * SUB_SIZE + curCol;
              checkArray[val][1] = curIndex; // Set last found to curIndex
            }
          }
        }
      }
      // Go through all values and see if frequency is 1, if so, output it
      for (int val = 0; val < VALUES; val++) {
        if (checkArray[val][0] == 1) {
          *value = val;
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
//  certain value in a house using other three "unique_in" functions
int* unique_in_range(int mode, int* value, int verbosity) {
  int* output = NULL;
  if ((output = unique_in_row(value)) != NULL) {
    if (verbosity == 1) {
      printf(ADD "Added %s at (%i, %i) as unique in row\n",
        valid[*value + 1], output[0], output[1]);
    }
    return output;
  }
  if ((output = unique_in_col(value)) != NULL) {
    if (verbosity == 1) {
      printf(ADD "Added %s at (%i, %i) as unique in column\n",
        valid[*value + 1], output[0], output[1]);
    }
    return output;
  }
  if ((output = unique_in_box(value)) != NULL) {
    if (verbosity == 1) {
      printf(ADD "Added %s at (%i, %i) as unique in box\n",
        valid[*value + 1], output[0], output[1]);
    }
    return output;
  }
  return NULL;
}


/* --- REMOVERS --- */
// Ignore values in the mask that have been set

// Removes a value from the possibilities of all others in the same row
int remove_value_from_row(int row, int value, int mask[VALUES], int print) {
  int output = 0;
  for (int col = 0; col < VALUES; col++) {
    if (mask[col] == 1) {
      continue;
    }
    // Only if the value is in its possibilities
    if (possible[row][col][value] == 1) {
      output = 1;
      possible[row][col][value] = 0;
      possibleCount[row][col] -= 1;
      if (print > 0) {
        printf(REMOVE "Eliminated: %s at (%i, %i)\n",
          valid[value + 1], row, col);
      }
    }
  }
  return output;
}

// Removes a value from the possibilities of all others in the same column
int remove_value_from_column(int col, int value, int mask[VALUES], int print) {
  int output = 0;
  for (int row = 0; row < VALUES; row++) {
    if (mask[row] == 1) {
      continue;
    }
    // Only if the value is in its possibilities
    if (possible[row][col][value] == 1) {
      output = 1;
      possible[row][col][value] = 0;
      possibleCount[row][col] -= 1;
      if (print > 0) {
        printf(REMOVE "Eliminated: %s at (%i, %i)\n",
          valid[value + 1], row, col);
      }
    }
  }
  return output;
}

// Removes a value from the possibilities of all others in the same box
int remove_value_from_box(int row, int col, int value,
    int mask[VALUES], int print) {
  int output = 0;
  int subRow = (row / SUB_SIZE) * SUB_SIZE;
  int subCol = (col / SUB_SIZE) * SUB_SIZE;
  for (int i = 0; i < VALUES; i++) {
    int inRow = i / SUB_SIZE;
    int inCol = i % SUB_SIZE;
    if (mask[i] == 1 || (inCol + subCol == col && inRow + subRow == row)) {
      continue;
    }
    if (possible[subRow + inRow][subCol + inCol][value] == 1) {
      output = 1;
      possible[subRow + inRow][subCol + inCol][value] = 0;
      possibleCount[subRow + inRow][subCol + inCol] -= 1;
      if (print > 0) {
        printf(REMOVE "Eliminated: %s at (%i, %i)\n",
          valid[value + 1], subRow + inRow, subCol + inCol);
      }
    }
  }
  return output;
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

// Checks if the value is valid (no duplicates) in the given box
int valid_in_box(int value, int row, int col) {
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
int brute_force_rec(int curRow, int curCol, int verbosity) {
  if (verbosity == 2) {
    printf(FOUND "Brute Force(%i, %i)\n", curRow, curCol);
  }
  int colSet = 0;
  for (int row = curRow; row < VALUES; row++) {
    for (int col = 0; col < VALUES; col++) {
      if (colSet == 0) {
        col = curCol;
        colSet = 1;
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
              valid_in_box(value, row, col) != 0) {
            continue;
          }
          if (verbosity == 2) {
            printf(ADD "Forced: %i, %i, %i\n", row, col, value);
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
        printf("\x1B[36m' \x1B[0mSkipped: %i, %i\n", row, col);
      }
    }
  }
  return 0;
}

// Wrapper function to handle result of recursive brute force appropriately
void brute_force(int verbosity) {
  if (verbosity > 0) {
    printf(PROCESS "Attempting brute force...\n");
  }
  if (brute_force_rec(0, 0, verbosity) == 1) {
    printf("Brute Force failed.\n");
  }
}


/* --- REMOVEUNDANCY REMOVERS --- */

// Returns the maximum of two integers
int max(int a, int b) {
  if (a >= b) {
    return a;
  } else {
    return b;
  }
}

// Returns overlapping house where possible locations of a value lie in
int focus_house(int houseBM[SUB_SIZE]) {
  int focus = -1;
  for (int i = 0; i < SUB_SIZE; i++) {
    if (houseBM[i] == 1) {
      if (focus != -1) { // Re-assignment means no focus
        return -1;
      }
      focus = i;
    }
  }
  return focus;
}

// Find value in only overlapping house and remove from rest of other houses
// Aggregation of pointing groups and box line reduction techniques
int intersection_removal(int verbosity) {
  int output = 0;
  if (verbosity > 0) {
    printf(PROCESS "Using pointing groups...\n");
  }
  // SUBGRID
  int rowBM[SUB_SIZE];
  int colBM[SUB_SIZE];
  for (int subRow = 0; subRow < SUB_SIZE; subRow++) {
    for (int subCol = 0; subCol < SUB_SIZE; subCol++) {
      for (int value = 1; value <= VALUES; value++) {
        memset(rowBM, 0, sizeof(rowBM));
        memset(colBM, 0, sizeof(colBM));
        for (int pos = 0; pos < VALUES; pos++) {
          int curRow = subRow * SUB_SIZE + (pos / SUB_SIZE);
          int curCol = subCol * SUB_SIZE + (pos % SUB_SIZE);
          if (possible[curRow][curCol][value] == 1) {
            rowBM[pos / SUB_SIZE] = 1;
            colBM[pos % SUB_SIZE] = 1;
          }
        }
        int focus = focus_house(rowBM);
        if (focus != -1) { // Row Ommision by Box (specialize REMOVERS)
          int printed = 0;
          int curRow = subRow * SUB_SIZE + focus;
          for (int i = 0; i < VALUES; i++) {
            // Not the same box but on the row
            if (i / SUB_SIZE != subCol && possible[curRow][i][value] == 1) {
              output = 1;
              possible[curRow][i][value] = 0;
              possibleCount[curRow][i] -= 1;
              if (verbosity > 0) {
                if (printed == 0) {
                  printf(FOUND "Using pointing groups on %s to remove from"
                    " the box's %ith row:\n", valid[value + 1], focus);
                  printed = 1;
                }
                printf(REMOVE "Eliminated: %s at (%i, %i)\n",
                  valid[value + 1], curRow, i);
              }
            }
          }
        }
        focus = focus_house(colBM);
        if (focus != -1) { // Col Ommision by Box (specialize REMOVERS)
          int printed = 0;
          int curCol = subCol * SUB_SIZE + focus;
          for (int i = 0; i < VALUES; i++) {
            // Not the same box but on the col
            if (i / SUB_SIZE != subRow && possible[i][curCol][value] == 1) {
              output = 1;
              possible[i][curCol][value] = 0;
              possibleCount[i][curCol] -= 1;
              if (verbosity > 0) {
                if (printed == 0) {
                  printf(FOUND "Using pointing groups on %s to remove from"
                    " the box's %ith column:\n", valid[value + 1], focus);
                  printed = 1;
                }
                printf(REMOVE "Eliminated: %s at (%i, %i)\n",
                  valid[value + 1], i, curCol);
              }
            }
          }
        }
      }
    }
  }
  if (verbosity > 0) {
    printf(PROCESS "Using box line reduction...\n");
  }
  int houseBM[SUB_SIZE];
  // ROW
  for (int row = 0; row < VALUES; row++) {
    for (int value = 0; value < VALUES; value++) {
      memset(houseBM, 0, sizeof(houseBM));
      for (int col = 0; col < VALUES; col++) {
        if (possible[row][col][value] == 1) {
          houseBM[col / SUB_SIZE] = 1;
        }
      }
      int focus = focus_house(houseBM); // Box for the row
      if (focus != -1) { // Box Ommision by Row (specialize REMOVERS)
        int printed = 0;
        int subRow = (row / SUB_SIZE) * SUB_SIZE;
        int subCol = focus * SUB_SIZE;
        for (int pos = 0; pos < VALUES; pos++) {
          // Not the same row but in the box
          int curRow = subRow + pos / SUB_SIZE;
          int curCol = subCol + pos % SUB_SIZE;
          if (curRow != row && possible[curRow][curCol][value] == 1) {
            output = 1;
            possible[curRow][curCol][value] = 0;
            possibleCount[curRow][curCol] -= 1;
            if (verbosity > 0) {
              if (printed == 0) {
                printf(FOUND "Using box line reduction to remove %s from the"
                  " box except its %ith row:\n", valid[value + 1], focus);
                printed = 1;
              }
              printf(REMOVE "Eliminated: %s at (%i, %i)\n",
                valid[value + 1], curRow, curCol);
            }
          }
        }
      }
    }
  }
  // COLUMN
  for (int col = 0; col < VALUES; col++) {
    for (int value = 0; value < VALUES; value++) {
      memset(houseBM, 0, sizeof(houseBM));
      for (int row = 0; row < VALUES; row++) {
        if (possible[row][col][value] == 1) {
          houseBM[row / SUB_SIZE] = 1;
        }
      }
      int focus = focus_house(houseBM); // Box for the row
      if (focus != -1) { // Box Ommision by Row (specialize REMOVERS)
        int printed = 0;
        int subRow = focus * SUB_SIZE;
        int subCol = (col / SUB_SIZE) * SUB_SIZE;
        for (int pos = 0; pos < VALUES; pos++) {
          // Not the same col but in the box
          int curRow = subRow + pos / SUB_SIZE;
          int curCol = subCol + pos % SUB_SIZE;
          if (curCol != col && possible[curRow][curCol][value] == 1) {
            output = 1;
            possible[curRow][curCol][value] = 0;
            possibleCount[curRow][curCol] -= 1;
            if (verbosity > 0) {
              if (printed == 0) {
                printf(FOUND "Using box line reduction to remove %s from the"
                  " box except its %ith column:\n", valid[value + 1], focus);
                printed = 1;
              }
              printf(REMOVE "Eliminated: %s at (%i, %i)\n",
                valid[value + 1], curRow, curCol);
            }
          }
        }
      }
    }
  }
  return output;
}

// Removes values according to the contents of a known naked group
int remove_naked_group(int* rows, int* cols, int* shared, int cells,
                      int mode, int verbosity) {
  int output = 0;
  if (verbosity > 0) {
    printf(FOUND "Found naked group (");
    for (int i = 0; i < cells - 1; i++) {
      printf("%s, ", valid[shared[i] + 1]);
    }
    printf("%s) using %s at ", valid[shared[cells - 1] + 1], MODE_LABELS[mode]);
    for (int i = 0; i < cells; i++) {
      printf("(%i, %i) ", rows[i], cols[i]);
    }
    printf("\n");
  }

  int mask[VALUES];
  memset(mask, 0, sizeof(mask));
  switch (mode) {
    case 0: { // ROW
      // Apply mask to all columns
      for (int i = 0; i < cells; i++) {
        mask[cols[i]] = 1;
      }
      for (int i = 0; i < cells; i++) {
        output = max(output,
          remove_value_from_row(rows[i], shared[i], mask, verbosity));
      }
      break;
    }
    case 1: { // COL
      // Apply mask to all rows
      for (int i = 0; i < cells; i++) {
        mask[rows[i]] = 1;
      }
      for (int i = 0; i < cells; i++) {
        output = max(output,
          remove_value_from_column(cols[i], shared[i], mask, verbosity));
      }
      break;
    }
    case 2: { // SUBGRID (same one)
      // Apply mask to all positions
      for (int i = 0; i < cells; i++) {
        mask[SUB_SIZE * (rows[i] % SUB_SIZE) + cols[i] % SUB_SIZE] = 1;
      }
      for (int i = 0; i < cells; i++) {
        output = max(output,
          remove_value_from_box(rows[i], cols[i], shared[i], mask, verbosity));
      }
      break;
    }
    default: {
      fprintf(stderr, "Invalid mode for removing naked group.");
      exit(1);
    }
  }
  return output;
}

// Removes values according to the contents of a known hidden group
int remove_hidden_group(int* rows, int* cols, int* shared, int cells,
                      int mode, int verbosity) {
  int output = 0;
  if (verbosity > 0) {
    printf(FOUND "Found hidden group (");
    for (int i = 0; i < cells - 1; i++) {
      printf("%s, ", valid[shared[i] + 1]);
    }
    printf("%s) using %s at ", valid[shared[cells - 1] + 1], MODE_LABELS[mode]);
    for (int i = 0; i < cells; i++) {
      printf("(%i, %i) ", rows[i], cols[i]);
    }
    printf("\n");
  }

  // Use shared as mask
  for (int pos = 0; pos < cells; pos++) {
    for (int value = 0; value < VALUES; value++) {
      // Skip values in shared (use as mask)
      int skip = 0;
      for (int i = 0; i < cells; i++) {
        if (value == shared[i]) {
          skip = 1;
          break;
        }
      }
      if (skip == 1) {
        continue;
      }
      // Remove possibility from cells
      if (possible[rows[pos]][cols[pos]][value] == 1) {
        output = 1;
        possible[rows[pos]][cols[pos]][value] = 0;
        possibleCount[rows[pos]][cols[pos]] -= 1;
        if (verbosity > 0) {
          printf(REMOVE "Eliminated: %s at (%i, %i)\n",
            valid[value + 1], rows[pos], cols[pos]);
        }
      }
    }
  }
  return output;
}

// Evaluates a naked group by checking validity and using to remove redundancy
int eval_naked_group(int* candidates, int size, int pos,
    int mode, int verbosity) {
  // Loop through candidates to obtain shared values
  int preShared[VALUES];
  memset(preShared, 0, sizeof(preShared));
  switch (mode) {
    case 0: { // ROW
      for (int i = 0; i < size; i++) {
        for (int value = 0; value < VALUES; value++) {
          if (possible[pos][candidates[i]][value] == 1) {
            preShared[value] += 1;
          }
        }
      }
      break;
    }
    case 1: { // COL
      for (int i = 0; i < size; i++) {
        for (int value = 0; value < VALUES; value++) {
          if (possible[candidates[i]][pos][value] == 1) {
            preShared[value] += 1;
          }
        }
      }
      break;
    }
    case 2: { // SUBGRID
      int subRow = (pos / SUB_SIZE) * SUB_SIZE;
      int subCol = (pos % SUB_SIZE) * SUB_SIZE;
      for (int i = 0; i < size; i++) {
        int curRow = subRow + (candidates[i] / SUB_SIZE);
        int curCol = subCol + (candidates[i] % SUB_SIZE);
        for (int value = 0; value < VALUES; value++) {
          if (possible[curRow][curCol][value] == 1) {
            preShared[value] += 1;
          }
        }
      }
    }
  }

  // Determine if valid naked group and collect shared values
  int shared[size];
  memset(shared, 0, sizeof(shared));
  int sharedCounter = 0;
  for (int value = 0; value < VALUES; value++) {
    if (preShared[value] > 0) {
      if (sharedCounter == size) {
        return 0; // Too many values to be a naked group
      }
      shared[sharedCounter] = value;
      sharedCounter += 1;
    }
  }

  int output = 0;
  int rows[size];
  int cols[size];
  memset(rows, 0, sizeof(rows));
  memset(cols, 0, sizeof(cols));
  // Mode specific construction of rows and cols
  switch (mode) {
    case 0: { // Row
      for (int i = 0; i < size; i++) { // All have same row
        rows[i] = pos;
        cols[i] = candidates[i];
      }
      break;
    }
    case 1: { // Col
      for (int i = 0; i < size; i++) { // All have same col
        rows[i] = candidates[i];
        cols[i] = pos;
      }
      break;
    }
    case 2: { // Box
      int subRow = (pos / SUB_SIZE) * SUB_SIZE;
      int subCol = (pos % SUB_SIZE) * SUB_SIZE;
      for (int i = 0; i < size; i++) {
        rows[i] = subRow + (candidates[i] / SUB_SIZE);
        cols[i] = subCol + (candidates[i] % SUB_SIZE);
      }
      break;
    }
  }

  output = max(output,
    remove_naked_group(rows, cols, shared, size, mode, verbosity));

  // Additional shared groups if any
  int sharedValue = -1;
  int sharesGroup = 1;
  if (mode < 2) {
    for (int i = 0; i < size; i++) {
      if (sharedValue == -1) {
        sharedValue = candidates[i] / SUB_SIZE;
      } else if (sharedValue != candidates[i] / SUB_SIZE) {
        sharesGroup = 0;
        break;
      }
    }
    if (sharesGroup == 1) {
      output = max(output,
        remove_naked_group(rows, cols, shared, size, 2, verbosity));
    }
  } else {
    for (int i = 0; i < size; i++) {
      if (sharedValue == -1) {
        sharedValue = rows[i];
      } else if (sharedValue != rows[i]) {
        sharesGroup = 0;
        break;
      }
    }
    if (sharesGroup == 1) {
      output = max(output,
        remove_naked_group(rows, cols, shared, size, 0, verbosity));
    }

    sharedValue = -1;
    sharesGroup = 1;
    for (int i = 0; i < size; i++) {
      if (sharedValue == -1) {
        sharedValue = cols[i];
      } else if (sharedValue != cols[i]) {
        sharesGroup = 0;
        break;
      }
    }
    if (sharesGroup == 1) {
      output = max(output,
        remove_naked_group(rows, cols, shared, size, 1, verbosity));
    }
  }
  return output;
}

// Evaluates a hidden group by checking validity and using to remove redundancy
int eval_hidden_group(int* candidates, int size, int pos,
    int mode, int verbosity) {
  // Loop through candidates to obtain shared positions
  int preShared[VALUES];
  memset(preShared, 0, sizeof(preShared));
  switch (mode) {
    case 0: { // ROW
      for (int i = 0; i < size; i++) { // value
        for (int col = 0; col < VALUES; col++) {
          if (possible[pos][col][candidates[i]] == 1) {
            preShared[col] += 1;
          }
        }
      }
      break;
    }
    case 1: { // COL
      for (int i = 0; i < size; i++) {
        for (int row = 0; row < VALUES; row++) {
          if (possible[row][pos][candidates[i]] == 1) {
            preShared[row] += 1;
          }
        }
      }
      break;
    }
    case 2: { // SUBGRID
      int subRow = (pos / SUB_SIZE) * SUB_SIZE;
      int subCol = (pos % SUB_SIZE) * SUB_SIZE;
      for (int i = 0; i < size; i++) {
        for (int inPos = 0; inPos < VALUES; inPos++) {
          int curRow = subRow + (inPos / SUB_SIZE);
          int curCol = subCol + (inPos % SUB_SIZE);
          if (possible[curRow][curCol][candidates[i]] == 1) {
            preShared[inPos] += 1;
          }
        }
      }
    }
  }

  // Determine if valid hidden group and collect shared positions
  int shared[size];
  memset(shared, 0, sizeof(shared));
  int sharedCounter = 0;
  for (int position = 0; position < VALUES; position++) {
    if (preShared[position] > 0) {
      if (sharedCounter == size) {
        return 0; // Too many values to be a hidden group
      }
      shared[sharedCounter] = position;
      sharedCounter += 1;
    }
  }

  if (sharedCounter < size) {
    return 0;
  }

  int rows[size];
  int cols[size];
  memset(rows, 0, sizeof(rows));
  memset(cols, 0, sizeof(cols));
  // Mode specific construction of rows and cols
  switch (mode) {
    case 0: { // Row
      for (int i = 0; i < size; i++) { // All have same row
        rows[i] = pos;
        cols[i] = shared[i];
      }
      break;
    }
    case 1: { // Col
      for (int i = 0; i < size; i++) { // All have same col
        rows[i] = shared[i];
        cols[i] = pos;
      }
      break;
    }
    case 2: { // Box
      int subRow = (pos / SUB_SIZE) * SUB_SIZE;
      int subCol = (pos % SUB_SIZE) * SUB_SIZE;
      for (int i = 0; i < size; i++) {
        rows[i] = subRow + (shared[i] / SUB_SIZE);
        cols[i] = subCol + (shared[i] % SUB_SIZE);
      }
      break;
    }
  }

  return remove_hidden_group(rows, cols, candidates, size, mode, verbosity);
}

// OPTIMIZATION: only last candidate size - cur size are evaluated
//  must count up and pass candidates parsed as parameter to allow this

// Recursive function to create groups of candidate numbers to be evaluated
//  as groups of the given size (used for both naked and hidden)
int candid_group(int considered[VALUES], int curIndex, int groupSize,
    int curSize, int* candidates, int pos, int mode,
    int verbosity, int isHidden) {
  if (curSize == groupSize) {
    if (isHidden == 1) {
      return eval_hidden_group(candidates, groupSize, pos, mode, verbosity);
    } else {
      return eval_naked_group(candidates, groupSize, pos, mode, verbosity);
    }
  }
  // use curIndex and candidates to get next curIndex
  int output = 0;
  int nextIndex;
  do {
    nextIndex = -1;
    for (int i = curIndex + 1; i < VALUES; i++) {
      if (considered[i] == 1) {
        nextIndex = i;
        break;
      }
    }
    // if valid index, then recurse further
    if (nextIndex != -1) {
      candidates[curSize] = nextIndex;
      output = max(candid_group(considered, nextIndex, groupSize, curSize + 1,
        candidates, pos, mode, verbosity, isHidden), output);
      // update curIndex to allow next value to be considered
      curIndex = nextIndex;
    }
  } while (nextIndex != -1);
  return output;
  // if invalid index, then return (candidates only used after full assignment)
}

// Wrapper function for forming naked groups and evaluating them
int find_group(int considered[VALUES], int size, int pos,
    int mode, int verbosity, int isHidden) {
  int candidates[size];
  memset(candidates, 0, sizeof(candidates));
  return candid_group(considered, -1, size, 0, candidates, pos,
    mode, verbosity, isHidden);
}

// Loop through all houses and find cells containing same group of values,
//  and remove from these values from all other linked houses
int naked_groups(int size, int verbosity) {
  int output = 0;
  if (verbosity > 0) {
    printf(PROCESS "Finding naked groups of size %i...\n", size);
  }

  // Consider if two or more cells with possibleCount between 2 & size
  int count = 0;
  int considered[VALUES];

  // ROW
  for (int row = 0; row < VALUES; row++) {
    count = 0;
    memset(considered, 0, sizeof(considered));
    for (int col = 0; col < VALUES; col++) {
      if (possibleCount[row][col] >= 2 && possibleCount[row][col] <= size) {
        count++;
        considered[col] = 1;
      }
    }
    // If we consider enough cells, begin to find naked groups
    if (count >= size) {
      output = max(output, find_group(considered, size, row, 0, verbosity, 0));
    }
  }
  // COL
  for (int col = 0; col < VALUES; col++) {
    count = 0;
    memset(considered, 0, sizeof(considered));
    for (int row = 0; row < VALUES; row++) {
      if (possibleCount[row][col] >= 2 && possibleCount[row][col] <= size) {
        count++;
        considered[row] = 1;
      }
    }
    // If we consider enough cells, begin to find naked groups
    if (count >= size) {
      output = max(output, find_group(considered, size, col, 1, verbosity, 0));
    }
  }
  // SUBGRID
  for (int subRow = 0; subRow < SUB_SIZE; subRow++) {
    for (int subCol = 0; subCol < SUB_SIZE; subCol++) {
      count = 0;
      memset(considered, 0, sizeof(considered));
      for (int pos = 0; pos < VALUES; pos++) {
        int curRow = subRow * SUB_SIZE + pos / SUB_SIZE;
        int curCol = subCol * SUB_SIZE + pos % SUB_SIZE;
        int possibilities = possibleCount[curRow][curCol];
        if (possibilities >= 2 && possibilities <= size) {
          count++;
          considered[pos] = 1;
        }
      }
      // If we consider enough cells, begin to find naked groups
      if (count >= size) {
        output = max(find_group(considered, size, (subRow * SUB_SIZE) + subCol,
          2, verbosity, 0), output);
      }
    }
  }
  return output;
}

// Loop through all houses and find 2 cells with pair that only appear there,
//  and remove all the other values from those cells
int hidden_groups(int size, int verbosity) {
  int output = 0;
  if (verbosity > 0) {
    printf(PROCESS "Finding hidden groups of size %i...\n", size);
  }

  // Consider if two or more values appearing between 2 & size
  int valueFrequency = 0;
  int consideredCount = 0;
  int considered[VALUES];

  // ROW
  for (int row = 0; row < VALUES; row++) {
    consideredCount = 0;
    memset(considered, 0, sizeof(considered));
    for (int value = 0; value < VALUES; value++) {
      valueFrequency = 0;
      for (int col = 0; col < VALUES; col++) {
        if (possible[row][col][value] == 1) {
          valueFrequency++;
        }
      }
      if (valueFrequency >= 2 && valueFrequency <= size) {
        considered[value] = 1;
        consideredCount += 1;
      }
    }
    // If we consider enough cells, begin to find naked groups
    if (valueFrequency >= size) {
      output = max(output, find_group(considered, size, row, 0, verbosity, 1));
    }
  }
  // COL
  for (int col = 0; col < VALUES; col++) {
    consideredCount = 0;
    memset(considered, 0, sizeof(considered));
    for (int value = 0; value < VALUES; value++) {
      valueFrequency = 0;
      for (int row = 0; row < VALUES; row++) {
        if (possible[row][col][value] == 1) {
          valueFrequency++;
        }
      }
      if (valueFrequency >= 2 && valueFrequency <= size) {
        considered[value] = 1;
        consideredCount += 1;
      }
    }
    // If we consider enough cells, begin to find naked groups
    if (valueFrequency >= size) {
      output = max(output, find_group(considered, size, col, 1, verbosity, 1));
    }
  }
  // SUBGRID
  for (int subRow = 0; subRow < SUB_SIZE; subRow++) {
    for (int subCol = 0; subCol < SUB_SIZE; subCol++) {
      consideredCount = 0;
      memset(considered, 0, sizeof(considered));
      for (int value = 0; value < VALUES; value++) {
        valueFrequency = 0;
        for (int pos = 0; pos < VALUES; pos++) {
          int curRow = subRow * SUB_SIZE + pos / SUB_SIZE;
          int curCol = subCol * SUB_SIZE + pos % SUB_SIZE;
          if (possible[curRow][curCol][value] == 1) {
            valueFrequency++;
          }
        }
        if (valueFrequency >= 2 && valueFrequency <= size) {
          considered[value] = 1;
          consideredCount += 1;
        }
      }
      // If we consider enough cells, begin to find naked groups
      if (valueFrequency >= size) {
        output = max(find_group(considered, size, (subRow * SUB_SIZE) + subCol,
          2, verbosity, 1), output);
      }
    }
  }
  return output;
}

// Elminates redundant possibilities using the techniques the solver knows
int eliminate_redundant(int verbosity) {
  int output = 0;
  int step = 0;
  if (verbosity > 0) {
    printf(PROCESS "Removing redundant values...\n");
  }
  while (output == 0 && step < 3) {
    switch (step) {
      case 0: {
        for (int size = 2; size <= 4; size++) {
          output = max(output, naked_groups(size, verbosity));
        }
        break;
      }
      case 1: {
        for (int size = 2; size <= 4; size++) {
          output = max(output, hidden_groups(size, verbosity));
        }
        break;
      }
      case 2: {
        output = max(output, intersection_removal(verbosity));
        break;
      }
    }
    step += 1;
  }
  return output;
}


/* --- PROCESS LOOP --- */

// Process the next iteration of the loop
int process_next(int* cellsLeft, int verbosity, int forceFlag) {
  if (*cellsLeft == 0) {
    return 1;
  }
  int value = -1;
  int* pair = find_single(); // Find cell with only one left
  if (pair == NULL) { // If not found, try other methods
    for (int mode = 0; mode < 3; mode++) { // Loop through row, col, box
      if ((pair = unique_in_range(mode, &value, verbosity)) != NULL) {
        break; // If found, stop checking
      }
    }
    if (pair == NULL) { // If there still is no pair found, remove redundancy
      // If nothing was eliminated (stalemate), then brute force or fail
      if (eliminate_redundant(verbosity) == 1) {
        return 0;
      } else {
        if (verbosity > 0 && forceFlag == -1) {
          fprintf(stderr, "Didn't find next pair. Cells left: %i\n",
            *cellsLeft);
        }
        if (forceFlag != -1) { // If brute force not disabled, use it to finish
          brute_force(verbosity);
        }
        return 1;
      }
    }
  }
  // Only for find_single, since unique in range sets appropriate value
  if (value == -1) {
    value = get_possible_value(pair);
    if (verbosity > 0 && value != -1) {
      printf(ADD "Added %s at (%i, %i) as single possibility\n",
        valid[value + 1], pair[0], pair[1]);
    }
  }
  if (value == -1) {
    fprintf(stderr, "Empty cell processed\n");
    return 1;
  }

  grid[pair[0]][pair[1]] = valid[value + 1];
  // Clear filled cell of all possibilities
  for (int val = 0; val < VALUES; val++) {
    possible[pair[0]][pair[1]][val] = 0;
  }
  possibleCount[pair[0]][pair[1]] = 0;
  *cellsLeft -= 1;

  int mask[VALUES];
  memset(mask, 0, sizeof(mask));
  mask[pair[1]] = 1;
  remove_value_from_row(pair[0], value, mask, 0);
  mask[pair[1]] = 0;
  mask[pair[0]] = 1;
  remove_value_from_column(pair[1], value, mask, 0);
  mask[pair[0]] = 0;
  remove_value_from_box(pair[0], pair[1], value, mask, 0);

  free(pair);
  return 0;
}


/* --- PRINTERS --- */

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
//  and solution grid stored as VALUES^2 character strings (so it is dependant
//  on single character value tokens)
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
      pre_process(&cellsLeft);
      while (process_next(&cellsLeft, 0, forceFlag) != 1) {}
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
    pre_process(&cellsLeft);
    while (process_next(&cellsLeft, verbosity, forceFlag) != 1) {}
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
