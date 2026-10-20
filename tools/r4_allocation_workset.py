"""Requested-byte arithmetic for one restricted R4 Core 1.5.3 candidate.

This is an offline source-derived model of known enumerated allocations, not
all libc/global dynamic storage, an allocator simulation, runtime measurement
or admission check. GCC 7 C++11-ABI strings start in 15-character
SSO storage and these parser strings only append one character or clear. The
old and new character blocks coexist during growth; clearing retains capacity.
No arbitrary caller reserve/capacity, direct native WiFi calls, TLS or BLE is
covered. String objects/SSO live on the separately budgeted stack.
IPv4 connects use the guarded client's 16-byte stack text, not the native
IPAddress.toString() heap temporary. Native IPv6 fallback is outside this model.
"""

UINT32_MAX = (1 << 32) - 1
STRING_MAX_SIZE = (UINT32_MAX - 1) // 2
STRING_LOCAL_CAPACITY = 15

# One frozen, non-TLS, two-client profile. These are requested object/character
# bytes only; allocator metadata, alignment, free chunks and fragmentation are
# deliberately not estimated as zero.
FRAME_BYTES = 256
TOPIC_BYTES = 96
MQTT_HEADER_BYTES = 9
INITIAL_MQTT_BYTES = 256
NATIVE_CLIENTS = 2
NATIVE_FIFO_BYTES = 1036
NATIVE_CONTROL_BYTES = 16
CORE_AGT_BYTES = 36  # Main Core time/FspTimer path allocates one AGTimer.
COMMAND_BYTES = 32
RESPONSE_BYTES = 1024
MAX_PROMPT_BYTES = 20  # Direct 19-character CLIENTCONNECTNAME constructor + NUL.
IPV4_TEXT_STACK_BYTES = 16  # Guarded IPv4 connect text; not a heap request.


def _u32(value: int) -> int:
    if type(value) is not int or not 0 <= value <= UINT32_MAX:
        raise ValueError("workset values must be unsigned 32-bit integers")
    return value


def _sum_u32(*values: int) -> int:
    total = 0
    for value in values:
        total += _u32(value)
        _u32(total)
    return total


def string_growth(maximum: int) -> dict:
    """Bound fresh single-character growth; at most 28 steps on this ABI.

    max_size follows this GCC basic_string<char> ABI, not Python integer size.
    A zero/SSO-only string has no heap character request. This does not model
    prior arbitrary reserve(), assignment growth or any host std::string ABI.
    """
    maximum = _u32(maximum)
    if maximum > STRING_MAX_SIZE:
        raise ValueError("maximum exceeds the reviewed 32-bit string max_size")
    capacity = STRING_LOCAL_CAPACITY
    retained = 0
    peak = 0
    steps = 0
    while capacity < maximum:
        new_capacity = min(capacity * 2, STRING_MAX_SIZE)
        new_request = _sum_u32(new_capacity, 1)
        peak = max(peak, _sum_u32(retained, new_request))
        capacity = new_capacity
        retained = new_request
        steps += 1
    return {
        "maximumCharacters": maximum,
        "capacityCharacters": capacity,
        "retainedRequestedBytes": retained,
        "growthPeakRequestedBytes": peak,
        "growthSteps": steps,
    }


def restricted_workset(initial_mqtt_bytes: int = INITIAL_MQTT_BYTES) -> dict:
    """Return disjoint lifetime envelopes, never a 'fits' or admitted result."""
    initial_mqtt_bytes = _u32(initial_mqtt_bytes)
    # Only the reviewed baseline or exact final packet size is supported. This
    # models avoiding that one native realloc growth, not fixing the allocator.
    if initial_mqtt_bytes not in (256, 361):
        raise ValueError("only the reviewed 256/361-byte MQTT initialization is supported")
    command = string_growth(COMMAND_BYTES)
    response = string_growth(RESPONSE_BYTES)
    native_object = _sum_u32(NATIVE_FIFO_BYTES, NATIVE_CONTROL_BYTES)
    native_clients = _u32(_u32(NATIVE_CLIENTS) * native_object)
    known_persistent = _sum_u32(native_clients, CORE_AGT_BYTES)
    mqtt = _sum_u32(FRAME_BYTES, TOPIC_BYTES, MQTT_HEADER_BYTES)
    cold = _sum_u32(known_persistent, initial_mqtt_bytes)
    steady = _sum_u32(known_persistent, mqtt)
    # realloc's possible old+new coexistence is a separate phase, not a second
    # simultaneous RPC allocation. No assumption that realloc grows in place.
    growth_avoided = initial_mqtt_bytes >= mqtt
    mqtt_resize = (steady if growth_avoided
                   else _sum_u32(known_persistent, initial_mqtt_bytes, mqtt))
    # One cooperative parser executes one append at a time, so do not sum the
    # growth peaks of both strings as though both allocations were concurrent.
    parser = max(
        _sum_u32(response["growthPeakRequestedBytes"],
                 command["retainedRequestedBytes"]),
        _sum_u32(command["growthPeakRequestedBytes"],
                 response["retainedRequestedBytes"]),
    )
    network_parser = _sum_u32(parser, MAX_PROMPT_BYTES)
    return {
        "schema": "internal-r4-requested-workset/2",
        "core": "1.5.3",
        "wordBits": 32,
        "scope": "known-enumerated-allocations",
        "requestedBytesOnly": True,
        "metadataExcluded": True,
        "alignmentExcluded": True,
        "fragmentationExcluded": True,
        "oomRecoverable": False,
        "formalTaskAdmission": False,
        "initialMqttPacketBytes": initial_mqtt_bytes,
        "nativeReallocGrowthAvoided": growth_avoided,
        "profile": {
            "frameBytes": _u32(FRAME_BYTES),
            "topicBytes": _u32(TOPIC_BYTES),
            "mqttHeaderBytes": _u32(MQTT_HEADER_BYTES),
            "initialMqttBytes": initial_mqtt_bytes,
            "nativeClients": _u32(NATIVE_CLIENTS),
            "nativeFifoBytes": _u32(NATIVE_FIFO_BYTES),
            "nativeControlBytes": _u32(NATIVE_CONTROL_BYTES),
            "coreAgtBytes": _u32(CORE_AGT_BYTES),
            "maxPromptRequestedBytes": _u32(MAX_PROMPT_BYTES),
            "ipv4StringRequestedBytes": 0,
            "ipv4TextStackBytes": _u32(IPV4_TEXT_STACK_BYTES),
        },
        "strings": {"command": command, "response": response},
        "persistentRequestedBytes": {
            "nativeClients": native_clients,
            "coreAgt": _u32(CORE_AGT_BYTES),
            "mqttAfterResize": mqtt,
            "cold": cold,
            "steady": steady,
        },
        "phasesRequestedBytes": {
            "mqttResize": mqtt_resize,
            "parserJointTransient": parser,
            "networkParserWithPrompt": network_parser,
            "storageRpcWithSteady": _sum_u32(steady, parser),
            "networkRpcWithSteady": _sum_u32(steady, network_parser),
            "initialHttpConnect": _sum_u32(cold, network_parser),
            "refreshHttpConnect": _sum_u32(steady, network_parser),
        },
        "constraints": [
            "Fresh parser response callers; no arbitrary reserve/capacity.",
            "Clear/reset retains capacity; one cooperative parser grows one string at a time.",
            "WiFi station sets native timeout to zero; no nested begin/status response lifetime.",
            "Guarded IPv4 connects use 16-byte stack text, not a heap request; native IPv6 fallback is excluded.",
            "MQTT domain connect has no simultaneous IPAddress temporary.",
            "No TLS, BLE, user native WiFi calls or allocator/IRQ concurrency.",
            "No stack budget, allocator overhead, contiguous allocation or OOM recovery proof.",
            "Known enumerated allocations only, not a complete census of libc/global dynamic storage.",
        ],
    }
