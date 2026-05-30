# CuteModel

CuteModel is a C++ library for Qt providing templated model types.

## Technology
1. Qt
2. C++17 (maybe 20 is needed)
3. CMake. Everything defined through targets, never or almost using `set` on global variable
4. Exposed through vcpkg.
5. GTests

## Principle

1. Build the model around lazy loading using `canFetchMore` and `fetchMore`.
2. Construct `QPointer<Ref<T>>` with `Ref<T>` inheriting from `QObject` and follow an object using a `QPersistentModelIndex`. The free function is something close to: `getAsRefObject<Default = Ref<T>>(QModelIndex)`
3. Most function follow the standard containers API. `push_back`...
4. Propose a `VariantWrapper<T>` for object not eligible to `Q_DECLARE_METATYPE`. Using `reinterpret_cast`, `std::array<std::byte, N>` and `std::launder`.
5. Don't care much about roles, except for QML.
6. Use `multiData` instead of `data`.
7. Edition will be done through a cache with `submit` / `revert`.

## Different Models targetted
1. BaseModel<T> : Inherit from QAbstractItemModel. Handle drop(T) events, edit with caching, expose getValue function etc.
1. BasicListModel<T> : inherit from BaseModel
2. BasicTableModel<T> : inherit from BaseModel
3. BasicTreeModel<T> : inherit from BaseModel
4. AggregatedListModel<Model (maybe not templated)> : inherit from BaseModel and add Model inside instead of elements

## Proxy Models

1. RowFilterModel
2. RowSortModel
3. FlattenTreeModel

## How to build

The build uses CMake presets. Qt is consumed from a system install (never hardcoded
in the build files), and GTest comes from vcpkg. No absolute path lives in any CMake file.

### Required environment

- `VCPKG_ROOT` — path to a vcpkg checkout (used by the toolchain in `CMakePresets.json`).
- Qt: discovered automatically. If it is not on the default search path, set **one** of
  `QT_ROOT_DIR`, `Qt6_DIR`, `QTDIR`, or `CMAKE_PREFIX_PATH` to the Qt install prefix
  (e.g. `~/Qt/6.8.3/macos`). The top-level `CMakeLists.txt` prepends these to
  `CMAKE_PREFIX_PATH` before `find_package(Qt6)`.

### Commands

```bash
export VCPKG_ROOT=/path/to/vcpkg
export QT_ROOT_DIR=~/Qt/6.8.3/macos   # only if Qt is not auto-detected

cmake --preset default
cmake --build --preset default
ctest --preset default --output-on-failure
```

### Targets

- `CuteModel` (alias `CuteModel::CuteModel`) — the static library, installable/exportable
  for downstream consumption (e.g. via vcpkg).
- `cutemodel_tests` — GTest suite (`CUTEMODEL_BUILD_TESTS`, default ON).
- `refobject` — example executable (`CUTEMODEL_BUILD_EXAMPLES`, default ON).

## Testing
1. Always use `QAbstractItemModelTester`.
2. Always try to catch signals through `QSignalSpy`. Never use lambda that increment something or things like that.