# LayerKeySort Mini v1.0.0 Stable

Mini is an independent MIT-licensed C17 library for local mutable sequences.
The non-intrusive doubly linked list stores caller-owned payload pointers and
assigns each insertion a stable live occurrence handle, including duplicate
and NULL payloads. Removal or order destruction invalidates affected handles.

The 18 public functions cover order creation/destruction, size, endpoints,
neighbor traversal, item lookup, four insertions, removal, four moves and live
comparison. Known-handle movement is O(1); arbitrary comparison is O(n);
each occurrence requires one allocation. Movement/compare allocate nothing.
Mini does not provide persistence, automatic sorting, indexed random access,
built-in synchronization or invalid/freed-handle detection.

Full is a separately versioned product for complete ordering, snapshots and
advanced workflows. Mini imports none of its production implementation.
Tag `mini-v1.0.0` identifies Mini; historical Full `v1.0.0` is unrelated.
Full v4.0.0 remains the repository-wide Latest Release.

## Download and verify

[Official release](https://github.com/RXY712200/LayerKeySort/releases/tag/mini-v1.0.0) provides exactly two product assets:

- [LayerKeySort-Mini-v1.0.0-source.zip](https://github.com/RXY712200/LayerKeySort/releases/download/mini-v1.0.0/LayerKeySort-Mini-v1.0.0-source.zip)
- [LayerKeySort-Mini-v1.0.0-SHA256SUMS.txt](https://github.com/RXY712200/LayerKeySort/releases/download/mini-v1.0.0/LayerKeySort-Mini-v1.0.0-SHA256SUMS.txt)

Download both into one directory. GitHub's automatic Source code archive is
the entire repository, not this Mini-only product package. On Linux:

```sh
sha256sum -c LayerKeySort-Mini-v1.0.0-SHA256SUMS.txt
unzip LayerKeySort-Mini-v1.0.0-source.zip
cd LayerKeySort-Mini-v1.0.0
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

macOS can use `shasum -a 256 -c` for the manifest; Windows PowerShell can
compare `Get-FileHash -Algorithm SHA256` with the manifest and use Expand-Archive.
CMake3.16+, a C17 compiler and matching build tool suffice. Visual Studio uses
--config Debug; MinGW may require -G "MinGW Makefiles". The standalone four-test
suite includes public behavior, internal invariants/fault injection, example
and the independent fixed-seed model. Optional coexistence is not required.

## Verification and compatibility

The release process requires successful exact-commit Mini GCC, Clang, MSVC,
AppleClang and Linux Clang ASan/UBSan jobs, independent package extraction,
external consumers and unchanged Full validation. Actual final results,
compiler versions, source commit and checksums belong in the GitHub release
notes and completion report. Earlier [development evidence](DEVELOPMENT.md)
retains its original commit scope. No claim covers all ABIs or workloads.

Compatible 1.x updates preserve the [public contract](CONTRACT.md). Private
layouts are not public ABI. [Performance](PERFORMANCE.md) records measured
RC.1 timings and qualified sizeof estimates; allocator-inclusive memory and
portable latency guarantees are not claimed. Broader measurements are optional
future work, not an existing defect.

## 中文说明

这是独立的 Mini v1.0.0 Stable，采用双向链表，完整提供18个公开函数。
业务对象由调用者拥有；已知句柄移动O(1)，任意顺序比较O(n)，每实例一次分配。
不提供持久化、自动排序、随机访问、内置锁或无效/释放句柄检测。
下载上方仅Mini源码ZIP和校验文件，校验后解压即可独立构建、测试。
Full保持独立版本，仓库Latest仍为Full v4.0.0。
