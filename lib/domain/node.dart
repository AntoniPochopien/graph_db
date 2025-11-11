abstract class Node {
  final String id;

  const Node({required this.id});

  Map<String, dynamic> toJson();
}

abstract class NodeFactory<T extends Node> {
  T fromJson(Map<String, dynamic> json);
}
