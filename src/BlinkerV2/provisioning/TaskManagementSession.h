#ifndef BLINKER_PROVISIONING_TASKMANAGEMENTSESSION_H
#define BLINKER_PROVISIONING_TASKMANAGEMENTSESSION_H

#include "TaskAuthorityContract.h"
#include "TaskManagementContract.h"
#include "../runtime/TaskEngine.h"
#include "../interface/IClock.h"
#include "../interface/IRandom.h"

namespace blinker {

// ONE optional pending slot for management and retirement. No queue, heap,
// transport or raw credential ownership. Call only from the cooperative owner,
// never native receive/property callbacks. poll ages TTL even while offline.
// channel is a trusted nonzero local handle unique to transport session AND
// routed peer; NOT a wire-supplied account/session number or authorization.
// Adapter must cancel on disconnect/peer retirement and validate admission.
class TaskManagementSession {
public:
    TaskManagementSession(TaskEngine& engine, TaskAuthorityCache& authority,
        IRandom& random, IServerSignatureVerifier& verifier, IClock& clock,
        uint32_t lifetimeMillis, uint32_t minimumChallengeIntervalMillis);
    Result prepareAuthority(uint64_t channel, const TaskAuthorityChange& change, TaskManagementChallenge& output);
    Result prepareMutation(uint64_t channel, const TaskMutation& request, TaskManagementChallenge& output);
    Result prepareRead(uint64_t channel, ByteView requestId, uint32_t revision, uint32_t index,
        TaskManagementChallenge& output);
    Result prepareRead(uint64_t channel, const TaskReadPrepare& request, TaskManagementChallenge& output);
    Result prepareRetirement(uint64_t channel, const TaskRetirementPrepare& request, bool query,
        TaskManagementChallenge& output);
    // Consumes BEFORE verification/Flash; failure, WouldBlock or lost ACK needs
    // a fresh challenge. Engine CAS/replay handles durable retries, not nonce.
    // Success means an authenticated exchange completed; inspect output.kind:
    // Rejected is a pre-write refusal of THIS attempt, not mutation success.
    // Optional completed context is copied from the actual consumed pending,
    // never echoed from peer input; use it to bind an authenticated response.
    Result commit(uint64_t channel, ByteView grant, TaskManagementResult& output,
        TaskManagementChallenge* completed = nullptr);
    void poll();
    void cancel();
    void disconnect(uint64_t channel);
    bool pending() const { return pending_; }

private:
    enum class Operation : uint8_t { Initialize, Rebind, Mutate, Read, Retire, RetirementRead, Catalog };
    Result retirementBinding(TaskAuthorityBinding& output) const;
    Result available(uint64_t channel);
    Result issue(uint64_t channel, uint32_t generation, TaskManagementChallenge& output);
    bool validContext() const;
    TaskEngine& engine_;
    TaskAuthorityCache& authority_;
    IRandom& random_;
    IServerSignatureVerifier& verifier_;
    IClock& clock_;
    // Reuse owned request ID / CAS / id for authority and read, rather than
    // separate bootstrap/query queues. Definition storage is never borrowed.
    TaskMutation request_;
    // Retirement pins the stored binding here; current authentication root is
    // independently pinned by generation_ and the signed operation digest.
    TaskAuthorityBinding expected_;
    uint8_t nonce_[16];
    uint8_t digest_[32];
    uint64_t channel_;
    uint32_t generation_;
    uint32_t lifetime_;
    uint32_t interval_;
    uint32_t lastTick_;
    uint32_t remaining_;
    uint32_t cooldown_;
    Operation operation_;
    bool pending_;
    bool busy_;
    bool aborted_;
    TaskManagementSession(const TaskManagementSession&);
    TaskManagementSession& operator=(const TaskManagementSession&);
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(sizeof(TaskManagementSession) <= 352U, "Task pending slot exceeds its 32-bit ABI budget");
#endif

} // namespace blinker
#endif
