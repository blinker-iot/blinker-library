"""Print selected actual projected C bridge functions for a narrow Host test.

This is not an ABI or complete native lwIP compilation test. Extraction retains
the generated function bodies; it never substitutes a fixture copy of them.
"""
import argparse
import importlib.util
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--projection", type=Path, required=True)
    parser.add_argument("--client-tool", type=Path, required=True)
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location("actual_wio_client_transform", args.client_tool)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    sockets = (args.projection / "lwip/sockets.c").read_text(encoding="utf-8")
    netdb = (args.projection / "lwip/netdb.c").read_text(encoding="utf-8")
    helper = module._COPY_HELPER
    if sockets.count(helper) != 1 or netdb.count(helper) != 1:
        raise ValueError("actual bridge copy-helper anchor mismatch")
    parts = ["// Actual projected bodies; Host-only ABI/OS seams live in the fixture.", helper]
    for text, names in ((sockets, ("lwip_errno", "lwip_getpeername", "lwip_getsockopt", "lwip_read")),
                        (netdb, ("netconn_gethostbyname",))):
        for name in names:
            captured = []
            def capture(body):
                captured.append(body)
                return body
            module._function(text, name, capture)
            if len(captured) != 1:
                raise ValueError("actual bridge function extraction failed: " + name)
            parts.extend(("\n// " + name, captured[0]))
    print("\n".join(parts))


if __name__ == "__main__":
    main()
