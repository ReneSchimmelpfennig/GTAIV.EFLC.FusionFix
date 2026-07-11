module;

#include <common.hxx>

export module depotradio;

import common;
import comvars;

// ============================================================================
// Fix: silent radio at Roman's taxi depot (TAXI_GARAGE ambient emitter)
//
// GTA IV hard-codes a special case for the ambient emitter named "TAXI_GARAGE"
// (the radio at Roman's depot) in the per-frame ambient-emitter update. When a
// global byte flag is set, the emitter's volume is forced to the mute value
// (-100 dB), regardless of anything else. That flag is a savegame-persisted
// radio-state byte; on The Complete Edition it ends up set, so the depot radio
// is permanently silent, whereas on 1.0.x it plays.
//
// CE 1.2.0.59 (image base 0x400000), inside the emitter update loop:
//   00987fa3  cmp   [esi+edi+0xD8], eax     ; emitter name hash == joaat("TAXI_GARAGE") ?
//   00987faa  jnz   short normal
//   00987fac  cmp   byte [0x01238955], 0    ; mute flag set ?
//   00987fb3  jz    short normal            ; <-- turned into an unconditional jmp
//   00987fb5  movss xmm0, [0x00fe8df8]      ; volume = -100 dB (mute)
//   00987fbd  movss [esp+0x18], xmm0
//
// Fix: change the conditional jump guarding the mute branch (74 = JZ/JE) into an
// unconditional jump (EB = JMP), so the mute branch is always skipped and the
// depot emitter is never force-muted. The savegame byte is left untouched.
//
// The same special case exists on 1.0.7.0/1.0.8.0 (at 0x008441df).
// Verified live: with the flag = 1, this single-byte edit restores Radio Broker
// at the depot.
// ============================================================================

class DepotRadio
{
public:
    DepotRadio()
    {
        FusionFix::onInitEventAsync() += []()
        {
            // 1.2.0.59:  cmp [esi+edi+0xD8],eax / jnz / cmp byte [imm32],0 / jz 10
            // 1.0.7.0/1.0.8.0: cmp [esi+0xD8],ecx / jnz / cmp byte [imm32],0 / je 10
            auto pattern = find_pattern(
                "39 84 3E D8 00 00 00 75 19 80 3D ?? ?? ?? ?? 00 74 10",   // 1.2.0.59
                "39 8E D8 00 00 00 75 19 80 3D ?? ?? ?? ?? 00 74 10");     // 1.0.7.0 / 1.0.8.0

            if (!pattern.empty())
            {
                // The guard is the trailing "74 10" of the match. Flip 0x74 (JZ/JE)
                // to 0xEB (JMP) so the mute branch is always skipped.
                auto* pJump = pattern.get_first<uint8_t>(0);
                for (int i = 0; i < 18; ++i)
                {
                    if (pJump[i] == 0x74 && pJump[i + 1] == 0x10)
                    {
                        injector::WriteMemory<uint8_t>(&pJump[i], 0xEB, true);
                        break;
                    }
                }
            }
        };
    }
} DepotRadio;
