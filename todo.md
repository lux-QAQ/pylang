对于test.py python的正常输出为:
(base) lux@Lux:~/code/language/python-cpp$ python test.py
Running verify()...
Sieve initialized with limit: 100
Starting sieve calculation...
Sieve loops finished, omitting squares...
Searching for prefix: 2
Total primes found: 25
Base primes check for 100: True True True
Generating trie for primes, list size: 25
Trie generation complete.
Navigating to prefix node...
Link check for char: 2
Prefix node found. Terminal state of prefix node: True
Starting queue traversal. Initial queue size: 1
Queue pop: 2 Terminal: True
Found children chars at 2 : ['3', '9']
Pushing to queue: 23
Pushing to queue: 29
Queue pop: 23 Terminal: True
Queue pop: 29 Terminal: True
Find completed. Results count: 3 Values: [2, 23, 29]
Verify results match: True
Expected: [2, 23, 29]
Actual: [2, 23, 29]
Starting main calculation (UPPER_BOUND = 5000000 )
Sieve initialized with limit: 5000000
Starting sieve calculation...
Processing loop_x, x = 500
Processing loop_x, x = 1000
Processing loop_x, x = 1500
Processing loop_x, x = 2000
Sieve loops finished, omitting squares...
Searching for prefix: 32338
Total primes found: 348513
Generating trie for primes, list size: 348513
Trie generation complete.
Navigating to prefix node...
Link check for char: 3
Link check for char: 2
Link check for char: 3
Link check for char: 3
Link check for char: 8
Prefix node found. Terminal state of prefix node: False
Starting queue traversal. Initial queue size: 1
Queue pop: 32338 Terminal: False
Found children chars at 32338 : ['1', '3', '0', '5', '6', '7', '8', '9']
Pushing to queue: 323381
Pushing to queue: 323383
Pushing to queue: 323380
Pushing to queue: 323385
Pushing to queue: 323386
Pushing to queue: 323387
Pushing to queue: 323388
Pushing to queue: 323389
Queue pop: 323381 Terminal: True
Queue pop: 323383 Terminal: True
Queue pop: 323380 Terminal: False
Found children chars at 323380 : ['3', '9']
Pushing to queue: 3233803
Pushing to queue: 3233809
Queue pop: 323385 Terminal: False
Found children chars at 323385 : ['1']
Pushing to queue: 3233851
Queue pop: 323386 Terminal: False
Found children chars at 323386 : ['3']
Pushing to queue: 3233863
Queue pop: 323387 Terminal: False
Found children chars at 323387 : ['3']
Pushing to queue: 3233873
Queue pop: 323388 Terminal: False
Found children chars at 323388 : ['7']
Pushing to queue: 3233887
Queue pop: 323389 Terminal: False
Found children chars at 323389 : ['7']
Pushing to queue: 3233897
Queue pop: 3233803 Terminal: True
Queue pop: 3233809 Terminal: True
Queue pop: 3233851 Terminal: True
Queue pop: 3233863 Terminal: True
Queue pop: 3233873 Terminal: True
Queue pop: 3233887 Terminal: True
Queue pop: 3233897 Terminal: True
Find completed. Results count: 9 Values: [323381, 323383, 3233803, 3233809, 3233851, 3233863, 3233873, 3233887, 3233897]
Final Results: [323381, 323383, 3233803, 3233809, 3233851, 3233863, 3233873, 3233887, 3233897]

而使用我的编译器编译test.py执行却有问题:
(base) lux@Lux:~/code/language/python-cpp$ /home/lux/code/language/python-cpp/build/debug/src/pyc ./test.py
[09:28:28.072] [compiler] [info] Parsing './test.py'...
[09:28:28.172] [compiler] [info] TargetMachine: x86_64-pc-linux-gnu (opt=O2)
[09:28:28.172] [compiler] [info] Using separated runtime compilation (cached object: /tmp/pylang_runtime_cache.o)
[09:28:36.482] [linker] [info] Parsed 107 annotations, skipped 0
[09:28:36.482] [compiler] [info] SimpleDriver created (opt: O2, separate_link: true)
[2026-03-15 09:28:36.482] [info] [timer] SimpleDriver::create: 8310.70ms
[09:28:36.483] [compiler] [info] Stage 1: Codegen for 'test'
[09:28:36.486] [codegen] [info] Module 'test' compiled successfully
[2026-03-15 09:28:36.486] [info] [timer] stage_codegen: 3.32ms
[09:28:36.486] [compiler] [info] Stage 2: Skipping IR linking (separate compilation active)
[2026-03-15 09:28:36.486] [info] [timer] stage_link_runtime: 0.00ms
[09:28:36.486] [compiler] [info] Stage 3: Optimizing 'test' at O2
[09:28:36.507] [opt] [info] Optimization complete
[2026-03-15 09:28:36.507] [info] [timer] stage_optimize: 21.57ms
[09:28:36.507] [compiler] [info] Stage 4b: Emitting object file → /tmp/test.o
[09:28:36.543] [compiler] [info] Object file written: /tmp/test.o
[2026-03-15 09:28:36.544] [info] [timer] stage_to_object: 36.04ms
[09:28:36.544] [compiler] [info] Stage 5: Linking → /tmp/test
[09:28:36.675] [compiler] [info] Executable written: /tmp/test
[2026-03-15 09:28:36.675] [info] [timer] stage_link_executable: 131.88ms
Build successful: /tmp/test
(base) lux@Lux:~/code/language/python-cpp$ /tmp/test
Running verify()...
Sieve initialized with limit: 100
Starting sieve calculation...
Sieve loops finished, omitting squares...
Searching for prefix: 2
Total primes found: 25
[2026-03-15 09:28:54.594] [error] Unhandled exception: 'int' object is not iterable

这可能是codegen的问题也可能是是runtime的问题请找到问题并修复它