# Restricted R4 allocator replay

This offline check executes the **original ARM machine code** from the reviewed
Core 1.5.3 R4 ELF. It does not replace malloc/free/realloc, `_sbrk`, their heap
cap wrapper, the allocator locks, or memcpy. Unicorn is a temporary test tool,
not an Arduino or published-library dependency.

```powershell
python replay.py --elf <reviewed-ELF> --runtime <temporary-unicorn-directory> --output <new-private-report.json>
```

Only the ELF SHA256 frozen in section 11.50 is accepted. The script loads its
ELF32 ARM program segments, initializes `.data` and zero-filled memory, takes
the real `_impure_ptr`, and calls the original functions with the ARM ABI.
The exact 28 B read-only option/config ROM segment is recorded but deliberately
not mapped; its hardware configuration is not simulated and any access fails.
Each call has instruction and wall-clock limits. Only audited allocator/helper
function ranges may execute. MMIO, unknown code, bounds errors, failed required
allocations, and an existing output report all fail closed. The first failure
is saved; the tool never automatically retries or relaxes the image/hooks.

The fixed trace models the reviewed requested-byte/lifetime envelope: AGT,
two native FIFO/control pairs, and the 361 B MQTT buffer stay allocated;
bounded command/response strings allocate a new block before releasing the old
one. Six short RPC windows cover different response lengths, storage-shaped
windows without a heap prompt, and one repeated maximal network/storage pair. This
is **not** a recording or execution of the complete product. No setup, user
callback, UART, network, business request, physical interrupt, or hardware is run.
The source-derived allocation order and supported exclusions are deliberately
part of this limited test contract, not a claim of arbitrary-Sketch safety.

The report records every modeled allocation/free, actual returned address,
break and free-list shape, active-block overlap, alignment, and the largest
free chunk. The cold first network window is not the complete-family baseline:
the first storage window may cause additional real heap extension. Equal states
after the first complete family and repeated maximal network/storage pair show
a fixed point for this explicitly listed trace only. The report retains the
actual high-water break, existing free chunk and unextended break tail separately.
These results do not prove all fragmentation histories, startup/IRQ
stack bounds, recoverable C++ OOM, or formal task admission. No result opens a
profile/device ID, changes the SDK, or removes the original UNKNOWN/logs.
