import 'dart:convert';
import 'dart:ffi' as ffi;
import 'dart:io' show Directory, Platform;
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

class Box {
  final gdb.GraphDbBindings _bindings;
  final ffi.Pointer<gdb.Box> _handle;
  final String _boxName;

  Box._(this._bindings, this._handle, this._boxName);

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
    return Box._(bindings, handle, boxName);
  }

  Future<void> saveNodes(Node node) async {
    // C++ code expects a JSON array of nodes, not a single object
    final jsonData = jsonEncode([node.toJson()]);
    final ptr = jsonData.toNativeUtf8().cast<ffi.Char>();
    _bindings.graphdb_save_nodes(_handle, ptr);

    try {
      final dir = await getApplicationDocumentsDirectory();
      final dbPath = '${dir.path}/$_boxName';
      final nodesDir = Directory('$dbPath/nodes');
      print('Nodes directory path: ${nodesDir.path}');

      if (await nodesDir.exists()) {
        final files = await nodesDir.list().toList();
        print('Files in "nodes" directory (Proof of save):');
        for (var file in files) {
          print('  - ${file.path.split('/').last}');
        }
      } else {
        print('Nodes directory not found. CRITICAL ERROR.');
      }
    } catch (e) {
      print('Error listing directory content: $e');
    }
    malloc.free(ptr);
  }

  T? loadNode<T>(String nodeId, {required T Function(Map<String, dynamic>) serializer}) {
    final ptr = nodeId.toNativeUtf8().cast<ffi.Char>();
    final resultPtr = _bindings.graphdb_load_node(_handle, ptr);
    malloc.free(ptr);

    if (resultPtr == ffi.nullptr) {
      print('loadNode: Node not found with id: $nodeId');
      return null;
    }
    
    try {
      final result = resultPtr.cast<Utf8>().toDartString();
      final jsonData = jsonDecode(result) as Map<String, dynamic>;
      return serializer(jsonData);
    } catch (e) {
      print('loadNode: Error deserializing node with id $nodeId: $e');
      return null;
    } finally {
      // Always free the pointer, even if an exception occurs
      _bindings.graphdb_free_string(resultPtr);
    }
  }

  Future<void> saveEdges(Edge edge) async {
    // C++ code expects a JSON array of edges, not a single object
    final jsonData = jsonEncode([edge.toJson()]);
    final ptr = jsonData.toNativeUtf8().cast<ffi.Char>();
    _bindings.graphdb_save_edges(_handle, ptr);
    malloc.free(ptr);
  }

  List<T> loadEdges<T>(
    String fromNodeId, {
    required T Function(Map<String, dynamic>) serializer,
  }) {
    final ptr = fromNodeId.toNativeUtf8().cast<ffi.Char>();
    final resultPtr = _bindings.graphdb_load_edges(_handle, ptr);
    malloc.free(ptr);

    if (resultPtr == ffi.nullptr) {
      print('loadEdges: No edges found for node id: $fromNodeId');
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
      print('loadEdges: Error deserializing edges for node id $fromNodeId: $e');
      return [];
    }
  }
}
