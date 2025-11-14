import 'dart:convert';
import 'dart:developer';
import 'dart:ffi' as ffi;
import 'dart:io' show Platform;
import 'package:ffi/ffi.dart';
import 'package:graph_db/domain/node.dart';
import 'package:graph_db/domain/edge.dart';
import 'package:graph_db/graph_db_bindings_generated.dart' as gdb;
import 'package:path_provider/path_provider.dart';

const String _libName = 'graph_db';

final ffi.DynamicLibrary _dylib = () {
  if (Platform.isMacOS || Platform.isIOS) {
    return ffi.DynamicLibrary.open('$_libName.framework/$_libName');
  }
  if (Platform.isAndroid || Platform.isLinux) {
    return ffi.DynamicLibrary.open('lib$_libName.so');
  }
  if (Platform.isWindows) {
    return ffi.DynamicLibrary.open('$_libName.dll');
  }
  throw UnsupportedError('Unknown platform: ${Platform.operatingSystem}');
}();

/// A graph database box that provides storage and retrieval of nodes and edges.
///
/// The [Box] class is the main interface for interacting with the graph database.
/// It provides methods to save and load nodes and edges, enabling you to build
/// and query graph structures in your Flutter application.
///
/// Example:
/// ```dart
/// final box = await Box.init('my_graph_db');
/// await box.saveNodes(myNode);
/// final node = box.loadNode('node_id', serializer: MyNode.fromJson);
/// ```
class Box {
  final gdb.GraphDbBindings _bindings;
  final ffi.Pointer<gdb.Box> _handle;

  Box._(this._bindings, this._handle);

  /// Initializes a new graph database box with the specified name.
  ///
  /// The database will be stored in the application's documents directory
  /// with the provided [boxName]. If a database with this name already exists,
  /// it will be opened; otherwise, a new database will be created.
  ///
  /// Throws an [Exception] if the database initialization fails.
  ///
  /// Example:
  /// ```dart
  /// final box = await Box.init('my_graph');
  /// ```
  static Future<Box> init(String boxName) async {
    final dir = await getApplicationDocumentsDirectory();
    final dbPath = '${dir.path}/$boxName';

    final dbPathPtr = dbPath.toNativeUtf8().cast<ffi.Char>();

    final bindings = gdb.GraphDbBindings(_dylib);

    final handle = bindings.graphdb_init(dbPathPtr);

    malloc.free(dbPathPtr);

    if (handle == ffi.nullptr) {
      throw Exception('Failed to initialize GraphDB at path $dbPath');
    }
    return Box._(bindings, handle);
  }

  /// Saves a node to the graph database.
  ///
  /// The [node] will be persisted to the database and can be retrieved later
  /// using [loadNode]. The node must implement the [Node] interface and
  /// provide a valid [Node.toJson] method.
  ///
  /// Example:
  /// ```dart
  /// final node = MyNode(id: '1', name: 'Alice');
  /// await box.saveNodes(node);
  /// ```
  Future<void> saveNodes(Node node) async {
    // C++ code expects a JSON array of nodes, not a single object
    final jsonData = jsonEncode([node.toJson()]);
    final ptr = jsonData.toNativeUtf8().cast<ffi.Char>();
    _bindings.graphdb_save_nodes(_handle, ptr);

    malloc.free(ptr);
  }

  /// Loads a node from the graph database by its ID.
  ///
  /// Returns the deserialized node if found, or `null` if no node with the
  /// given [nodeId] exists in the database.
  ///
  /// The [serializer] function is used to convert the JSON data back into
  /// your custom node type. It should match the structure of your node's
  /// `fromJson` method.
  ///
  /// Example:
  /// ```dart
  /// final node = box.loadNode<MyNode>(
  ///   'node_id',
  ///   serializer: (json) => MyNode.fromJson(json),
  /// );
  /// ```
  ///
  /// Returns `null` if the node is not found or if deserialization fails.
  T? loadNode<T>(String nodeId, {required T Function(Map<String, dynamic>) serializer}) {
    final ptr = nodeId.toNativeUtf8().cast<ffi.Char>();
    final resultPtr = _bindings.graphdb_load_node(_handle, ptr);
    malloc.free(ptr);

    if (resultPtr == ffi.nullptr) {
      log('loadNode: Node not found with id: $nodeId');
      return null;
    }
    
    try {
      final result = resultPtr.cast<Utf8>().toDartString();
      final jsonData = jsonDecode(result) as Map<String, dynamic>;
      return serializer(jsonData);
    } catch (e) {
      log('loadNode: Error deserializing node with id $nodeId: $e');
      return null;
    } finally {
      // Always free the pointer, even if an exception occurs
      _bindings.graphdb_free_string(resultPtr);
    }
  }

  /// Saves an edge to the graph database.
  ///
  /// The [edge] represents a connection between two nodes and will be
  /// persisted to the database. Edges can be retrieved using [loadEdges]
  /// by querying from a specific node. The edge must implement the [Edge]
  /// interface and provide a valid [Edge.toJson] method.
  ///
  /// Example:
  /// ```dart
  /// final edge = MyEdge(from: 'node1', to: 'node2', weight: 1.0);
  /// await box.saveEdges(edge);
  /// ```
  Future<void> saveEdges(Edge edge) async {
    // C++ code expects a JSON array of edges, not a single object
    final jsonData = jsonEncode([edge.toJson()]);
    final ptr = jsonData.toNativeUtf8().cast<ffi.Char>();
    _bindings.graphdb_save_edges(_handle, ptr);
    malloc.free(ptr);
  }

  /// Loads all edges originating from a specific node.
  ///
  /// Returns a list of all edges that start from the node with the given
  /// [fromNodeId]. If no edges are found, an empty list is returned.
  ///
  /// The [serializer] function is used to convert the JSON data back into
  /// your custom edge type. It should match the structure of your edge's
  /// `fromJson` method.
  ///
  /// Example:
  /// ```dart
  /// final edges = box.loadEdges<MyEdge>(
  ///   'node_id',
  ///   serializer: (json) => MyEdge.fromJson(json),
  /// );
  /// ```
  ///
  /// Returns an empty list if no edges are found or if deserialization fails.
  List<T> loadEdges<T>(
    String fromNodeId, {
    required T Function(Map<String, dynamic>) serializer,
  }) {
    final ptr = fromNodeId.toNativeUtf8().cast<ffi.Char>();
    final resultPtr = _bindings.graphdb_load_edges(_handle, ptr);
    malloc.free(ptr);

    if (resultPtr == ffi.nullptr) {
      log('loadEdges: No edges found for node id: $fromNodeId');
      return [];
    }

    try {
      final result = resultPtr.cast<Utf8>().toDartString();
      _bindings.graphdb_free_string(resultPtr);
      final jsonData = jsonDecode(result) as List<dynamic>;
      return jsonData
          .map((item) => serializer(item as Map<String, dynamic>))
          .toList();
    } catch (e) {
      _bindings.graphdb_free_string(resultPtr);
      log('loadEdges: Error deserializing edges for node id $fromNodeId: $e');
      return [];
    }
  }
}
