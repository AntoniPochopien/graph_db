abstract class Edge {
  String get from;
  String get to;
  double get weight;

  Map<String, dynamic> toJson();
}

abstract class EdgeFactory<T extends Edge> {
  T fromJson(Map<String, dynamic> json);
}

