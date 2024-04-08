all: 
	gcc  master.c -o master
	gcc  mmu.c -o mmu
	gcc  sched.c -o scheduler
	gcc  process.c -o process
	./master

clean:
	-rm master mmu scheduler process result.txt
	ipcrm --all=shm 
	ipcrm --all=sem 
	ipcrm --all=msg
