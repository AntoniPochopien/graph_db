/// Abstract base class for edges in the graph database.
///
/// An edge represents a connection between two nodes in the graph.
/// All edges must have a source node ([from]), a target node ([to]),
/// and a weight value. Implement this class to create custom edge types
/// for your graph database.
///
/// Example:
/// ```dart
/// class FriendshipEdge extends Edge {
///   @override
///   final String from;
///
///   @override
///   final String to;
///
///   @override
///   final double weight;
///
///   FriendshipEdge({
///     required this.from,
///     required this.to,
///     this.weight = 1.0,
///   });
///
///   @override
///   Map<String, dynamic> toJson() {
///     return {
///       'from': from,
///       'to': to,
///       'weight': weight,
///     };
///   }
/// }
/// ```
abstract class Edge {
  /// The ID of the source node this edge originates from.
  String get from;

  /// The ID of the target node this edge points to.
  String get to;

  /// The weight of this edge, typically used for graph algorithms.
  double get weight;

  /// Converts this edge to a JSON representation.
  ///
  /// The returned map must include at least 'from', 'to', and 'weight' keys.
  Map<String, dynamic> toJson();
}

/// Factory interface for creating edge instances from JSON data.
///
/// Implement this interface to provide deserialization logic for your
/// custom edge types. This is used by [Box.loadEdges] to convert JSON
/// data back into edge objects.
///
/// Example:
/// ```dart
/// class FriendshipEdgeFactory implements EdgeFactory<FriendshipEdge> {
///   @override
///   FriendshipEdge fromJson(Map<String, dynamic> json) {
///     return FriendshipEdge(
///       from: json['from'] as String,
///       to: json['to'] as String,
///       weight: (json['weight'] as num).toDouble(),
///     );
///   }
/// }
/// ```
abstract class EdgeFactory<T extends Edge> {
  /// Creates an edge instance from JSON data.
  ///
  /// The [json] map should contain the edge data as returned by [Edge.toJson].
  T fromJson(Map<String, dynamic> json);
}

