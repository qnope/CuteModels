# CuteModel

CuteModel is a C++17 library for Qt that provides **templated, type-safe model classes** built on top of `QAbstractItemModel`. Instead of juggling `QVariant`s, roles and raw indexes, you store real C++ values of type `T` in your models and manipulate them with an API that feels like the standard containers (`push_back`, `emplace_back`, `insert`, `erase`, `clear`, `resize`, …).

All classes live in the `cute` namespace and are header-only templates (except for a small `RefBase` translation unit).

## Why CuteModel?

Writing a `QAbstractItemModel` subclass by hand is repetitive and error-prone: the data is stringly-typed behind `QVariant`, every model reimplements the same index bookkeeping, and observing a single element across inserts and removals requires manual `QPersistentModelIndex` plumbing. CuteModel addresses this with a few core ideas:

- **Typed storage** — models own values of `T`. A single pure-virtual customization point, `data(const T &, const QModelIndex &, int role)`, projects a value to display roles; everything else (index math, `multiData`, `flags` dispatch, MIME handling) is provided.
- **Container-like mutation API** — `push_back`, `emplace_back`, `insert`, `append_range`, `erase`, `resize`, `reset`… every operation emits the correct model signals for you.
- **Persistent typed references** — `getRef(index)` returns a `Ref` object (a `QObject`) that follows an element through inserts, removals and layout changes, exposes `getValue()` / `setValue()`, and notifies via signals when the element changes or disappears.
- **Opt-in QML/`QVariant` exposure** — the `is_compatible_with_value_role<T>` trait decides at compile time whether a model exposes its values through `ValueRole` (role name `"value"`). Types that cannot travel through `QVariant` simply do not expose it; nothing breaks.
- **`multiData`-first** — role dispatch is centralized in a `final` `multiData` override; `data(QModelIndex, role)` is implemented in terms of it.
- **RAII change notification** — `ItemProxy` and `RangeList` give direct mutable access to stored values and emit `dataChanged` automatically when they go out of scope.

## Quick example

```cpp
#include <CuteModel/BasicListModel.h>
#include <CuteModel/RefBase.h>

class StringListModel : public cute::BasicListModel<QString>
{
public:
    using BasicListModel<QString>::BasicListModel;
    using BasicListModel<QString>::data;

    QVariant data(const QString &value, const QModelIndex &index, int role) const override
    {
        if (index.column() == 0 && (role == Qt::DisplayRole || role == Qt::EditRole))
            return value;
        return {};
    }
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    StringListModel model;
    model.push_back("alpha");
    model.push_back("beta");
    model.push_back("gamma");

    // A typed, persistent reference to the second element.
    auto ref = model.getRef(model.index(1, 0));

    QObject::connect(ref.get(), &cute::RefBase::valueChanged,
                     [&ref] { qInfo() << "value changed to:" << ref->getValue(); });

    ref->setValue("BETA"); // updates the model, emits dataChanged, triggers valueChanged
}
```

A complete runnable version of this example lives in [`Examples/refobject/main.cpp`](Examples/refobject/main.cpp).

## Class reference

### Core models

#### `cute::AbstractSourceModel<T>`

*Header: [`CuteModel/include/CuteModel/AbstractSourceModel.h`](CuteModel/include/CuteModel/AbstractSourceModel.h)*

The base class of every source model. It inherits both `QAbstractItemModel` and `ValueModelAccessor<T>` and owns the dispatch logic shared by all concrete models:

- **Role data dispatch** — `data(QModelIndex, role)` and `multiData` are `final`. For each requested role, `multiData` either fills `ValueRole` directly from storage (when `is_compatible_with_value_role_v<T>` holds) or delegates to the pure-virtual customization point:

  ```cpp
  virtual QVariant data(const T &value, const QModelIndex &index, int role) const = 0;
  ```

- **Flags dispatch** — `flags(QModelIndex)` is `final`. For valid indexes it resolves the stored element and delegates to the virtual customization point `flags(const T &, const QModelIndex &)` (default: `Qt::ItemIsSelectable | Qt::ItemIsEnabled`). The flags returned for the invalid/root index are a settable attribute: `rootFlags()` / `setRootFlags()` (default `Qt::NoItemFlags`).
- **ValueRole support** — guarded with `if constexpr (is_compatible_with_value_role_v<T>)` in `multiData`, `setData` and `roleNames`. When enabled, `setData(index, value, ValueRole)` writes a whole `T` back into storage, and `roleNames()` exposes the `"value"` role name for QML.
- **Drag & drop** — `mimeData`, `canDropMimeData` and `dropMimeData` are `final` and route through typed customization points:

  ```cpp
  virtual QString mimeTypeForValue() const;
  virtual void encodeMimeData(QDataStream &stream, const std::vector<T> &values) const;
  virtual std::vector<T> decodeMimeData(QDataStream &stream) const;
  virtual bool canDropOnElement(const T &element, const QModelIndex &index, Qt::DropAction action, const QMimeData *data) const;
  virtual bool dropOnElement(const T &element, const QModelIndex &index, Qt::DropAction action, const QMimeData *data);
  virtual bool canDropInsertion(int row, int column, const QModelIndex &parent, Qt::DropAction action, const QMimeData *data) const;
  virtual bool dropInsertion(int row, int column, const QModelIndex &parent, Qt::DropAction action, const QMimeData *data);
  ```

  A drop on a valid element dispatches to `dropOnElement`; a drop between rows dispatches to `dropInsertion`. `valuesFromMimeData(const QMimeData *)` decodes the typed payload. MIME serialization is only available when `T` is copy-constructible.

#### `cute::BasicListModel<T>`

*Header: [`CuteModel/include/CuteModel/BasicListModel.h`](CuteModel/include/CuteModel/BasicListModel.h)*

**1D storage, N-column display.** Elements are stored in a `std::vector<T>`; a `QStringList headers` constructor argument fixes the column count (and the horizontal `headerData`). A row is one whole `T`: subclasses project a `T` to its columns by overriding `data(const T &, index, role)` and switching on `index.column()`. Any edit rewrites the entire `T` and emits `dataChanged` for every column of the row.

```cpp
explicit BasicListModel(QObject *parent = nullptr);
explicit BasicListModel(QStringList headers, QObject *parent = nullptr);

int  size() const noexcept;            bool empty() const noexcept;
void push_back(T value);               T &emplace_back(Args &&...args);
void insert(int row, T value);
void append_range(const std::vector<T> &values);
void insert_range(int row, const std::vector<T> &values);
void erase(int row);                   void erase(int first, int last);
void clear();                          void reset(std::vector<T> newItems);
void resize(int count, T defaultValue = T());

ItemProxy<const T> at(int row) const;  ItemProxy<const T> operator[](int row) const;
ItemProxy<T>       getMutable(int row);
RangeList<const T> iter() const;       RangeList<const T> iter(int first, int number) const;
RangeList<T>       iter_mut();         RangeList<T>       iter_mut(int first, int number);
```

The standard `insertRows` works when `T` is default-constructible; `removeRows` always works.

#### `cute::BasicTableModel<T>`

*Header: [`CuteModel/include/CuteModel/BasicTableModel.h`](CuteModel/include/CuteModel/BasicTableModel.h)*

**True 2D storage** — an independent `T` per cell, backed by the `Table<T>` container. Distinct from `BasicListModel` (which is 1D data merely *displayed* over N columns): here rows *and* columns are first-class, and every cell is edited independently.

```cpp
explicit BasicTableModel(QObject *parent = nullptr);
BasicTableModel(int rows, int columns, QObject *parent = nullptr);
BasicTableModel(int rows, int columns, const T &defaultValue, QObject *parent = nullptr);

int rows() const noexcept;   int columns() const noexcept;   bool empty() const noexcept;

// Row operations
void push_back_row(T defaultValue = T());
void insert_row(int row, T defaultValue = T());
void insert_rows(int row, int count, T defaultValue = T());
bool append_range_row(const std::vector<T> &values);
bool insert_range_row(int row, const std::vector<T> &values);
void erase_row(int row);     void erase_rows(int first, int last);
void resize_rows(int count, T defaultValue = T());

// Column operations (same shape)
void push_back_column(...);  void insert_column(...);  void insert_columns(...);
bool append_range_column(...);  bool insert_range_column(...);
void erase_column(...);      void erase_columns(...);
void resize_columns(int count, T defaultValue = T());

void clear();
bool reset(std::vector<std::vector<T>> newRows); // pads ragged rows if T is default-constructible

ItemProxy<const T> at(int row, int column) const;
ItemProxy<T>       getMutable(int row, int column);
```

#### `cute::TreeModel<NodePtr>` and `cute::TreeNode<T>`

*Headers: [`CuteModel/include/CuteModel/TreeModel.h`](CuteModel/include/CuteModel/TreeModel.h), [`CuteModel/include/CuteModel/TreeNode.h`](CuteModel/include/CuteModel/TreeNode.h)*

A hierarchical model whose elements are `std::shared_ptr<TreeNode<T>>`. The tree structure is owned by the nodes themselves; the model provides the `QAbstractItemModel` view over it.

`TreeNode<T>`:
- Constructs its `T` payload **in place** (forwarding constructor); non-copyable and non-movable.
- `payload()` / `setPayload(T)` — read and replace the payload. When the node is attached to a model, `setPayload` goes through the model so `dataChanged` is emitted.
- Hierarchy access: `parent()`, `indexInParent()`, `childCount()`, `empty()`, `child(int)`, `children()`, `model()`, `modelIndex(int column = 0)`.
- Mutation (all emit the proper model signals when attached): `push_back_child`, `insert_child`, `append_range`, `insert_range`, `erase_child`, `erase_children`, `clear_children`.

`TreeModel<NodePtr>` (with `node_ptr`, `node_type` and `payload_type` aliases):

```cpp
explicit TreeModel(QObject *parent = nullptr);
explicit TreeModel(QStringList headers, QObject *parent = nullptr); // fixes column count

const node_ptr &root() const noexcept;          // the (payload-less) internal root
node_ptr nodeAt(const QModelIndex &index) const; // shared_ptr from an index
void clear();
```

`insertRows` creates fresh default-constructed nodes (when `payload_type` is default-constructible); `removeRows` detaches the corresponding children. As with the other models, you subclass and override `data(const node_ptr &, index, role)` to project payloads to roles.

### References & typed access

#### `cute::ValueModelAccessor<T>` and `Ref`

*Header: [`CuteModel/include/CuteModel/ValueModelAccessor.h`](CuteModel/include/CuteModel/ValueModelAccessor.h)*

The typed-access interface shared by all source models **and** proxies:

```cpp
virtual const T &getStorageValue(const QModelIndex &index) const = 0;
virtual void setStorageValue(const QModelIndex &index, T value) = 0;

template <typename R = Ref>
std::unique_ptr<R> getRef(const QModelIndex &index); // nullptr for an invalid index
```

The nested `Ref` class derives from `RefBase` and adds the typed accessors `getValue()` / `setValue(const T &)`. Both throw `NullObjectException` if the underlying element no longer exists. `getRef<R>` accepts any `R` derived from `Ref`, so you can hand out application-specific reference types.

#### `cute::RefBase`

*Header: [`CuteModel/include/CuteModel/RefBase.h`](CuteModel/include/CuteModel/RefBase.h)*

A `QObject` that follows one model element via a `QPersistentModelIndex` (`index()` accessor) and translates model signals into element-centric ones:

| Signal | Emitted when |
|---|---|
| `valueChanged()` | the element's data changed (`dataChanged` covering its index) |
| `underlyingHierarchyChanged()` | one of the element's ancestors moved/changed |
| `underlyingValueDestroyed()` | the element disappeared (row/column removed, layout change, model reset) |

This is the type to use in `QObject::connect`; the typed `Ref` subclass adds value access on top.

#### `cute::ItemProxy<T>` and `cute::RangeList<U>`

*Headers: [`CuteModel/include/CuteModel/ItemProxy.h`](CuteModel/include/CuteModel/ItemProxy.h), [`CuteModel/include/CuteModel/RangeList.h`](CuteModel/include/CuteModel/RangeList.h)*

RAII accessors returned by `at()` / `getMutable()` / `iter()` / `iter_mut()`. They give direct access to stored values; the **mutable** variants emit `dataChanged` over the covered indexes when destroyed, so a batch of in-place edits produces exactly one notification:

```cpp
{
    auto range = model.iter_mut(2, 5);
    for (auto &item : range)
        item.touch();
} // one dataChanged(top-left, bottom-right) emitted here
```

`ItemProxy<T>` exposes `operator*`, `operator->`, `operator=(T)` (mutable only) and `index()`. `RangeList<U>` exposes `begin()`, `end()`, `size()`, `empty()`. Instantiations over `const T` are read-only and emit nothing. Both are non-copyable and non-movable — use them as short-lived scoped objects.

### Proxies & selection

#### `cute::RowFilterProxyModel<T>`

*Header: [`CuteModel/include/CuteModel/RowFilterProxyModel.h`](CuteModel/include/CuteModel/RowFilterProxyModel.h)*

A `QSortFilterProxyModel` that filters rows with a **typed predicate** while remaining a `ValueModelAccessor<T>` itself — so `getRef`, `getStorageValue` and `setStorageValue` keep working through the proxy (indexes are mapped to the source automatically).

```cpp
using FilterPredicate = std::function<bool(const T &, const QModelIndex &)>;

explicit RowFilterProxyModel(QObject *parent = nullptr);
explicit RowFilterProxyModel(QAbstractItemModel *source, QObject *parent = nullptr);

void setSourceModel(QAbstractItemModel *source) override; // throws std::invalid_argument
                                                          // if source is not a ValueModelAccessor<T>
ValueModelAccessor<T> *sourceAccessor() const;
void setFilterPredicate(FilterPredicate predicate);
bool hasFilter() const;
void clearFilter();
```

#### `cute::SelectionModel<T>`

*Header: [`CuteModel/include/CuteModel/SelectionModel.h`](CuteModel/include/CuteModel/SelectionModel.h)*

A `QItemSelectionModel` that understands the stored type:

```cpp
enum class SelectionMode { List, Table };

explicit SelectionModel(AbstractSourceModel<T> *model,
                        SelectionMode mode = SelectionMode::List,
                        QObject *parent = nullptr);

std::vector<T>   selectedValues() const;  // requires copy-constructible T
std::optional<T> currentValue() const;    // requires copy-constructible T

template <typename R = Ref> std::vector<std::unique_ptr<R>> selectedRefs() const;
template <typename R = Ref> std::unique_ptr<R>              currentRef() const;

// requires equality-comparable T
void select(const T &value, QItemSelectionModel::SelectionFlags command);
void deselect(const T &value);
bool isSelected(const T &value) const;
```

In `List` mode a multi-column selection is deduplicated to one entry per row (a row is one `T`); in `Table` mode every selected cell counts.

### Utilities & traits

#### `ValueRole` and `is_compatible_with_value_role<T>`

*Header: [`CuteModel/include/CuteModel/ValueRole.h`](CuteModel/include/CuteModel/ValueRole.h)*

```cpp
constexpr int ValueRole = Qt::UserRole + 1;

template <typename T>
struct is_compatible_with_value_role; // default-constructible && destructible && copy-constructible

template <typename T>
constexpr bool is_compatible_with_value_role_v = is_compatible_with_value_role<T>::value;
```

The trait gates every ValueRole touchpoint in `AbstractSourceModel` (`multiData`, `setData`, `roleNames`) with `if constexpr`, making whole-value `QVariant`/QML exposure strictly opt-in by type.

#### Model traversal

*Header: [`CuteModel/include/CuteModel/ModelTraversal.h`](CuteModel/include/CuteModel/ModelTraversal.h)*

Free functions to walk any typed model (source or proxy), depth-first:

```cpp
enum class ColumnPolicy { FirstColumnOnly, AllColumns };

void forEachIndex(const ValueModelAccessor<T> &accessor, const QAbstractItemModel &model,
                  Visitor &&visit, ColumnPolicy columns = ColumnPolicy::AllColumns,
                  const QModelIndex &parent = QModelIndex());
void forEachIndex(const AbstractSourceModel<T> &model, Visitor &&visit, ...); // convenience

std::vector<QModelIndex> indexesMatching(const ValueModelAccessor<T> &accessor,
                                         const QAbstractItemModel &model, Predicate &&pred, ...);
std::vector<QModelIndex> indexesMatching(const AbstractSourceModel<T> &model, Predicate &&pred, ...);
```

Visitors receive `(const T &, const QModelIndex &)`; predicates return `bool`.

#### `cute::Table<T>`

*Header: [`CuteModel/include/CuteModel/Table.h`](CuteModel/include/CuteModel/Table.h)*

The plain (non-Qt) 2D container backing `BasicTableModel`: `rows()`, `columns()`, `at(row, column)`, plus `insert_rows` / `erase_rows` / `resize_rows` and the column equivalents, and `reset`. Usable on its own when you need matrix storage without a model.

#### `cute::NullObjectException`

*Header: [`CuteModel/include/CuteModel/Exceptions.h`](CuteModel/include/CuteModel/Exceptions.h)*

A `std::runtime_error` thrown by `Ref::getValue()` / `Ref::setValue()` when the referenced element no longer exists. Listen to `RefBase::underlyingValueDestroyed()` to avoid hitting it.

## Building

The build uses CMake presets. Qt is consumed from a system install and GTest comes from vcpkg; no absolute path lives in any CMake file.

**Required environment:**
- `VCPKG_ROOT` — path to a vcpkg checkout (used by the toolchain in `CMakePresets.json`).
- Qt is discovered automatically. If it is not on the default search path, set **one** of `QT_ROOT_DIR`, `Qt6_DIR`, `QTDIR`, or `CMAKE_PREFIX_PATH` to the Qt install prefix (e.g. `~/Qt/6.8.3/macos`).

```bash
export VCPKG_ROOT=/path/to/vcpkg
export QT_ROOT_DIR=~/Qt/6.8.3/macos   # only if Qt is not auto-detected

cmake --preset default
cmake --build --preset default
ctest --preset default --output-on-failure
```

**Targets:**

| Target | Description |
|---|---|
| `CuteModel` (alias `CuteModel::CuteModel`) | the static library, installable/exportable for downstream consumption (e.g. via vcpkg) |
| `cutemodel_tests` | GTest suite — option `CUTEMODEL_BUILD_TESTS`, default `ON` |
| `refobject` | console example executable — option `CUTEMODEL_BUILD_EXAMPLES`, default `ON` |
| `widgets_list`, `widgets_table`, `widgets_tree` | Qt Widgets demos for `BasicListModel`, `BasicTableModel` and `TreeModel` — option `CUTEMODEL_BUILD_EXAMPLES` |
| `qml_list`, `qml_table`, `qml_tree` | QML demos for the same three models — option `CUTEMODEL_BUILD_EXAMPLES` |

Each demo app shares the same layout — a filter `LineEdit`, the model view, the
list of selected elements, and a live `Ref` of the current element at the
bottom — built on the shared `cutemodel_examples_common` library
([`Examples/common/`](Examples/common/)). The Widgets and QML variants of a
model reuse the same source model, `RowFilterProxyModel` and view controller.

## Testing

The test suite (in [`Tests/`](Tests/)) covers every public component. Two rules apply throughout:

1. Every model is validated against the `QAbstractItemModel` contract with `QAbstractItemModelTester`.
2. Signal emissions are asserted with `QSignalSpy` — never with counting lambdas.

## Roadmap

Planned but not yet implemented:

- `AggregatedListModel` — a source model aggregating other models instead of elements.
- `RowSortModel` — typed row sorting proxy.
- `FlattenTreeModel` — proxy presenting a tree as a flat list.

## License

[MIT](LICENSE) © 2026 Antoine MORRIER
