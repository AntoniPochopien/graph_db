import 'package:graph_db/graph_db.dart';

class UserModel implements Node {
  @override
  final String id;
  final String username;
  final int age;

  const UserModel({
    required this.id,
    required this.age,
    required this.username,
  });

  @override
  Map<String, dynamic> toJson() => {
        'id': id,
        'properties': {
          'username': username,
          'age': age,
        },
      };
}

class UserModelFactory implements NodeFactory<UserModel> {
  @override
  UserModel fromJson(Map<String, dynamic> json) {
    final id = json['id'] as String?;
    final properties = json['properties'] as Map<String, dynamic>?;
    
    if (id == null || properties == null) {
      throw FormatException(
        'Invalid UserModel JSON: missing required fields. '
        'id: $id, properties: $properties',
      );
    }
    
    final username = properties['username'] as String?;
    final age = properties['age'] as int?;
    
    if (username == null || age == null) {
      throw FormatException(
        'Invalid UserModel JSON: missing required properties. '
        'username: $username, age: $age',
      );
    }
    
    return UserModel(
      id: id,
      username: username,
      age: age,
    );
  }
}
