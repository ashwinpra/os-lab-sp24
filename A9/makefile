run: 
	gcc rsync.c -o rsync 
	./rsync test1 test2 

clean: 
	rm rsync

reset: 
	rm -rf test1 test2
	cp -R test_dirs/test1 test_dirs/test2 . 