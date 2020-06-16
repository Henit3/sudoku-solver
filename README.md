# sudoku-solver
> A side project written in C to solve Sudoku puzzles.

Compilation:
`clang -o sudoku.out sudoku.c` or `gcc -o sudoku.out sudoku.c -lm`

Usage:
`./sudoku.out [[-v]|[-vv]] [-b] [-t] [[-f]|[-nf]] [-d] [-o <new_csv_name>] [-m <missing_arg>] <csv_path>`

Options:
 * -v verbose output, lists steps in solving
 * -vv extra verbose output, lists brute force steps
 * -b solves a batch execution of sudokus (one per line)
 * -t shows the benchmarked time for reading and execution
 * -f uses brute force for solving immediately
 * -nf disables use of brute force
 * -d debug mode, shows all possibilities for each cell
 * -o exports solved grid to a new csv
 * -m provide missing argument if not in the grid

Originally worked on to both learn sudoku solving techniques and become more familiar with C,
it supports CSV reading and writing of sudoku grids. This program is capable of working with
any NxN square grid where N is a square number (e.g. 4x4, 9x9, 16x16...), with any set of strings
used as the N values that it contains, and has two modes of operation, deduction and brute force.

When operating under deduction mode (the default setting), it is capable of listing the steps
performed to arrive at the solution that a human can follow on and understand why the step
generates a valid value (using the `-v` option). It utilizes conventional deduction techniques,
such as locating cells that may hold only one possible value, finding the only position available
for a value in a house (row, column or box), and using more advanced techniques such as
pointing groups, box line reduction and disjoint subsets (hidden and naked groups) to restrict
values to certain positions, until it cannot carry on processing safely using these rules
(resorting to brute force mode) or until it has produced a complete grid.

Operation under the brute force mode will execute a mindless recursive backtracking algorithm
where values are tried in each cell, reverting to a previous point if the current cell has
conflicts for all possible values, until the sudoku is solved. This behaviour can be enforced
as default with the `-f` option, and can be disabled with `-nf` - particularly useful if the
steps to arrive at the solution are required. Specifying the `-vv` option will show all recursive
calls, and cells traversed (skipped or values inserted), which can be used to understand the
processing done by the solver (e.g. for debugging).

Bench-marking can be performed for the time taken to solve puzzles by using the `-t` option, which
works more effectively under bulk solving mode (set by using `-b`) due to small fluctuations appearing
between runs of the same or similar input. A bulk CSV sudoku file has each line represent a separate
sudoku puzzle and its intended solution by serializing the values, using 0 to encode blank cells
(Source: https://www.kaggle.com/bryanpark/sudoku/data).

Additional features include visualising the input and output sudoku grid in a simple but functional
ASCII format on the terminal, the option to write this output to another CSV file using the `-o`
flag, and the ability to define a missing value that the sudoku puzzle may hold but the initial grid
skips out on (this defaults to "X", but `-m` may be specifed to set this value).
