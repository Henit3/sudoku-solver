#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char grid[9][9]; // Don't need it to be int as no processing done
char valid[] = {'-', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

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
  while (fgets(buffer, 100, input) && row < 9) { // Read through file
    buffer[strcspn(buffer, "\n")] = 0;
    char* token = strtok(buffer, ","); // Split the line with ',' as delimiter
    int col = 0;
    while (token != NULL && col < 9) { // Store all token values into the grid
      grid[row][col] = get_token_value(token);
      token = strtok(NULL, ",");
      col++;
    }
    if (col < 9) { // If fewer values than expected detected
      fprintf(stderr,
        "Too few columns in row %i: Expected 9, not %i", row + 1, col);
      fclose(input);
      return 1;
    }
    row++;
  }
  fclose(input);
  if (row < 9) { // If fewer rows than expected detected
    fprintf(stderr, "Too few rows in grid: Expected 9, not %i.", row);
    return 1;
  }
  return 0;
}

// Prints the grid's current contents
void print_grid() {
  printf("_________________________\n"); // Top
  for (int row = 0; row < 9; row++) {
    if (row % 3 == 0 && row != 0) {
      printf("-------------------------\n"); // Row sectioner
    }
    printf("| "); // Left
    for (int col = 0; col < 9; col++) {
      printf("%c ", grid[row][col]);
      if ((col+1) % 3 == 0) {
        printf("| "); // Col sectioner and Right
      }
    }
    printf("\n");
  }
  printf("_________________________\n"); // Bottom
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

  // Display solution
  // printf("Output:\n");
  // print_grid();
}
