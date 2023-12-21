# UnityTool
  This tool is developed in C++ as part of my application process for an internship at JetBrains. There is a single-threaded version (tool.cpp) and a multi-threaded one (tool_threads.cpp) with running time comparisons below.
  The tool parses the yaml scene files to extract the objects and scripts present in each scene, then constructs the scene hierarchy for each one in a .dump file. Finally it outputs into a .csv the guids and relative paths of all the unused scripts.


## Comparison of the performance of the multi-threaded vs single-threaded versions
(All tests have been ran 1000 times to get the average running time)

  Single-threaded test2 average running time = 37452 microseconds 
  
  Single-threaded test1 average running time = 34442 microseconds 

  Multi-threaded test2 average running time = 26997 microseconds
  
  Multi-threaded test1 average running time = 23729 microseconds
