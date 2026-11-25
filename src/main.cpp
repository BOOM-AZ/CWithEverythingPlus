/*

cmake -S . -B build
cmake --build build
./build/allc

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/allc

*/

#include <iostream>


int main() {
    
    printf("GOODBYE, world.\n");

    return 0;
}