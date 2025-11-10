import 'dart:convert';

import 'package:flutter/material.dart';
import 'dart:async';

import 'package:graph_db/graph_db.dart';

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
    _box.saveNodes(jsonEncode([
      {'id': "1", 'properties': {'test': 'test'}}
    ]));
    final x =_box.loadNode("1");
    print('fds: $x');
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
