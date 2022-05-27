//
// Created by ziyan on 22-5-10.
//
#include <mutex>
typedef std::mutex Mutex;

class BankAccount {
private:
    Mutex mu;
    int   balance GUARDED_BY(mu);

    void depositImpl(int amount) {
        balance += amount;       // WARNING! Cannot write balance without locking mu.
    }

    void withdrawImpl(int amount) EXCLUSIVE_LOCKS_REQUIRED(mu) {
            balance -= amount;       // OK. Caller must have locked mu.
    }

public:
    void withdraw(int amount) {
        mu.Lock();
        withdrawImpl(amount);    // OK.  We've locked mu.
    }                          // WARNING!  Failed to unlock mu.

    void transferFrom(BankAccount& b, int amount) {
        mu.Lock();
        b.withdrawImpl(amount);  // WARNING!  Calling withdrawImpl() requires locking b.mu.
        depositImpl(amount);     // OK.  depositImpl() has no requirements.
        mu.Unlock();
    }
};