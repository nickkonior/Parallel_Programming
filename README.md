# Parallel_Programming
Parallel_Progamming school projects 1 &amp; 2 . UNIWA

Project 1 : 
Processor 0 reads a vector sized "n" 

The programm calculates parallely and prints (through processor 0) : 
  *The average value "av".
  *Max value.  
  *Variance = ((x0 - av)^2 + (x1 - av)^2 + ... + (xn-1 - av)^2)/n.
  *Vector X' where X[i]' = |X[i] - max|^2.


Project 2: 
Processor 0 reads number "N" and then the array A[N][N].

The programm calculates parallely and prints (through processor 0) :
  *If the array is diagonally dominant * 
  *The max element the diagonal. 
  *Array B where B[i][j] = max - |A[i][j]|,    i!=j.
                B[i][i] = max,                i==j.
  *The min value of array B and it's position.

An array is diagonally dominant when the element of the diagonal is greater than the sum of the elements in the same row. 
