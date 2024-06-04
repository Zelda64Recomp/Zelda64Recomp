// Quicksaving is disabled for now

#if 0

#include "librecomp/helpers.hpp"
#include "librecomp/input.hpp"
#include "ultramodern/ultramodern.hpp"

enum class QuicksaveAction {
    None,
    Save,
    Load
};

std::atomic<QuicksaveAction> cur_quicksave_action = QuicksaveAction::None;

void zelda64::quicksave_save() {
    cur_quicksave_action.store(QuicksaveAction::Save);
}

void zelda64::quicksave_load() {
    cur_quicksave_action.store(QuicksaveAction::Load);
}

uint8_t saved_rdram[ultramodern::rdram_size];

thread_local recomp_context saved_context;

void save_context(recomp_context* ctx) {
    saved_context = *ctx;
}

void load_context(recomp_context* ctx) {
    *ctx = saved_context;

    // Restore the pointer to the odd floats for correctly handling mips3 float mode.
    if (ctx->mips3_float_mode) {
        // FR = 1, odd single floats point to their own registers
        ctx->f_odd = &ctx->f1.u32l;
    }
    else {
        // FR = 0, odd single floats point to the upper half of the previous register
        ctx->f_odd = &ctx->f0.u32h;
    }
}

extern "C" void recomp_handle_quicksave_actions(uint8_t* rdram, recomp_context* ctx) {
    QuicksaveAction action = cur_quicksave_action.load();

    if (action != QuicksaveAction::None) {
        PTR(OSMesgQueue) quicksave_enter_mq = _arg<0, PTR(OSMesgQueue)>(rdram, ctx);
        PTR(OSMesgQueue) quicksave_exit_mq = _arg<1, PTR(OSMesgQueue)>(rdram, ctx);

        printf("saving context for thread %d\n", TO_PTR(OSThread, ultramodern::this_thread())->id);

        // Save or load the thread's context as needed based on the action.
        if (action == QuicksaveAction::Save) {
            save_context(ctx);
        }
        else if (action == QuicksaveAction::Load) {
            load_context(ctx);
        }
        else {
            assert(false);
        }
    
        // Tell the main thread that one of the other permanent threads is ready for performing a quicksave action.
        osSendMesg(rdram, quicksave_enter_mq, NULLPTR, OS_MESG_NOBLOCK);
        // Wait for the main thread to signal that other permanent threads are safe to continue.
        osRecvMesg(rdram, quicksave_exit_mq, NULLPTR, OS_MESG_BLOCK);
    }
}

extern "C" void wake_threads_for_quicksave_action(uint8_t* rdram, recomp_context* ctx);

extern "C" void recomp_handle_quicksave_actions_main(uint8_t* rdram, recomp_context* ctx) {
    QuicksaveAction action = cur_quicksave_action.load();
    
    if (action != QuicksaveAction::None) {
        PTR(OSMesgQueue) quicksave_enter_mq = _arg<0, PTR(OSMesgQueue)>(rdram, ctx);
        PTR(OSMesgQueue) quicksave_exit_mq = _arg<1, PTR(OSMesgQueue)>(rdram, ctx);

        wake_threads_for_quicksave_action(rdram, ctx);

        // Wait for all other permanent threads (hence the minus 1) to signal that they're ready for a quicksave action.
        for (uint32_t i = 0; i < ultramodern::permanent_thread_count() - 1; i++) {
            osRecvMesg(rdram, quicksave_enter_mq, NULLPTR, OS_MESG_BLOCK);
        }

        // Allow any temporary threads to complete by lowering this thread's priority to 0.
        // TODO this won't cause all temporary threads to complete if any are blocked by permanent threads
        // or events like timers. Situations like that will need to be handed on a case-by-case basis for a given game.
        if (ultramodern::temporary_thread_count() != 0) {
            OSPri old_pri = osGetThreadPri(rdram, NULLPTR);
            osSetThreadPri(rdram, NULLPTR, 0);

            osSetThreadPri(rdram, NULLPTR, old_pri);
        }

        if (action == QuicksaveAction::Save) {
            std::copy(rdram, rdram + ultramodern::rdram_size, saved_rdram);
        }
        else if (action == QuicksaveAction::Load) {
            std::copy(saved_rdram, saved_rdram + ultramodern::rdram_size, rdram);
        }
        else {
            assert(false);
        }

        printf("Quicksave action complete\n");

        cur_quicksave_action.store(QuicksaveAction::None);

        // Tell all other permanent threads that they're good to continue.
        for (uint32_t i = 0; i < ultramodern::permanent_thread_count() - 1; i++) {
            osSendMesg(rdram, quicksave_exit_mq, NULLPTR, OS_MESG_BLOCK);
        }
    }
}

#endif
