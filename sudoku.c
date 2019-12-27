#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define ROW_BUF_SIZE 500
#define PAD_SIZE 15

char*** grid; // Don't need it to be int as no processing done
int*** possible; // Stores all possibilities (Space n^3)!
int** possibleCount; // Trades off space in favour of time
char** valid;
int VALUES, SUB_SIZE;

// Processes the command line arguments provided
int process_arguments(int argc, char* argv[], int* flag,
                      char** inCsv, char** outCsv, char** missing) {
  // Check if enough arguments provided
  if (argc < 2) {
    fprintf(stderr, "Not enough arguments\n");
    return 1;
  }
  // Go through all arguments (not first since that's the command)
  int inFound = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp("-l", argv[i]) == 0) { // List flag
      if (*flag == 0) { // Check if already specified
        *flag = 1;
      } else {
        fprintf(stderr, "Duplicate -l flag found\n");
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
int initialize_globals(char buffer[ROW_BUF_SIZE]) {
  // Gets number of values
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

  if (initialize_globals(fgets(buffer, ROW_BUF_SIZE, input)) != 0) {
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
      fprintf(stderr,
        "Too few columns in row %i: Expected %i, not %i\n", row + 1, VALUES, col);
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

  if (tokenCount == VALUES) { // If not enough values
    if (missing != NULL) {
      valid[VALUES] = strdup(missing);
    } else {
      fprintf(stderr, "Missing one values so X substituted\n");
      valid[VALUES] = strdup("X");
    }
  } else if (tokenCount < VALUES) {
    fprintf(stderr, "Not enough values provided, expected: %i\n", VALUES);
    return 1;
  }
  return 0;
}

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

// Gets pair of row and col that is only place for a possible value in a row
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
        printf("Unique value in row\n");
        *value = val + 1;
        return make_pair(row, checkArray[val][1]);
      }
    }
  }
  return NULL;
}

// Gets pair of row and col that is only place for a possible value in a col
int* unique_in_col(int* value) {
  int checkArray[VALUES][2]; // checkArray[X][0] = frequency, [1] = last found
  for (int col = 0; col < VALUES; col++) { // Check in each subGrid
    memset(checkArray, 0, sizeof(checkArray)); // Reset array
    for (int row = 0; row < VALUES; row++) { // Go through cells in subGrid // Update value in checkArray
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
        printf("Unique value in col\n");
        *value = val + 1;
        return make_pair(checkArray[val][1], col);
      }
    }
  }
  return NULL;
}

// Gets pair of row and col that is only place for a possible value in a subGrid
// Not sure if ever needed so couldn't be tested
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
          printf("Unique value in subGrid\n");
          int* pair = calloc(2, sizeof(int));
          int pairRow = (row * SUB_SIZE) + (checkArray[val][1] / SUB_SIZE);
          int pairCol = (col * SUB_SIZE) + (checkArray[val][1] % SUB_SIZE);
          *value = val + 1;
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

// Assumes there is only one value in the bitmap and retrieves it
int get_possible_value(int* pair) {
  for (int i = 0; i < VALUES; i++) {
    if (possible[pair[0]][pair[1]][i] == 1) {
      return i + 1;
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
  printf("\n_________________________\n"); // Top
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

// Process the next iteration of the loop
int process_next(int* cellsLeft, int listSteps) {
  if (*cellsLeft == 0) {
    return 1;
  }
  int value = -1;
  int* pair = find_single(); // Find cell with only one left
  if (pair == NULL) { // If not found, try other methods
    for (int mode = 0; mode < 3; mode++) { // Loop through row, then col, then subGrid
      if ((pair = unique_in_range(mode, &value)) != NULL) { // If found, stop checking
        break;
      }
    }
    if (pair == NULL) { // If there still is not pair found then throw error
      fprintf(stderr, "Didn't find next pair. Cells left: %i\n", *cellsLeft);
      return 1;
    }
  } else {
    value = get_possible_value(pair);
  }
  if (value == -1) {
    fprintf(stderr, "Empty cell processed\n");
    return 1;
  }

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
  if (listSteps == 1) {
    printf("Added %s at (%i, %i)\n", valid[value], pair[0], pair[1]);
  }
  free(pair);
  return 0;
}

// Prints the current state of the grid in a formatted output
void print_grid() {
  printf("\n_________________________\n"); // Top
  for (int row = 0; row < VALUES; row++) {
    if (row % SUB_SIZE == 0 && row != 0) {
      printf("-------------------------\n"); // Row sectioner
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
  printf("_________________________\n\n"); // Bottom
}

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

int main(int argc, char* argv[]) {
  // Read and store arguments in easy to use manner
  char* inCsv = NULL;
  char* outCsv = NULL;
  char* missing = NULL;
  int listFlag = 0; // flag structure -> +1 for -l, +2 for -o, +4 for -m
  if (process_arguments(argc, argv, &listFlag, &inCsv, &outCsv, &missing) != 0) {
    fprintf(stderr, "\nUsage: ./sudoku.out [-l] [-o <new_csv_name>] "
                    "[-m <missing_arg>] <csv_path>\nOptions:\n"
                    " -l lists steps in solving\n"
                    " -o exports solved grid to a new csv\n"
                    " -m provide missing argument if not in the grid");
    return 1;
  }

  // Load up contents of the file into grid
  switch (read_csv(inCsv, missing)) {
    case 1:
      free_globals();
    case -1:
      fprintf(stderr, "File could not be read\n");
      return 1;
  }

  // Show the contents of the file
  printf("Input:");
  print_grid();

  // Calculate the result
  int cellsLeft = 0;
  pre_process(&cellsLeft);
  while (process_next(&cellsLeft, listFlag) != 1) {}

  // Display solution
  printf("\nOutput:");
  print_grid();
  if (outCsv != NULL) { // If output set, export
    if (export_grid(outCsv) != 0) {
      free_globals();
      return 1;
    }
  }
  free_globals();
}
