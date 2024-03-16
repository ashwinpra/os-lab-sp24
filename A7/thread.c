/*
    all threads have same pid and ppid, but different tid (gettid())
    main thread - leader 
    main thread + all other threads - thread group

    first problem - if all threads run with same PID, and for some reason leader thread exits, all other threads will also exit
    exits of all threads must be synchronized - leader thread must not exit before all other threads have exited
    we cannot use wait() or waitpid() as they all have same PID
    have to make thread_exit() function to fix this problem - will need some kind of synchronization

    also need to design mutexes and barriers
    main diff between semaphore and mutex - only same thread can unlock mutex, but any thread can unlock semaphore
    also need to make sure semaphores are not unlocked more than they are locked - keep it binary
    attempt to unlock a semaphore that is already unlocked should raise an error


*/