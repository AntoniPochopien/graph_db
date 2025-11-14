/// Abstract base class for nodes in the graph database.
///
/// A node represents a vertex in the graph structure. All nodes must have
/// a unique identifier ([id]) and be able to serialize themselves to JSON.
/// Implement this class to create custom node types for your graph database.
///
/// Example:
/// ```dart
/// class PersonNode extends Node {
///   final String name;
///   final int age;
///
///   PersonNode({
///     required String id,
///     required this.name,
///     required this.age,
///   }) : super(id: id);
///
///   @override
///   Map<String, dynamic> toJson() {
///     return {
///       'id': id,
///       'name': name,
///       'age': age,
///     };
///   }
/// }
/// ```
abstract class Node {
  /// The unique identifier for this node.
  ///
  /// This ID is used to reference the node when creating edges and
  /// when loading nodes from the database.
  final String id;

  /// Creates a new node with the specified [id].
  const Node({required this.id});

  /// Converts this node to a JSON representation.
  ///
  /// The returned map must include at least the 'id' key.
  Map<String, dynamic> toJson();
}

/// Factory interface for creating node instances from JSON data.
///
/// Implement this interface to provide deserialization logic for your
/// custom node types. This is used by [Box.loadNode] to convert JSON
/// data back into node objects.
///
/// Example:
/// ```dart
/// class PersonNodeFactory implements NodeFactory<PersonNode> {
///   @override
///   PersonNode fromJson(Map<String, dynamic> json) {
///     return PersonNode(
///       id: json['id'] as String,
///       name: json['name'] as String,
///       age: json['age'] as int,
///     );
///   }
/// }
/// ```
abstract class NodeFactory<T extends Node> {
  /// Creates a node instance from JSON data.
  ///
  /// The [json] map should contain the node data as returned by [Node.toJson].
  T fromJson(Map<String, dynamic> json);
}
