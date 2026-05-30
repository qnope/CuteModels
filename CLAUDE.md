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

## Ref information
```cpp

constexpr int ValueRole = Qt::UserRole + 1;

class RefBase : public QObject 
{
    Q_OBJECT
    public:
    RefBase(QPersistentModelIndex index)
        : QObject{index.model()}
        , m_index(index)
    {
        connect(index.model(), &QAbstractItemModel::dataChanged, this, [this](auto topLeft, auto bottomRight)
        {
            if (isInside(m_index, topLeft, bottomRight))
                emit valueChanged();
            // If topLeft.parent() (or parent.parent.parent...)
            // is m_index, emit this signal
            else if (isInParentChain(m_index, topLeft))
                emit underlyingHierarchyChanged();
        })
    }

    // these signals are public
    signals:
    void valueChanged();
    void underlyingHierarchyChanged();

protected:
    QPersistentModelIndex m_index;
};

template<typename T>
class Ref : public RefBase
{
    T getValue() const { 
        if (m_index.isValid() == false)
            std::terminate();
        return m_index.data(ValueRole).value<T>();
    }
};
```

## Different Models targetted
1. BasicListModel<T> : inherit from QAbstractListModel
2. BasicTableModel<T> : inherit from QAbstractTableModel
3. BasicTreeModel<T> : inherit from QAbstractItemModel
4. AggregatedListModel<Model (maybe not templated)> : inherit from QAbstractItemModel and add Model inside instead of elements

## List information
List will have `operator[](ListIndex)` which return a Wrapper. The mutable wrapper will, once deleted, emit the dataChanged.

List are fully iterable. If mutable iteration, emit dataChanged() on the full model.
List are also partially iterable. If mutable iteration, emit dataChanged on the partial part of the model.

## Table information
Table will have `operator[](ListTable)` which return a Wrapper. The mutable wrapper will, once deleted, emit the dataChanged

## Proxy Models

1. RowFilterModel
2. RowSortModel
3. FlattenTreeModel

## Drag And Drop
1. Drop / canDrop will operate directly on the Value instead of a QModelIndex. For Drop on the root, `canDropOnRoot` or `dropOnRoot` will be called.

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