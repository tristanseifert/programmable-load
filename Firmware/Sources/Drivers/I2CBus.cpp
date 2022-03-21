#include "I2CBus.h"

#include "Log/Logger.h"

using namespace Drivers;

/**
 * @brief Validate the provided transactions
 *
 * Ensure that various invariants in each transaction are met, such as the continuation flag, and
 * that buffers are the correct size.
 *
 * @param transactions List of transactions to validate
 *
 * @return 0 on success, or a negative error code
 */
int I2CBus::ValidateTransactions(etl::span<const Transaction> &transactions) {
    for(size_t i = 0; i < transactions.size(); i++) {
        const auto &txn = transactions[i];

        // transaction length may not be 0
        if(!txn.length) {
            return -1000;
        }
        // buffer must be valid
        if(!txn.data.data() || txn.data.empty()) {
            return -1001;
        }

        // validate continuation
        if(txn.continuation) {
            // may not be first entry
            if(!i) {
                return -1002;
            }
            // ensure the previous entry is the same address and rw mode
            const auto &prev = transactions[i - 1];
            if(prev.address != txn.address || prev.read != txn.read) {
                return -1003;
            }
        }
    }

    return 0;
}

