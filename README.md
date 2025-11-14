# graph_db

A Flutter FFI plugin for graph database functionality, enabling you to store and query graph structures (nodes and edges) in your Flutter applications.

## ⚠️ Alpha Version

This package is currently in **alpha** status. The API may change in future versions. Use in production environments is not recomended.

## Platform Support

This package is designed to work on the following platforms:
- ✅ **Android** (tested)
- ✅ **iOS** (planned)
- ✅ **Linux** (planned)
- ✅ **macOS** (planned)
- ✅ **Windows** (planned)
- ❌ **Web** (not supported)

**Note:** Currently, only Android has been tested. Support for other platforms (excluding web) is planned but not yet verified.

## Features

- Store and retrieve graph nodes
- Create and query edges between nodes
- Persistent storage using native C++ implementation
- Type-safe API with Dart generics
- JSON serialization support

## Getting Started

### Installation

Add `graph_db` to your `pubspec.yaml`:

```yaml
dependencies:
  graph_db: ^0.0.1
```

### Usage

#### 1. Define your Node and Edge classes

```dart
import 'package:graph_db/domain/node.dart';
import 'package:graph_db/domain/edge.dart';

class PersonNode extends Node {
  final String name;
  final int age;

  PersonNode({
    required String id,
    required this.name,
    required this.age,
  }) : super(id: id);

  @override
  Map<String, dynamic> toJson() {
    return {
      'id': id,
      'name': name,
      'age': age,
    };
  }
}

class FriendshipEdge extends Edge {
  @override
  final String from;
  @override
  final String to;
  @override
  final double weight;

  FriendshipEdge({
    required this.from,
    required this.to,
    this.weight = 1.0,
  });

  @override
  Map<String, dynamic> toJson() {
    return {
      'from': from,
      'to': to,
      'weight': weight,
    };
  }
}
```

#### 2. Initialize the database

```dart
import 'package:graph_db/domain/box.dart';

final box = await Box.init('my_graph_db');
```

#### 3. Save nodes and edges

```dart
// Save nodes
final alice = PersonNode(id: '1', name: 'Alice', age: 30);
final bob = PersonNode(id: '2', name: 'Bob', age: 25);

await box.saveNodes(alice);
await box.saveNodes(bob);

// Save edges
final friendship = FriendshipEdge(from: '1', to: '2', weight: 1.0);
await box.saveEdges(friendship);
```

#### 4. Load nodes and edges

```dart
// Load a node
final person = box.loadNode<PersonNode>(
  '1',
  serializer: (json) => PersonNode(
    id: json['id'] as String,
    name: json['name'] as String,
    age: json['age'] as int,
  ),
);

// Load edges from a node
final edges = box.loadEdges<FriendshipEdge>(
  '1',
  serializer: (json) => FriendshipEdge(
    from: json['from'] as String,
    to: json['to'] as String,
    weight: (json['weight'] as num).toDouble(),
  ),
);
```

## Project Structure

This plugin uses Flutter FFI to interact with native C++ code:

* `src`: Contains the native C++ source code and CMakeLists.txt for building the dynamic library
* `lib`: Contains the Dart code that defines the API and calls into native code using `dart:ffi`
* Platform folders (`android`, `ios`, `windows`, etc.): Contains build files for bundling the native library

## Building Native Code

The native bindings are generated from the header file (`src/graph_db.h`) using `package:ffigen`.

To regenerate the bindings:

```bash
dart run ffigen --config ffigen.yaml
```

## License

This package is free and open source. See [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
