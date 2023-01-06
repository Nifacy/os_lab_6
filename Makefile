c = compiled/ofiles

all: compiled/main compiled/worker

$(c)/mq.o: src/mq/mq.c
	gcc -c src/mq/mq.c -o $(c)/mq.o

$(c)/queue.o: src/queue/queue.c
	gcc -c src/queue/queue.c -o $(c)/queue.o

$(c)/timer.o: src/timer/timer.c
	gcc -c src/timer/timer.c -o $(c)/timer.o

$(c)/worker.o: src/worker/worker.c
	gcc -c src/worker/worker.c -o $(c)/worker.o

$(c)/worker_tree.o: src/worker_tree/worker_tree.c
	gcc -c src/worker_tree/worker_tree.c -o $(c)/worker_tree.o

$(c)/main.o: src/main.c
	gcc -c src/main.c -o $(c)/main.o

$(c)/node.o: src/node.c
	gcc -c src/node.c -o $(c)/node.o

compiled/main: $(c)/main.o $(c)/worker.o $(c)/mq.o $(c)/worker_tree.o $(c)/queue.o
	gcc $(c)/main.o $(c)/worker.o $(c)/mq.o $(c)/worker_tree.o $(c)/queue.o -lzmq -o compiled/main

compiled/worker: $(c)/node.o $(c)/mq.o $(c)/worker.o $(c)/timer.o
	gcc $(c)/node.o $(c)/mq.o $(c)/worker.o $(c)/timer.o -lzmq -o compiled/worker