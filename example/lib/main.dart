import 'package:flutter/material.dart';
import 'dart:async';

import 'package:graph_db/graph_db.dart';
import 'package:graph_db_example/user_model.dart';
import 'package:graph_db_example/friendship_edge.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  late final Box _box;
  String _output = 'Initializing...';

  @override
  void initState() {
    super.initState();
    _initializeAndDemo();
  }

  Future<void> _initializeAndDemo() async {
    try {
      _box = await Box.init('graph_db_example');
      await _demonstrateNodesAndEdges();
    } catch (e) {
      setState(() {
        _output = 'Error: $e';
      });
    }
  }

  Future<void> _demonstrateNodesAndEdges() async {
    final output = StringBuffer();
    output.writeln('=== Graph Database Demo ===\n');

    // ========== NODE DEMONSTRATION ==========
    output.writeln('--- Creating Users (Nodes) ---');
    
    // Create multiple users
    final alice = UserModel(id: 'alice', age: 25, username: 'Alice');
    final bob = UserModel(id: 'bob', age: 30, username: 'Bob');
    final charlie = UserModel(id: 'charlie', age: 28, username: 'Charlie');
    
    await _box.saveNodes(alice);
    output.writeln('✓ Saved user: ${alice.username} (id: ${alice.id})');
    
    await _box.saveNodes(bob);
    output.writeln('✓ Saved user: ${bob.username} (id: ${bob.id})');
    
    await _box.saveNodes(charlie);
    output.writeln('✓ Saved user: ${charlie.username} (id: ${charlie.id})');
    
    output.writeln('');

    // Load and display users
    output.writeln('--- Loading Users (Nodes) ---');
    
    final loadedAlice = _box.loadNode<UserModel>(
      'alice',
      serializer: UserModelFactory().fromJson,
    );
    if (loadedAlice != null) {
      output.writeln('✓ Loaded: ${loadedAlice.username} (age: ${loadedAlice.age})');
    } else {
      output.writeln('✗ Failed to load Alice');
    }

    final loadedBob = _box.loadNode<UserModel>(
      'bob',
      serializer: UserModelFactory().fromJson,
    );
    if (loadedBob != null) {
      output.writeln('✓ Loaded: ${loadedBob.username} (age: ${loadedBob.age})');
    } else {
      output.writeln('✗ Failed to load Bob');
    }

    // Test updating a node (should delete old and save new)
    output.writeln('');
    output.writeln('--- Updating User (Node Update) ---');
    final updatedAlice = UserModel(id: 'alice', age: 26, username: 'Alice Updated');
    await _box.saveNodes(updatedAlice);
    output.writeln('✓ Updated Alice (old version deleted, new version saved)');
    
    final reloadedAlice = _box.loadNode<UserModel>(
      'alice',
      serializer: UserModelFactory().fromJson,
    );
    if (reloadedAlice != null) {
      output.writeln('✓ Reloaded: ${reloadedAlice.username} (new age: ${reloadedAlice.age})');
    }

    output.writeln('');

    // ========== EDGE DEMONSTRATION ==========
    output.writeln('--- Creating Friendships (Edges) ---');
    
    // Create friendship edges
    final friendship1 = FriendshipEdge(
      from: 'alice',
      to: 'bob',
      weight: 1.0,
      createdAt: DateTime.now(),
      status: 'active',
    );
    
    final friendship2 = FriendshipEdge(
      from: 'alice',
      to: 'charlie',
      weight: 0.8,
      createdAt: DateTime.now().subtract(const Duration(days: 5)),
      status: 'active',
    );
    
    final friendship3 = FriendshipEdge(
      from: 'bob',
      to: 'charlie',
      weight: 0.9,
      createdAt: DateTime.now().subtract(const Duration(days: 10)),
      status: 'active',
    );
    
    await _box.saveEdges(friendship1);
    output.writeln('✓ Created friendship: ${friendship1.from} -> ${friendship1.to} (weight: ${friendship1.weight})');
    
    await _box.saveEdges(friendship2);
    output.writeln('✓ Created friendship: ${friendship2.from} -> ${friendship2.to} (weight: ${friendship2.weight})');
    
    await _box.saveEdges(friendship3);
    output.writeln('✓ Created friendship: ${friendship3.from} -> ${friendship3.to} (weight: ${friendship3.weight})');
    
    output.writeln('');

    // Load edges from a node
    output.writeln('--- Loading Friendships (Edges) from Alice ---');
    
    final aliceFriendships = _box.loadEdges<FriendshipEdge>(
      'alice',
      serializer: FriendshipEdgeFactory().fromJson,
    );
    
    output.writeln('✓ Found ${aliceFriendships.length} friendships from Alice:');
    for (final edge in aliceFriendships) {
      output.writeln('  - Alice -> ${edge.to} (weight: ${edge.weight}, status: ${edge.status})');
      output.writeln('    Created: ${edge.createdAt.toLocal()}');
    }

    output.writeln('');
    output.writeln('--- Loading Friendships (Edges) from Bob ---');
    
    final bobFriendships = _box.loadEdges<FriendshipEdge>(
      'bob',
      serializer: FriendshipEdgeFactory().fromJson,
    );
    
    output.writeln('✓ Found ${bobFriendships.length} friendships from Bob:');
    for (final edge in bobFriendships) {
      output.writeln('  - Bob -> ${edge.to} (weight: ${edge.weight}, status: ${edge.status})');
    }

    output.writeln('');
    output.writeln('=== Demo Complete ===');

    setState(() {
      _output = output.toString();
    });
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: const Text('Graph Database Demo'),
          backgroundColor: Colors.blue,
        ),
        body: SingleChildScrollView(
          padding: const EdgeInsets.all(16.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Text(
                'Graph Database Example',
                style: TextStyle(
                  fontSize: 24,
                  fontWeight: FontWeight.bold,
                ),
              ),
              const SizedBox(height: 16),
              const Text(
                'This demo shows how to use Nodes and Edges:',
                style: TextStyle(fontSize: 16),
              ),
              const SizedBox(height: 8),
              const Text('• Nodes represent entities (e.g., Users)'),
              const Text('• Edges represent relationships (e.g., Friendships)'),
              const SizedBox(height: 16),
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: Colors.grey[200],
                  borderRadius: BorderRadius.circular(8),
                ),
                child: SelectableText(
                  _output,
                  style: const TextStyle(
                    fontFamily: 'monospace',
                    fontSize: 12,
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
