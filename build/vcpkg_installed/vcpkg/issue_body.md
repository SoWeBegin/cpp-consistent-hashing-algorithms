Package: cxxopts:x64-linux -> 3.1.1

**Host Environment**

- Host: x64-linux
- Compiler: GNU 11.4.0
-    vcpkg-tool version: 2023-11-16-4c1df40a3c5c5e18de299a99e9accb03c2a82e1e
    vcpkg-scripts version: 72010900b 2023-12-04 (3 months ago)

**To Reproduce**

`vcpkg install `
**Failure logs**

```
-- Using cached jarro2783-cxxopts-v3.1.1.tar.gz.
-- Cleaning sources at /mnt/c/scuola/progetto di semestre/ProgettoSemestre/cpp-consistent-hashing-algorithms/vcpkg/buildtrees/cxxopts/src/v3.1.1-6b7980cd99.clean. Use --editable to skip cleaning for the packages you specify.
-- Extracting source /mnt/c/scuola/progetto di semestre/ProgettoSemestre/cpp-consistent-hashing-algorithms/vcpkg/downloads/jarro2783-cxxopts-v3.1.1.tar.gz
-- Using source at /mnt/c/scuola/progetto di semestre/ProgettoSemestre/cpp-consistent-hashing-algorithms/vcpkg/buildtrees/cxxopts/src/v3.1.1-6b7980cd99.clean
-- Configuring x64-linux
-- Building x64-linux-dbg
-- Building x64-linux-rel
-- Fixing pkgconfig file: /mnt/c/scuola/progetto di semestre/ProgettoSemestre/cpp-consistent-hashing-algorithms/vcpkg/packages/cxxopts_x64-linux/lib/pkgconfig/cxxopts.pc
CMake Error at scripts/cmake/vcpkg_find_acquire_program.cmake:162 (message):
  Could not find pkg-config.  Please install it via your package manager:

      sudo apt-get install pkg-config
Call Stack (most recent call first):
  scripts/cmake/vcpkg_fixup_pkgconfig.cmake:203 (vcpkg_find_acquire_program)
  ports/cxxopts/portfile.cmake:21 (vcpkg_fixup_pkgconfig)
  scripts/ports.cmake:170 (include)



```
**Additional context**

<details><summary>vcpkg.json</summary>

```
{
  "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
  "name": "mementohash",
  "version-string": "0.0.1",
  "dependencies": [
    "boost-unordered",
    "fmt",
    "cxxopts",
    "xxhash",
    "pcg",
    "gtl"
  ]
}

```
</details>
