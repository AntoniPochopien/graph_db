import 'dart:ffi' as ffi;
import 'dart:io' show Directory, Platform;
import 'package:ffi/ffi.dart';
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

final _bindings = gdb.GraphDbBindings(_dylib);

class Box {
  final gdb.GraphDbBindings _bindings;
  final ffi.Pointer<gdb.Box> _handle;

  Box._(this._bindings, this._handle);

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

  void saveNodes(String jsonData) async {
    final ptr = jsonData.toNativeUtf8().cast<ffi.Char>();
    _bindings.graphdb_save_nodes(_handle, ptr);

    try {
      final dir = await getApplicationDocumentsDirectory();
      final dbPath = '${dir.path}/test';
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

  String? loadNode(String nodeId) {
    final ptr = nodeId.toNativeUtf8().cast<ffi.Char>();
    final resultPtr = _bindings.graphdb_load_node(_handle, ptr);
    malloc.free(ptr);

    if (resultPtr == ffi.nullptr) {
      print('loadNode: Received nullptr from C++ for nodeId: $nodeId');
      return null;
    }
    final result = resultPtr.cast<Utf8>().toDartString();
    _bindings.graphdb_free_string(resultPtr);
    return result;
  }
}
