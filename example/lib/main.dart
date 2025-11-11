import 'package:flutter/material.dart';
import 'dart:async';

import 'package:graph_db/graph_db.dart';
import 'package:graph_db_example/user_model.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  final _controller = TextEditingController();
  late final Box _box;

  @override
  void initState() {
    super.initState();
    x();
  }

  Future<void> x() async {
    _box = await Box.init('test');
    final user = UserModel(id: 'test', age: 10, username: 'super name');
    await _box.saveNodes(user);
    final x = _box.loadNode<UserModel>(
      "hkjh",
      serializer: UserModelFactory().fromJson,
    );
    if (x == null) {
      print('Node not found with id: test');
    } else {
      print(x.id);
      print(x.username);
      print(x.age);
    }
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(title: const Text('Native Packages')),
        body: Column(
          children: [
            TextFormField(controller: _controller),
            ElevatedButton(onPressed: () {}, child: Text('zapisz node')),
          ],
        ),
      ),
    );
  }
}
