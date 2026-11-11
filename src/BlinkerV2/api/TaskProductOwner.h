#ifndef BLINKER_API_TASKPRODUCTOWNER_H
#define BLINKER_API_TASKPRODUCTOWNER_H

namespace blinker {

// Optional non-blocking ownership around the ORIGINAL task lifecycle cycle.
// The platform may defer a cycle while a native root mutation owns storage.
class ITaskProductOwner {
public:
    virtual ~ITaskProductOwner() {}
    virtual bool enterTaskOwner() = 0;
    virtual void leaveTaskOwner() = 0;
};

// Empty-base policy keeps ordinary/manual products free of an owner pointer.
struct NoTaskProductOwner {
    enum : bool { stopOnDestruction = true };
    bool enterTaskOwner() { return true; }
    void leaveTaskOwner() {}
};

class BorrowedTaskProductOwner {
public:
    enum : bool { stopOnDestruction = false };
    explicit BorrowedTaskProductOwner(ITaskProductOwner* owner = nullptr) : owner_(owner) {}
    bool enterTaskOwner() { return owner_ != nullptr && owner_->enterTaskOwner(); }
    void leaveTaskOwner() { if (owner_ != nullptr) owner_->leaveTaskOwner(); }
private:
    ITaskProductOwner* owner_;
};

} // namespace blinker
#endif
