# Parallel_Programming
Parallel_Progamming school projects 1 &amp; 2 . UNIWA

Project 1 : 
Processor 0 reads a vector sized "n" 

The programm calculates parallely and prints (through processor 0)
  the average value "av"
  max value 
  variance = ((x0 - av)^2 + (x1 - av)^2 + ... + (xn-1 - av)^2)/n
  vector X' where X[i]' = |X[i] - max|^2


Project 2: 
Processor 0 reads number "N" and then the array A[N][N]

The programm calculates parallely and prints (through processor 0)
  if the array is diagonally dominant * 
  the max element the diagonal 
  array B where B[i][j] = max - |A[i][j]|,    i!=j
                B[i][i] = max,                i==j
  the min value of array B and it's position
