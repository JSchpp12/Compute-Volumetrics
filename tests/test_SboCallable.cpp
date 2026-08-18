// Unit test for the reusable SboCallable<Sig, N> type-erased callable template added in
// Step 2 of the RenderPhase composition refactor (see renderphase_composition_refactor_plan.txt
// section 4.2). Exercises the Task-style inline-buffer + trampoline mechanism in isolation
// (store / invoke / move-relocate / destroy), before it is wired into RenderPhase.
//
// The header is dependency-free (no Vulkan / RenderPhase types), so this test can be built
// without the rendering stack.

#include <gtest/gtest.h>

#include <starlight/core/TypeErasedCallable.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

namespace
{
// ---- Instrumentation counters ----
int g_trackerConstructs = 0;
int g_trackerDestroys = 0;
int g_invokes = 0;
int g_recConstructs = 0;
int g_recDestroys = 0;

struct Tracker
{
    int payload;
    explicit Tracker(int p) : payload(p)
    {
        ++g_trackerConstructs;
    }
    Tracker(const Tracker &) = delete;
    Tracker &operator=(const Tracker &) = delete;
    Tracker(Tracker &&) noexcept
    {
        ++g_trackerConstructs;
    }
    Tracker &operator=(Tracker &&) = delete;
    ~Tracker()
    {
        ++g_trackerDestroys;
    }
};

// void(int&, int, int) signature with reference + value args. Tracks its own lifecycle.
struct Recorder
{
    int value = 7;
    std::string note = "rec";
    Recorder()
    {
        ++g_recConstructs;
    }
    Recorder(int v, std::string n) : value(v), note(std::move(n))
    {
        ++g_recConstructs;
    }
    Recorder(Recorder &&o) noexcept : value(o.value), note(std::move(o.note))
    {
        ++g_recConstructs;
    }
    Recorder(const Recorder &) = delete;
    Recorder &operator=(const Recorder &) = delete;
    Recorder &operator=(Recorder &&) = delete;
    ~Recorder()
    {
        ++g_recDestroys;
    }
    void operator()(int &out, int a, int b) const
    {
        ++g_invokes;
        out = value + a + b;
    }
};

// Non-const operator() to confirm the mutable inline buffer lets a const SboCallable dispatch a
// mutating payload (std::function semantics).
struct MutatingUpdater
{
    int state = 100;
    int operator()(int delta)
    {
        state += delta;
        ++g_invokes;
        return state;
    }
};

// Heap-owning payload (unique_ptr) to validate R5-style ownership: the destroy trampoline
// deletes the owned object exactly once.
struct OwningPayload
{
    std::unique_ptr<Tracker> owned;
    explicit OwningPayload(int p) : owned(std::make_unique<Tracker>(p))
    {
    }
    int operator()()
    {
        ++g_invokes;
        return owned ? owned->payload : -1;
    }
};

void resetCounters()
{
    g_trackerConstructs = 0;
    g_trackerDestroys = 0;
    g_invokes = 0;
    g_recConstructs = 0;
    g_recDestroys = 0;
}
} // namespace

using VoidSig = void();
using RefSig = void(int &, int, int);
using IntSig = int(int);
using NoargSig = int();

TEST(SboCallable, EmptyCallableIsFalsyAndDoesNotInvoke)
{
    resetCounters();
    star::core::SboCallable<VoidSig, 64> empty;
    EXPECT_FALSE(static_cast<bool>(empty));
    EXPECT_FALSE(empty);
    EXPECT_EQ(g_invokes, 0);
}

TEST(SboCallable, MakeStoresAndExecInvokesPayload)
{
    resetCounters();
    // make of a prvalue: the temporary + placement-new move-construct = 2 constructions,
    // 1 destroy of the temporary. The SboCallable owns exactly one live Recorder.
    const int recC0 = g_recConstructs;
    auto rec = star::core::SboCallable<RefSig, 64>::make(Recorder{});
    EXPECT_EQ(g_recConstructs, recC0 + 2);
    EXPECT_EQ(g_recDestroys, 1);
    EXPECT_TRUE(rec);

    int out = 0;
    rec.exec(out, 3, 4);
    EXPECT_EQ(out, 14); // 7 + 3 + 4
    EXPECT_EQ(g_invokes, 1);
}

TEST(SboCallable, ConstExecDispatchesMutatingPayloadViaMutableBuffer)
{
    resetCounters();
    auto mut = star::core::SboCallable<IntSig, 64>::make(MutatingUpdater{});
    const auto &cmut = mut;

    int r = cmut.exec(5); // const SboCallable, mutating operator()
    EXPECT_EQ(r, 105);    // 100 + 5
    EXPECT_EQ(g_invokes, 1);

    r = mut.exec(1);
    EXPECT_EQ(r, 106); // 105 + 1
    EXPECT_EQ(g_invokes, 2);
}

TEST(SboCallable, MoveConstructRelocatesAndNullsSource)
{
    resetCounters();
    auto rec = star::core::SboCallable<RefSig, 64>::make(Recorder{});
    int out = 0;
    rec.exec(out, 0, 0);
    ASSERT_EQ(g_invokes, 1);

    auto moved = std::move(rec);
    EXPECT_FALSE(rec); // source trampolines nulled
    EXPECT_TRUE(moved);

    int out2 = 0;
    moved.exec(out2, 10, 10);
    EXPECT_EQ(out2, 27); // 7 + 10 + 10
    EXPECT_EQ(g_invokes, 2);
    // The move trampoline destroyed the source's Recorder after relocating it.
    EXPECT_GE(g_recDestroys, 1);
}

TEST(SboCallable, MoveAssignmentOverEngagedSlotDestroysOldThenRelocatesSource)
{
    resetCounters();
    auto moved = star::core::SboCallable<RefSig, 64>::make(Recorder{});
    auto target = star::core::SboCallable<RefSig, 64>::make(Recorder{42, "tgt"});

    const int recDestroysBefore = g_recDestroys;
    target = std::move(moved);
    EXPECT_FALSE(moved); // source nulled
    EXPECT_TRUE(target);
    // +2: target's old (value=42) payload destroyed by m_destroy, AND moved's source
    // (value=7) payload destroyed by the move trampoline after relocating into target.
    EXPECT_EQ(g_recDestroys, recDestroysBefore + 2);

    int out = 0;
    target.exec(out, 1, 2);
    EXPECT_EQ(out, 10); // 7 + 1 + 2 (the relocated payload, value=7)
}

TEST(SboCallable, HeapOwningPayloadIsDeletedExactlyOnceOnDestroy)
{
    resetCounters();
    {
        auto own = star::core::SboCallable<NoargSig, 64>::make(OwningPayload{123});
        EXPECT_EQ(own.exec(), 123);
        EXPECT_EQ(g_trackerDestroys, 0); // not destroyed yet
    }
    EXPECT_EQ(g_trackerDestroys, 1); // exactly one destroy after scope exit
    EXPECT_EQ(g_trackerConstructs, 1);
}

TEST(SboCallable, SelfMoveAssignmentIsNoOp)
{
    resetCounters();
    auto selfp = star::core::SboCallable<RefSig, 64>::make(Recorder{9, "s"});
    const int invokesBefore = g_invokes;
    selfp = std::move(selfp); // guarded by `if (this != &other)`
    int out = 0;
    selfp.exec(out, 0, 0);
    EXPECT_EQ(out, 9); // still intact
    EXPECT_EQ(g_invokes, invokesBefore + 1);
}

// Compile-time guard (invariant I3): an oversized payload must fail the static_assert in
// SboCallable::make. This is verified out-of-band by a standalone compile that must fail; the
// hard cap is exercised by the standalone sbo_oversize_must_fail.cpp translation unit.
