"""Finite original-ARM allocator replay for one reviewed restricted R4 ELF.

No allocator, sbrk, lock, or memcpy function is emulated in Python. Python only
drives the reviewed source-derived request trace and inspects resulting memory.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import time
import traceback


EXPECTED_ELF = "189cef3529e0cfc2a743c1fb2a55ef52bf1ff35c3e93ead5cbb3e23a7f312bad"
FLASH_BASE, FLASH_BYTES = 0, 0x40000
SRAM_BASE, SRAM_BYTES = 0x20000000, 0x8000
SENTINEL = 0x3FF00
MAX_INSTRUCTIONS, CALL_TIMEOUT_US = 200000, 1000000
FUNCTIONS = (
    "_malloc_r", "_free_r", "_realloc_r", "_malloc_usable_size_r",
    "__wrap__malloc_r", "__wrap__free_r", "__wrap__realloc_r",
    "_sbrk_r", "__wrap__sbrk", "_sbrk", "__malloc_lock",
    "__malloc_unlock", "__errno", "memcpy",
    "_ZN10task_probe24sampleAllocationBoundaryEv",
)


def require(condition, message):
    if not condition:
        raise ValueError(message)


class Elf32:
    def __init__(self, path):
        self.data = path.read_bytes()
        self.sha256 = hashlib.sha256(self.data).hexdigest()
        require(self.sha256 == EXPECTED_ELF, "ELF does not match section 11.50")
        require(self.data[:7] == b"\x7fELF\x01\x01\x01", "expected little-endian ELF32")
        header = struct.unpack_from("<HHIIIIIHHHHHH", self.data, 16)
        require(header[1] == 40 and header[0] == 2, "expected executable ARM ELF")
        phoff, shoff = header[4], header[5]
        phsize, phcount, shsize, shcount = header[8], header[9], header[10], header[11]
        require(phsize == 32 and shsize == 40, "unexpected ELF table sizes")
        self.loads = []
        for index in range(phcount):
            fields = struct.unpack_from("<IIIIIIII", self.data, phoff + index * phsize)
            if fields[0] == 1:
                _, offset, address, physical, filesz, memsz, flags, align = fields
                require(filesz <= memsz and offset + filesz <= len(self.data), "bad LOAD bounds")
                self.loads.append(dict(offset=offset, address=address, physical=physical,
                                       filesz=filesz, memsz=memsz, flags=flags, align=align))
        self.symbols, self.duplicates = {}, set()
        sections = [struct.unpack_from("<IIIIIIIIII", self.data, shoff + i * shsize)
                    for i in range(shcount)]
        for section in sections:
            if section[1] != 2:
                continue
            offset, size, link, entsize = section[4], section[5], section[6], section[9]
            require(entsize == 16 and link < len(sections), "bad symbol table")
            strings = sections[link]
            names = self.data[strings[4]:strings[4] + strings[5]]
            for pos in range(offset, offset + size, entsize):
                name, value, length, info, other, defined = struct.unpack_from("<IIIBBH", self.data, pos)
                if not name or not defined:
                    continue
                end = names.find(b"\0", name)
                require(end >= name, "bad symbol name")
                text = names[name:end].decode("utf-8")
                item = dict(value=value, size=length, type=info & 15)
                if text in self.symbols and self.symbols[text] != item:
                    self.duplicates.add(text)
                self.symbols[text] = item

    def symbol(self, name):
        require(name in self.symbols, "missing symbol: " + name)
        require(name not in self.duplicates, "nonunique required symbol: " + name)
        return self.symbols[name]

    def function_binding(self, name):
        item = self.symbol(name)
        address = item["value"] & ~1
        for load in self.loads:
            if load["address"] <= address <= load["address"] + load["filesz"] - item["size"]:
                offset = load["offset"] + address - load["address"]
                body = self.data[offset:offset + item["size"]]
                return dict(name=name, address=address, bytes=item["size"],
                            sha256=hashlib.sha256(body).hexdigest())
        raise ValueError("function bytes outside LOAD: " + name)


class Replay:
    def __init__(self, elf, report):
        import unicorn
        from unicorn import arm_const

        require(unicorn.__version__ == "2.1.4", "temporary runtime must be unicorn 2.1.4")
        self.api, self.reg = unicorn, arm_const
        self.elf, self.report = elf, report
        self.uc = unicorn.Uc(unicorn.UC_ARCH_ARM, unicorn.UC_MODE_THUMB | unicorn.UC_MODE_MCLASS)
        self.uc.ctl_set_cpu_model(arm_const.UC_CPU_ARM_CORTEX_M4)
        self.uc.mem_map(FLASH_BASE, FLASH_BYTES, unicorn.UC_PROT_ALL)
        self.uc.mem_map(SRAM_BASE, SRAM_BYTES, unicorn.UC_PROT_READ | unicorn.UC_PROT_WRITE)
        omitted = []
        for load in elf.loads:
            address, length = load["address"], load["memsz"]
            content = elf.data[load["offset"]:load["offset"] + load["filesz"]]
            if not self.in_memory(address, length):
                require((address, load["physical"], load["filesz"], length, load["flags"])
                        == (0x01010018, 0x01010018, 28, 28, 4)
                        and hashlib.sha256(content).hexdigest()
                        == "326a8ca97825794cbe04eb7e9ac623ed0335ada08fec52df0140973f3c11bf48",
                        "unsupported LOAD outside real flash/SRAM")
                omitted.append(dict(**load, mapped=False,
                                    sha256=hashlib.sha256(content).hexdigest(),
                                    reason="option/config ROM is not executed or emulated"))
                continue
            self.uc.mem_write(address, content)
            if load["physical"] != address and content:
                require(0 <= load["physical"] <= FLASH_BYTES - len(content), "data LMA outside flash")
                self.uc.mem_write(load["physical"], content)
        self.uc.mem_protect(FLASH_BASE, FLASH_BYTES, unicorn.UC_PROT_READ | unicorn.UC_PROT_EXEC)
        self.base = elf.symbol("__HeapBase")["value"]
        self.top = elf.symbol("__StackTop")["value"]
        self.cap = self.top - 6144
        require((self.base, self.cap, self.top) == (0x20004418, 0x20006700, 0x20007F00),
                "unexpected reviewed heap/stack boundaries")
        self.heap_end = elf.symbol("current_heap_end.0")["value"]
        self.free_head = elf.symbol("__malloc_free_list")["value"]
        self.reent = self.word(elf.symbol("_impure_ptr")["value"])
        require(SRAM_BASE <= self.reent < self.base, "invalid original reent pointer")
        require(self.word(self.heap_end) == self.word(self.free_head) == 0, "nonzero ELF allocator initial state")
        self.ranges = []
        for name in FUNCTIONS:
            item = elf.symbol(name)
            require(item["type"] == 2 and item["size"] > 0, "missing function range: " + name)
            start = item["value"] & ~1
            self.ranges.append((start, start + item["size"], name))
        self.active, self.events, self.windows = {}, [], []
        self.instructions, self.minimum_sp, self.max_break = 0, self.top, self.base
        self.current, self.reached, self.fault = None, False, None
        self.called = set()
        self.uc.hook_add(unicorn.UC_HOOK_CODE, self.on_code)
        self.uc.hook_add(unicorn.UC_HOOK_MEM_INVALID, self.on_invalid)
        self.uc.hook_add(unicorn.UC_HOOK_MEM_WRITE, self.on_write)
        report.update(runtime=unicorn.__version__, image=elf.sha256,
                      heapBase=self.base, heapCap=self.cap, stackTop=self.top,
                      initialReent=self.reent, executedFunctions=[],
                      imageLoads=elf.loads,
                      omittedLoads=omitted,
                      functionBindings=[elf.function_binding(name) for name in FUNCTIONS],
                      functionReplacementHooks=[],
                      observerHooks=["instruction/range/stack", "invalid-memory", "SRAM-write-bound"])

    @staticmethod
    def in_memory(address, size):
        return ((0 <= address <= FLASH_BYTES - size)
                or (SRAM_BASE <= address <= SRAM_BASE + SRAM_BYTES - size))

    def word(self, address):
        return struct.unpack("<I", self.uc.mem_read(address, 4))[0]

    def stop_fault(self, text):
        self.fault = text
        self.uc.emu_stop()

    def on_code(self, uc, address, size, _):
        if address == SENTINEL:
            self.reached = True
            uc.emu_stop()
            return
        self.instructions += 1
        for start, end, name in self.ranges:
            if start <= address and address + size <= end:
                self.called.add(name)
                break
        else:
            return self.stop_fault("execution outside audited helpers at " + hex(address))
        sp = uc.reg_read(self.reg.UC_ARM_REG_SP)
        self.minimum_sp = min(self.minimum_sp, sp)
        if not self.cap <= sp <= self.top or sp & 3:
            self.stop_fault("stack crossed reserved interval")

    def on_invalid(self, uc, access, address, size, value, _):
        self.fault = "unmapped/protected access at " + hex(address)
        return False

    def on_write(self, uc, access, address, size, value, _):
        if not SRAM_BASE <= address <= self.top - size:
            self.stop_fault("write outside supported SRAM at " + hex(address))

    def call(self, name, arguments):
        require(self.current is None, "nested driver call")
        self.current, self.reached, self.fault = name, False, None
        registers = (self.reg.UC_ARM_REG_R0, self.reg.UC_ARM_REG_R1,
                     self.reg.UC_ARM_REG_R2, self.reg.UC_ARM_REG_R3)
        for index, register in enumerate(registers):
            self.uc.reg_write(register, arguments[index] if index < len(arguments) else 0)
        self.uc.reg_write(self.reg.UC_ARM_REG_MSP, self.top)
        self.uc.reg_write(self.reg.UC_ARM_REG_SP, self.top)
        self.uc.reg_write(self.reg.UC_ARM_REG_LR, SENTINEL | 1)
        self.uc.reg_write(self.reg.UC_ARM_REG_CONTROL, 0)
        self.uc.reg_write(self.reg.UC_ARM_REG_PRIMASK, 0)
        before, started = self.instructions, time.monotonic()
        try:
            self.uc.emu_start(self.elf.symbol(name)["value"] | 1, SENTINEL + 2,
                              timeout=CALL_TIMEOUT_US, count=MAX_INSTRUCTIONS)
            require(self.fault is None, self.fault)
            require(self.reached, "call exceeded instruction/time budget: " + name)
            require(time.monotonic() - started < 2, "host call wall-clock budget exceeded")
            return self.uc.reg_read(self.reg.UC_ARM_REG_R0)
        finally:
            self.report["lastCall"] = dict(name=name, instructions=self.instructions - before,
                                           returned=self.reached, fault=self.fault)
            self.current = None

    def chunk(self, pointer):
        offset = struct.unpack("<i", self.uc.mem_read(pointer - 4, 4))[0]
        base = pointer - 4 + (offset if offset < 0 else 0)
        size = self.word(base)
        require(self.base <= base < self.cap and size >= 12 and size % 4 == 0,
                "invalid allocated chunk")
        require(base + size <= self.break_value(), "chunk exceeds break")
        return base, size

    def break_value(self):
        value = self.word(self.heap_end)
        value = value or self.base
        require(self.base <= value <= self.cap, "break crossed reviewed cap")
        self.max_break = max(self.max_break, value)
        return value

    def snapshot(self):
        brk, cursor, free = self.break_value(), self.word(self.free_head), []
        previous_end = None
        while cursor:
            require(len(free) < 128 and self.base <= cursor <= brk - 8,
                    "free list cycle or out of bounds")
            length, following = self.word(cursor), self.word(cursor + 4)
            require(length >= 12 and length % 4 == 0 and cursor + length <= brk,
                    "invalid free chunk")
            require(previous_end is None or cursor > previous_end,
                    "overlapping or uncoalesced adjacent free chunks")
            free.append(dict(address=cursor, bytes=length))
            previous_end, cursor = cursor + length, following
        ranges = sorted((item["base"], item["base"] + item["chunk"], key)
                        for key, item in self.active.items())
        for index, (start, end, label) in enumerate(ranges):
            require(index == 0 or start >= ranges[index - 1][1], "active allocation overlap")
            for item in free:
                require(end <= item["address"] or start >= item["address"] + item["bytes"],
                        "free/active allocation overlap")
        return dict(breakAddress=brk, freeList=free,
                    largestFreeChunk=max((item["bytes"] for item in free), default=0),
                    remainingBreakBytes=self.cap - brk,
                    activeRequestedBytes=sum(item["request"] for item in self.active.values()),
                    activeChunkBytes=sum(item["chunk"] for item in self.active.values()))

    def event(self, operation, label, request=None, pointer=None):
        self.events.append(dict(index=len(self.events), operation=operation,
                                label=label, request=request, pointer=pointer,
                                **self.snapshot()))

    def allocate(self, label, size):
        require(label not in self.active and 0 < size <= 1921, "invalid reviewed allocation")
        pointer = self.call("__wrap__malloc_r", [self.reent, size])
        require(pointer != 0 and pointer % 8 == 0, "required malloc failed or misaligned")
        base, length = self.chunk(pointer)
        require(pointer + size <= base + length, "requested bytes exceed chunk")
        self.active[label] = dict(pointer=pointer, request=size, base=base, chunk=length)
        self.uc.mem_write(pointer, bytes([len(self.events) % 251 + 1]) * size)
        self.event("malloc", label, size, pointer)
        return pointer

    def free(self, label):
        require(label in self.active, "free of unknown trace label")
        item = self.active.pop(label)
        self.call("__wrap__free_r", [self.reent, item["pointer"]])
        self.event("free", label, item["request"], item["pointer"])

    def grow(self, label, sizes):
        for index, size in enumerate(sizes):
            new_label = label + ":" + str(index)
            self.allocate(new_label, size)
            if index:
                old_label = label + ":" + str(index - 1)
                old, new = self.active[old_label], self.active[new_label]
                copied = old["request"] - 1
                expected = bytes(self.uc.mem_read(old["pointer"], copied))
                returned = self.call("memcpy", [new["pointer"], old["pointer"], copied])
                require(returned == new["pointer"]
                        and bytes(self.uc.mem_read(new["pointer"], copied)) == expected,
                        "original memcpy did not preserve the modeled string prefix")
                self.event("string-prefix-copy", new_label, copied, returned)
                self.free(old_label)
        return label + ":" + str(len(sizes) - 1)

    def rpc_window(self, name, response_bytes, heap_prompt):
        prefix = "rpc:" + name
        if heap_prompt:
            self.allocate(prefix + ":prompt", 20)
        command = self.grow(prefix + ":command", [31, 61])
        response_sizes, capacity = [], 15
        while capacity < response_bytes:
            capacity *= 2
            response_sizes.append(capacity + 1)
        response = self.grow(prefix + ":response", response_sizes)
        # buf_read's command is destroyed first, then the prompt temporary at
        # the write expression boundary, and finally the caller's response.
        self.free(command)
        if heap_prompt:
            self.free(prefix + ":prompt")
        self.free(response)
        snapshot = self.snapshot()
        require(len(self.active) == 6, "RPC retained a transient allocation")
        self.windows.append(dict(name=name, responseLimit=response_bytes,
                                 heapPrompt=heap_prompt, **snapshot))

    def run(self):
        # Core arduino_main starts AGT before setup. The lazy WiFi product then
        # constructs control/MQTT clients in member order, followed by PubSub.
        for label, size in (("agt", 36), ("http-fifo", 1036), ("http-control", 16),
                            ("mqtt-fifo", 1036), ("mqtt-control", 16), ("mqtt", 361)):
            self.allocate(label, size)
        pointer = self.active["mqtt"]["pointer"]
        before = self.snapshot()
        returned = self.call("__wrap__realloc_r", [self.reent, pointer, 361])
        require(returned == pointer and self.snapshot() == before,
                "same-size MQTT realloc changed original allocation")
        self.event("realloc-same", "mqtt", 361, returned)
        for args in (("network-max", 1024, True), ("network-small", 256, True),
                     ("storage-max", 1024, False), ("network-smallest", 64, True),
                     ("network-max-repeat", 1024, True), ("storage-max-repeat", 1024, False)):
            self.rpc_window(*args)
        reference = self.windows[2]
        require(all(reference["freeList"] == window["freeList"]
                    and reference["breakAddress"] == window["breakAddress"]
                    for window in self.windows[-2:]),
                "complete request family did not reach the bounded fixed point")
        require(self.windows[-1]["largestFreeChunk"] >= 1932,
                "no existing contiguous maximum-size nano chunk")
        self.allocate("contiguous-probe", 1921)
        self.free("contiguous-probe")
        require(self.snapshot()["freeList"] == self.windows[-1]["freeList"],
                "maximum contiguous probe did not restore free shape")
        self.report.update(status="passed-limited-trace", events=self.events, windows=self.windows,
                           maximumBreakAddress=self.max_break, maximumBreakBytes=self.max_break - self.base,
                           maximumRequestedBytes=max(event["activeRequestedBytes"] for event in self.events),
                           maximumActiveChunkBytes=max(event["activeChunkBytes"] for event in self.events),
                           maximumSingleRequest=max(event["request"] for event in self.events
                                                    if event["operation"] == "malloc"),
                           fixedPointReference=reference["name"],
                           fixedPointComparedWindows=[window["name"] for window in self.windows[-2:]],
                           extraBreakAfterFirstNetworkWindow=(reference["breakAddress"]
                                                              - self.windows[0]["breakAddress"]),
                           minimumCallStackPointer=self.minimum_sp,
                           executedFunctions=sorted(self.called), instructions=self.instructions,
                           noAllocatorHooks=True, formalTaskAdmission=False,
                           limits=["source-derived trace, not full product execution",
                                   "no physical IRQ, cold boot, UART, network, or callback execution",
                                   "no arbitrary fragmentation, user heap, or recoverable C++ OOM proof"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--runtime", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    require(not args.output.exists(), "output already exists; original evidence must not be overwritten")
    require(args.output.parent.is_dir(), "output parent must already exist")
    report = dict(schema="r4-original-arm-allocator-replay/1", status="started", events=[],
                  expectedElf=EXPECTED_ELF, formalTaskAdmission=False,
                  driverSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    replay = None
    try:
        sys.path.insert(0, str(args.runtime.resolve(strict=True)))
        elf = Elf32(args.elf.resolve(strict=True))
        replay = Replay(elf, report)
        replay.run()
        result = 0
    except Exception as error:
        report.update(status="failed", error=type(error).__name__ + ": " + str(error),
                      traceback=traceback.format_exc())
        if replay is not None:
            report.update(events=replay.events, windows=replay.windows,
                          executedFunctions=sorted(replay.called), instructions=replay.instructions)
        result = 1
    with args.output.open("x", encoding="utf-8") as output:
        json.dump(report, output, indent=2)
        output.write("\n")
    print(json.dumps(dict(status=report["status"], events=len(report["events"]),
                          output=str(args.output), error=report.get("error"))))
    return result


if __name__ == "__main__":
    raise SystemExit(main())
