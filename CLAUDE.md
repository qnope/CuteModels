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
4. ValueRole support is opt-in via the `is_compatible_with_value_role<T>` trait: `AbstractSourceModel` guards every ValueRole touchpoint (`data`/`multiData`, `setData`, `roleNames`) with `if constexpr`, so a T not eligible to travel through `QVariant` simply does not expose ValueRole rather than being wrapped.
5. Don't care much about roles, except for QML.
6. Use `multiData` instead of `data`.
7. Edition will be done through a cache with `submit` / `revert`.
8. Pas de commentaire.

## Different Models targetted
1. AbstractSourceModel<T> : Inherit from QAbstractItemModel. Handle drop(T) events, edit with caching, expose getValue function etc. Owns the `flags()` dispatch: the `QModelIndex` override is `final` and resolves the element before delegating to the virtual `flags(const T&, const QModelIndex&)` customization point (default `Qt::ItemIsSelectable | Qt::ItemIsEnabled`). The flags returned for the invalid/root index come from a settable `rootFlags()` / `setRootFlags()` attribute (default `Qt::NoItemFlags`). Also owns the role-data dispatch: `multiData` is `final` and, for each role in the span, either fills ValueRole from `getStorageValue` (when `is_compatible_with_value_role_v<T>`) or delegates to the pure-virtual `data(const T&, const QModelIndex&, int role)` customization point.
1. BasicListModel<T> : inherit from AbstractSourceModel. **1D storage (`std::vector<T>`) but N-column displayable** — a `QStringList headers` ctor argument fixes the column count; subclasses project each T to roles by overriding the inherited `data(const T&, const QModelIndex&, int role)` (use `index.column()`). A row is one whole T: any edit rewrites the entire T and refreshes every column of the row (only the row matters).
2. BasicTableModel<T> : inherit from AbstractSourceModel. **True 2D storage** — independent T per cell, per-cell edit cache for multi-column row commits. Distinct from BasicListModel, not subsumed by it.
3. BasicTreeModel<T> : inherit from AbstractSourceModel
4. AggregatedListModel<Model (maybe not templated)> : inherit from AbstractSourceModel and add Model inside instead of elements

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

### Sanitizers

`CUTEMODEL_ENABLE_SANITIZERS` (default OFF) builds the library, tests and examples with
ASan + UBSan on GCC/Clang, and ASan on MSVC (UBSan does not exist there; `/RTC1` is
stripped from the Debug flags since it conflicts with `/fsanitize=address`). The flags
live on the `cutemodel_sanitizers` INTERFACE target (`cmake/Sanitizers.cmake`), linked
into `CuteModel` through `$<BUILD_INTERFACE:...>` so the installed export is unaffected.
The `sanitize` preset turns it on:

```bash
cmake --preset sanitize
cmake --build --preset sanitize
ctest --preset sanitize --output-on-failure
```

CI runs both presets on every platform (full `os` x `preset` matrix). Leak detection is
LSan's platform default (on by default on Linux, unavailable with Apple Clang and MSVC).

## Testing
1. Always use `QAbstractItemModelTester`.
2. Always try to catch signals through `QSignalSpy`. Never use lambda that increment something or things like that.