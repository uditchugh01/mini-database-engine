# MiniDB

A small relational database engine written from scratch in C++ (no external libraries, no SQLite).
It parses a subset of SQL, stores data on disk, and speeds up lookups with a hand-written **B-tree index**.

I built it to understand how databases work internally: parsing, storage, indexing and query execution.

## Features

- **SQL subset**: `CREATE TABLE`, `INSERT`, `SELECT`, `UPDATE`, `DELETE`, `CREATE INDEX`
- **`WHERE` clauses** with `=`, `!=`, `<`, `>`, `<=`, `>=`
- **Data types**: `INT` and `TEXT`, with type checking on every command
- **Hand-written tokenizer and recursive-descent parser** (case-insensitive keywords)
- **B-tree index** on `INT` columns: used for `=`, `<`, `>`, `<=`, `>=`
- **Persistent storage**: data is saved to disk after every change and reloaded on start
- **Crash-safe saving**: writes to a temporary file first, then replaces the old file
- **Corruption handling**: a damaged data file is moved aside instead of being overwritten
- **Clear error messages** for syntax errors, unknown tables/columns and type mismatches

## Example session

```
minidb> CREATE TABLE students (id INT, name TEXT)
Table 'students' created.

minidb> INSERT INTO students VALUES (1, "Udit Kumar")
1 row inserted.

minidb> INSERT INTO students VALUES (2, "Rahul")
1 row inserted.

minidb> INSERT INTO students VALUES (3, "Aman")
1 row inserted.

minidb> CREATE INDEX ON students (id)
Index created on students(id): 3 row(s), tree height 1.

minidb> SELECT * FROM students WHERE id >= 2
id	name
-------------------------
2	Rahul
3	Aman
(2 row(s)) [used index]

minidb> UPDATE students SET name = "Rahul Sharma" WHERE id = 2
1 row(s) updated.

minidb> DELETE FROM students WHERE id = 1
1 row(s) deleted.
```

## Build and run

Requires a C++17 compiler (g++ or clang++).

```bash
make            # builds ./minidb and ./benchmark
./minidb
```

Without `make`:

```bash
g++ -std=c++17 -O2 main.cpp tokenizer.cpp table.cpp database.cpp utils.cpp btree.cpp -o minidb
```

Type `exit` to quit. Data is stored in `minidb.dat` in the folder you run the program from.

## Supported commands

```sql
CREATE TABLE name (col INT, col TEXT, ...)
CREATE INDEX ON name (int_column)
INSERT INTO name VALUES (1, "text")
SELECT * FROM name [WHERE col op value]
SELECT col1, col2 FROM name [WHERE col op value]
UPDATE name SET col = value [WHERE col op value]
DELETE FROM name [WHERE col op value]
```

`TEXT` values go inside double quotes. `UPDATE` and `DELETE` without `WHERE` affect every row.

## Project structure

| File | Responsibility |
|---|---|
| `main.cpp` | Input loop |
| `tokenizer.*` | Splits a command into tokens (identifiers, numbers, strings, symbols) |
| `database.*` | Parser and executor, plus saving and loading the data file |
| `table.*` | Table storage, `WHERE` evaluation, index maintenance |
| `btree.*` | B-tree used for indexes |
| `utils.*` | Small helper functions |
| `benchmark.cpp` | Full scan vs index benchmark |

**Flow of a command:** text → `tokenize` → tokens → `Database::execute` (parser picks the command)
→ `Table` operations (uses the B-tree when possible) → auto-save to disk.

## How the index works

`CREATE INDEX ON students (id)` builds a B-tree that maps each `id` to the positions of the rows having it.

- Every node holds up to 31 sorted keys, so the tree stays very shallow
  (a few hundred thousand rows need only about 5 levels).
- A lookup walks from the root to a leaf, so it takes **O(log n)** steps instead of scanning every row (**O(n)**).
- Nodes are split when they become full, which keeps the tree balanced.
- Duplicate values are supported (each key stores a list of row positions).
- Range conditions (`<`, `>`, `<=`, `>=`) walk the tree in order and skip subtrees outside the range.
- The tree owns raw pointers, so it implements the Rule of Three (destructor, copy constructor, copy assignment).

## Benchmark

Run `./benchmark` (built with `-O2`). It performs 100 lookups of the form `SELECT * FROM t WHERE id = <value>`.

Measured on: **<your CPU / laptop, OS>**

| Rows | Full scan (per lookup) | With index (per lookup) | Speed-up |
|---|---|---|---|
| 10,000 | 6.66 ms | 0.0047 ms | ~1430x |
| 100,000 | 5.529 ms | 0.0201 | ~393.579 |

A full scan grows linearly with the table size, while the indexed lookup stays almost flat.

## Data file format

A plain-text file, one item per line, so it is easy to inspect and debug:
a header (`MINIDB 2`), then for each table: its name, columns (name and type), rows, and the list of indexed columns.
Index trees are not stored; they are rebuilt when the file is loaded.

## Limitations and future work

- Only `INT` and `TEXT` types; no negative numbers or `FLOAT` yet
- Only one condition per `WHERE` (no `AND` / `OR`)
- No `JOIN`, `ORDER BY` or `DROP TABLE`
- Only `INT` columns can be indexed
- Deleting rows rebuilds the indexes of that table (a real B-tree delete would be faster)
- The whole database is rewritten on every change, which is fine for small data but not for large databases
- Single user, no transactions

## What I learned

- Writing a tokenizer and a recursive-descent parser
- Designing a multi-file C++ project with headers, include guards and encapsulation
- Implementing a B-tree (node splitting, range search) and managing raw-pointer memory correctly
- File I/O, safe writes and handling corrupted input
- Measuring performance and comparing O(n) with O(log n) on real data