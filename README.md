# cpp-consistent-hashing-algorithms
This project collects C++ implementations and benchmarking tools of some of the most prominent consistent hashing algorithms for non-peer-to-peer contexts.

The implemented algorithms are:
* [2014] __jump hash__ by [Lamping and Veach](https://arxiv.org/pdf/1406.2294.pdf)
* [2020] __anchor hash__ by [Gal Mendelson et al.](https://arxiv.org/pdf/1812.09674.pdf), using the implementation found on [Github](https://github.com/anchorhash/cpp-anchorhash)
* [2023] __power consistent hash__ by [Eric Leu](https://arxiv.org/pdf/2307.12448.pdf)
* [2023] __memento hash__ by [M. Coluzzi et al.](https://arxiv.org/pdf/2306.09783.pdf)
* [2023] __dx hash__ by [Chaos Dong et al.](https://arxiv.org/pdf/2107.07930)

## Benchmarks

The project includes six benchmarking tools: **lookup_time**, **balance**, **monotonicity**, **memory-usage***, **init_time** and **resize_time**.

**memory_usage** is recorded through **lookup_time**: it measures **heap allocations** and the maximum allocated heap space.

## Building

Clone the repository:
```bash
git clone https://github.com/SoWeBegin/cpp-consistent-hashing-algorithms.git
```
Move into the project's directory and update the [vcpkg](https://vcpkg.io/en/) submodule:
```bash
cd cpp-consistent-hashing-algorithms/
git submodule update --init
cd vcpkg
./bootstrap-vcpkg.sh -useSystemBinaries
```
Generate the build file (for [Ninja](https://ninja-build.org/)):
```bash
cd ..
cmake -B build/ -S . -GNinja -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
```
Move into the **build** directory and start building:
```bash
cd build
ninja
```

## Running the benchmarks
* The program requires you to provide some parameters that are needed for each benchmark (through .yaml file). For an overview, take a look at https://github.com/SUPSI-DTI-ISIN/java-consistent-hashing-algorithms/blob/main/src/main/resources/configs/template.yaml
* Once you have done the steps above, you should go in your build directory and insert your template.yaml file there. The name must be exactly "template.yaml".
* Then, simply start the program with `./cpp-consistent.hashing` inside the build directory.
* All the output files (in `.csv` format) will be written inside the `build` directory.

The **lookup** benchmark is needed to test the speed of lookup time.

The **balance** benchmark performs a balance test, that is, it checks whether the nodes contain a similar amount of keys.

The **monotonicity** benchmark performs a monotonicity test to find out how many keys are removed out of nodes (and which ones) once some nodes are removed and added back.

The **resize** benchmark checks how many units of time are needed to complete a resize.

The **memory** benchmark simply counts the number of allocations, deallocations and how many bytes were allocated and deallocated.

The **init** benchmark finds out how many units of time are needed to initialize the internal structures of the provided algorithms.

## Running the unit test
* The program requires you to provide some parameters that are needed for each benchmark (through .yaml file). For an overview, take a look at https://github.com/SUPSI-DTI-ISIN/java-consistent-hashing-algorithms/blob/main/src/main/resources/configs/template.yaml
* Once you have done the steps above, you should go in your build directory and insert your template.yaml file there. The name must be exactly "template.yaml".
* Then, to run the unit test, type ctest inside the build directory.

## Java implementation
This C++ implementation aims to provide the same output as the following [Java](https://github.com/SUPSI-DTI-ISIN/java-consistent-hashing-algorithms) implementation.
It also aims to be as complete and is meant to be used to compare the two implementations.

## Credits
The AnchorHash implementation is based on code Copyright (c) 2020 anchorhash released under the MIT License
MementoHash is based on the Java implementation found on [this repository](https://github.com/SUPSI-DTI-ISIN/java-consistent-hashing-algorithms), released under the GNU GPL-3.0 license 
