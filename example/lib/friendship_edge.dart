import 'package:graph_db/graph_db.dart';

class FriendshipEdge implements Edge {
  @override
  final String from;
  @override
  final String to;
  @override
  final double weight;
  final DateTime createdAt;
  final String status;

  const FriendshipEdge({
    required this.from,
    required this.to,
    required this.weight,
    required this.createdAt,
    required this.status,
  });

  @override
  Map<String, dynamic> toJson() => {
        'from': from,
        'to': to,
        'weight': weight,
        'properties': {
          'createdAt': createdAt.toIso8601String(),
          'status': status,
        },
      };
}

class FriendshipEdgeFactory implements EdgeFactory<FriendshipEdge> {
  @override
  FriendshipEdge fromJson(Map<String, dynamic> json) {
    final from = json['from'] as String?;
    final to = json['to'] as String?;
    final weight = json['weight'] as double?;
    final properties = json['properties'] as Map<String, dynamic>?;

    if (from == null || to == null || weight == null || properties == null) {
      throw FormatException(
        'Invalid FriendshipEdge JSON: missing required fields. '
        'from: $from, to: $to, weight: $weight, properties: $properties',
      );
    }

    final createdAtStr = properties['createdAt'] as String?;
    final status = properties['status'] as String?;

    if (createdAtStr == null || status == null) {
      throw FormatException(
        'Invalid FriendshipEdge JSON: missing required properties. '
        'createdAt: $createdAtStr, status: $status',
      );
    }

    return FriendshipEdge(
      from: from,
      to: to,
      weight: weight,
      createdAt: DateTime.parse(createdAtStr),
      status: status,
    );
  }
}


